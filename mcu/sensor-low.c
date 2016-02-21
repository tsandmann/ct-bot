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
 * \file 	sensor-low.c
 * \brief 	Low-Level Routinen fuer die Sensor Steuerung des c't-Bots
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	01.12.2005
 */

#ifdef MCU

#include "ct-Bot.h"

#include <avr/io.h>
#include <string.h>
#include <math.h>
#include "adc.h"
#include "ena.h"
#include "sensor.h"
#include "mouse.h"
#include "motor.h"
#include "timer.h"
#include "sensor_correction.h"
#include "bot-local.h"
#include "bot-logic/bot-logic.h"
#include "display.h"
#include "led.h"
#include "sensor-low.h"
#include "i2c.h"
#include "ir-rc5.h"
#include "math_utils.h"
#include "srf10.h"
#include "botfs.h"
#include "init.h"

// ADC-PINS
#define SENS_ABST_L		0	/**< ADC-PIN Abstandssensor Links */
#define SENS_ABST_R		1	/**< ADC-PIN Abstandssensor Rechts */
#define SENS_M_L		2	/**< ADC-PIN Liniensensor Links */
#define SENS_M_R		3	/**< ADC-PIN Liniensensor Rechts */
#define SENS_LDR_L		4	/**< ADC-PIN Lichtsensor Links */
#define SENS_LDR_R		5	/**< ADC-PIN Lichtsensor Rechts */
#define SENS_KANTE_L	6	/**< ADC-PIN Kantensensor Links */
#define SENS_KANTE_R	7	/**< ADC-PIN Kantensensor Rechts */

#define DIST_SENS_UPDATE_TIME 50

#ifdef DISTSENS_TYPE_GP2Y0A60SZ
#undef DIST_SENS_UPDATE_TIME
#define DIST_SENS_UPDATE_TIME 0
#endif // DISTSENS_TYPE_GP2Y0A60SZ

// Sonstige Sensoren
#define SENS_DOOR_PINR 	PIND	/**< Port an dem der Klappensensor haengt */
#define SENS_DOOR_DDR 	DDRD	/**< DDR fuer den Klappensensor */
#define SENS_DOOR		6		/**< Pin an dem der Klappensensor haengt */

#ifdef SPI_AVAILABLE
#define SENS_ENCL_PINR	PINC	/**< Port an dem der linke Encoder haengt */
#define SENS_ENCL_DDR	DDRC	/**< DDR fuer den linken Encoder  */
#define SENS_ENCL		5		/**< Pin an dem der linke Encoder haengt */
#else
#define SENS_ENCL_PINR	PINB	/**< Port an dem der linke Encoder haengt */
#define SENS_ENCL_DDR	DDRB	/**< DDR fuer den linken Encoder  */
#define SENS_ENCL		4		/**< Pin an dem der linke Encoder haengt */
#endif	// SPI_AVAILABLE

#define SENS_ENCR_PINR	PIND	/**< Port an dem der rechte Encoder haengt */
#define SENS_ENCR_DDR	DDRD	/**< DDR fuer den rechten Encoder  */
#define SENS_ENCR		3		/**< Pin an dem der rechte Encoder haengt */

#define SENS_ERROR_PINR	PINB	/**< Port an dem die Fehlerueberwachung haengt */
#define SENS_ERROR_DDR	DDRB	/**< DDR fuer die Fehlerueberwachung */
#define SENS_ERROR		2		/**< Pin an dem die Fehlerueberwachung haengt */

#define SENS_TRANS_PINR	PINB	/**< Port an dem die Transportfachueberwachung haengt */
#define SENS_TRANS_PORT	PORTB	/**< Port an dem die Transportfachueberwachung haengt */
#define SENS_TRANS_DDR	DDRB	/**< DDR fuer die Transportfachueberwachung */
#define SENS_TRANS		0		/**< Pin an dem die Transportfachueberwachung haengt */

#define ENC_L ((SENS_ENCL_PINR >> SENS_ENCL) & 0x01)	/**< Abkuerzung zum Zugriff auf Encoder */
#define ENC_R ((SENS_ENCR_PINR >> SENS_ENCR) & 0x01)	/**< Abkuerzung zum Zugriff auf Encoder */

