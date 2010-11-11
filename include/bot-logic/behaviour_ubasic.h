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
 * \file 	behaviour_ubasic.h
 * \brief 	Basic-Interpreter als Verhalten
 * \author 	Frank Menzel (menzelfr@gmx.de)
 * \date 	10.06.2010
 */

#ifndef BEHAVIOUR_UBASIC_H_
#define BEHAVIOUR_UBASIC_H_

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "botfs_config.h"
#include "botfs_types.h"

#define UBASIC_BEH_ZEILENLAENGE 80 /*!< max. Laenge der einzelnen Zeilen */

extern Behaviour_t * ubasic_behaviour_data; /*!< Verhaltensdatensatz des ubasis-Verhaltens */
extern uint32_t ubasic_wait_until; /*!< Systemzeit, bis zu der gewartet werden soll (WAIT in Basic) */

/*!
 * Startet das uBasic-Verhalten
 * \param *caller Zeiger auf den Verhaltensdatensatz des Aufrufers
 */
void bot_ubasic(Behaviour_t * caller);

/*!
 * uBasic als ct-Bot Verhalten
 * \param *data Zeiger auf den Datensatz des Verhaltens
 */
void bot_ubasic_behaviour(Behaviour_t * data);

/*!
 * Hilfsroutine, um in Basic innerhalb eines Steps beide Variablen mit der Bot-Geschwindigkeit belegen zu koennen
 * \param speedLeft	 Geschwindigkeitswert fuer links
 * \param speedRight Geschwindigkeitswert fuer rechts
 */
void bot_ubasic_speed(int16_t speedLeft, int16_t speedRight);

/*!
 * Initialisiert den Zeilenpuffer und laedt die erste Zeile
 * aus der Programm-Datei
 * \param **p_prog Zeiger auf einen Zeiger zum Zeilenpuffer
 */
void ubasic_buffer_init(const char * * p_prog);

/*!
 * Laedt ein uBasic-Programm aus deiner BotFS-Datei
 * \param *file Zeiger auf Dateideskriptor der Programmdatei
 */
void ubasic_load_file(botfs_file_descr_t * file);

/*!
 * Laedt die naechste Zeile aus der Programm-Datei
 * \param **p_prog Zeiger auf einen Zeiger zum Zeilenpuffer
 */
void ubasic_load_next_line(const char * * p_prog);

/*!
 * Display fuer das uBasic-Verhalten
 */
void ubasic_display(void);

#endif // BEHAVIOUR_UBASIC_AVAILABLE
#endif // BEHAVIOUR_UBASIC_H_
