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
 * \file 	behaviour_cancel_behaviour.h
 * \brief 	Deaktiviert ein anderes Verhalten in Abhaengigkeit einer Check-Funktion
 *
 * So kann z.B. der Wandfolger (bot_solve_maze) beendet werden, falls dieser auf
 * eine Linie faehrt und der Linienfolger uebernehmen.
 * \author 	Frank Menzel (Menzelfr@gmx.net)
 * \date 	19.10.2007
 */

#ifndef BEHAVIOUR_CANCEL_H_
#define BEHAVIOUR_CANCEL_H_

#ifdef BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE

/*!
 * Verhalten zum bedingten Deaktivieren eines anderen Verhaltens
 * \param *data	Verhaltensdatensatz
 */
void bot_cancel_behaviour_behaviour(Behaviour_t * data);

/*!
 * Botenfunktion zum Deaktivieren eines Verhaltens, wenn die Abbruchbedingung erfuellt ist
 * \param *caller	Verhaltensdatensatz des Aufrufers
 * \param behaviour	abzubrechendes Verhalten
 * \param *check 	Zeiger auf die Abbruchfunktion; liefert diese True, wird das Verhalten beendet
 */
void bot_cancel_behaviour(Behaviour_t * caller, BehaviourFunc behaviour, uint8_t (*check)(void));

#endif // BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
#endif // BEHAVIOUR_CANCEL_H_
