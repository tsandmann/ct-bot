/*
 * c't-Bot
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/**
 * \file 	motor.c
 * \brief 	High-Level-Routinen fuer die Motorsteuerung des c't-Bot
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	17.10.2006
 */

#include <stdlib.h>
#include <string.h>
#include "ct-Bot.h"
#include "motor.h"
#include "bot-local.h"
#include "motor-low.h"
#include "timer.h"
#include "sensor.h"
#include "sensor-low.h"
#include "display.h"
#include "bot-logic/bot-logic.h"
#include "rc5-codes.h"
#include "eeprom.h"
#include "math_utils.h"

int16_t speed_l = 0;	/*!< Sollgeschwindigkeit linker Motor */
int16_t speed_r = 0;	/*!< Sollgeschwindigkeit rechter Motor */

#ifdef SPEED_CONTROL_AVAILABLE
#ifdef ADJUST_PID_PARAMS
/* PID-Parameter variabel */
int8_t Kp = PID_Kp;	/*!< PID-Parameter proportional */
int8_t Ki = PID_Ki;	/*!< PID-Parameter intergral */
int8_t Kd = PID_Kd;	/*!< PID-Parameter differential */
#else
/* PID-Koeffizienten aus Parametern berechnen */
#define Q0 ( PID_Kp + PID_Kd/PID_Ta)					/*!< PID-Koeffizient Fehler */
#define Q1 (-PID_Kp - 2*PID_Kd/PID_Ta + PID_Ki*PID_Ta)	/*!< PID-Koeffizient letzter Fehler */
#define Q2 ( PID_Kd/PID_Ta)								/*!< PID-Koeffizient vorletzter Fehler */
#endif	// ADJUST_PID_PARAMS

/*! Dividend fuer Umrechnung von Ticks [176 us] in Geschwindigkeit [mm/s] */
#define TICKS_TO_SPEED		(uint16_t)((float)WHEEL_PERIMETER/ENCODER_MARKS*1000000/TIMER_STEPS)	// = 8475*2
#define TICKS_TO_SPEED_0	(TICKS_TO_SPEED / 2)		/*!< Dividend fuer shift == 0 */
#define TICKS_TO_SPEED_1	(TICKS_TO_SPEED / 2 * 2)	/*!< Dividend fuer shift == 1 */
#define TICKS_TO_SPEED_2	(TICKS_TO_SPEED / 2 * 4) 	/*!< Dividend fuer shift == 2 */

/*! Typ fuer PWM-Lookup-Werte */
typedef struct {
	uint8_t pwm;	/*!< PWM-Wert/2 */
	uint8_t rating;	/*!< Qualitaet des PWM-Werts */
} pwmMap_t;

static uint8_t encoderTargetRate[2] = {0,0};	/*!< Fuehrungsgroesse absolut [0; 256] */
static uint8_t start_signal[2] = {0,0};
static volatile pwmMap_t pwm_values[4] = {{0,255},{0,255},{0,255},{0,255}};		/*!< Lookup fuer Zuordnung GeschwindigkeitSLOW <-> PWM */
#ifdef DISPLAY_REGELUNG_AVAILABLE
uint8_t encoderRateInfo[2];		/*!< Puffer fuer Displayausgabe der Ist-Geschwindigkeit */
#endif

#endif // SPEED_CONTROL_AVAILABLE

/* EEPROM-Variable immer deklarieren, damit die Adresse sich nicht veraendert je nach #define */
uint8_t EEPROM pwmSlow[4] = {255, 255, 255, 255};	/*!< EEPROM-Kopie von pwm_values */

direction_t direction;		/*!< Drehrichtung der Motoren */

