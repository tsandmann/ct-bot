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

#include "bot-logic.h"
#include "adc.h"
#include "ena.h"
#include "sensor.h"
#include "mouse.h"
#include "motor.h"
#include "timer.h"
#include "sensor_correction.h"
#include "bot-local.h"
#include "display.h"
#include "led.h"
#include "sensor-low.h"
#include "i2c.h"
#include "ir-rc5.h"
#include "math_utils.h"
#include "srf10.h"
#include "init.h"
#include "log.h"
#include <avr/io.h>
#include <string.h>
#include <math.h>

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
#define FILTER_SHIFT 3U

#ifdef DISTSENS_TYPE_GP2Y0A60
#undef DIST_SENS_UPDATE_TIME
#define DIST_SENS_UPDATE_TIME 0
#undef FILTER_SHIFT
#define FILTER_SHIFT 4U
#endif // DISTSENS_TYPE_GP2Y0A60

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
#ifdef SDFAT_AVAILABLE
#define SPEED_LOG_ENTRIES 20
#else
#define SPEED_LOG_ENTRIES 15
#endif // SDFAT_AVAILABLE
volatile uint8_t slog_i[2] = {0,0}; /**< Array-Index */
static slog_t slog_buffer;
slog_t* const slog = &slog_buffer;	/**< Puffer fuer Speed-Log Daten */
#ifdef SDFAT_AVAILABLE
pFatFile speedlog_file;				/**< Datei fuer das Speed-Log */
static char slog_out_buffer[256];
static uint8_t slog_file_dirty = 0;
#endif
#endif // SPEED_LOG_AVAILABLE

static uint16_t filter_l, filter_r;


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
#ifdef SDFAT_AVAILABLE
	/* Datei oeffnen / anlegen */
	if (sdfat_open(SPEEDLOG_FILE_NAME, &speedlog_file, 2 | 0x10 | 0x40) != 0) {
		LOG_ERROR("sdfat_open(%s) failed", SPEEDLOG_FILE_NAME);
	}

	if (speedlog_file) {
		int16_t n = snprintf_P(slog_out_buffer, sizeof(slog_out_buffer) - 1,
#ifdef SPEED_CONTROL_AVAILABLE
			PSTR("time_l\tenc_l\tencRate_l\ttargetRate_l\ttime_r\tenc_r\tencRate_r\ttargetRate_r\n")
#else
			PSTR("time_l\tenc_l\ttime_r\tenc_r\n")
#endif
		);
		if (sdfat_write(speedlog_file, slog_out_buffer, (uint16_t) n) != n) {
			LOG_ERROR("sdfat_write(%d) failed.", n);
		}
		if (sdfat_sync(speedlog_file)) {
			LOG_ERROR("sdfat_sync() failed");
		}
	}
#else // SDFAT_AVAILABLE
#ifdef SPEED_CONTROL_AVAILABLE
	LOG_RAW("time_l\tenc_l\tencRate_l\ttargetRate_l\ttime_r\tenc_r\tencRate_r\ttargetRate_r");
#else
	LOG_RAW("time_l\tenc_l\ttime_r\tenc_r");
