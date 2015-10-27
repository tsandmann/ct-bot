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
 * \file 	os_thread.c
 * \brief 	Threadmanagement fuer BotOS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	02.10.2007
 */

/*
 * Dokumentation: siehe Documentation/BotOS.html
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef OS_AVAILABLE
#include "os_thread.h"
#include "os_utils.h"
#include "log.h"
#include "map.h"
#include <string.h>
#include <stdio.h>

/** Datentyp fuer Instruction-Pointer */
typedef union {
	void (* ip)(void);	/**< Instruction-Pointer des Threads */
	struct {
		uint8_t lo8;	/**< untere 8 Bit der Adresse */
		uint8_t hi8;	/**< obere 8 Bit der Adresse */
	} PACKED_FORCE bytes;
} os_ip_t;

Tcb_t os_threads[OS_MAX_THREADS];				/**< Array aller TCBs */
Tcb_t * os_thread_running = NULL;				/**< Zeiger auf den TCB des Threads, der gerade laeuft */
uint8_t os_kernel_stack[OS_KERNEL_STACKSIZE];	/**< Kernel-Stack */
os_signal_t dummy_signal;						/**< Signal, das referenziert wird, wenn sonst keins gesetzt ist */

/**
 * Legt einen neuen Thread an.
 * Der zuerst angelegt Thread bekommt die hoechste Prioritaet,
 * je spaeter ein Thread erzeugt wird, desto niedriger ist seine
 * Prioritaet, das laesst sich auch nicht mehr aendern!
 * \param *pStack	Zeiger auf den Stack (Ende!) des neuen Threads
 * \param *pIp		Zeiger auf die Main-Funktion des Threads (Instruction-Pointer)
 * \return			Zeiger auf den TCB des angelegten Threads
 */
Tcb_t * os_create_thread(void * pStack, void (* pIp)(void)) {
	os_enterCS(); // Scheduler deaktivieren
	uint8_t i;
	Tcb_t * ptr = os_threads;
	// os_scheduling_allowed == 0 wegen os_enterCS()
	for (i = os_scheduling_allowed; i < OS_MAX_THREADS; ++i, ++ptr) {
		if (ptr->stack == NULL) {
			ptr->wait_for = &dummy_signal; // wait_for belegen
			if (os_thread_running == NULL) {
				/* Main-Thread anlegen (laeuft bereits) */
				os_thread_running = ptr;
				ptr->stack = pStack;
#ifdef OS_DEBUG
				os_mask_stack(os_kernel_stack, OS_KERNEL_STACKSIZE);
#endif
			} else {
				/* "normalen" Thread anlegen */
				os_ip_t tmp;
				tmp.ip = pIp;
				/* Stack so aufbauen, als waere der Thread bereits einmal unterbrochen worden */
				uint8_t * sp = pStack; // Entry-Point des Threads
				*sp = tmp.bytes.lo8; // Return-Adresse liegt in Big-Endian auf dem Stack!
				*(sp - 1) = tmp.bytes.hi8;
				tmp.ip = &os_exitCS; // setzt os_scheduling_allowed auf 1, nachdem der Thread das erste Mal geschedult wurde
				*(sp - 2) = tmp.bytes.lo8;
				*(sp - 3) = tmp.bytes.hi8;
				*(sp - 4) = SREG; // beim ersten Wechsel auf diesen Thread wird das Statusregister vom Stack geholt, darum hier auf den Stack schreiben
				uint8_t * p = pStack;
				ptr->stack = p - (4 + OS_CONTEXT_SIZE); // 4x push und OS_CONTEXT_SIZE Bytes fuer Kontext => Stack-Pointer - (4 + OS_CONTEXT_SIZE)
			}
			os_scheduling_allowed = 1; // Scheduling wieder erlaubt
			/* TCB zurueckgeben */
			return ptr;
		}
	}
	os_scheduling_allowed = 1; // Scheduling wieder erlaubt
	return NULL; // Fehler :(
}

/**
 * Beendet den kritischen Abschnitt wieder, der mit os_enterCS began.
 * Falls ein Scheduler-Aufruf ansteht, wird er nun ausgefuehrt.
 */
void os_exitCS(void) {
	if (test_and_set((uint8_t *) &os_scheduling_allowed, 1) == 2) {
		os_schedule(TIMER_GET_TICKCOUNT_32);
	}
	__asm__ __volatile__("":::"memory");
}

