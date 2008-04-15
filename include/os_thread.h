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
 * @file 	os_thread.h
 * @brief 	Threadmanagement fuer BotOS
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	02.10.2007
 */

#ifndef _THREAD_H
#define _THREAD_H

#include "ct-Bot.h"
#ifdef OS_AVAILABLE
#include "timer.h"
#include "os_scheduler.h"
#define OS_MAX_THREADS	2	/*!< maximale Anzahl an Threads im System */

#ifdef MCU
/*! TCB eines Threads */
typedef struct {
	uint8_t * stack;		/*!< Stack-Pointer */
	uint32_t nextSchedule;	/*!< Zeitpunkt der naechsten Ausfuehrung. Ergibt im Zusammenhang mit der aktuellen Zeit den Status eines Threads. */
	uint8_t lastSchedule;	/*!< Zeitpunkt der letzten Ausfuehrung, untere 8 Bit */
} Tcb_t;

extern Tcb_t os_threads[OS_MAX_THREADS];	/*!< Thread-Pool (ist gleichzeitig running- und waiting-queue) */
extern Tcb_t * os_thread_running;			/*!< Zeiger auf den Thread, der zurzeit laeuft */ 

/*!
 * Schuetzt den folgenden Block (bis exitCS()) vor Threadswitches.
 * Ermoeglicht einfaches Locking zum exklusiven Ressourcen-Zugriff.
 */
#define os_enterCS() {								\
	os_scheduling_allowed = 0;						\
}

/*!
 * Beendet den kritischen Abschnitt wieder, der mit enterCS began.
 * Falls ein Scheduler-Aufruf ansteht, wird er nun ausgefuehrt.
 */
void os_exitCS(void);

/*!
 * Schaltet "von aussen" auf einen neuen Thread um. 
 * => kernel threadswitch
 * Achtung, es wird erwartet, dass Interrupts an sind. 
 * Sollte eigentlich nur vom Scheduler aus aufgerufen werden!
 * @param *from	Zeiger auf TCB des aktuell laufenden Threads
 * @param *to	Zeiger auf TCB des Threads, der nun laufen soll
 */
void os_switch_thread(Tcb_t * from, Tcb_t * to);

/*!
 * Blockiert den aktuellten Thread fuer die angegebene Zeit und schaltet
 * auf einen anderen Thread um 
 * => coorporative threadswitch
 * @param sleep		Zeit in ms, die der aktuelle Thread blockiert wird
 */
static inline void os_thread_sleep(uint32_t sleep) {
	uint32_t sleep_ticks = MS_TO_TICKS(sleep);
	uint32_t now = TIMER_GET_TICKCOUNT_32;
	os_thread_running->nextSchedule = now + sleep_ticks;
	os_schedule(now);
}

#else	// PC
#include <pthread.h>

typedef pthread_t Tcb_t;

extern Tcb_t os_threads[OS_MAX_THREADS];	/*!< Thread-Pool (ist gleichzeitig running- und waiting-queue) */

/*!
 * Schuetzt den folgenden Block (bis exitCS()) vor Threadswitches.
 * Ermoeglicht einfaches Locking zum exklusiven Ressourcen-Zugriff.
 */
#define os_enterCS()

/*!
 * Beendet den kritischen Abschnitt wieder, der mit enterCS began.
 * Falls ein Scheduler-Aufruf ansteht, wird er nun ausgefuehrt.
 */
#define os_exitCS()

#endif	// MCU 

/*!
 * Legt einen neuen Thread an und setzt ihn auf runnable.
 * Der zuerst angelegt Thread bekommt die hoechste Prioritaet,
 * je spaeter ein Thread erzeugt wird, desto niedriger ist seine
 * Prioritaet, das laesst sich auch nicht mehr aendern!
 * @param *pStack	Zeiger auf den Stack (Ende!) des neuen Threads
 * @param *pIp		Zeiger auf die Main-Funktion des Threads (Instruction-Pointer)
 * @return			Zeiger auf den TCB des angelegten Threads
 */
Tcb_t * os_create_thread(uint8_t * pStack, void * pIp);

/*!
 * Schaltet auf den Thread mit der naechst niedrigeren Prioritaet um, der lauffaehig ist,
 * indem diesem der Rest der Zeitscheibe geschenkt wird.
 */
void os_thread_yield(void);

/*!
 * Weckt einen wartenden Thread auf, falls dieser eine hoehere Prioritaet hat
 * @param *thread	Zeiger auf TCB des zu weckenden Threads
 */
void os_thread_wakeup(Tcb_t * thread);
#else 	// OS_AVAILABLE

#define os_enterCS()	// NOP
#define os_exitCS()		// NOP

#endif	// OS_AVAILABLE
#endif	// _THREAD_H