#ifdef SPEED_CONTROL_AVAILABLE
/**
 * \brief Drehzahlregelung fuer die Motoren des c't-Bots
 * \param dev		0: linker Motor, 1: rechter Motor
 * \param actVar	Zeiger auf Stellgroesse (nicht volatile, da Aufruf aus ISR heraus)
 * \param encTime	Zeiger auf Encodertimestamps, mit denen gerechnet werden soll
 * \param i_time	Index des aktuellen Timestamps in encTime
 * \param enc		Encoder-Pegel (binaer) von dev
 *
 * Drehzahlregelung sorgt fuer konstante Drehzahl und somit annaehernd Geradeauslauf.
 * Feintuning von PID_Kp bis PID_SPEED_THRESHOLD (bot-local.h) verbessert die Genauigkeit und Schnelligkeit der Regelung.
 * Mit PWMMIN, PWMSTART_L und PWMSTART_R laesst sich der Minimal- bzw. Startwert fuer die Motoren anpassen.
 */
void speed_control(uint8_t dev, int16_t * actVar, uint16_t * encTime, uint8_t i_time, uint8_t enc) {
	/* Speicher fuer alte Regeldifferenzen */
	static int16_t lastErr[2] = {0,0};
	static int16_t last2Err[2] = {0,0};
	static uint8_t orignalTargetRate[2] = {0,0};

	if (encoderTargetRate[dev] == 0) {
		/* Fuer Stopp einfach alles nullsetzen */
		*actVar = 0;
		lastErr[dev] = 0;
		last2Err[dev] = 0;
	} else {
		/* Zeitdifferenz zwischen aktueller und ([vor- | 4.-])letzter Encoderflanke berechnen [176 us] */
		register uint8_t * p_time = (uint8_t *) encTime;
		uint16_t ticks_to_speed;
		uint16_t dt = *(uint16_t *) (p_time + i_time);	// aktueller Timestamp
		int8_t enc_correct = 0;

		/* Beim ersten Aufruf mit neuem Speed beruecksichtigen wir die Beschleunigung */
		if (start_signal[dev] == PID_START_DELAY) {
			orignalTargetRate[dev] = encoderTargetRate[dev];	// Zielgeschwindigkeit merken
			encoderTargetRate[dev] = BOT_SPEED_SLOW / 2;	// laaaangsam anfahren

#ifdef SPEED_LOG_AVAILABLE
			/* Daten loggen */
			register uint8_t index = slog_i[dev];
			if (index < 24) {
				slog->data[dev][index].encRate = 1; // Regelgroesse
				slog->data[dev][index].err = encoderTargetRate[dev]; // Regeldifferenz
				slog->data[dev][index].pwm = *actVar; // Stellgroesse
				slog->data[dev][index].targetRate = encoderTargetRate[dev]; // Fuehrungsgroesse
				slog->data[dev][index++].time = tickCount.u32; // Timestamp
				slog_i[dev] = (uint8_t) (index > 24 ? 0 : index); // Z/25Z
			}
#endif // SPEED_LOG_AVAILABLE

		} else { // 1. Aufruf => es gibt noch keinen korrekten Timestamp in der Vergangenheit => bis zum 2. Aufruf nix tun
			if (encoderTargetRate[dev] >= PID_SPEED_THRESHOLD) {
				i_time = (uint8_t) (i_time - 4 * sizeof(encTime[0])); // Index 4.letzter Timestamp
				ticks_to_speed = TICKS_TO_SPEED_2;
			} else if (encoderTargetRate[dev] < PID_SPEED_THRESHOLD / 2) {
				i_time = (uint8_t) (i_time - sizeof(encTime[0])); // Index letzter Timestamp
				ticks_to_speed = TICKS_TO_SPEED_0;
				/* Regelgroesse korrigieren, wenn mit jeder Encoderflanke gerechnet wurde */
				enc_correct = dev == 0 ? ENC_CORRECT_L : ENC_CORRECT_R;
				if (enc == 1)  {
					enc_correct = (int8_t) -enc_correct;
				}
			} else {
				i_time = (uint8_t) (i_time - 2 * sizeof(encTime[0])); // Index vorletzter Timestamp
				ticks_to_speed = TICKS_TO_SPEED_1;
			}
			i_time = (uint8_t) (i_time & 0xf);						// Index auf 4 Bit begrenzen
			dt -=  *(uint16_t *) (p_time + i_time);	// gewaehlten Timestamp subtrahieren

			/* Bei Fahrt Regelgroesse berechnen */
			uint8_t encoderRate = (uint8_t) (ticks_to_speed / dt); // <dt> = [37; 800] -> <encoderRate> = [229; 10]
			if (encoderRate > 6) {
				encoderRate = (uint8_t) (encoderRate + enc_correct);
			}
			/* Regeldifferenz berechnen */
			int16_t err = (encoderTargetRate[dev] - encoderRate);
			int16_t diff;
#ifdef ADJUST_PID_PARAMS
			/* PID-Koeffizienten berechnen, falls PID-Parameter variabel */
			int16_t q0 = Kp + Kd / PID_Ta;
			int16_t q1 = -Kp - 2 * Kd / PID_Ta + Ki * PID_Ta;
			int16_t q2 = Kd / PID_Ta;

		/* Stellgroesse mit PID-Reglergleichung berechnen */
			diff = (q0 * err + q1 * lastErr[dev] + q2 * last2Err[dev]) >> PID_SHIFT;
#else
			diff = (Q0 * err + Q1 * lastErr[dev] + Q2 * last2Err[dev]) >> PID_SHIFT;
#endif // ADJUST_PID_PARAMS
			*actVar += diff;

			/* berechnete Stellgroesse auf zulaessige Werte begrenzen */
			if (*actVar > PWMMAX) *actVar = PWMMAX;
			else if (*actVar < PWMMIN) *actVar = PWMMIN;

			/* PWM-Lookup updaten */
			if (diff != 0 && encoderTargetRate[dev] == BOT_SPEED_SLOW/2 && start_signal[dev] == 0) {
				uint8_t lastErrors = (uint8_t) (abs(last2Err[dev]) + abs(lastErr[dev]) + abs(err));
				uint8_t turn = 0;
				if (direction.left != direction.right) turn = 2;
				if (pwm_values[dev + turn].rating >= lastErrors) {
					pwm_values[dev + turn].pwm = (uint8_t) (*actVar >> 1);
					pwm_values[dev + turn].rating = lastErrors;
				}
			}

			/* Regeldifferenzen speichern */
			last2Err[dev] = lastErr[dev];
			lastErr[dev]  = err;

			/* Regelgroesse speichern, falls Displayausgabe gewuenscht */
#ifdef DISPLAY_REGELUNG_AVAILABLE
			encoderRateInfo[dev] = encoderRate;
#endif

#ifdef SPEED_LOG_AVAILABLE
			/* Daten loggen */
			register uint8_t index = slog_i[dev];
			if (index < 24) {
				slog->data[dev][index].encRate = encoderRate; // Regelgroesse
				slog->data[dev][index].err = err; // Regeldifferenz
				slog->data[dev][index].pwm = *actVar; // Stellgroesse
				slog->data[dev][index].targetRate = encoderTargetRate[dev]; // Fuehrungsgroesse
				slog->data[dev][index++].time = tickCount.u32; // Timestamp
				slog_i[dev] = (uint8_t) (index > 24 ? 0 : index); // Z/25Z
			}
#endif // SPEED_LOG_AVAILABLE
		}

		if (start_signal[dev] > 0) {
			start_signal[dev]--;
			/* langsam beschleunigen - eigentlich muesste man das sinusartig tun, aber das ist zu aufwendig
			 * Verbesserungsmoeglichkeit: Faktoren bei START_DELAY optimieren (=> Sinus) */
			if (start_signal[dev] == (uint8_t) (PID_START_DELAY * 0.75f)) {
				encoderTargetRate[dev] = (uint8_t) (encoderTargetRate[dev] + ((orignalTargetRate[dev] - BOT_SPEED_SLOW / 2) >> 2)); // +1/4
			} else if (start_signal[dev] == (uint8_t) (PID_START_DELAY * 0.5f)) {
				encoderTargetRate[dev] = (uint8_t) (encoderTargetRate[dev] + ((orignalTargetRate[dev] - BOT_SPEED_SLOW / 2) >> 2));	// +2/4
			} else if (start_signal[dev] == (uint8_t) (PID_START_DELAY * 0.25f)) {
				encoderTargetRate[dev] = (uint8_t) (encoderTargetRate[dev] + ((orignalTargetRate[dev] - BOT_SPEED_SLOW / 2) >> 2));	// +3/4
			} else if (start_signal[dev] == 0) {
				encoderTargetRate[dev] = orignalTargetRate[dev];
			}
		}
	}
	/* PWM-Wert aktualisieren */
	motor_update(dev);
}

