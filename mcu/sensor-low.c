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
 * @file 	sensor-low.c
 * @brief 	Low-Level Routinen fuer die Sensor Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
 */

#ifdef MCU

#include <avr/io.h>
#include "adc.h"
#include "global.h"

#include "ena.h"
#include "sensor.h"
#include "mouse.h"
#include "motor.h"
#include "timer.h"
#include "sensor_correction.h"
#include "bot-local.h"
#include "bot-logic/available_behaviours.h"
#include "bot-logic/behaviour_servo.h"
#include "display.h"
#include "mmc.h"
#include "mini-fat.h"
#include "led.h"
#include "sensor-low.h"
#include "i2c.h"
#include <string.h>

// ADC-PINS
#define SENS_ABST_L		0		/*!< ADC-PIN Abstandssensor Links */
#define SENS_ABST_R		1		/*!< ADC-PIN Abstandssensor Rechts */
#define SENS_M_L		2		/*!< ADC-PIN Liniensensor Links */
#define SENS_M_R		3		/*!< ADC-PIN Liniensensor Rechts */
#define SENS_LDR_L		4		/*!< ADC-PIN Lichtsensor Links */
#define SENS_LDR_R		5		/*!< ADC-PIN Lichtsensor Rechts */
#define SENS_KANTE_L	6		/*!< ADC-PIN Kantensensor Links */
#define SENS_KANTE_R	7		/*!< ADC-PIN Kantensensor Rechts */

// Sonstige Sensoren
#define SENS_DOOR_PINR 		PIND	/*!< Port an dem der Klappensensor hängt */
#define SENS_DOOR_DDR 		DDRD	/*!< DDR für den Klappensensor */
#define SENS_DOOR			6		/*!< Pin  an dem der Klappensensor hängt */

#ifdef SPI_AVAILABLE
#define SENS_ENCL_PINR		PINC	/*!< Port an dem der linke Encoder hängt */
#define SENS_ENCL_DDR		DDRC	/*!< DDR für den linken Encoder  */
#define SENS_ENCL			5		/*!< Pin an dem der linke Encoder hängt */
#else
#define SENS_ENCL_PINR		PINB	/*!< Port an dem der linke Encoder hängt */
#define SENS_ENCL_DDR		DDRB	/*!< DDR für den linken Encoder  */
#define SENS_ENCL			4		/*!< Pin an dem der linke Encoder hängt */
#endif	// SPI_AVAILABLE

#define SENS_ENCR_PINR		PIND	/*!< Port an dem der rechte Encoder hängt */
#define SENS_ENCR_DDR		DDRD	/*!< DDR für den rechten Encoder  */
#define SENS_ENCR			3		/*!< Pin an dem der rechte Encoder hängt */

#define SENS_ERROR_PINR		PINB	/*!< Port an dem die Fehlerüberwachung hängt */
#define SENS_ERROR_DDR		DDRB	/*!< DDR für die Fehlerüberwachung */
#define SENS_ERROR			2		/*!< Pin an dem die Fehlerüberwachung hängt */

#define SENS_TRANS_PINR		PINB	/*!< Port an dem die Transportfachueberwachung haengt */
#define SENS_TRANS_PORT		PORTB	/*!< Port an dem die Transportfachueberwachung haengt */
#define SENS_TRANS_DDR		DDRB	/*!< DDR für die Transportfachueberwachung */
#define SENS_TRANS			0		/*!< Pin an dem die Transportfachueberwachung haengt */

#define ENC_L ((SENS_ENCL_PINR >> SENS_ENCL) & 0x01)	/*!< Abkuerzung zum Zugriff auf Encoder */
#define ENC_R ((SENS_ENCR_PINR >> SENS_ENCR) & 0x01)	/*!< Abkuerzung zum Zugriff auf Encoder */

#define ENC_ENTPRELL	12		/*!< Nur wenn der Encoder ein paar mal den gleichen wert gibt uebernehmen */

#ifdef SPEED_CONTROL_AVAILABLE
uint16_t encTimeL[8] = {0};	/*!< Timestamps linker Encoder */
uint16_t encTimeR[8] = {0};	/*!< Timestamps rechter Encoder */
uint8_t i_encTimeL = 0;		/*!< Array-Index auf letzten Timestampeintrag links */
uint8_t i_encTimeR = 0;		/*!< Array-Index auf letzten Timestampeintrag rechts */
uint8_t timeCorrectL = 0;
uint8_t timeCorrectR = 0;
#endif // SPEED_CONTROL_AVAILABLE

