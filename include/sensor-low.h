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
 * \file 	sensor-low.h
 * \brief 	Low-Level Routinen fuer die Sensor-Steuerung des c't-Bots
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	01.12.2005
 */
#ifndef SENSOR_LOW_H_
#define SENSOR_LOW_H_

#include "sdcard_wrapper.h"

/**
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void);

/**
 * Alle Sensoren aktualisieren
 */
void bot_sens(void);

/**
 * \brief Kuemmert sich um die Radencoder
 *
 * Das muss schneller gehen als die anderen Sensoren,
 * daher Update per Timer-Interrupt und nicht ueber sensor_update() und
 * die Bot-Hauptschleife.
 */
void bot_encoder_isr(void);

#ifdef SPEED_CONTROL_AVAILABLE
extern uint16_t encTimeL[8];	/**< Timestamps linker Encoder */
extern uint16_t encTimeR[8];	/**< Timestamps rechter Encoder */
extern uint8_t i_encTimeL;		/**< Array-Index auf letzten Timestampeintrag links */
extern uint8_t i_encTimeR;		/**< Array-Index auf letzten Timestampeintrag rechts */
#endif // SPEED_CONTROL_AVAILABLE

#ifdef SPEED_LOG_AVAILABLE
/** Datenstruktur fuer Speedlog-Eintraege */
typedef struct {
#ifdef SPEED_CONTROL_AVAILABLE
	uint8_t encRate;	/**< Ist-Geschwindigkeit (halbiert) */
	uint8_t targetRate;	/**< Soll-Geschwindigkeit (halbiert) */
	int16_t err;		/**< Regelfehler */
#endif // SPEED_CONTROL_AVAILABLE
	int16_t pwm;		/**< aktueller PWM-Wert */
	uint32_t time;		/**< Timestamp */
	uint8_t enc;		/**< Encoder Pegel */
} PACKED_FORCE slog_data_t;

typedef union {
	slog_data_t data[2][25]; /**< Speed-Log Daten */
} slog_t; /**< Speed-Log Datentyp */
extern volatile uint8_t slog_i[2]; /**< Array-Index */
extern slog_t* const slog; /**< Puffer fuer Speed-Log Daten */
extern pFatFile speedlog_file; /**< Datei fuer das Speed-Log */
#endif // SPEED_LOG_AVAILABLE

#define SPEEDLOG_FILE_NAME "speedlog.txt" /**< Dateiname der Speedlog-Datei */
#endif // SENSOR_LOW_H_
