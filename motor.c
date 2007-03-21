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

/*! 
 * @file 	motor.c
 * @brief 	High-Level-Routinen fuer die Motorsteuerung des c't-Bot
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	17.10.2006
 */

//TODO:	* Geschwindkeits-Ist-Daten filtern? (z.B. max. Beschleunigung beruecksichtigen?)
//		* sofortiger Richtungswechsel (nicht über STOP) holprig, wenn |speed2| > |speed1|

#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "ct-Bot.h"
#include "motor.h"
#include "bot-local.h"
#include "motor-low.h"
#include "timer.h"
#include "sensor.h"
#include "sensor-low.h"
#include "display.h"
#include "bot-logic/bot-logik.h"
#include "rc5-codes.h"

#ifdef MCU
	#include <avr/eeprom.h>
#endif

int16 speed_l = 0;	/*!< Sollgeschwindigkeit linker Motor */
int16 speed_r = 0;	/*!< Sollgeschwindigkeit rechter Motor */

#ifdef SPEED_CONTROL_AVAILABLE
	#ifdef ADJUST_PID_PARAMS
		/* PID-Parameter variabel */
		static int8 Kp = PID_Kp;	/*!< PID-Parameter proportional */
		static int8 Ki = PID_Ki;	/*!< PID-Parameter intergral */
		static int8 Kd = PID_Kd;	/*!< PID-Parameter differential */
	#else
		/* PID-Koeffizienten aus Parametern berechnen */
		#define Q0 ( PID_Kp + PID_Kd/PID_Ta)					/*!< PID-Koeffizient Fehler */
		#define Q1 (-PID_Kp - 2*PID_Kd/PID_Ta + PID_Ki*PID_Ta)	/*!< PID-Koeffizient letzter Fehler */
		#define Q2 ( PID_Kd/PID_Ta)								/*!< PID-Koeffizient vorletzter Fehler */
	#endif	// ADJUST_PID_PARAMS
	
	/*!< Dividend fuer Umrechnung von Ticks [176 us] in Geschwindigkeit [mm/s] */
	#define TICKS_TO_SPEED		(uint16)((float)WHEEL_PERIMETER/ENCODER_MARKS*1000000/TIMER_STEPS)	// = 8475*2
	#define TICKS_TO_SPEED_0	(TICKS_TO_SPEED / 2)		// fuer shift == 0
	#define TICKS_TO_SPEED_1	(TICKS_TO_SPEED / 2 * 2)	// fuer shift == 1
	#define TICKS_TO_SPEED_2	(TICKS_TO_SPEED / 2 * 4) 	// fuer shift == 2
	
	/*! Typ fuer PWM-Lookup-Werte */
	typedef struct{
		uint8 pwm;		/*!< PWM-Wert/2 */	
		uint8 rating;	/*!< Qualitaet des PWM-Werts */
	} pwmMap_t;	
		
	static uint8 encoderTargetRate[2] = {0,0};	/*!< Fuehrungsgroesse absolut [0; 256] */
	static uint8 start_signal[2] = {0,0};
	static volatile pwmMap_t pwm_values[4] = {{0,255},{0,255},{0,255},{0,255}};		/*!< Lookup fuer Zuordnung GeschwindigkeitSLOW <-> PWM */
	#ifdef DISPLAY_REGELUNG_AVAILABLE
		static uint8 encoderRateInfo[2];		/*!< Puffer fuer Displayausgabe der Ist-Geschwindigkeit */
//		static uint8 timer_reg1, timer_reg2;
	#endif
	
//	static volatile uint8 acc_test[2];			/*!< nur Testcase */
//	static volatile uint16 acc_test_dt[2];		/*!< nur Testcase */
#endif	// SPEED_CONTROL_AVAILABLE
#ifdef MCU
	/* EEPROM-Variable immer deklarieren, damit die Adresse sich nicht veraendert je nach #define */
	uint8_t __attribute__ ((section (".eeprom"),aligned (1))) pwmSlow[4] = {255, 255, 255, 255};	/*!< EEPROM-Kopie von pwm_values */
#endif
	
direction_t direction;		/*!< Drehrichtung der Motoren */