#ifdef SPEED_LOG_AVAILABLE
/* Some Debug-Loggings */
volatile slog_t slog_data[2][25] = {{{0}}, {{0}}};	/*!< Speed-Log Daten */
volatile uint8_t slog_i[2] = {0,0};					/*!< Array-Index */
uint32_t slog_sector = 0;							/*!< Sektor auf der MMC fuer die Daten */
volatile uint8_t slog_count[2] = {0,0};				/*!< Anzahl Loggings seit letztem Rueckschreiben */
#endif // SPEED_LOG_AVAILABLE

/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void) {
	ENA_init();
	adc_init(0xFF);		// Alle ADC-Ports aktivieren

	ENA_set(ENA_RADLED | ENA_ABSTAND);		// Alle Sensoren bis auf Radencoder & Abstandssensoren deaktivieren

	SENS_DOOR_DDR	&= ~(1<<SENS_DOOR);		// Input

	SENS_ERROR_DDR	&= ~(1<<SENS_ERROR);	// Input

	SENS_TRANS_DDR	&= ~(1<<SENS_TRANS);	// Input
	SENS_TRANS_PORT	|=  (1<<SENS_TRANS);	// Pullup an

	SENS_ENCL_DDR	&= ~(1<<SENS_ENCL);		// Input
	SENS_ENCR_DDR	&= ~(1<<SENS_ENCR);		// Input

	timer_2_init();
	sensEncL = 0;
	sensEncR = 0;
}


/*!
 * Alle Sensoren aktualisieren
 */
