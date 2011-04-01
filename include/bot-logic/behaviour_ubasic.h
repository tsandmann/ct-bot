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

extern botfs_file_descr_t ubasic_prog_file; /**< Basic-Programmdatei */
extern Behaviour_t * ubasic_behaviour_data; /**< Verhaltensdatensatz des ubasis-Verhaltens */
extern char ubasic_content; /**< aktuelles Zeichen des Basic-Programms */
extern uint16_t ubasic_ptr; /**< aktuelle Position im Basic-Programm */

/**
 * Startet das uBasic-Verhalten
 * \param *caller Zeiger auf den Verhaltensdatensatz des Aufrufers
 */
void bot_ubasic(Behaviour_t * caller);

/**
 * uBasic als ct-Bot Verhalten
 * \param *data Zeiger auf den Datensatz des Verhaltens
 */
void bot_ubasic_behaviour(Behaviour_t * data);

/**
 * bricht das aktuelle Basic-Programm ab
 */
void bot_ubasic_break(void);

/**
 * Hilfsroutine, um in Basic innerhalb eines Steps beide Variablen mit der Bot-Geschwindigkeit belegen zu koennen
 * \param speedLeft	 Geschwindigkeitswert fuer links
 * \param speedRight Geschwindigkeitswert fuer rechts
 */
void bot_ubasic_speed(int16_t speedLeft, int16_t speedRight);

/**
 * Rueckgabe ob das zuletzt aufgerufene Verhalten noch aktiv ist oder nicht; festgestellt anhand der Verhaltens-Data-Struktur des ubasic-Verhaltens
 * \param *behaviour	Zeiger auf Verhaltensdatensatz zum abzufragenden Verhalten
 * \return 				!= 0 wenn das zuletzt aufgerufene Verhalten noch laeuft; 0 wenn es nicht mehr laeuft (Achtung: wait ist auch ein Verhalten)
 */
uint8_t behaviour_is_active(Behaviour_t * behaviour);

/**
 * Laedt ein uBasic-Programm aus deiner BotFS-Datei
 * \param *filename Dateiname des Programms
 * \param *file Zeiger auf Dateideskriptor der Programmdatei
 */
void bot_ubasic_load_file(char * filename, botfs_file_descr_t * file);

/**
 * Display fuer das uBasic-Verhalten
 */
void ubasic_display(void);

#endif // BEHAVIOUR_UBASIC_AVAILABLE
#endif // BEHAVIOUR_UBASIC_H_