#ifdef SPEED_CONTROL_AVAILABLE
	/*!
	 * @brief 			Drehzahlregelung fuer die Motoren des c't-Bots
	 * @author 			Timo Sandmann (mail@timosandmann.de)
	 * @date 			17.10.2006
	 * @param dev		0: linker Motor, 1: rechter Motor
	 * @param actVar	Zeiger auf Stellgroesse (nicht volatile, da Aufruf aus ISR heraus)
	 * @param encTime	Zeiger auf Encodertimestamps, mit denen gerechnet werden soll
	 * @param i_time	Index des aktuellen Timestamps in encTime
	 * @param enc		Encoder-Pegel (binaer) von dev
	 * Drehzahlregelung sorgt fuer konstante Drehzahl und somit annaehernd Geradeauslauf.
	 * Feintuning von PID_Kp bis PID_SPEED_THRESHOLD (bot-local.h) verbessert die Genauigkeit und Schnelligkeit der Regelung.
	 * Mit PWMMIN, PWMSTART_L und PWMSTART_R laesst sich der Minimal- bzw. Startwert fuer die Motoren anpassen.
	 */
	void speed_control(uint8 dev, int16* actVar, uint16* encTime, uint8 i_time, uint8 enc){
//		timer_reg1 = TCNT2;
		/* Speicher fuer alte Regeldifferenzen */
		static int16 lastErr[2] = {0,0};
		static int16 last2Err[2] = {0,0};
		static uint8 orignalTargetRate[2] = {0,0};
		
		if (encoderTargetRate[dev] == 0){
			/* Fuer Stopp einfach alles nullsetzen */  
			*actVar = 0;
			lastErr[dev] = 0; 
			last2Err[dev] = 0;
		} else{
			/* Zeitdifferenz zwischen aktueller und ([vor- | 4.-])letzter Encoderflanke berechnen [176 us] */					
			register uint8* p_time = (uint8*)encTime;
			uint16 ticks_to_speed;
			uint16 dt = *(uint16*)(p_time + i_time);	// aktueller Timestamp
			int8 enc_correct = 0;
			
			/* Beim ersten Aufruf mit neuem Speed beruecksichtigen wir die Beschleunigung */
			if (start_signal[dev] == PID_START_DELAY){
				orignalTargetRate[dev] = encoderTargetRate[dev];	// Zielgeschwindigkeit merken
				encoderTargetRate[dev] = BOT_SPEED_SLOW/2;	// laaaangsam anfahren
			} else{	// 1. Aufruf => es gibt noch keinen korrekten Timestamp in der Vergangenheit => bis zum 2. Aufruf nix tun
				if (encoderTargetRate[dev] >= PID_SPEED_THRESHOLD){
					i_time -= 4 * sizeof(encTime[0]);	// Index 4.letzter Timestamp
					ticks_to_speed = TICKS_TO_SPEED_2;
				} else if (encoderTargetRate[dev] < PID_SPEED_THRESHOLD/2){ 
					i_time -= 1 * sizeof(encTime[0]);	// Index letzter Timestamp 
					ticks_to_speed = TICKS_TO_SPEED_0;
					/* Regelgroesse korrigieren, wenn mit jeder Encoderflanke gerechnet wurde */
					enc_correct = dev == 0 ? ENC_CORRECT_L : ENC_CORRECT_R;
					if (enc == 1) enc_correct = -enc_correct;
				} else /*if (encoderTargetRate[dev] < PID_SPEED_THRESHOLD)*/{
					i_time -= 2 * sizeof(encTime[0]);	// Index vorletzter Timestamp
					ticks_to_speed = TICKS_TO_SPEED_1;
				} /*else {
					i_time -= 4 * sizeof(encTime[0]);	// Index 4.letzter Timestamp
					ticks_to_speed = TICKS_TO_SPEED_2;
				}*/
				i_time &= 0xf;						// Index auf 4 Bit begrenzen
				dt -=  *(uint16*)(p_time + i_time);	// gewaehlten Timestamp subtrahieren
				
				/* <testcase> */
//				if (acc_test[dev] == 1){
//					acc_test[dev] = 2;
//					*actVar = PWMMAX;
//					motor_update(dev);
//					return;
//				} else if (acc_test[dev] == 2){
//					acc_test[dev] = 0;
//					acc_test_dt[dev] = dt;
//					*actVar = 0;
//					motor_update(dev);	
//					return;
//				}
				/* </testcase> */
				
				/* Bei Fahrt Regelgroesse berechnen */	
				uint8 encoderRate = ticks_to_speed / dt + enc_correct; // <dt> = [37; 800] -> <encoderRate> = [229; 10]
				/* Regeldifferenz berechnen */	
				int16 err = (encoderTargetRate[dev] - encoderRate); 
	
				#ifdef ADJUST_PID_PARAMS
					/* PID-Koeffizienten berechnen, falls PID-Parameter variabel */ 
					int16 q0 = Kp + Kd/PID_Ta;
					int16 q1 = -Kp - 2*Kd/PID_Ta + Ki*PID_Ta;
					int16 q2 = Kd/PID_Ta;
					
				/* Stellgroesse mit PID-Reglergleichung berechnen */
					*actVar += (q0*err + q1*lastErr[dev] + q2*last2Err[dev]) >> PID_SHIFT;
				#else
					*actVar += (Q0*err + Q1*lastErr[dev] + Q2*last2Err[dev]) >> PID_SHIFT;
				#endif
				
				/* berechnete Stellgroesse auf zulaessige Werte begrenzen */
				if (*actVar > PWMMAX) *actVar = PWMMAX;
				else if (*actVar < PWMMIN) *actVar = PWMMIN; 
			
				/* PWM-Lookup updaten */
				if (encoderTargetRate[dev] == BOT_SPEED_SLOW/2 && start_signal[dev] == 0){
					uint8 lastErrors = (abs(last2Err[dev]) + abs(lastErr[dev]) + abs(err));
					uint8 turn = 0;
					if (direction.left != direction.right) turn = 2;
					if (pwm_values[dev+turn].rating >= lastErrors){
						pwm_values[dev+turn].pwm = *actVar >> 1;
						pwm_values[dev+turn].rating = lastErrors;
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
					register uint8 index = slog_i[dev];	
					slog_data[dev][index].encRate = encoderRate;				// Regelgroesse
					slog_data[dev][index].err = err;							// Regeldifferenz
					slog_data[dev][index].pwm = *actVar;						// Stellgroesse
					slog_data[dev][index].targetRate = encoderTargetRate[dev];	// Fuehrungsgroesse
					slog_data[dev][index++].time = TIMER_GET_TICKCOUNT_32;		// Timestamp
					slog_i[dev] = index > 24 ? 0 : index;	// Z/25Z
					slog_count[dev]++;
				#endif	// SPEED_LOG_AVAILABLE		
			}

			if (start_signal[dev] > 0){
				start_signal[dev]--;
				// TODO: Faktoren bei START_DELAY optimieren (=> Sinus)
				/* langsam beschleunigen - eigentlich muesste man das sinusartig tun, aber das ist zu aufwendig */
				if (start_signal[dev] == (uint8)(PID_START_DELAY*0.75)) encoderTargetRate[dev] += (orignalTargetRate[dev]-BOT_SPEED_SLOW/2) >> 2;	// +1/4
				else if (start_signal[dev] == (uint8)(PID_START_DELAY*0.5)) encoderTargetRate[dev] += (orignalTargetRate[dev]-BOT_SPEED_SLOW/2) >> 2;	// +2/4
				else if (start_signal[dev] == (uint8)(PID_START_DELAY*0.25)) encoderTargetRate[dev] += (orignalTargetRate[dev]-BOT_SPEED_SLOW/2) >> 2;	// +3/4
				else if (start_signal[dev] == 0) encoderTargetRate[dev] = orignalTargetRate[dev];	
			}
		}	
		/* PWM-Wert aktualisieren */
		motor_update(dev);
//		timer_reg2 = TCNT2;
	}
	
// deprecated
//	/*! 
//	 * @brief 			PWM-Wert-Berechnung
//	 * @author 			Timo Sandmann (mail@timosandmann.de)
//	 * @date 			20.10.2006	
//	 * @param map_data 	Zeiger auf einen Lookup-Table-Eintrag
//	 * @param speed 	Soll-Geschwindigkeit (halbiert)
//	 * @return			PWM-Stellwert
//	 * Berechnet einen PWM-Stellwert aus PWM-Lookup-Table und gewuenschter Geschwindigkeit durch lineare Interpolation
//	 */
//	int16 calc_pwm(volatile pwmMap_t* map_data, uint8 speed){
//		if (speed == 0) return 0;
//		int8 index = speed>>4;	// speed ist halbiert
//		volatile pwmMap_t* tmp = map_data + index;
//		int8 tmp_index = index - 1;
//		uint8 tmp_rating;
//		/* Rating vom linken und rechten Nachbarn vergleichen */
//		if (tmp_index >= 0)
//			tmp_rating = (tmp-1)->rating;
//		else
//			tmp_rating = 8;	// gibt links keinen
//		if (tmp_index+2 < 15 && (tmp+1)->rating < tmp_rating){
//			tmp++;	// der rechte Nachbar hat ein besseres Rating
//			tmp_index += 2;
//		} else tmp--;	// der linke Nachbar ist besser
//		/* linear interpolieren */
//		uint16 speed1 = tmp->speed + (tmp_index<<5);
//		uint16 speed2 = map_data[index].speed + (index<<5);
//		//float m = ((float)tmp->pwm - (float)map_data[index].pwm) / ((float)speed1 - (float)speed2);
//		int16 m = ((int16)(tmp->pwm - map_data[index].pwm)<<7) / ((int8)(speed1 - speed2));	// um 7 Bit hochskaliert
//		//float b = tmp->pwm - m*speed1;
//		int8 speed_diff = (speed<<1) - speed1;	// speed ist halbiert
//		//return (int16)(m * (speed<<1) + b) << 1;
//		return ((int16)((speed_diff*m)>>7) + tmp->pwm) << 1;	// pwm in LT ist halbiert
//	}
	
	#ifdef DISPLAY_REGELUNG_AVAILABLE
		/*!
		 * @brief	Zeigt Debug-Informationen der Motorregelung an.
		 * @author 	Timo Sandmann (mail@timosandmann.de)
	 	 * @date 	12.02.2007	 
	 	 * Dargestellt werden pro Moto Ist- / Sollgeschwindigkeit, die Differenz davon, der PWM-Stellwert und die 
	 	 * Reglerparameter Kp, Ki und Kd.
	 	 * Die Tasten 1 und 4 veraendern Kp, 2 und 5 veraendern Ki, 3 und 6 veraendern Kd, wenn ADJUST_PID_PARAMS an ist. 
		 */
		void speedcontrol_display(void){
			if (speed_l != 0){
				display_cursor(1,2);
				display_printf("%3u/%3u",encoderRateInfo[0]<<1,abs(speed_l));
				display_cursor(2,1);
				display_printf("e = %4d",abs(speed_l)-(encoderRateInfo[0]<<1));					
				display_cursor(3,1);
				display_printf("L = %4d",motor_left);				
			}
			if (speed_r != 0){
				display_cursor(1,12);
				display_printf("%3d/%3d",encoderRateInfo[1]<<1,abs(speed_r));
				display_cursor(2,11);
				display_printf("e = %4d",abs(speed_r)-(encoderRateInfo[1]<<1));
				display_cursor(3,11);
				display_printf("L = %4d",motor_right);
			}
			display_cursor(4,1);
			#ifdef ADJUST_PID_PARAMS
				display_printf("Kp=%3d Ki=%3d Kd=%3d",Kp,Ki,Kd);
//				display_printf("%3u us", (timer_reg2-timer_reg1)<<2);
			#else
				display_printf("Kp=%3d Ki=%3d Kd=%3d",PID_Kp,PID_Ki,PID_Kd);
//				display_printf("%3u us", (timer_reg2-timer_reg1)<<2);
			#endif	// ADJUST_PID_PARAMS	
			
			/* Keyhandler */
			switch (RC5_Code){			
				/* PWM-Parameter einstellbar */
				#ifdef ADJUST_PID_PARAMS
					case RC5_CODE_1: Kp++; RC5_Code = 0; break;
					case RC5_CODE_4: Kp--; RC5_Code = 0; break;
					case RC5_CODE_2: Ki++; RC5_Code = 0; break;
					case RC5_CODE_5: Ki--; RC5_Code = 0; break;
					case RC5_CODE_3: Kd++; RC5_Code = 0; break;
					case RC5_CODE_6: Kd--; RC5_Code = 0; break;
				#endif	// ADJUST_PID_PARAMS

				case RC5_CODE_7:
					target_speed_l = BOT_SPEED_FOLLOW; target_speed_r = BOT_SPEED_FOLLOW; RC5_Code = 0; break;
				case RC5_CODE_8:
					target_speed_l = BOT_SPEED_MEDIUM; target_speed_r = BOT_SPEED_MEDIUM; RC5_Code = 0; break;
				case RC5_CODE_9:
					target_speed_l = BOT_SPEED_FAST; target_speed_r = BOT_SPEED_FAST; RC5_Code = 0; break;
			}				
		}
	#endif
#endif

/*!
 * @brief		Direkter Zugriff auf den Motor
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		17.10.2006 
 * @param left	Geschwindigkeit fuer den linken Motor
 * @param right	Geschwindigkeit fuer den linken Motor
 * Geschwindigkeit liegt zwischen -450 und +450. 0 bedeutet Stillstand, 450 volle Kraft voraus, -450 volle Kraft zurueck.
 * Sinnvoll ist die Verwendung der Konstanten: BOT_SPEED_XXX, also z.B. motor_set(BOT_SPEED_SLOW,-BOT_SPEED_SLOW) fuer eine langsame Drehung
 */
void motor_set(int16 left, int16 right){
	/* Drehrichtung fuer beide Motoren ermitteln */
	int8 speedSignLeft = 1;	// 1: vor; -1: zurueck
	if (left < 0){
		speedSignLeft = -1;
		left = -left;	// Richtung zum Rechnen zunaechst verwerfen
	}
	int8 speedSignRight = 1;
	if (right < 0){
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
		if (speed_l != left*speedSignLeft && (start_signal[0] == 0 || left == 0)){
			if (encoderTargetRate[0] == 0){
				start_signal[0] = PID_START_DELAY;
				if (speedSignLeft == speedSignRight) motor_left = (pwm_values[0].pwm << 1) + 150;	// [0; 511]
				else motor_left = (pwm_values[2].pwm << 1) + 100;
			}
			encoderTargetRate[0] = left >> 1;	// [0; 225]
			if ((left>>1) == 0){
				motor_left = 0;
				start_signal[0] = 0;
			}
			
			/* Geschwindigkeit und Richtung links speichern */
			if (speedSignLeft > 0){
				direction.left = DIRECTION_FORWARD;
				speed_l = left;
			} else {
				direction.left = DIRECTION_BACKWARD;
				speed_l = -left;
			}
			/* PWM-Wert setzen */
			motor_update(0);
		}
		/* Neue Fuehrungsgroesse rechts setzen */
		if (speed_r != right*speedSignRight && (start_signal[1] == 0 || right == 0)){
			if (encoderTargetRate[1] == 0){
				start_signal[1] = PID_START_DELAY;
				if (speedSignLeft == speedSignRight) motor_right = (pwm_values[1].pwm << 1) + 150;	// [0; 511]
				else motor_right = (pwm_values[3].pwm << 1) + 100;
			}
			encoderTargetRate[1] = right >> 1;	// [0; 225]
			if ((right>>1) == 0){
				motor_right = 0;
				start_signal[1] = 0;
			}
									
			/* Geschwindigkeit und Richtung rechts speichern */
			if (speedSignRight > 0){
				direction.right = DIRECTION_FORWARD;
				speed_r = right;
			} else {
				direction.right = DIRECTION_BACKWARD;
				speed_r = -right;
			}
			motor_update(1);
		}
		
		/* PWM-Lookup im EEPROM updaten */
		static uint16 old_pwm_ticks = 0;
		register uint16 pwm_ticks = TIMER_GET_TICKCOUNT_16;
		if ((pwm_ticks-old_pwm_ticks) > MS_TO_TICKS(10000L)){	// alle 10 s
			static uint8 i=0;				// nachdem wir 1 Byte geschrieben haben, muessten wir 3.3 ms mit busy-waiting vertroedeln, 
			uint8 tmp = pwm_values[i].pwm;	// darum schreiben wir erst im naechsten Aufruf das 2. Byte ins EEPROM usw. :-)
			if (eeprom_read_byte(&pwmSlow[i]) != tmp) eeprom_write_byte(&pwmSlow[i], tmp);				
			if (++i == 4){	// alle Daten gesichert => 10 s schlafen
				old_pwm_ticks = pwm_ticks;
				i = 0;
			}	
		}
	#else
		#ifdef MCU
			/* Geschwindigkeit als PWM-Wert an die Motoren weitergeben */
			if (speedSignLeft > 0){
				direction.left = DIRECTION_FORWARD;
				speed_l = left;
			} else{
				direction.left = DIRECTION_BACKWARD;
				speed_l = -left;
			}
			uint16 pwm = (float)(PWMMAX/BOT_SPEED_MAX) * left + 10;	// lineare Motorkennline annehmen
			motor_left = left == 0 ? 0 : pwm;
			if (speedSignRight > 0){
				direction.right = DIRECTION_FORWARD;
				speed_r = right;
			} else{
				direction.right = DIRECTION_BACKWARD;
				speed_r = -right;
			}
			pwm = (float)(PWMMAX/BOT_SPEED_MAX) * right + 10;	// lineare Motorkennline annehmen
			motor_right = right == 0 ? 0 : pwm;
			motor_update(0);
			motor_update(1);
		#else
			/* PC-Version */
			speed_l = left * speedSignLeft / 2;
			speed_r = right * speedSignRight / 2;
			bot_motor(speed_l, speed_r);
		#endif // MCU
	#endif	// SPEED_CONTROL_AVAILABLE
}

/*!
 * @brief	Initialisiere den Motorkrams
 */
void motor_init(void){
	#ifdef SPEED_CONTROL_AVAILABLE
		/* links */
		uint8 tmp = eeprom_read_byte(&pwmSlow[0]);
		if (tmp >= PWMSTART_L/2 && tmp < 255) pwm_values[0].pwm = tmp;
		else pwm_values[0].pwm = PWMSTART_L/2;
		tmp = eeprom_read_byte(&pwmSlow[2]);
		if (tmp >= PWMSTART_L/2 && tmp < 255) pwm_values[2].pwm = tmp;
		else pwm_values[2].pwm = PWMSTART_L/2;
		/* rechts */
		tmp = eeprom_read_byte(&pwmSlow[1]);
		if (tmp >= PWMSTART_R/2 && tmp < 255) pwm_values[1].pwm = tmp;
		else pwm_values[1].pwm = PWMSTART_R/2; 
		tmp = eeprom_read_byte(&pwmSlow[3]);
		if (tmp >= PWMSTART_R/2 && tmp < 255) pwm_values[3].pwm = tmp;
		else pwm_values[3].pwm = PWMSTART_R/2;
	#endif	// SPEED_CONTROL_AVAILABLE

	motor_low_init();
}

/*!
 * @brief		Stellt die Servos
 * @param servo	Nummer des Servos
 * @param servo	Zielwert
 * Sinnvolle Werte liegen zwischen 7 und 16, oder 0 fuer Servo aus 
 */
void servo_set(uint8 servo, uint8 pos){
	if ((servo == SERVO1) && (pos != SERVO_OFF)){
		if (pos < DOOR_CLOSE) pos = DOOR_CLOSE;
		if (pos > DOOR_OPEN) pos = DOOR_OPEN;
	}
	servo_low(servo,pos);
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
	#if PID_Ta > 127 || PID_Ta < 0
		#error PID_Ta out of range [0; 127]!
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
	#if PWMSTART_L > PWMMAX || PWMSTART_L < PWMMIN
		#error PWMSTART_L out of range [PWMMIN; PWMMAX]!
	#endif	
	#if PWMSTART_R > PWMMAX || PWMSTART_R < PWMMIN
		#error PWMSTART_R out of range [PWMMIN; PWMMAX]!
	#endif
#endif	// SPEED_CONTROL_AVAILABLE