void bot_sens(void) {
	ENA_on(ENA_KANTLED|ENA_LINE|ENA_SCHRANKE|ENA_KLAPPLED);	// Die Distanzsensoren sind im Normalfall an, da sie 50 ms zum booten brauchen

#ifdef CMPS03_AVAILABLE
	cmps03_get_bearing(&sensCmps03);
#endif

	/* aktualisiere Distanz-Sensoren, interrupt-driven I/O */
#ifdef DISTSENS_AVERAGE
	static uint8_t measure_count = 0;
	static int16_t distLeft[4];
	static int16_t distRight[4];
#endif	// DISTSENS_AVERAGE
	static uint16_t old_dist;		// Zeit der letzten Messung der Distanzsensoren

	/* Auswertung der Distanzsensoren alle 50 ms */
	uint16_t dist_ticks = TIMER_GET_TICKCOUNT_16;
	if ((uint16_t)(dist_ticks - old_dist) > MS_TO_TICKS(50)) {
		int16_t * pDistL, * pDistR;
#ifdef DISTSENS_AVERAGE
		pDistL = &distLeft[measure_count];
		pDistR = &distRight[measure_count];
#else
		pDistL = &sensDistL;
		pDistR = &sensDistR;
#endif	// DISTSENS_AVERAGE
		adc_read_int(SENS_ABST_L, pDistL);
#ifdef BEHAVIOUR_SERVO_AVAILABLE
		if ((servo_active & SERVO1) == 0)	// Wenn die Transportfachklappe bewegt wird, stimmt der Messwert des rechten Sensor nicht
#endif
			adc_read_int(SENS_ABST_R, pDistR);
#ifdef DISTSENS_AVERAGE
		measure_count++;
		measure_count &= 0x3;	// Z/4Z
#endif
	}

	/* die anderen analogen Sensoren, auch int-driven I/O */
	adc_read_int(SENS_M_L, &sensLineL);
	adc_read_int(SENS_M_R, &sensLineR);

	adc_read_int(SENS_LDR_L, &sensLDRL);
	adc_read_int(SENS_LDR_R, &sensLDRR);

	adc_read_int(SENS_KANTE_L, &sensBorderL);
	adc_read_int(SENS_KANTE_R, &sensBorderR);

#ifdef MOUSE_AVAILABLE
	// Aktualisiere die Position des Maussensors
	sensMouseDX = mouse_sens_read(MOUSE_DELTA_X_REG);
	sensMouseDY = mouse_sens_read(MOUSE_DELTA_Y_REG);
#endif

	/* alle digitalen Sensoren */
	sensDoor = (SENS_DOOR_PINR >> SENS_DOOR) & 0x01;
	sensTrans = (SENS_TRANS_PINR >> SENS_TRANS) & 0x01;
	sensError = (SENS_ERROR_PINR >> SENS_ERROR) & 0x01;

#ifdef SPEED_CONTROL_AVAILABLE
	/* Aufruf der Motorregler, falls Stillstand */
	register uint16_t pid_ticks = TIMER_GET_TICKCOUNT_16;	// Ticks sichern [178 us]
	register uint8_t i_time;
	register uint8_t * p_time;
	/* Index auf Encodertimestamps zwischenspeichern */
	i_time = i_encTimeL;
	p_time = (uint8_t *)encTimeL;
	/* Bei Stillstand Regleraufruf links nach PID_TIME ms */
	if (pid_ticks - *(uint16_t *)(p_time + i_time) > PID_TIME * 50 / TIMER_STEPS * 20) {
		/* Timestamp links verschieben / speichern */
		i_time = (i_time + sizeof(encTimeL[0])) & 0xf;	// encTime ist Z/8Z und jeder Eintrag hat 2 Byte => 0xf
		*(uint16_t *)(p_time + i_time) = pid_ticks;
		i_encTimeL = i_time;
		/* Regleraufruf */
		speed_control(0,  (int16_t *)&motor_left, (uint16_t *)encTimeL, i_encTimeL, 0);
		timeCorrectL = 1;
	}
	/* Bei Stillstand Regleraufruf rechts nach PID_TIME ms */
	i_time = i_encTimeR;
	p_time = (uint8_t *)encTimeR;
	if (pid_ticks - *(uint16_t *)(p_time + i_time) > PID_TIME * 50 / TIMER_STEPS * 20) {
		/* Timestamp rechts verschieben / speichern */
		i_time = (i_time + sizeof(encTimeR[0])) & 0xf;	// encTime ist Z/8Z und jeder Eintrag hat 2 Byte => 0xf
		*(uint16_t *)(p_time + i_time) = pid_ticks;
		i_encTimeR = i_time;
		/* Regleraufruf rechts */
		speed_control(1, (int16_t *)&motor_right, (uint16_t *)encTimeR, i_encTimeR, 0);
		timeCorrectR = 1;
	}

#ifdef SPEED_LOG_AVAILABLE
	/* Speed-Log-Daten auf MMC schreiben, falls Puffer voll */
	if (slog_sector == 0) {
		/* init-stuff here */
		slog_sector = mini_fat_find_block("slog", __builtin_alloca(512));
		//slog_start_sector = slog_sector;
	}
	if (slog_count[0] > 20 || slog_count[1] > 20) {	// etwas Luft lassen, denn die Daten kommen per ISR
		uint8_t i;
		for (i=slog_count[0]+1; i<25; i++){
			memset((uint8_t *)&slog_data[0][i], 0, sizeof(slog_t));	// q&d
		}
		for (i=slog_count[1]+1; i<25; i++){
			memset((uint8_t *)&slog_data[1][i], 0, sizeof(slog_t));	// q&d
		}
		mmc_write_sector(slog_sector++, (uint8_t *)slog_data);	// swap-out
		/* Index-Reset */
		slog_i[0] = 0;
		slog_count[0] = 0;
		slog_i[1] = 0;
		slog_count[1] = 0;
	}
#endif // SPEED_LOG_AVAILABLE
#endif // SPEED_CONTROL_AVAILABLE

	sensor_update(); // Weiterverarbeitung der rohen Sensordaten

	if ((uint16_t)(dist_ticks - old_dist) > MS_TO_TICKS(50)) {
		old_dist = dist_ticks;	// Zeit fuer naechste Messung merken
		// dieser Block braucht insgesamt ca. 80 us (MCU)
		/* Dist-Sensor links */
		while (adc_get_active_channel() < 1) {}
		uint16_t volt;
#ifdef DISTSENS_AVERAGE
		volt = (distLeft[0] + distLeft[1] + distLeft[2] + distLeft[3]) >> 2;
#else
		volt = sensDistL;
#endif
		(*sensor_update_distance)(&sensDistL, &sensDistLToggle, sensDistDataL, volt);
#ifdef TEST_AVAILABLE_ANALOG
		sensDistL = volt;
#endif
		/* Dist-Sensor rechts */
		while (adc_get_active_channel() < 2) {}
#ifdef DISTSENS_AVERAGE
		volt = (distRight[0] + distRight[1] + distRight[2] + distRight[3]) >> 2;
#else
		volt = sensDistR;
#endif
		(*sensor_update_distance)(&sensDistR, &sensDistRToggle, sensDistDataR, volt);
#ifdef TEST_AVAILABLE_ANALOG
		sensDistR = volt;
#endif
	}

#ifdef CMPS03_AVAILABLE
	cmps03_finish(&sensCmps03);
	heading = (float)sensCmps03.bearing / 10.0;
#endif

	/* alle anderen analogen Sensoren */
	while (adc_get_active_channel() != 255) {}	// restliche Zeit verbrauchen
	// in den Testmodi bleibt immer alles an.
#ifndef TEST_AVAILABLE
  	ENA_off(ENA_KANTLED|ENA_LINE|ENA_SCHRANKE|ENA_KLAPPLED);	// Kanten (ENA_KANTLED), Liniensensoren (ENA_LINE), Transportfach-LED und Klappensensor aus
#endif

	/* LEDs updaten */
	led_update();
#ifdef LED_AVAILABLE
	/* Sollen die LEDs mit den Rohdaten der Sensoren arbeiten,
	 * kommentiert man die folgenden Zeilen ein */

	//if (voltL > 80) LED_on(LED_LINKS);
	//else LED_off(LED_LINKS);
	//if (voltR > 80) LED_on(LED_RECHTS);
	//else LED_off(LED_RECHTS);
#endif	// LED_AVAILABLE
}