#define ENC_ENTPRELL		12	/**< Nur wenn der Encoder ein paar mal den gleichen wert gibt uebernehmen */

#ifdef SPEED_CONTROL_AVAILABLE
uint16_t encTimeL[8] = {0};		/**< Timestamps linker Encoder */
uint16_t encTimeR[8] = {0};		/**< Timestamps rechter Encoder */
uint8_t i_encTimeL = 0;			/**< Array-Index auf letzten Timestampeintrag links */
uint8_t i_encTimeR = 0;			/**< Array-Index auf letzten Timestampeintrag rechts */
uint8_t timeCorrectL = 0;		/**< markiert, ob der Encoder-Timestamp des linken Rads ungueltig ist (wg. Stillstand) */
uint8_t timeCorrectR = 0;		/**< markiert, ob der Encoder-Timestamp des rechten Rads ungueltig ist (wg. Stillstand) */
#endif // SPEED_CONTROL_AVAILABLE

#ifdef SPEED_LOG_AVAILABLE
/* Debug-Loggings */
volatile uint8_t slog_i[2] = {0,0}; /**< Array-Index */
slog_t * const slog = &GET_MMC_BUFFER(speedlog);	/**< Puffer fuer Speed-Log Daten */
static botfs_file_descr_t speedlog_file;			/**< BotFS-Datei fuer das Speed-Log */
#define SPEEDLOG_FILE_SIZE (1024 * (1024 / BOTFS_BLOCK_SIZE)) /**< Groesse der Speed-Log-Datei in Bloecken */
#endif // SPEED_LOG_AVAILABLE

#define FILTER_SHIFT 3U
static uint16_t filter_l, filter_r;

#if 0
#define FILTER_SIZE (sizeof(dist_fir_coeffs) / sizeof(dist_fir_coeffs[0]))

static const float dist_fir_coeffs[] = { 4.f / 64.f, 0.f, -7.f / 64.f, 0.f, 20.f / 64.f, 31.f / 64.f, 20.f / 64.f, 0.f, -7.f / 64.f, 0.f, 4.f / 64.f };
static float dist_fir_buffer_l[FILTER_SIZE] = { 0.f };
static float* dist_fir_position_l = dist_fir_buffer_l;
static float dist_fir_buffer_r[FILTER_SIZE] = { 0.f };
static float* dist_fir_position_r = dist_fir_buffer_r;

static float fir_filter(float* fir_buffer, float** fir_pos, float new_value) {
	const float* p_coeff = dist_fir_coeffs;
	float* p_buffer = *fir_pos;
	float result = 0;
	**fir_pos = new_value;

	while (p_buffer < fir_buffer + FILTER_SIZE) {
		result += (*p_coeff) * (*p_buffer);
		++p_coeff;
		++p_buffer;
	}

	p_buffer = fir_buffer;
	while (p_coeff < dist_fir_coeffs + FILTER_SIZE) {
		result += (*p_coeff) * (*p_buffer);
		++p_coeff;
		++p_buffer;
	}

	if (--*fir_pos < fir_buffer) {
		*fir_pos = fir_buffer + FILTER_SIZE - 1;
	}

	return result;
}
#endif // 0


/**
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void) {
#if defined BPS_AVAILABLE && BPS_PIN == 4
	adc_init(0xff & ~_BV(BPS_PIN)); // ADC-Ports (ausser BPS-Pin) aktivieren
#else
	adc_init(0xff); // Alle ADC-Ports aktivieren
#endif // BPS_AVAILABLE

	ENA_set(ENA_RADLED | ENA_ABSTAND); // Alle Sensoren bis auf Radencoder & Abstandssensoren deaktivieren

	SENS_DOOR_DDR = (uint8_t) (SENS_DOOR_DDR & ~_BV(SENS_DOOR)); // Input

	SENS_ERROR_DDR = (uint8_t) (SENS_ERROR_DDR & ~_BV(SENS_ERROR)); // Input

	SENS_TRANS_DDR = (uint8_t) (SENS_TRANS_DDR & ~_BV(SENS_TRANS)); // Input
	SENS_TRANS_PORT	|= (1 << SENS_TRANS); // Pullup an

	SENS_ENCL_DDR = (uint8_t) (SENS_ENCL_DDR & ~_BV(SENS_ENCL)); // Input
	SENS_ENCR_DDR = (uint8_t) (SENS_ENCR_DDR & ~_BV(SENS_ENCR)); // Input

	/* Reset Encoderstaende */
	sensEncL = 0;
	sensEncR = 0;

