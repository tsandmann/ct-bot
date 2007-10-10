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
 * @file 	os_thread.c
 * @brief 	Threadmanagement fuer BotOS
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	02.10.2007
 */

/*
 * Unser kleines BotOS kann mehrere Aufgaben "parallel" abarbeiten.
 * Die ausfuehrenden Einheiten heissen "Thread" (denn sie leben ja
 * alle im selben Adressraum).
 * Es gibt nur einen Scheduler, daher wird nicht weiter zwischen
 * Kernel-Level-Threads und User-Level-Threads unterschieden.
 * Ein Thread besteht aus einem Stack, einem Timestamp "fruehester Zeitpunkt der naechsten 
 * Ausfuehrung" und der letzten Laufzeit, die zusammen den Thread Control Block (TCB) bilden, 
 * und indirekt einer Prioritaet. 
 * Der zuerst angelegte Thread hat die hoechste Prioritaet, der naechste 
 * die Zweithoechste usw. 
 * Es kann somit keine Prioritaet doppelt vergeben werden.
 * Threads koennen zwar zur Laufzeit erzeugt, aber nicht wieder beendet 
 * werden. 
 * 
 * Ein Thread kann eine beliebige Anzahl an ms schlafen und somit die Kontrolle
 * an einen anderen Thread abgeben. 
 * Ruft ein Thread os_thread_yield() auf, schlaeft er (mindestens) fuer die Dauer 
 * einer Zeitscheibe minus seiner bisherigen Laufzeit. Man muss yield() also periodisch 
 * aufrufen, damit das Ganze Sinn macht.
 * Eine Zeitscheibe dauert derzeit 5 ms.
 * Wacht ein hoeher prioraer Thread wieder auf (= Schlafzeit zuende), wird 
 * dem Aktuellen die Ausfuehrung entzogen.
 * Identifiziert werden Threads ueber einen Zeiger auf ihren TCB
 * 
 * 
 * Status: Beta, Testcase funktioniert
 */


/* TODO:
 *  - Kontextsicherung pruefen
 *  - Code optimieren, unnoetiges Zeug rauswerfen
 *  - Stackgroessen nachrechnen
 *  - Doku
 */

#include "ct-Bot.h"
#ifdef MCU
#ifdef OS_AVAILABLE

#include "os_thread.h"
#include <stdlib.h>

/*! Datentyp fuer Instruction-Pointer */
typedef union {
	void * ip;			/*!< Instruction-Pointer des Threads */
	struct {
		uint8_t lo8;	/*!< untere 8 Bit der Adresse */
		uint8_t hi8;	/*!< obere 8 Bit der Adresse */
	};
} os_ip_t;

Tcb_t os_threads[OS_MAX_THREADS];	/*!< Array aller TCBs */
Tcb_t * os_thread_running = NULL;	/*!< Zeiger auf den TCB des Threads, der gerade laeuft */

/*!
 * Legt einen neuen Thread an.
 * Der zuerst angelegt Thread bekommt die hoechste Prioritaet,
 * je spaeter ein Thread erzeugt wird, desto niedriger ist seine
 * Prioritaet, das laesst sich auch nicht mehr aendern!
 * @param *pStack	Zeiger auf den Stack (Ende!) des neuen Threads
 * @param *pIp		Zeiger auf die Main-Funktion des Threads (Instruction-Pointer)
 * @return			Zeiger auf den TCB des angelegten Threads
 */
Tcb_t * os_create_thread(uint8_t * pStack, void * pIp) {
	uint8_t i;
	Tcb_t * ptr = os_threads;
	for (i=0; i<OS_MAX_THREADS; i++, ptr++) {
		if (ptr->stack == NULL) {
			if (os_thread_running == NULL) {
				/* Main-Thread anlegen (laeuft bereits) */
				os_thread_running = ptr;
				ptr->stack = pStack;
			} else {
				/* "normalen" Thread anlegen */
				os_ip_t tmp;
				tmp.ip = pIp;
				/* Return-Adresse liegt in Big-Endian auf dem Stack! */
				*pStack = tmp.lo8;
				*(pStack-1) = tmp.hi8;
				ptr->stack = pStack-2;	// 2x push => Stack-Pointer - 2
			}
			/* TCB zurueckgeben */
			return ptr;
		}
	}
	return NULL;	// Fehler :(
}

