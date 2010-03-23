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
 * @file 	bot-2-sim.h
 * @brief 	Verbindung c't-Bot zu c't-Sim
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.2005
 */
#ifndef BOT2SIM_H_
#define BOT2SIM_H_

#include "ct-Bot.h"
#include <stdio.h>

#ifdef CREATE_TRACEFILE_AVAILABLE
extern FILE * tracefile;	/*!< Trace-Datei zum Debugging */
#endif	// CREATE_TRACEFILE_AVAILABLE

#ifdef BOT_2_SIM_AVAILABLE
/*!
 * Ein wenig Initilisierung kann nicht schaden
 */
void bot_2_sim_init(void);

/*!
 * Empfaengt alle Kommondos vom Sim
 */
void bot_2_sim_listen(void);

#ifdef MCU
/*!
 * Diese Funktion informiert den Sim ueber alle Sensor und Aktuator-Werte
 */
void bot_2_sim_inform(void);

#else // PC

/*!
 * Diese Funktion informiert den Sim ueber alle Sensor und Aktuator-Werte.
 * Dummy fuer PC-Code
 */
static inline void bot_2_sim_inform(void) {
	// NOP
}

/*!
 * Schleife, die Kommandos empfaengt und bearbeitet, bis ein Kommando vom Typ frame kommt
 * @param frame Kommando zum Abbruch
 * @return		Fehlercode
 */
int8_t receive_until_Frame(uint8_t frame);

#include <sys/time.h>
#ifdef WIN32
/*!
 * Hilfsfunktion, die es nur auf dem PC gibt
 */
void gettimeofday_win(struct timeval * p, void * tz /* IGNORED */);
#define GETTIMEOFDAY gettimeofday_win	/*!< nur fuer Windows implementierte Funktion */
#else
#define GETTIMEOFDAY gettimeofday	/*!< unter nicht-Win benutzen wir die Systemfunktion fuer gettimeofday */
#endif	// WIN32
#endif	// MCU

#endif	// BOT_2_SIM_AVAILABLE
#endif	// BOT2SIM_H_
