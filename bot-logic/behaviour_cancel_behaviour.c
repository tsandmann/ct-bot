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
 * \file 	behaviour_cancel_behaviour.c
 * \brief 	Deaktiviert ein anderes Verhalten in Abhaengigkeit einer Check-Funktion
 *
 * So kann z.B. der Wandfolger (bot_solve_maze) beendet werden, falls dieser auf
 * eine Linie faehrt und der Linienfolger uebernehmen.
 * \author 	Frank Menzel (Menzelfr@gmx.net)
 * \date 	19.10.2007
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
#include "log.h"
#include <stdlib.h>

//#define DEBUG_CANCEL	// Debug-Code an

#ifndef DEBUG_CANCEL
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif

/*!
 * Zeiger auf Abbruchfunktion des Verhaltens.
 * Die Funktion muss True (1) zurueckgeben, wenn abgebrochen werden soll, sonst False (0).
 */
static uint8_t (* check_function)(void) = NULL;

/*!
 * Zeiger auf die Verhaltensfunktion des zu deaktivierenden Verhaltens, falls Check True ergibt.
 */
static BehaviourFunc behaviourFuncCancel = NULL;

/*!
 * Verhalten zum bedingten Deaktivieren eines anderen Verhaltens
 * \param *data	Verhaltensdatensatz
 */
void bot_cancel_behaviour_behaviour(Behaviour_t * data) {
	if (check_function != NULL && check_function() != 0) {
		/* Check-Funktion vorhanden und Abbruchbedingung erfuellt */
		LOG_DEBUG("Abbruch des Verhaltens 0x%x", behaviourFuncCancel);
		check_function = NULL;
		deactivateCalledBehaviours(behaviourFuncCancel);	// vom Zielverhalten aufgerufenen Verhalten beenden
		deactivateBehaviour(behaviourFuncCancel);	// Zielverhalten beenden
		return_from_behaviour(data);	// sich selbst beenden
	}
}

/*!
 * Botenfunktion zum Deaktivieren eines Verhaltens, wenn die Abbruchbedingung erfuellt ist
 * \param *caller	Verhaltensdatensatz des Aufrufers
 * \param behaviour	abzubrechendes Verhalten
 * \param *check 	Zeiger auf die Abbruchfunktion; liefert diese True, wird das Verhalten beendet
 */
void bot_cancel_behaviour(Behaviour_t * caller, BehaviourFunc behaviour, uint8_t (* check)(void)) {
	LOG_DEBUG("cancel(0x%x, 0x%x, 0x%x)", caller, behaviour, check);
	check_function = check;
	behaviourFuncCancel = behaviour;
	activateBehaviour(caller, bot_cancel_behaviour_behaviour);
}

#endif // BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