#endif
#endif // SDFAT_AVAILABLE
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
	static uint16_t old_dist; // Zeit der letzten Messung der Distanzsensoren

	/* Auswertung der Distanzsensoren alle DIST_SENS_UPDATE_TIME ms */
	uint16_t dist_ticks = TIMER_GET_TICKCOUNT_16;

	if ((uint16_t)(dist_ticks - old_dist) > MS_TO_TICKS(DIST_SENS_UPDATE_TIME)) {
		int16_t * pDistL, * pDistR;
		pDistL = &sensDistL;
		pDistR = &sensDistR;
		adc_read_int(SENS_ABST_L, pDistL);
		if (servo_get_active(SERVO1) == 0) { // wenn die Transportfachklappe bewegt wird, stimmt der Messwert des rechten Sensors nicht
			adc_read_int(SENS_ABST_R, pDistR);
		}
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
	uint16_t pid_ticks = TIMER_GET_TICKCOUNT_16; // Ticks sichern [178 us]
	uint8_t i_time;
	uint8_t * p_time;
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
	/* Speed-Log-Daten auf MMC schreiben / per UART versenden, falls Puffer voll */
	const uint8_t sreg = SREG;
	__builtin_avr_cli();
	uint8_t idx_l = slog_i[0];
	uint8_t idx_r = slog_i[1];
	if (idx_l > SPEED_LOG_ENTRIES || idx_r > SPEED_LOG_ENTRIES) {
		const uint8_t max = idx_l > idx_r ? idx_l : idx_r;
		slog_i[0] = max;
		slog_i[1] = max;
		SREG = sreg;
		if (max > idx_l) {
			memset(&slog->data[0][idx_l], 0, sizeof(slog_data_t) * (size_t) (max - idx_l));
		} else if (max > idx_r) {
			memset(&slog->data[1][idx_r], 0, sizeof(slog_data_t) * (size_t) (max - idx_r));
		}
#ifndef SDFAT_AVAILABLE
		/* Daten via UART senden */
		uint8_t i;
		for (i = 0; i < max; ++i) {
#ifdef SPEED_CONTROL_AVAILABLE
			LOG_RAW("%lu\t%u\t%u\t%u\t%lu\t%u\t%u\t%u", slog->data[0][i].time, slog->data[0][i].enc, slog->data[0][i].encRate,
				slog->data[0][i].targetRate, slog->data[1][i].time, slog->data[1][i].enc, slog->data[1][i].encRate, slog->data[1][i].targetRate);
#else
			LOG_RAW("%lu\t%u\t%lu\t%u", slog->data[0][i].time, slog->data[0][i].enc, slog->data[1][i].time, slog->data[1][i].enc);
#endif // SPEED_CONTROL_AVAILABLE
		}
#endif // SDFAT_AVAILABLE
#ifdef SDFAT_AVAILABLE
		if (speedlog_file) {
			uint8_t i;
			for (i = 0; i < max; ++i) {
				int16_t n = snprintf_P(slog_out_buffer, sizeof(slog_out_buffer) - 1,
#ifdef SPEED_CONTROL_AVAILABLE
					PSTR("%lu\t%u\t%u\t%u\t%lu\t%u\t%u\t%u\n"), slog->data[0][i].time, slog->data[0][i].enc, slog->data[0][i].encRate,
					slog->data[0][i].targetRate, slog->data[1][i].time, slog->data[1][i].enc, slog->data[1][i].encRate, slog->data[1][i].targetRate
#else
					PSTR("%lu\t%u\t%lu\t%u\n"), slog->data[0][i].time, slog->data[0][i].enc, slog->data[1][i].time, slog->data[1][i].enc
#endif // SPEED_CONTROL_AVAILABLE
				);
				if (sdfat_write(speedlog_file, slog_out_buffer, (uint16_t) n) != n) {
					LOG_ERROR("sdfat_write(%u) failed.", n);
				}
			}
			slog_file_dirty = 1;
		}
#endif // SDFAT_AVAILABLE
		const uint8_t sreg = SREG;
		__builtin_avr_cli();
		uint8_t diff = (uint8_t) (slog_i[0] - max);
		if (diff) {
			memmove(&slog->data[0][0], &slog->data[0][max], sizeof(slog_data_t) * diff);
		}
		slog_i[0] = diff;

		diff = (uint8_t) (slog_i[1] - max);
		if (diff) {
			memmove(&slog->data[1][0], &slog->data[1][max], sizeof(slog_data_t) * diff);
		}
		slog_i[1] = diff;
		SREG = sreg;
	} else {
		SREG = sreg;
#ifdef SDFAT_AVAILABLE
		if (slog_file_dirty && speed_l == 0 && speed_r == 0 && speedlog_file) {
			sdfat_sync(speedlog_file);
			slog_file_dirty = 0;
		}
#endif // SDFAT_AVAILABLE
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

		filter_l = filter_l - (filter_l >> FILTER_SHIFT) + (uint16_t) sensDistL;
		uint16_t volt = (uint16_t) (filter_l >> FILTER_SHIFT);

		(*sensor_update_distance)(&sensDistL, &sensDistLToggle, sensDistDataL, volt);

		if (servo_get_active(SERVO1) == 0) {
			/* Dist-Sensor rechts */
			while (adc_get_active_channel() < 2) {}

			filter_r = filter_r - (filter_r >> FILTER_SHIFT) + (uint16_t) sensDistR;
			volt = (uint16_t) (filter_r >> FILTER_SHIFT);

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
			return;
#else // ! SPEED_CONTROL_AVAILABLE
#ifdef SPEED_LOG_AVAILABLE
			uint8_t index = slog_i[0];
			slog->data[0][index].pwm = motor_left;
			slog->data[0][index].enc = enc_tmp & 1;
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
			slog->data[1][index].enc = enc_tmp & 1;
			slog->data[1][index].time = tickCount.u32;
			index++;
			slog_i[1] = (uint8_t) (index > 24 ? 0 : index); // Z/25Z
#endif // SPEED_LOG_AVAILABLE
#endif // SPEED_CONTROL_AVAILABLE
		}
	}
}
#endif // MCU
