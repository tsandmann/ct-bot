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

volatile uint8_t os_scheduling_allowed = 1;
uint8_t os_reschedule = 0;

/*!
 * Aktualisiert den Schedule, prioritaetsbasiert
 * @param tickcount	Wert des Timer-Ticks (32 Bit)
 */
void os_schedule(uint32_t tickcount) {
	/* Mutex abfragen / setzen => Scheduler ist nicht reentrant! */
	if (test_and_set((uint8_t*)&os_scheduling_allowed, 0) == 0) {
		os_reschedule = 1;
		return;
	}
	
	/* Naechsten lauffaehigen Thread mit hoechster Prioritaet suchen. 
	 * Das ist zwar in O(n), aber wir haben nur eine sehr beschraenkte Anzahl an Threads! */
	uint8_t i;
	Tcb_t * ptr = os_threads;
	for (i=0; i<OS_MAX_THREADS; i++, ptr++) {
		if (ptr->stack != NULL) {
			if (ptr->nextSchedule <= tickcount) {
				if (ptr != os_thread_running) {
					/* switch Thread */
					os_switch_thread(os_thread_running, ptr);
					return;
				} else {
					/* aktiver Thread darf weiterlaufen */
					return;
				}
			}
		} else {
			/* Kein Thread lauffaehig => schlafen */
			// Derzeit ist der Bot schlaflos, wir verlassen den Scheduler hier einfach als waere nix gewesen 8-)
			// *irgendein* Thread muss ja schliesslich vorher gelaufen sein, allerdings ist unsere ReadyQueue nun irgendwie inkonsistent
			return;
			//TODO:	Noch mal ueberlegen, ob das ueberhaupt sein kann
		}
	}

	/* Mutex freigeben */
	os_scheduling_allowed = 1;	
}

#endif	// OS_AVAILABLE
#endif	// MCU

