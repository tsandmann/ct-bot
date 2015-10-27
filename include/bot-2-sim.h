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
 * \file 	bot-2-sim.h
 * \brief 	Verbindung c't-Bot zu c't-Sim
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */
#ifndef BOT2SIM_H_
#define BOT2SIM_H_

#ifdef BOT_2_SIM_AVAILABLE

#ifdef CREATE_TRACEFILE_AVAILABLE
#include <stdio.h>
extern FILE * tracefile;	/**< Trace-Datei zum Debugging */
#endif // CREATE_TRACEFILE_AVAILABLE

/**
 * Initialisiert die Kommunikation mit dem Sim
 */
void bot_2_sim_init(void);

/**
 * Empfaengt alle Kommondos vom Sim
 */
void bot_2_sim_listen(void);

#ifdef MCU
/**
 * Diese Funktion informiert den Sim ueber alle Sensor und Aktuator-Werte
 */
void bot_2_sim_inform(void);

#else // PC

/**
 * Diese Funktion informiert den Sim ueber alle Sensor und Aktuator-Werte.
 * Dummy fuer PC-Code
 */
void bot_2_sim_inform(void);


#include <sys/time.h>
#ifdef WIN32
/**
 * Hilfsfunktion, die es nur auf dem PC gibt
 */
void gettimeofday_win(struct timeval * p, void * tz /* IGNORED */);
#define GETTIMEOFDAY gettimeofday_win	/**< nur fuer Windows implementierte Funktion */
#else
#define GETTIMEOFDAY gettimeofday	/**< unter nicht-Win benutzen wir die Systemfunktion fuer gettimeofday */
#endif // WIN32
#endif // MCU

/**
 * Setzt den aktiven Kommunikationskanal auf TCP/IP fuer PC und UART fuer MCU (ATmega)
 */
void set_bot_2_sim(void);

#endif // BOT_2_SIM_AVAILABLE
#endif // BOT2SIM_H_