#ifdef DISPLAY_REGELUNG_AVAILABLE
/**
 * \brief Zeigt Debug-Informationen der Motorregelung an.
 *
 * Dargestellt werden pro Moto Ist- / Sollgeschwindigkeit, die Differenz davon, der PWM-Stellwert und die
 * Reglerparameter Kp, Ki und Kd.
 * Die Tasten 1 und 4 veraendern Kp, 2 und 5 veraendern Ki, 3 und 6 veraendern Kd, wenn ADJUST_PID_PARAMS an ist.
 */
void speedcontrol_display(void) {
	if (speed_l != 0) {
		display_cursor(1, 2);
		display_printf("%3u/%3u", encoderRateInfo[0] << 1, abs(speed_l));
#ifndef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
		display_cursor(2, 1);
		display_printf("e = %4d", abs(speed_l) - (encoderRateInfo[0] << 1));
#endif
		display_cursor(3, 1);
		display_printf("L = %4d", motor_left);
	}
	if (speed_r != 0) {
		display_cursor(1, 12);
		display_printf("%3d/%3d", encoderRateInfo[1] << 1, abs(speed_r));
#ifndef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
		display_cursor(2, 11);
		display_printf("e = %4d", abs(speed_r) - (encoderRateInfo[1] << 1));
#endif
		display_cursor(3, 11);
		display_printf("R = %4d", motor_right);
	}
#ifdef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
	display_cursor(2, 1);
	display_printf("ETE = %3u Minuten", cal_pid_ete / 60);
#endif
	display_cursor(4, 1);
#ifdef ADJUST_PID_PARAMS
	display_printf("Kp=%3d Ki=%3d Kd=%3d", Kp, Ki, Kd);
#else
	display_printf("Kp=%3d Ki=%3d Kd=%3d", PID_Kp, PID_Ki, PID_Kd);
#endif // ADJUST_PID_PARAMS

	/* Keyhandler */
	switch (RC5_Code) {
		/* PWM-Parameter einstellbar */
#ifdef ADJUST_PID_PARAMS
		case RC5_CODE_1: Kp++; RC5_Code = 0; break;
		case RC5_CODE_4: Kp--; RC5_Code = 0; break;
		case RC5_CODE_2: Ki++; RC5_Code = 0; break;
		case RC5_CODE_5: Ki--; RC5_Code = 0; break;
		case RC5_CODE_3: Kd++; RC5_Code = 0; break;
		case RC5_CODE_6: Kd--; RC5_Code = 0; break;
#endif // ADJUST_PID_PARAMS

		case RC5_CODE_7:
			target_speed_l = BOT_SPEED_FOLLOW; target_speed_r = BOT_SPEED_FOLLOW; RC5_Code = 0; break;
		case RC5_CODE_8:
			target_speed_l = BOT_SPEED_MEDIUM; target_speed_r = BOT_SPEED_MEDIUM; RC5_Code = 0; break;
		case RC5_CODE_9:
			target_speed_l = BOT_SPEED_FAST; target_speed_r = BOT_SPEED_FAST; RC5_Code = 0; break;
	}
}
#endif // DISPLAY_REGELUNG_AVAILABLE
#endif // SPEED_CONTROL_AVAILABLE