#ifdef SPEED_LOG_AVAILABLE
	void * const buffer = slog;
	/* Datei oeffnen / anlegen */
	int8_t res;
	if ((res = botfs_open(SPEEDLOG_FILE_NAME, &speedlog_file, BOTFS_MODE_W, buffer)) != 0) {
		if (res == -1) {
			botfs_create(SPEEDLOG_FILE_NAME, SPEEDLOG_FILE_SIZE, 0, buffer);
			if (botfs_open(SPEEDLOG_FILE_NAME, &speedlog_file, BOTFS_MODE_W, buffer) != 0) {
				return;
			}
		}
	}

	/* Typ in den Header schreiben (mit oder ohne Motorregelung) */
	uint8_t * header;
	if (botfs_read_header_data(&speedlog_file, &header, buffer) == 0) {
#ifdef SPEED_CONTROL_AVAILABLE
		*header = SLOG_WITH_SPEED_CONTROL;
#else // ! SPEED_CONTROL_AVAILABLE
		*header = SLOG_WITHOUT_SPEED_CONTROL;
#endif // SPEED_CONTROL_AVAILABLE
		botfs_write_header_data(&speedlog_file, buffer);
	}
	memset(buffer, 0, sizeof(slog));
#endif // SPEED_LOG_AVAILABLE
}

/**
 * Alle Sensoren aktualisieren
 */