/*!
 * Kuemmert sich um die Radencoder
 * Das muss schneller gehen als die anderen Sensoren,
 * daher Update per Timer-Interrupt und nicht per Polling
 */
void bot_encoder_isr(void) {
	static uint8_t enc_l = 0;		/*!< Puffer fuer die letzten Encoder-Staende */
	static uint8_t enc_r = 0;		/*!< Puffer fuer die letzten Encoder-Staende */
	static uint8_t enc_l_cnt = 0;	/*!< Entprell-Counter fuer L-Encoder */
	static uint8_t enc_r_cnt = 0;	/*!< Entprell-Counter fuer R-Encoder */
	register uint8_t enc_tmp; // Pegel der Encoderpins im Register zwischenspeichern

#ifdef SPEED_CONTROL_AVAILABLE
	register uint16_t ticks = tickCount.u16;	// aktuelle Systemzeit zwischenspeichern
	register uint8_t i_time;					// Index des Timestamparrays zwischenspeichern
#endif	// SPEED_CONTROL_AVAILABLE
	/* Rad-Encoder links */
	enc_tmp = ENC_L;
	if (enc_tmp != enc_l) {	// uns interesieren nur Veraenderungen
		enc_l=enc_tmp;		// neuen Wert sichern
		enc_l_cnt = 0;		// Counter zuruecksetzen
	} else {				// zaehlen, wie lange Pegel bleibt
		if (enc_l_cnt < ENC_ENTPRELL) // Nur bis zur Entprell-Marke
			enc_l_cnt++;
		else if (enc_l_cnt == ENC_ENTPRELL) { 	// wenn lange genug konstant
			enc_l_cnt++;	// diese Flanke nur einmal auswerten
			if (direction.left == DIRECTION_FORWARD) {	// Drehrichtung beachten
				sensEncL++;	//vorwaerts
			} else {
				sensEncL--;	//rueckwaerts
			}
#ifdef SPEED_CONTROL_AVAILABLE
			/* Timestamps fuer Regler links verschieben und speichern */
			i_time = (i_encTimeL + sizeof(encTimeL[0])) & 0xf;	// encTime ist Z/8Z und jeder Eintrag hat 2 Byte => 0xf
			*(uint16_t *)((uint8_t *)encTimeL + i_time) = ticks;
			i_encTimeL = i_time;
			/* Regleraufruf links */
			if (timeCorrectL == 0) {
				speed_control(0, (int16_t *)&motor_left, (uint16_t *)encTimeL, i_encTimeL, enc_tmp);
			} else {
				timeCorrectL = 0;
			}
			/* pro TIMER_STEP wird maximal ein Encoder ausgewertet, da max alle 6 ms (Fullspeed) eine Flanke kommen kann */
			return;	// hackhack
#endif // SPEED_CONTROL_AVAILABLE
		}
	}

	/* Rad-Encoder rechts */
	enc_tmp = ENC_R;
	if (enc_tmp != enc_r) {	// uns interesieren nur Veraenderungen
		enc_r=enc_tmp;		// neuen Wert sichern
		enc_r_cnt=0;		// Counter zuruecksetzen
	} else { 				// zaehlen, wie lange Pegel bleibt
		if (enc_r_cnt < ENC_ENTPRELL)	// nur bis zur Entprell-Marke
			enc_r_cnt++;
		else if (enc_r_cnt == ENC_ENTPRELL) { 	// wenn lange genug konstant
			enc_r_cnt++;	// diese Flanke nur einmal auswerten
			if (direction.right == DIRECTION_FORWARD) {	// Drehrichtung beachten
				sensEncR++;	//vorwaerts
			} else {
				sensEncR--;	//rueckwaerts
			}
#ifdef SPEED_CONTROL_AVAILABLE
			/* Timestamps fuer Regler rechts verschieben und speichern */
			i_time = (i_encTimeR + sizeof(encTimeR[0])) & 0xf;	// encTime ist Z/8Z und jeder Eintrag hat 2 Byte => 0xf
			*(uint16_t *)((uint8_t *)encTimeR + i_time) = ticks;
			i_encTimeR = i_time;
			/* Regleraufruf rechts */
			if (timeCorrectR == 0) {
				speed_control(1, (int16_t *)&motor_right, (uint16_t *)encTimeR, i_encTimeR, enc_tmp);
			} else {
				timeCorrectR = 0;
			}
#endif // SPEED_CONTROL_AVAILABLE
		}
	}
}
#endif	// MCU