/**
 * \brief Direkter Zugriff auf den Motor
 * \param left	Geschwindigkeit fuer den linken Motor
 * \param right	Geschwindigkeit fuer den rechten Motor
 *
 * Geschwindigkeit liegt zwischen -450 und +450. 0 bedeutet Stillstand, 450 volle Kraft voraus, -450 volle Kraft zurueck.
 * Sinnvoll ist die Verwendung der Konstanten: BOT_SPEED_XXX, also z.B. motor_set(BOT_SPEED_SLOW,-BOT_SPEED_SLOW) fuer eine langsame Drehung
 */
void motor_set(int16_t left, int16_t right) {
	/* Drehrichtung fuer beide Motoren ermitteln */
	int8_t speedSignLeft = 1; // 1: vor; -1: zurueck
	if (left < 0) {
		speedSignLeft = -1;
		left = -left;	// Richtung zum Rechnen zunaechst verwerfen
	}
	int8_t speedSignRight = 1;
	if (right < 0) {
		speedSignRight = -1;
		right = -right;
	}
	/* Geschwindigkeiten pruefen und begrenzen */
	if (left == BOT_SPEED_IGNORE) left = BOT_SPEED_STOP;
	else if (left > BOT_SPEED_MAX) left = BOT_SPEED_MAX;
	else if (left != 0 && left < BOT_SPEED_MIN) left = BOT_SPEED_MIN;

	if (right == BOT_SPEED_IGNORE) right = BOT_SPEED_STOP;
	else if (right > BOT_SPEED_MAX) right = BOT_SPEED_MAX;
	else if (right != 0 && right < BOT_SPEED_MIN) right = BOT_SPEED_MIN;

	/* Stellwerte und Zielgeschwindigkeit setzen */
#ifdef SPEED_CONTROL_AVAILABLE
	/* Bei aktivierter Motorregelung neue Fuehrungsgroesse links setzen */
	if (speed_l != left * speedSignLeft && (start_signal[0] == 0 || left == 0 || sign16(speed_l) != speedSignLeft)) {
		if (encoderTargetRate[0] == 0) {
			start_signal[0] = PID_START_DELAY;
			if (speedSignLeft == speedSignRight) {
				motor_left = (pwm_values[0].pwm << 1) + (int16_t)(PWMSTART_L * 1.5f); // [0; 511]
			} else {
				motor_left = (pwm_values[2].pwm << 1) + PWMSTART_L;
			}
		}
		encoderTargetRate[0] = (uint8_t) (left >> 1); // [0; 225]
		if ((left >> 1) == 0) {
			motor_left = 0;
			start_signal[0] = 0;
		}

		/* Geschwindigkeit und Richtung links speichern */
		if (left != 0) {
			if (speedSignLeft > 0) {
				direction.left = DIRECTION_FORWARD;
				speed_l = left;
			} else {
				direction.left = DIRECTION_BACKWARD;
				speed_l = -left;
			}
		} else {
			speed_l = 0;
		}
		/* PWM-Wert setzen */
		motor_update(0);

#ifdef SPEED_LOG_AVAILABLE
		/* Daten loggen */
		register uint8_t index = slog_i[0];
		if (index < 24) {
			slog->data[0][index].encRate = 1; // Regelgroesse
			slog->data[0][index].err = 0; // Regeldifferenz
			slog->data[0][index].pwm = 0; // Stellgroesse
			slog->data[0][index].targetRate = encoderTargetRate[0]; // Fuehrungsgroesse
			slog->data[0][index++].time = tickCount.u32; // Timestamp
			slog_i[0] = (uint8_t) (index > 24 ? 0 : index); // Z/25Z
		}
#endif // SPEED_LOG_AVAILABLE
	}
	/* Neue Fuehrungsgroesse rechts setzen */
	if (speed_r != right * speedSignRight && (start_signal[1] == 0 || right == 0 || sign16(speed_r) != speedSignRight)) {
		if (encoderTargetRate[1] == 0) {
			start_signal[1] = PID_START_DELAY;
			if (speedSignLeft == speedSignRight) {
				motor_right = (pwm_values[1].pwm << 1) + (int16_t)(PWMSTART_R * 1.5f); // [0; 511]
			} else {
				motor_right = (pwm_values[3].pwm << 1) + PWMSTART_R;
			}
		}
		encoderTargetRate[1] = (uint8_t) (right >> 1); // [0; 225]
		if ((right >> 1) == 0) {
			motor_right = 0;
			start_signal[1] = 0;
		}

		/* Geschwindigkeit und Richtung rechts speichern */
		if (right != 0) {
			if (speedSignRight > 0) {
				direction.right = DIRECTION_FORWARD;
				speed_r = right;
			} else {
				direction.right = DIRECTION_BACKWARD;
				speed_r = -right;
			}
		} else {
			speed_r = 0;
		}
		motor_update(1);

#ifdef SPEED_LOG_AVAILABLE
		/* Daten loggen */
		register uint8_t index = slog_i[1];
		if (index < 24) {
			slog->data[1][index].encRate = 1; // Regelgroesse
			slog->data[1][index].err = 0; // Regeldifferenz
			slog->data[1][index].pwm = 0; // Stellgroesse
			slog->data[1][index].targetRate = encoderTargetRate[1]; // Fuehrungsgroesse
			slog->data[1][index++].time = tickCount.u32; // Timestamp
			slog_i[1] = (uint8_t) (index > 24 ? 0 : index); // Z/25Z
		}
#endif // SPEED_LOG_AVAILABLE
	}

	/* PWM-Lookup im EEPROM updaten */
	static uint16_t old_pwm_ticks = 0;
	static uint8_t i = 0;	// nachdem wir 1 Byte geschrieben haben, muessten wir 3.3 ms warten,
	if (i != 0 || timer_ms_passed_16(&old_pwm_ticks, 10000)) {	// alle 10 s
		uint8_t tmp = pwm_values[i].pwm;	// darum schreiben wir erst im naechsten Aufruf das 2. Byte ins EEPROM usw. :-)
		ctbot_eeprom_update_byte(&pwmSlow[i], tmp);
		if (++i == 4) {	// alle Daten gesichert => 10 s schlafen
			i = 0;
		}
	}
#else // ! SPEED_CONTROL_AVAILABLE
#ifdef MCU
	/* Geschwindigkeit als PWM-Wert an die Motoren weitergeben */
	if (speedSignLeft > 0) {
		direction.left = DIRECTION_FORWARD;
		speed_l = left;
	} else{
		direction.left = DIRECTION_BACKWARD;
		speed_l = -left;
	}
	int16_t pwm;
	if (left <= BOT_SPEED_NORMAL) {
		pwm = (int16_t)(((float)PWMMAX / (BOT_SPEED_MAX * 4.0f)) * left) + 50;
	} else {
		pwm = (int16_t)(((float)PWMMAX / (BOT_SPEED_MAX * 1.1f)) * left) - 30;
	}
	motor_left = left == 0 ? 0 : pwm;
	if (speedSignRight > 0) {
		direction.right = DIRECTION_FORWARD;
		speed_r = right;
	} else{
		direction.right = DIRECTION_BACKWARD;
		speed_r = -right;
	}
	if (right <= BOT_SPEED_NORMAL) {
		pwm = (int16_t)(((float)PWMMAX / (BOT_SPEED_MAX * 4.0f)) * right) + 50;
	} else {
		pwm = (int16_t)(((float)PWMMAX / (BOT_SPEED_MAX * 1.1f)) * right) - 30;
	}
	motor_right = right == 0 ? 0 : pwm;
	motor_update(0);
	motor_update(1);
#else
	/* PC-Version */
	speed_l = left * speedSignLeft / 2;
	speed_r = right * speedSignRight / 2;
	bot_motor(speed_l, speed_r);
#endif // MCU
#endif // SPEED_CONTROL_AVAILABLE
}