void bot_sens(void) {
	ENA_on(ENA_KANTLED | ENA_LINE | ENA_SCHRANKE | ENA_KLAPPLED); // Die Distanzsensoren sind im Normalfall an, da sie 50 ms zum Booten brauchen

#ifdef CMPS03_AVAILABLE
	cmps03_get_bearing(&sensCmps03);
#endif

	/* aktualisiere Distanz-Sensoren, interrupt-driven I/O */
#ifdef DISTSENS_AVERAGE
	static uint8_t measure_count = 0;
	static int16_t distLeft[4];
	static int16_t distRight[4];
#endif // DISTSENS_AVERAGE

	static uint16_t old_dist; // Zeit der letzten Messung der Distanzsensoren

	/* Auswertung der Distanzsensoren alle DIST_SENS_UPDATE_TIME ms */
	uint16_t dist_ticks = TIMER_GET_TICKCOUNT_16;

	if ((uint16_t)(dist_ticks - old_dist) > MS_TO_TICKS(DIST_SENS_UPDATE_TIME)) {
		int16_t * pDistL, * pDistR;
#ifdef DISTSENS_AVERAGE
		pDistL = &distLeft[measure_count];
		pDistR = &distRight[measure_count];
#else
		pDistL = &sensDistL;
		pDistR = &sensDistR;
#endif // DISTSENS_AVERAGE
		adc_read_int(SENS_ABST_L, pDistL);
		if (servo_get(SERVO1) == SERVO_OFF) { // wenn die Transportfachklappe bewegt wird, stimmt der Messwert des rechten Sensors nicht
			adc_read_int(SENS_ABST_R, pDistR);
		}
#ifdef DISTSENS_AVERAGE
		measure_count++;
		measure_count &= 0x3; // Z/4Z
#endif
	}

	/* die anderen analogen Sensoren, auch int-driven I/O */
	adc_read_int(SENS_M_L, &sensLineL);
	adc_read_int(SENS_M_R, &sensLineR);

#if ! defined BPS_AVAILABLE || BPS_PIN != 4
	adc_read_int(SENS_LDR_L, &sensLDRL);
#endif
	adc_read_int(SENS_LDR_R, &sensLDRR);

	adc_read_int(SENS_KANTE_L, &sensBorderL);
	adc_read_int(SENS_KANTE_R, &sensBorderR);

#ifdef MOUSE_AVAILABLE
	// Aktualisiere die Position des Maussensors
	sensMouseDX = (int8_t) mouse_sens_read(MOUSE_DELTA_X_REG);
	sensMouseDY = (int8_t) mouse_sens_read(MOUSE_DELTA_Y_REG);
#endif

#ifdef SPEED_CONTROL_AVAILABLE
	/* Aufruf der Motorregler, falls Stillstand */
	register uint16_t pid_ticks = TIMER_GET_TICKCOUNT_16; // Ticks sichern [178 us]
	register uint8_t i_time;
	register uint8_t * p_time;
	/* Index auf Encodertimestamps zwischenspeichern */
	i_time = i_encTimeL;
	p_time = (uint8_t *) encTimeL;
	/* Bei Stillstand Regleraufruf links nach PID_TIME ms */
	if (pid_ticks - *(uint16_t *)(p_time + i_time) > PID_TIME * 50 / TIMER_STEPS * 20) {
		/* Timestamp links verschieben / speichern */
		i_time = (uint8_t) ((i_time + sizeof(encTimeL[0])) & 0xf); // encTime ist Z/8Z und jeder Eintrag hat 2 Byte => 0xf
		*(uint16_t *)(p_time + i_time) = pid_ticks;
		i_encTimeL = i_time;
		/* Regleraufruf */
		timeCorrectL = 1;
		speed_control(0,  &motor_left, (uint16_t *) encTimeL, i_encTimeL, 0);
	}
	/* Bei Stillstand Regleraufruf rechts nach PID_TIME ms */
	i_time = i_encTimeR;
	p_time = (uint8_t *) encTimeR;
	if (pid_ticks - *(uint16_t *)(p_time + i_time) > PID_TIME * 50 / TIMER_STEPS * 20) {
		/* Timestamp rechts verschieben / speichern */
		i_time = (uint8_t) ((i_time + sizeof(encTimeR[0])) & 0xf); // encTime ist Z/8Z und jeder Eintrag hat 2 Byte => 0xf
		*(uint16_t *)(p_time + i_time) = pid_ticks;
		i_encTimeR = i_time;
		/* Regleraufruf rechts */
		timeCorrectR = 1;
		speed_control(1, &motor_right, (uint16_t *) encTimeR, i_encTimeR, 0);
	}
#endif // SPEED_CONTROL_AVAILABLE

#ifdef SPEED_LOG_AVAILABLE
	/* Speed-Log-Daten auf MMC schreiben, falls Puffer voll */
	if (slog_i[0] > 20 || slog_i[1] > 20) { // etwas Luft lassen, denn die Daten kommen per ISR
		const size_t n = sizeof(slog->data[0]) / sizeof(slog->data[0][0]);
		uint8_t j;
		for (j = 0; j < 2; ++j) {
			const uint8_t i = (uint8_t) (slog_i[j] + 1);
			slog_i[j] = 0; // Index-Reset
			const uint16_t length = mul8(sizeof(slog_data_t), (uint8_t) (n - i));
			memset((uint8_t *) &slog->data[j][i], 0, length);
		}
		botfs_write(&speedlog_file, slog->data);
	}
#endif // SPEED_LOG_AVAILABLE

#ifdef BPS_AVAILABLE
	uint16_t tmp = ir_read(&bps_ir_data);
	sensBPS = tmp != BPS_NO_DATA ? tmp & 0xf : BPS_NO_DATA; // untere 4 Bit
#endif // BPS_AVAILABLE

	sensor_update(); // Weiterverarbeitung der rohen Sensordaten

	if ((uint16_t) (dist_ticks - old_dist) > MS_TO_TICKS(DIST_SENS_UPDATE_TIME)) {
		old_dist = dist_ticks; // Zeit fuer naechste Messung merken
		/* Dist-Sensor links */
		while (adc_get_active_channel() < 1) {}
		uint16_t volt;
#ifdef DISTSENS_AVERAGE
		volt = (distLeft[0] + distLeft[1] + distLeft[2] + distLeft[3]) >> 2;
#else
//		volt = (uint16_t) sensDistL;
//		volt = (uint16_t) fir_filter(dist_fir_buffer_l, &dist_fir_position_l, (float) sensDistL);

		filter_l = filter_l - (filter_l >> FILTER_SHIFT) + (uint16_t) sensDistL;
		volt = (uint16_t) (filter_l >> FILTER_SHIFT);
#endif
		(*sensor_update_distance)(&sensDistL, &sensDistLToggle, sensDistDataL, volt);

		if (servo_get(SERVO1) == SERVO_OFF) {
			/* Dist-Sensor rechts */
			while (adc_get_active_channel() < 2) {}
#ifdef DISTSENS_AVERAGE
			volt = (distRight[0] + distRight[1] + distRight[2] + distRight[3]) >> 2;
#else
//			volt = (uint16_t) sensDistR;
//			volt = (uint16_t) fir_filter(dist_fir_buffer_r, &dist_fir_position_r, (float) sensDistR);

			filter_r = filter_r - (filter_r >> FILTER_SHIFT) + (uint16_t) sensDistR;
			volt = (uint16_t) (filter_r >> FILTER_SHIFT);
#endif
			(*sensor_update_distance)(&sensDistR, &sensDistRToggle, sensDistDataR, volt);
		}
	}

#ifdef CMPS03_AVAILABLE
	cmps03_finish(&sensCmps03);
	heading_10_int = sensCmps03.bearing;
	heading_int = heading_10_int / 10;
	heading = (float) sensCmps03.bearing / 10.0f;
	const float h = rad(heading);
	heading_sin = sinf(h);
	heading_cos = cosf(h);
#endif // CMPS03_AVAILABLE

#ifdef SRF10_AVAILABLE
	static uint16_t srf_time = 0;
	if (timer_ms_passed_16(&srf_time, 250)) {
		sensSRF10 = srf10_get_measure(); // Messung Ultraschallsensor
	}
#endif // SRF10_AVAILABLE

	/* alle anderen analogen Sensoren */
	while (adc_get_active_channel() != 255) {} // restliche Zeit verbrauchen

	/* alle digitalen Sensoren */
	sensDoor = (uint8_t) ((SENS_DOOR_PINR >> SENS_DOOR) & 0x01);
	sensTrans = (uint8_t) ((SENS_TRANS_PINR >> SENS_TRANS) & 0x01);
	sensError = (uint8_t) ((SENS_ERROR_PINR >> SENS_ERROR) & 0x01);

#ifndef BEHAVIOUR_HW_TEST_AVAILABLE
	// Kanten (ENA_KANTLED), Liniensensoren (ENA_LINE), Transportfach-LED und Klappensensor aus, nur in den Testmodi bleibt immer alles an
  	ENA_off(ENA_KANTLED | ENA_LINE | ENA_SCHRANKE | ENA_KLAPPLED);
#endif

#ifndef BOT_2_RPI_AVAILABLE
	/* LEDs updaten */
	led_update();
#endif // BOT_2_RPI_AVAILABLE
#ifdef LED_AVAILABLE
	/* Sollen die LEDs mit den Rohdaten der Sensoren arbeiten,
	 * kommentiert man die folgenden Zeilen ein */

	//if (voltL > 80) LED_on(LED_LINKS);
	//else LED_off(LED_LINKS);
	//if (voltR > 80) LED_on(LED_RECHTS);
	//else LED_off(LED_RECHTS);
#endif // LED_AVAILABLE
}

