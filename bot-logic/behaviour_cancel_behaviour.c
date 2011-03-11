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

//#define DEBUG_CANCEL	/**< Schalter fuer Debug-Code */

#define MAX_JOBS	8	/**< maximale Anzahl an Jobs */

#ifndef DEBUG_CANCEL
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif

/** Liste aller Ueberwachungsauftraege */
static struct {
	Behaviour_t * beh;		/**< Zeiger auf Verhaltensdatensatz des zu ueberwachenden Verhaltens */
	uint8_t (* cond)(void);	/**< Zeiger auf Funktion fuer die Abbruchbedingung */
} PACKED jobs[MAX_JOBS] = { {NULL, NULL} };


/**
 * Verhalten zum bedingten Deaktivieren anderer Verhalten
 * \param *data	Verhaltensdatensatz
 */
void bot_behaviour_cancel_behaviour(Behaviour_t * data) {
	uint8_t idle = True;
	uint8_t i;
	for (i = 0; i < sizeof(jobs) / sizeof(jobs[0]); ++i) {
		if (jobs[i].beh != NULL) {
			if (jobs[i].beh->active != BEHAVIOUR_ACTIVE || jobs[i].cond == NULL) {
				LOG_DEBUG("erledigten Auftrag gefunden, wird entfernt");
				jobs[i].beh = NULL;
				continue;
			}
//			LOG_DEBUG("zu ueberwachendes Verhalten %u gefunden", jobs[i].beh->priority);
			idle = False;
			if (jobs[i].cond() != 0) {
				/* Check-Funktion vorhanden und Abbruchbedingung erfuellt */
				LOG_DEBUG("Abbruch des Verhaltens %u", jobs[i].beh->priority);
				deactivate_called_behaviours(jobs[i].beh); // vom Zielverhalten aufgerufene Verhalten beenden
				exit_behaviour(jobs[i].beh, BEHAVIOUR_SUBCANCEL); // Zielverhalten beenden, Caller reaktivieren
				jobs[i].beh = NULL;
			}
		}
	}

	if (idle) {
		/* keine Auftraege mehr -> sich selbst deaktivieren */
		LOG_DEBUG("keine weiteren Auftraege");
		deactivate_behaviour(data);
	}
}

/**
 * Botenfunktion zum Deaktivieren eines Verhaltens, wenn eine Abbruchbedingung erfuellt ist.
 * \param *caller		Verhaltensdatensatz des Aufrufers
 * \param *behaviour	Verhaltensdatensatz des abzubrechenden Verhaltens
 * \param *check 		Zeiger auf die Abbruchfunktion; liefert diese True, wird das Verhalten beendet
 * \return				Zeiger auf den eigenen Verhaltensdatensatz oder NULL im Fehlerfall
 */
Behaviour_t * bot_add_behaviour_to_cancel(Behaviour_t * caller, Behaviour_t * behaviour, uint8_t (* check)(void)) {
	(void) caller;
	if (behaviour == NULL) {
		LOG_DEBUG("Fehler, kein gueltiges Verhalten uebergeben");
		return NULL;
	}

	LOG_DEBUG("cancel(0x%x, 0x%x (Prio %u), 0x%x)", caller, behaviour, behaviour->priority, check);
	uint8_t i;
	for (i = 0; i < sizeof(jobs) / sizeof(jobs[0]); ++i) {
		if (jobs[i].beh == NULL) {
			LOG_DEBUG(" i=%u noch frei", i);
			jobs[i].beh = behaviour;
			jobs[i].cond = check;
			break;
		}
	}
	if (i == sizeof(jobs) / sizeof(jobs[0])) {
		LOG_DEBUG("Kein Slot mehr frei, Abbruch");
		return NULL;
	} else {
		LOG_DEBUG("Abbruchbedingung korrekt registriert");
		return activateBehaviour(NULL, bot_behaviour_cancel_behaviour);
	}
}

/**
 * Botenfunktion zum Deaktivieren eines Verhaltens, wenn die Abbruchbedingung erfuellt ist.
 * Alte Version, um Abwaertskompatibilitaet zu erhalten
 * \param *caller	Verhaltensdatensatz des Aufrufers
 * \param behaviour	abzubrechendes Verhalten
 * \param *check 	Zeiger auf die Abbruchfunktion; liefert diese True, wird das Verhalten beendet
 * \return			Zeiger auf den eigenen Verhaltensdatensatz oder NULL im Fehlerfall
 */
Behaviour_t * bot_cancel_behaviour(Behaviour_t * caller, BehaviourFunc_t behaviour, uint8_t (* check)(void)) {
	Behaviour_t * const beh = get_behaviour(behaviour);
	return bot_add_behaviour_to_cancel(caller, beh, check);
}

#endif // BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