/*!
 * Beendet den kritischen Abschnitt wieder, der mit enterCS began
 */
void os_exitCS(void) {
	uint8_t sreg = SREG;
	cli();
	os_scheduling_allowed = 1;
	if (os_reschedule == 1) {
		/* Ein Scheduler-Aufruf steht an, jetzt nachholen */
		SREG = sreg;
		os_schedule(TIMER_GET_TICKCOUNT_32);
	} else {
		SREG = sreg;
	}
}

/*!
 * Schaltet auf den Thread mit der naechst niedrigeren Prioritaet um, der lauffaehig ist,
 * indem ihm der Rest der Zeitscheibe geschenkt wird.
 */
void os_thread_yield(void) {
	uint32_t now = TIMER_GET_TICKCOUNT_32;
	if (now - os_thread_running->lastSchedule > MS_TO_TICKS(OS_TIME_SLICE)) {
		/* Zeitscheibe wurde ueberschritten */
		os_thread_running->nextSchedule = now;
	} else {
		/* Wir haben noch Zeit frei, die wir dem naechsten Thread schenken koennen */
		os_thread_running->nextSchedule = os_thread_running->lastSchedule + MS_TO_TICKS(OS_TIME_SLICE);
	}
	/* Scheduler wechselt die Threads */
	os_schedule(now);
}

/*!
 * Schaltet "von aussen" auf einen neuen Thread um. 
 * => kernel threadswitch
 * Achtung, es wird erwartet, dass Interrupts an sind. 
 * Sollte eigentlich nur vom Scheduler aus aufgerufen werden!
 * @param *from	Zeiger auf TCB des aktuell laufenden Threads
 * @param *to	Zeiger auf TCB des Threads, der nun laufen soll
 */
void os_switch_thread(Tcb_t * from, Tcb_t * to) {
	os_thread_running = to;
	// r18 bis r27 und Z wurden bereits vor dem Eintritt in die Funktion auf dem (korrekten) Stack gesichert!
	asm volatile(
//		"push r0									\n\t"
		"push r1				; save registers	\n\t"
		"push r2									\n\t"
		"push r3									\n\t"
		"push r4									\n\t"
		"push r5									\n\t"
		"push r6									\n\t"
		"push r7									\n\t"
		"push r8									\n\t"
		"push r9									\n\t"
		"push r10									\n\t"
		"push r11									\n\t"
		"push r12									\n\t"
		"push r13									\n\t"
		"push r14									\n\t"
		"push r15									\n\t"
		"push r16									\n\t"
		"push r17									\n\t"
		"in r0, __SREG__		; save SREG			\n\t"
		"push r0 									\n\t"
		"call os_switch_helper	; switch stacks		\n\t"
		"pop r0										\n\t"
		"out __SREG__, r0		; restore SREG		\n\t"
		"pop r17				; restore registers	\n\t"
		"pop r16									\n\t"			
		"pop r15									\n\t"
		"pop r14									\n\t"
		"pop r13									\n\t"
		"pop r12									\n\t"
		"pop r11									\n\t"
		"pop r10									\n\t"			
		"pop r9										\n\t"			
		"pop r8										\n\t"			
		"pop r7										\n\t"			
		"pop r6										\n\t"			
		"pop r5										\n\t"			
		"pop r4										\n\t"			
		"pop r3										\n\t"			
		"pop r2										\n\t"			
		"pop r1										\n\t"			
//		"pop r0											"
		::	"y"(&to->stack), "z"(&from->stack)	// Stackpointer
		:	"memory"
	);
}

/*
 * Hilfsfunktion fuer Thread-Switch, nicht beliebig aufrufbar!
 * (Erwartet Zeiger auf TCBs in Z und Y).
 * Als Funktion, um die Return-Adresse auf dem Stack zu haben.
 */
void os_switch_helper(void) {
	asm volatile(
		"cli				; Interrupts off		\n\t"			
		"in r16, __SP_L__	; switch Stacks			\n\t"
		"st Z+, r16									\n\t"	
		"in r16, __SP_H__							\n\t"
		"st Z, r16									\n\t"	
			//-- live changes here --//
		"ld r16, Y+ 								\n\t"
		"out __SP_L__, r16							\n\t"
		"ld r16, Y 									\n\t"
		"out __SP_H__, r16							\n\t"
		"sei				; Interrupts on				"
	);
}

#endif	// OS_AVAILABLE
#endif	// MCU