/**
 * Schaltet auf den Thread mit der naechst niedrigeren Prioritaet um, der lauffaehig ist,
 * indem diesem der Rest der Zeitscheibe geschenkt wird.
 */
void os_thread_yield(void) {
	os_enterCS();
	uint32_t now = TIMER_GET_TICKCOUNT_32;
	/* Zeitpunkt der letzten Ausfuehrung nur in 16 Bit, da yield() nur Sinn macht, wenn es
	 * regelmaessig aufgerufen wird, sonst sleep() benutzen. */
	uint16_t runtime = (uint16_t) now - (uint16_t) os_thread_running->lastSchedule;
	if (runtime > MS_TO_TICKS(OS_TIME_SLICE)) {
		/* Zeitscheibe wurde bereits ueberschritten ==> kein Threadwechsel */
#ifdef OS_KERNEL_LOG_AVAILABLE
		LOG_DEBUG("%u missed dl\t%5uus", os_thread_running - os_threads, runtime * TIMER_STEPS);
#endif // OS_KERNEL_LOG_AVAILABLE
#ifdef MEASURE_UTILIZATION
		os_thread_running->statistics.runtime += runtime;
		/* Zaehler fuer verpasste Deadlines erhoehen */
		os_thread_running->statistics.missed_deadlines++;
#endif // MEASURE_UTILIZATION
		/* Timestamp zuruecksetzen */
		os_thread_running->lastSchedule = (uint16_t) now;
	} else {
		/* Wir haben noch Zeit frei, die wir dem naechsten Thread schenken koennen */
		os_thread_running->nextSchedule = now + (uint16_t) (MS_TO_TICKS(OS_TIME_SLICE) - runtime);
		/* Scheduler wechselt die Threads, Aufruf am Ende der Funktion */
		os_scheduling_allowed = 2;
	}
	/* os_exitCS() */
	if (test_and_set((uint8_t *) &os_scheduling_allowed, 1) == 2) {
		os_schedule(now);
	}
}

/**
 * Blockiert den aktuellen Thread, bis ein Signal freigegeben wird
 * \param *signal	Zeiger auf Signal
 */
void os_signal_set(os_signal_t * signal) {
	os_thread_running->wait_for = signal;
	os_schedule(TIMER_GET_TICKCOUNT_32);
}

/**
 * Schaltet "von aussen" auf einen neuen Thread um.
 * => kernel threadswitch
 * Achtung, es wird erwartet, dass Interrupts an sind.
 * Sollte eigentlich nur vom Scheduler aus aufgerufen werden!
 * \param *from	Zeiger auf TCB des aktuell laufenden Threads
 * \param *to	Zeiger auf TCB des Threads, der nun laufen soll
 */
void os_switch_thread(Tcb_t * from, Tcb_t * to) {
	os_thread_running = to;
	/* r0, r1, r18 bis r27, Z und SREG werden hier nach folgender Ueberlegung NICHT gesichert:
	 * r0, r18 bis r27 und Z wurden bereits vor dem Eintritt in diese Funktion auf dem (korrekten) Stack gesichert!
	 * Falls wir aus der Timer-ISR kommen, wurden auch r1 und das Statusregister bereits gerettet. Falls nicht,
	 * duerfen wir das SREG ruhig ueberschreiben, weil der (noch) aktuelle Thread in diesem Fall den Scheduler explizit
	 * aufrufen wollte, der Compiler also weiss, dass sich das Statusregister aendern kann (kooperativer Threadswitch).
	 * In r1 muss vor jedem Funktionsaufruf bereits 0 stehen.
	 */
	__asm__ __volatile__(
		"in r1, __SREG__		; r1 == 0			\n\t"
		"cli					; interrupts off	\n\t"
		"push r1				; push SREG			\n\t"
		"push r2				; save GP registers	\n\t"
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
		"push r28									\n\t"
		"push r29									\n\t"
	//-- hier ist noch "from" (Z) der aktive Thread 	--//
		"in r16, __SP_L__	; switch Stacks			\n\t"
		"st Z+, r16									\n\t"
		"in r16, __SP_H__							\n\t"
		"st Z, r16									\n\t"
				//-- live changes here --//
		"ld r16, X+ 								\n\t"
		"out __SP_L__, r16							\n\t"
		"ld r16, X 									\n\t"
		"out __SP_H__, r16							\n\t"
	//-- jetzt ist schon "to" (X) der aktive Thread 	--//
		"pop r29			; restore registers		\n\t"
		"pop r28									\n\t"
		"pop r17									\n\t"
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
		"pop r1					; load SREG			\n\t"
		"out __SREG__, r1		; restore SREG		\n\t"
		"clr r1					; cleanup r1			"
		::	"x" (&to->stack), "z" (&from->stack) // Stackpointer
		:	"memory"
	);
}