/**
 * Initialisiere den Motorkrams
 */
void motor_init(void) {
#ifdef SPEED_CONTROL_AVAILABLE
	/* links */
	uint8_t tmp = ctbot_eeprom_read_byte(&pwmSlow[0]);
	if (tmp < (511 - (uint16_t) (PWMSTART_L * 1.5f)) / 2) pwm_values[0].pwm = tmp;
	else pwm_values[0].pwm = 0;
	tmp = ctbot_eeprom_read_byte(&pwmSlow[2]);
	if (tmp < (511 - PWMSTART_L) / 2) pwm_values[2].pwm = tmp;
	else pwm_values[2].pwm = 0;
	/* rechts */
	tmp = ctbot_eeprom_read_byte(&pwmSlow[1]);
	if (tmp < (511 - (uint16_t) (PWMSTART_R * 1.5f))/2) pwm_values[1].pwm = tmp;
	else pwm_values[1].pwm = 0;
	tmp = ctbot_eeprom_read_byte(&pwmSlow[3]);
	if (tmp < (511 - PWMSTART_R) / 2) pwm_values[3].pwm = tmp;
	else pwm_values[3].pwm = 0;
#endif // SPEED_CONTROL_AVAILABLE

	motor_low_init();
}

/**
 * \brief Stellt die Servos
 * \param servo	Nummer des Servos
 * \param pos	Zielwert
 *
 * Sinnvolle Werte liegen zwischen DOOR_CLOSE und DOOR_OPEN, oder SERVO_OFF fuer Servo aus
 */
