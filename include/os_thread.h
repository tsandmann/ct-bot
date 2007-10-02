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
#ifdef MCU
#ifdef OS_AVAILABLE

#include "timer.h"
#include "os_scheduler.h"

#define OS_MAX_THREADS	5

#define	OS_TS_RUNNABLE	0
#define OS_TS_RUNNING	1 
#define	OS_TS_BLOCKED	2 

typedef struct {
	uint8_t * stack;
	uint8_t state;
	uint32_t nextSchedule;
} Tcb_t;

extern Tcb_t os_threads[OS_MAX_THREADS];
extern Tcb_t * os_thread_running; 

/*!
 * Legt einen neuen Thread an und setzt ihn auf runnable.
 * Der zuerst angelegt Thread bekommt die hoechste Prioritaet,
 * je spaeter ein Thread erzeugt wird, desto niedriger ist seine
 * Prioritaet, das laesst sich auch nicht mehr aendern!
 * @param *pStack	Zeiger auf den Stack des neuen Threads
 * @param *pIp		Zeiger auf die Main-Funktion des Threads
 * @return			Zeiger auf den TCB des angelegten Threads
 */
Tcb_t * os_create_thread(void * pStack, void * pIp);

/*!
 * Schuetzt den folgenden Block (bis exitCS()) vor Threadswitches
 */
#define os_enterCS() {								\
	os_scheduling_allowed = 0;						\
}

/*!
 * Beendet den kritischen Abschnitt wieder, der mit enterCS began
 */
#define os_exitCS() {								\
	uint8_t sreg = SREG;							\
	cli();											\
	os_scheduling_allowed = 1;						\
	if (os_reschedule == 1) {						\
		SREG = sreg;								\
		os_schedule(TIMER_GET_TICKCOUNT_32);		\
	} else SREG = sreg;								\
}

/*!
 * Schaltet "von aussen" auf einen neuen Thread um. 
 * => kernel threadswitch
 * @param *from	Zeiger auf TCB des aktuell laufenden Threads
 * @param *to	Zeiger auf TCB des Threads, der nun laufen soll
 */
void os_switch_thread(Tcb_t * from, Tcb_t * to);

/*!
 * Blockiert den aktuellten Thread fuer die angegebene Zeit und schaltet
 * auf einen anderen Thread um 
 * => coorporative threadswitch
 * @param ms		Zeit in ms, die der aktuellte Thread blockiert wird
 * @param *thread	Zeiger auf den TCB des Threads, auf dne umgeschaltet wird	
 */
static inline void os_thread_sleep(uint32_t ms, Tcb_t * thread) { 
	//TODO:	Irgendwie dumm, wenn man einen Zielthread angeben muss, NULL-Fall ist aber 
	//		komplizierter zu behandeln
	os_enterCS();
	os_thread_running->nextSchedule = TIMER_GET_TICKCOUNT_32 + ms;
	os_thread_running->state = OS_TS_BLOCKED;
	thread->state = OS_TS_RUNNING;
	thread->nextSchedule = 0;
	asm volatile(
		"in r0, __SREG__	; save SREG				\n\t"
		"cli				; Interrupts off		\n\t"
		"ldi r16, lo8(1f)	; save IP				\n\t"
		"push r16									\n\t"
		"ldi r16, hi8(1f)							\n\t"
		"push r16									\n\t"
		"in r16, __SP_L__	; switch Stack			\n\t"
		"st Z+, r16									\n\t"	
		"in r16, __SP_H__							\n\t"
		"st Z, r16									\n\t"	
		"ld r16, X+ 								\n\t"
		"out __SP_L__, r16							\n\t"
		"ld r16, X 									\n\t"
		"out __SP_H__, r16							\n\t"
		"out __SREG__, r0	; restore SREG			\n\t" 
		"ret 				; continue as 'thread'	\n\t"
		"1:				 								"
		::	"x"(&thread->stack), "z"(&os_thread_running->stack)
			/* clobber */
		:	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			"r9",  "r10", "r11", "r12", "r13", "r14", "r15","r16",
			"r17", "r18", "r19", "r20", "r21", "r22","r23", "r24",
			"r25", "memory"
			//TODO:	Nachdenken, ob das alle Regs abdeckt
	);
	os_exitCS();
}

#endif	// OS_AVAILABLE
#endif	// MCU
#endif	// _THREAD_H
