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
 * @file 	os_scheduler.c
 * @brief 	Mini-Scheduler fuer BotOS
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	02.10.2007
 */

#include "ct-Bot.h"
#ifdef MCU
#ifdef OS_AVAILABLE
#include "os_utils.h"
#include "os_thread.h"
#include <stdlib.h>
#include "log.h"

volatile uint8_t os_scheduling_allowed = 1;	/*!< sperrt den Scheduler, falls != 1. Sollte nur per os_enterCS() / os_exitCS() veraendert werden! */

/*!
 * Aktualisiert den Schedule, prioritaetsbasiert
 * @param tickcount	Wert des Timer-Ticks (32 Bit)
 */
void os_schedule(uint32_t tickcount) {
	/* Mutex abfragen / setzen => Scheduler ist nicht reentrant! */
	if (test_and_set((uint8_t *)&os_scheduling_allowed, 0) != 1) {
		os_scheduling_allowed = 2;
		return;
	}
	
	/* Naechsten lauffaehigen Thread mit hoechster Prioritaet suchen. 
	 * Das ist zwar in O(n), aber wir haben nur eine sehr beschraenkte Anzahl an Threads! */
	uint8_t i;
	Tcb_t * ptr = os_threads;
	// os_scheduling_allowed == 0, sonst waeren wir nicht hier
	for (i=os_scheduling_allowed; i<OS_MAX_THREADS; i++, ptr++) { 
		if (ptr->stack != NULL) {
			/* Es existiert noch ein Thread in der Liste */
			if (ptr->wait_for->value == 0 && ptr->nextSchedule < tickcount) {
				/* Der Thread ist nicht blockiert */
				if (ptr != os_thread_running) {
					/* switch Thread */
					ptr->lastSchedule = tickcount;
					//-- hier laeuft noch der alte Thread (SP zeigt auf os_thread_running's Stack) --//
					os_switch_thread(os_thread_running, ptr);
					//-- jetzt laeuft bereits der neue Thread (SP zeigt auf ptr's Stack) --//
					// => return fuehrt den NEUEN Thread weiter aus -> (noch) KEIN Ruecksprung zum Caller
					break;
				} else {
					/* aktiver Thread darf weiterlaufen */
					// => return fuehrt den ALTEN Thread weiter aus -> Funktionsaufruf mit "normaler" Rueckkehr
					break;
				}
			}
		}
	}

	/* Mutex freigeben */
	os_scheduling_allowed = 1;
	/* Ruecksprung dorthin, wo der (neu) geschedulte Thread vor SEINER Unterbrechung war, 
	 * also nicht zwangsweise in die Funktion, die (direkt) vor dem Scheduler-Aufruf 
	 * ausgefuehrt wurde! */
}

#endif	// OS_AVAILABLE
#endif	// MCU

