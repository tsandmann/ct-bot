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
#include <stdlib.h>

#define OS_MAX_THREADS		3	/*!< maximale Anzahl an Threads im System */
#define OS_KERNEL_STACKSIZE	32	/*!< Groesse des Kernel-Stacks (fuer Timer-ISR) */
#define OS_IDLE_STACKSIZE	64	/*!< Groesse des Idle-Stacks */
//#define OS_DEBUG				/*!< Schalter fuer Debug-Code */
//#define OS_KERNEL_LOG_AVAILABLE	/*!< Aktiviert das Kernel-LOG mit laufenden Debug-Ausgaben */

#ifdef OS_KERNEL_LOG_AVAILABLE
#undef OS_IDLE_STACKSIZE
#define OS_IDLE_STACKSIZE	192
#endif

#if OS_MAX_THREADS < 2
#error "OS_MAX_THREADS muss >= 2 sein"
#endif

#ifdef MAP_AVAILABLE
#if OS_MAX_THREADS < 3
#error "OS_MAX_THREADS muss >= 3 sein, wenn MAP_AVAILABLE"
#endif
#endif

#ifdef PC
#undef OS_DEBUG
#include <pthread.h>
#endif

/*! Signal-Typ zur Threadsynchronisation */
typedef struct {
	volatile uint8_t value;	/*!< Signal-Wert */
#ifdef PC
	pthread_mutex_t mutex;	/*!< Mutex zur Synchronisation */
	pthread_cond_t cond;	/*!< Signal zur Synchronisation */
#endif
} os_signal_t;

/*! Statistikdaten wie Laufzeit und Anzahl der ueberschrittenen Deadlines */
typedef struct {
	uint16_t runtime;			/*!< Zeit, die der Thread gelaufen ist [176 us] */
	uint16_t missed_deadlines;	/*!< Anzahl der Deadlines, die der Thread ueberschritten hat */
} os_stat_data_t;

#ifdef MCU
/*! TCB eines Threads */
typedef struct {
	void * stack;				/*!< Stack-Pointer */
	uint32_t nextSchedule;		/*!< Zeitpunkt der naechsten Ausfuehrung. Ergibt im Zusammenhang mit der aktuellen Zeit den Status eines Threads. */
	uint16_t lastSchedule;		/*!< Zeitpunkt der letzten Ausfuehrung, untere 16 Bit */
	os_signal_t * wait_for;		/*!< Zeiger auf Signal, bis zu dessen Freigabe blockiert wird */
#ifdef MEASURE_UTILIZATION
	os_stat_data_t statistics;	/*!< Statistikdaten des Threads */
#endif
} Tcb_t;

extern Tcb_t os_threads[OS_MAX_THREADS];	/*!< Thread-Pool (ist gleichzeitig running- und waiting-queue) */
extern Tcb_t * os_thread_running;			/*!< Zeiger auf den Thread, der zurzeit laeuft */
extern uint8_t os_kernel_stack[];			/*!< Kernel-Stack */
extern uint8_t os_idle_stack[];				/*!< Stack des Idle-Threads */
extern os_signal_t dummy_signal; 			/*!< Signal, das referenziert wird, wenn sonst keins gesetzt ist */

#ifdef OS_KERNEL_LOG_AVAILABLE
#define OS_KERNEL_LOG_SIZE	32	/*!< Anzahl der Datensaetze, die im Kernel-LOG gepuffert werden koennen */

/*!
 * Initialisiert das Kernel-LOG
 */
void os_kernel_log_init(void);
#endif	// OS_KERNEL_LOG_AVAILABLE

/*!
 * Idle-Thread
 */
void os_idle(void) __attribute__((OS_task));

/*!
 * Schuetzt den folgenden Block (bis os_exitCS()) vor Threadswitches.
 * Ermoeglicht einfaches Locking zum exklusiven Ressourcen-Zugriff.
 * Es ist allerdings keine Verschachtelung moeglich! Zwischen os_enterCS()
 * und os_exitCS() sollte daher kein Funktionsaufruf erfolgen.
 */
#define os_enterCS() {								\
	os_scheduling_allowed = 0;						\
}

/*!
 * Beendet den kritischen Abschnitt wieder, der mit os_enterCS began.
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

/*!
 * Entfernt ein Signal vom aktuellen Thread
 * @param *signal	Signal, das entfernt werden soll
 */
static inline void os_signal_release(os_signal_t * signal) {
	os_thread_running->wait_for = &dummy_signal;
}

/*!
 * Sperrt ein Signal
 * @param *signal	Zu sperrendes Signal
 */
static inline void os_signal_lock(os_signal_t * signal) {
	signal->value = 1;
}

/*!
 * Gibt ein Signal frei
 * @param *signal	Freizugebendes Signal
 */
static inline void os_signal_unlock(os_signal_t * signal) {
	signal->value = 0;
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

/*!
 * Blockiert den aktuellten Thread fuer die angegebene Zeit und schaltet
 * auf einen anderen Thread um
 * => coorporative threadswitch
 * @param sleep		Zeit in ms, die der aktuelle Thread blockiert wird
 */
void os_thread_sleep(uint32_t sleep);

/*!
 * Entfernt ein Signal vom aktuellen Thread
 */
void os_signal_release(os_signal_t * signal);

/*!
 * Sperrt ein Signal
 * @param *signal	Zu sperrendes Signal
 */
void os_signal_lock(os_signal_t * signal);

/*!
 * Gibt ein Signal frei
 * @param *signal	Freizugebendes Signal
 */
void os_signal_unlock(os_signal_t * signal);
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
Tcb_t * os_create_thread(void * pStack, void * pIp);

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

/*!
 * Blockiert den aktuellen Thread, bis ein Signal freigegeben wird
 * @param *signal	Zeiger auf Signal
 */
void os_signal_set(os_signal_t * signal);

#ifdef OS_DEBUG
/*!
 * Maskiert einen Stack, um spaeter ermitteln zu koennen,
 * wieviel Byte ungenutzt bleiben
 * @param *stack	Anfangsadresse des Stacks
 * @param size		Groesse des Stacks in Byte
 */
void os_mask_stack(void * stack, size_t size);

/*!
 * Gibt per LOG aus, wieviel Bytes auf den Stacks der Thread noch nie benutzt wurden
 */
void os_print_stackusage(void);

/*!
 * Gibt den Inhalt des Stacks eines Threads per LOG aus
 * @param *thread	Zeiger auf den TCB des Threads
 * @param *stack	Zeiger auf die hoechste Adresse des Stacks (Anfang)
 * @param size		Groesse des Stacks in Byte
 */
void os_stack_dump(Tcb_t * thread, void * stack, uint16_t size);
#endif	// OS_DEBUG

#else 	// OS_AVAILABLE

#define os_enterCS()	// NOP
#define os_exitCS()		// NOP

#endif	// OS_AVAILABLE
#endif	// _THREAD_H
