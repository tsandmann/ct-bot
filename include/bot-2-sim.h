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
 * @date 	26.12.05
 */
#ifndef __bot_2_sim
#define __bot_2_sim

#include "ct-Bot.h"
#include "global.h"

/*!
 * Ein wenig Initilisierung kann nicht schaden 
 */
void bot_2_sim_init(void);

/*!
 *  Frage Simulator nach Daten
 */
int bot_2_sim_ask(uint8 command, uint8 subcommand,int16* data_l,int16* data_r);

/*!
 * Schickt einen Thread in die Warteposition
 * @param timeout_us Wartezeit in Mikrosekunden
 */
void wait_for_time(long timeout_us);
/*! 
 * Wartet auf die Antwort des PC
 * Achtung blockierend ohne Timeout
 * @param command Das kommando auf das gewartet wird.
 * @return 0, wenn Ok
 */
int wait_for_special_command(uint8_t command);

/*! 
 * Schleife, die Kommandis empfaengt und bearbeitet, bis ein Kommando vom Typ Frame kommt 
 * @param frame Kommando zum abbruch
 */
int8 receive_until_Frame(int8 frame);

#ifdef WIN32
	#include <sys/time.h>
	/*!
	 * Hilfsfunktion, die es nur auf dem PC gibt
	 */
	void gettimeofday_win(struct timeval* p, void* tz /* IGNORED */);
	#define GETTIMEOFDAY gettimeofday_win	/*!< nur fuer Windows implementierte Funktion */
#else
	#define GETTIMEOFDAY gettimeofday	/*!< unter nicht-Win benutzen wir die Systemfunktion fuer gettimeofday */
#endif	// WIN32

#endif