void servo_set(uint8_t servo, uint8_t pos) {
	if ((servo == SERVO1) && (pos != SERVO_OFF)) {
		if (pos < DOOR_CLOSE) pos = DOOR_CLOSE;
		if (pos > DOOR_OPEN) pos = DOOR_OPEN;
	}
	servo_low(servo, pos);
}

#ifdef SPEED_CONTROL_AVAILABLE
	/* Parameter-Wertebereiche pruefen */
	#if PID_Kp > 127 || PID_Kp < 0
		#error PID_Kp out of range [0; 127]!
	#endif
	#if PID_Ki > 127 || PID_Ki < 0
		#error PID_Ki out of range [0; 127]!
	#endif
	#if PID_Kd > 127 || PID_Kd < 0
		#error PID_Kd out of range [0; 127]!
	#endif
	#if PID_Ta > 127 || PID_Ta < 1
		#error PID_Ta out of range [1; 127]!
	#endif
	#if PID_SHIFT > 127 || PID_SHIFT < 0
		#error PID_SHIFT out of range [0; 127]!
	#endif
	#if PID_TIME > 655 || PID_TIME < 0
		#error PID_TIME out of range [0; 655]!
	#endif
	#if PID_SPEED_THRESHOLD > (BOT_SPEED_MAX/2) || PID_SPEED_THRESHOLD < BOT_SPEED_SLOW
		#error PID_SPEED_THRESHOLD out of range [BOT_SPEED_SLOW; BOT_SPEED_MAX/2]!
	#endif
	#if PID_START_DELAY > 255 || PID_START_DELAY < 0
		#error PID_START_DELAY out of range [0; 255]!
	#endif
	#if PWMMAX > 511
		#error PWMMAX out of range [PWMMIN; 511]!
	#endif
	#if PWMMIN >= PWMMAX || PWMMIN < 0
		#error PWMMIN out of range [0; PWMMAX)!
	#endif
	#if PWMMAX <= PWMMIN
		#error PWMMAX out of range (PWMMIN; 511]!
	#endif
	#if PWMSTART_L*3/2 > PWMMAX || PWMSTART_L < PWMMIN
		#error PWMSTART_L out of range [PWMMIN; PWMMAX/1.5]!
	#endif
	#if PWMSTART_R*3/2 > PWMMAX || PWMSTART_R < PWMMIN
		#error PWMSTART_R out of range [PWMMIN; PWMMAX/1.5]!
	#endif
#endif // SPEED_CONTROL_AVAILABLE
