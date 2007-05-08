/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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
 * @file 	sensor-low.h  
 * @brief 	Low-Level Routinen fuer die Sensor-Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
 */
#ifndef sens_low_H_
#define sens_low_H_

/*!
 * Initialisiere alle Sensoren
 */
extern void bot_sens_init(void);

/*!
 * Alle Sensoren aktualisieren
 */
extern void bot_sens_isr(void);

/*!
 * Kuemmert sich um die Radencoder
 * Das muss schneller gehen als die anderen Sensoren,
 * daher Update per ISR
 */
extern void bot_encoder_isr(void);

/*! Datenstruktur fuer Speedlog-Eintraege */
typedef struct {
	uint8 encRate;
	uint8 targetRate;
	int16 err;
	int16 pwm;
	uint32 time;
} slog_t;

#ifdef SPEED_CONTROL_AVAILABLE
	extern uint16 encTimeL[8];	/*!< Timestamps linker Encoder */
	extern uint16 encTimeR[8];	/*!< Timestamps rechter Encoder */
	extern uint8 i_encTimeL;		/*!< Array-Index auf letzten Timestampeintrag links */
	extern uint8 i_encTimeR;		/*!< Array-Index auf letzten Timestampeintrag rechts */
#endif // SPEED_CONTROL_AVAILABLE

#ifdef SPEED_LOG_AVAILABLE
	extern volatile slog_t slog_data[2][25];	/*!< Speed-Log Daten */
	extern volatile uint8 slog_i[2];			/*!< Array-Index */
	extern uint32 slog_sector;					/*!< Sektor auf der MMC fuer die Daten */
	extern volatile uint8 slog_count[2];		/*!< Anzahl Loggings seit letztem Rueckschreiben */
#endif // SPEED_LOG_AVAILABLE
#endif