/**
 * \brief Kuemmert sich um die Radencoder
 *
 * Das muss schneller gehen als die anderen Sensoren,
 * daher Update per Timer-Interrupt und nicht ueber sensor_update() und
 * die Bot-Hauptschleife.
 */
void bot_encoder_isr(void) {
	static uint8_t enc_l = 0; // Puffer fuer die letzte Encoder-Staende
	static uint8_t enc_r = 0; // Puffer fuer die letzte Encoder-Staende
	static uint8_t enc_l_cnt = 0; // Entprell-Counter fuer L-Encoder
	static uint8_t enc_r_cnt = 0; // Entprell-Counter fuer R-Encoder
	register uint8_t enc_tmp; // Pegel der Encoderpins im Register zwischenspeichern

#ifdef SPEED_CONTROL_AVAILABLE
	register uint16_t ticks = tickCount.u16;	// aktuelle Systemzeit zwischenspeichern
	register uint8_t i_time;					// Index des Timestamparrays zwischenspeichern
#endif // SPEED_CONTROL_AVAILABLE
	/* Rad-Encoder links */
	enc_tmp = (uint8_t) ENC_L;
	if (enc_tmp != enc_l) { // uns interessieren nur Veraenderungen
		enc_l = enc_tmp; // neuen Wert sichern
		enc_l_cnt = 0; // Counter zuruecksetzen
	} else { // zaehlen, wie lange Pegel bleibt
		if (enc_l_cnt < ENC_ENTPRELL) { // Nur bis zur Entprell-Marke
			enc_l_cnt++;
		} else if (enc_l_cnt == ENC_ENTPRELL) { // wenn lange genug konstant
			enc_l_cnt++; // diese Flanke nur einmal auswerten
			if (direction.left == DIRECTION_FORWARD) { // Drehrichtung beachten
				sensEncL++;	// vorwaerts
			} else {
				sensEncL--;	// rueckwaerts
			}
#ifdef SPEED_CONTROL_AVAILABLE
			/* Timestamps fuer Regler links verschieben und speichern */
			i_time = (uint8_t) ((i_encTimeL + sizeof(encTimeL[0])) & 0xf); // encTime ist Z/8Z und jeder Eintrag hat 2 Byte => 0xf
			*(uint16_t *)((uint8_t *) encTimeL + i_time) = ticks;
			i_encTimeL = i_time;
			/* Regleraufruf links */
			if (timeCorrectL == 0) {
				speed_control(0, &motor_left, (uint16_t *) encTimeL, i_encTimeL, enc_tmp);
			} else {
				timeCorrectL = 0;
			}
			/* pro TIMER_STEP wird maximal ein Encoder ausgewertet, da max alle 6 ms (Fullspeed) eine Flanke kommen kann */
			return; // hackhack
#else // ! SPEED_CONTROL_AVAILABLE
#ifdef SPEED_LOG_AVAILABLE
			uint8_t index = slog_i[0];
			slog->data[0][index].pwm = motor_left;
			slog->data[0][index].time = tickCount.u32;
			index++;
			slog_i[0] = (uint8_t) (index > 24 ? 0 : index); // Z/25Z
#endif // SPEED_LOG_AVAILABLE
#endif // SPEED_CONTROL_AVAILABLE
		}
	}

	/* Rad-Encoder rechts */
	enc_tmp = (uint8_t) ENC_R;
	if (enc_tmp != enc_r) { // uns interessieren nur Veraenderungen
		enc_r = enc_tmp; // neuen Wert sichern
		enc_r_cnt=0; // Counter zuruecksetzen
	} else { // zaehlen, wie lange Pegel bleibt
		if (enc_r_cnt < ENC_ENTPRELL) { // nur bis zur Entprell-Marke
			enc_r_cnt++;
		} else if (enc_r_cnt == ENC_ENTPRELL) { // wenn lange genug konstant
			enc_r_cnt++; // diese Flanke nur einmal auswerten
			if (direction.right == DIRECTION_FORWARD) { // Drehrichtung beachten
				sensEncR++; // vorwaerts
			} else {
				sensEncR--; // rueckwaerts
			}
#ifdef SPEED_CONTROL_AVAILABLE
			/* Timestamps fuer Regler rechts verschieben und speichern */
			i_time = (uint8_t) ((i_encTimeR + sizeof(encTimeR[0])) & 0xf); // encTime ist Z/8Z und jeder Eintrag hat 2 Byte => 0xf
			*(uint16_t *)((uint8_t *) encTimeR + i_time) = ticks;
			i_encTimeR = i_time;
			/* Regleraufruf rechts */
			if (timeCorrectR == 0) {
				speed_control(1, &motor_right, (uint16_t *) encTimeR, i_encTimeR, enc_tmp);
			} else {
				timeCorrectR = 0;
			}
#else // ! SPEED_CONTROL_AVAILABLE
#ifdef SPEED_LOG_AVAILABLE
			uint8_t index = slog_i[1];
			slog->data[1][index].pwm = motor_right;
			slog->data[1][index].time = tickCount.u32;
			index++;
			slog_i[1] = (uint8_t) (index > 24 ? 0 : index); // Z/25Z
#endif // SPEED_LOG_AVAILABLE
#endif // SPEED_CONTROL_AVAILABLE
		}
	}
}
#endif // MCU