#ifdef OS_DEBUG
/**
 * Maskiert einen Stack, um spaeter ermitteln zu koennen,
 * wieviel Byte ungenutzt bleiben
 * \param *stack	Anfangsadresse des Stacks
 * \param size		Groesse des Stacks in Byte
 */
void os_mask_stack(void * stack, size_t size) {
	memset(stack, 0x42, size);
}

/**
 * Ermittelt wieviel Bytes auf einem Stack bisher
 * ungenutzt sind. Der Stack muss dafuer VOR der
 * Initialisierung seines Threads mit
 * os_stack_mask() praepariert worden sein!
 * \param *stack	Anfangsadresse des Stacks
 */
static uint16_t os_stack_unused(void * stack) {
	uint8_t * ptr = stack;
	uint16_t unused = 0;
	while (*ptr == 0x42) {
		unused++;
		ptr++;
	}
	return unused;
}

/**
 * Gibt per LOG aus, wieviel Bytes auf den Stacks der Threads noch nie benutzt wurden
 */
void os_print_stackusage(void) {
	uint16_t tmp;
#ifdef MAP_AVAILABLE
	static uint16_t map_stack_free = UINT16_MAX;
	tmp = os_stack_unused(map_update_stack);
	if (tmp < map_stack_free) {
		map_stack_free = tmp;
		LOG_INFO("Map-Stack unused=%u", tmp);
	}
#ifdef MAP_2_SIM_AVAILABLE
	static uint16_t map_2_sim_stack_free = UINT16_MAX;
	tmp = os_stack_unused(map_2_sim_worker_stack);
	if (tmp < map_2_sim_stack_free) {
		map_2_sim_stack_free = tmp;
		LOG_INFO("Map-2-Sim-Stack unused=%u", tmp);
	}
#endif // MAP_2_SIM_AVAILABLE
#endif // MAP_AVAILABLE
	static uint16_t kernel_stack_free = UINT16_MAX;
	tmp = os_stack_unused(os_kernel_stack);
	if (tmp < kernel_stack_free) {
		kernel_stack_free = tmp;
		LOG_INFO("Kernel-Stack unused=%u", tmp);
	}
	static uint16_t idle_stack_free = UINT16_MAX;
	tmp = os_stack_unused(os_idle_stack);
	if (tmp < idle_stack_free) {
		idle_stack_free = tmp;
		LOG_INFO("Idle-Stack unused=%u", tmp);
	}
}

/**
 * Gibt den Inhalt des Stacks eines Threads per LOG aus
 * \param *thread	Zeiger auf den TCB des Threads
 * \param *stack	Zeiger auf die hoechste Adresse des Stacks (Anfang)
 * \param size		Groesse des Stacks in Byte
 */
void os_stack_dump(Tcb_t * thread, void * stack, uint16_t size) {
	size_t n;
	if (thread == os_thread_running) {
		n = (size_t) stack - (size_t) (void *) SP;
	} else {
		n = (size_t) stack - (size_t) thread->stack;
	}
	LOG_INFO("Thread 0x%04x\t uses %u Bytes", (size_t) thread, n);
	size_t i;
	uint8_t * ptr = stack;
	for (i=n; i>0; i--) {
		LOG_INFO("0x%04x:\t0x%02x", ptr, *ptr);
		ptr--;
	}
	LOG_INFO("0x%04x:\t0x%02x <= SP", ptr, *ptr);
	ptr--;
	for (i=size-n; i>1; i--) {
		LOG_INFO("0x%04x:\t0x%02x", ptr, *ptr);
		ptr--;
	}
}
#endif // OS_DEBUG

#endif // OS_AVAILABLE
#endif // MCU
