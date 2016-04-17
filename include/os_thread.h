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
 * \file 	os_thread.h
 * \brief 	Threadmanagement fuer BotOS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	02.10.2007
 */

#ifndef _OS_THREAD_H_
#define _OS_THREAD_H_

#ifdef PC
#undef OS_DEBUG
#include <pthread.h>
#endif // PC

#ifdef OS_AVAILABLE
#include "timer.h"
#include "os_scheduler.h"

/** Signal-Typ zur Threadsynchronisation */
typedef struct {
	volatile uint8_t value;	/**< Signal-Wert */
#ifdef PC
	pthread_mutex_t mutex;	/**< Mutex zur Synchronisation */
	pthread_cond_t cond;	/**< Signal zur Synchronisation */
#endif
} os_signal_t;


typedef void (* os_delayed_func_ptr_t)(void*); /** Zeiger-Typ fuer verzoegerte Funktionen */

/** Interne Datenstruktur fuer die Registrierung von verzoegert auszufuehrende Funktionen */
typedef struct {
	os_delayed_func_ptr_t p_func; /**< Zeiger auf Funktion */
	void* p_data; /**< Zeiger auf Daten fuer Funktion (optional) */
	uint32_t runtime; /**< fruehst moegliche Ausfuehrungszeit in Timer-Ticks */
} os_delayed_func_t;

#define OS_MAX_THREADS		4	/**< maximale Anzahl an Threads im System */
#define OS_KERNEL_STACKSIZE	36	/**< Groesse des Kernel-Stacks (fuer Timer-ISR) [Byte] */
#define OS_IDLE_STACKSIZE	64	/**< Groesse des Idle-Stacks [Byte] */
#define OS_CONTEXT_SIZE		19	/**< Groesse des Kontextes eines Threads [Byte], muss zum Code in os_switch_thread() passen! */
#define OS_DELAYED_FUNC_CNT	8	/**< Anzahl der maximal registrierbaren Funktionen zur verzoegerten Ausfuehrung */

//#define OS_DEBUG				/**< Schalter fuer Debug-Code */
//#define OS_KERNEL_LOG_AVAILABLE	/**< Aktiviert das Kernel-LOG mit laufenden Debug-Ausgaben */

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

/** Statistikdaten wie Laufzeit und Anzahl der ueberschrittenen Deadlines */
typedef struct {
	uint16_t runtime;			/**< Zeit, die der Thread gelaufen ist [176 us] */
	uint16_t missed_deadlines;	/**< Anzahl der Deadlines, die der Thread ueberschritten hat */
} os_stat_data_t;

#ifdef MCU

#define OS_TASK_ATTR __attribute__((OS_task)) /**< Attribut fuer main-Funktion eines Threads */
#define OS_SIGNAL_INITIALIZER {0}  /**< Initialisierungsdaten fuer os_signal_t */

/** TCB eines Threads */
typedef struct {
	void * stack;				/**< Stack-Pointer */
	uint32_t nextSchedule;		/**< Zeitpunkt der naechsten Ausfuehrung. Ergibt im Zusammenhang mit der aktuellen Zeit den Status eines Threads. */
	uint16_t lastSchedule;		/**< Zeitpunkt der letzten Ausfuehrung, untere 16 Bit */
	os_signal_t * wait_for;		/**< Zeiger auf Signal, bis zu dessen Freigabe blockiert wird */
#ifdef MEASURE_UTILIZATION
	os_stat_data_t statistics;	/**< Statistikdaten des Threads */
#endif
} Tcb_t;

extern Tcb_t os_threads[OS_MAX_THREADS];	/**< Thread-Pool (ist gleichzeitig running- und waiting-queue) */
extern Tcb_t * os_thread_running;			/**< Zeiger auf den Thread, der zurzeit laeuft */
extern uint8_t os_kernel_stack[];			/**< Kernel-Stack */
extern uint8_t os_idle_stack[];				/**< Stack des Idle-Threads */
extern os_signal_t dummy_signal; 			/**< Signal, das referenziert wird, wenn sonst keins gesetzt ist */
extern os_delayed_func_t os_delayed_func[];	/**< Registrierte Funktionen zur verzoegerten Ausfuehrung */
extern volatile os_delayed_func_t* os_delayed_next_p; /**< Zeiger auf die naechste auszufuehrende verzoegerte Funktion */


#ifdef OS_KERNEL_LOG_AVAILABLE
#define OS_KERNEL_LOG_SIZE	32	/**< Anzahl der Datensaetze, die im Kernel-LOG gepuffert werden koennen */

/**
 * Initialisiert das Kernel-LOG
 */
void os_kernel_log_init(void);
#endif // OS_KERNEL_LOG_AVAILABLE

/**
 * Idle-Thread
 */
void os_idle(void) OS_TASK_ATTR;

/**
 * Schuetzt den folgenden Block (bis os_exitCS()) vor Threadswitches.
 * Ermoeglicht einfaches Locking zum exklusiven Ressourcen-Zugriff.
 * Es ist allerdings keine Verschachtelung moeglich! Zwischen os_enterCS()
 * und os_exitCS() sollte daher kein Funktionsaufruf erfolgen.
 */
#define os_enterCS() { \
	os_scheduling_allowed = 0; \
	__asm__ __volatile__("":::"memory"); \
}

/**
 * Beendet den kritischen Abschnitt wieder, der mit os_enterCS began.
 * Falls ein Scheduler-Aufruf ansteht, wird er nun ausgefuehrt.
 */
void os_exitCS(void);

/**
 * Schaltet "von aussen" auf einen neuen Thread um.
 * => kernel threadswitch
 * Achtung, es wird erwartet, dass Interrupts an sind.
 * Sollte eigentlich nur vom Scheduler aus aufgerufen werden!
 * \param *from	Zeiger auf TCB des aktuell laufenden Threads
 * \param *to	Zeiger auf TCB des Threads, der nun laufen soll
 */
void os_switch_thread(Tcb_t * from, Tcb_t * to);

/**
 * Blockiert den aktuellten Thread fuer die angegebene Zeit und schaltet
 * auf einen anderen Thread um
 * => coorporative threadswitch
 * \param ms	Zeit in ms, die der aktuelle Thread (mindestens) blockiert wird
 */
static inline void os_thread_sleep(uint32_t ms) {
	uint32_t sleep_ticks = MS_TO_TICKS(ms); // Zeitspanne in Timer-Ticks umrechnen
	uint32_t now = TIMER_GET_TICKCOUNT_32;  // Aktuelle Systemzeit
	os_thread_running->nextSchedule = now + sleep_ticks;
	os_schedule(now); // Aufruf des Schedulers
}

/**
 * Blockiert den aktuellten Thread fuer die angegebene Zeit und schaltet
 * auf einen anderen Thread um
 * => coorporative threadswitch
 * \param ticks	Zeit in 176 us, die der aktuelle Thread (mindestens) blockiert wird
 */
static inline void os_thread_sleep_ticks(uint16_t ticks) {
	if (ticks) {
		uint32_t now = TIMER_GET_TICKCOUNT_32; // Aktuelle Systemzeit
		os_thread_running->nextSchedule = now + ticks;
		os_schedule(now); // Aufruf des Schedulers
	}
}

/**
 * Entfernt ein Signal vom aktuellen Thread
 * \param *signal	Signal, das entfernt werden soll
 */
static inline void os_signal_release(os_signal_t * signal) {
	(void) signal; // kein warning
	os_thread_running->wait_for = &dummy_signal;
}

/**
 * Sperrt ein Signal
 * \param *signal	Zu sperrendes Signal
 */
static inline void os_signal_lock(os_signal_t * signal) {
	signal->value = 1;
}

/**
 * Gibt ein Signal frei
 * \param *signal	Freizugebendes Signal
 */
static inline void os_signal_unlock(os_signal_t * signal) {
	signal->value = 0;
	os_schedule(TIMER_GET_TICKCOUNT_32);
}

#else // PC

#define OS_TASK_ATTR /**< Attribut fuer main-Funktion eines Threads (Dummy fuer PC) */
#define OS_SIGNAL_INITIALIZER {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER} /**< Initialisierungsdaten fuer os_signal_t */

typedef pthread_t Tcb_t;

extern Tcb_t os_threads[OS_MAX_THREADS]; /**< Thread-Pool (ist gleichzeitig running- und waiting-queue) */
extern pthread_mutex_t os_enterCS_mutex; /**< Mutex fuer os_enterCS() / os_exitCS() auf PC */

/**
 * Schuetzt den folgenden Block (bis exitCS()) vor konkurrierenden Zugriffen
 * verschiedener Threads.
 * Ermoeglicht einfaches Locking zum exklusiven Ressourcen-Zugriff.
 */
#define os_enterCS() pthread_mutex_lock(&os_enterCS_mutex);

/**
 * Beendet den kritischen Abschnitt wieder, der mit enterCS began.
 */
#define os_exitCS() pthread_mutex_unlock(&os_enterCS_mutex);

/**
 * Blockiert den aktuellten Thread fuer die angegebene Zeit und schaltet
 * auf einen anderen Thread um
 * => coorporative threadswitch
 * \param sleep		Zeit in ms, die der aktuelle Thread blockiert wird
 */
void os_thread_sleep(uint32_t sleep);

/**
 * Entfernt ein Signal vom aktuellen Thread
 */
void os_signal_release(os_signal_t * signal);

/**
 * Sperrt ein Signal
 * \param *signal	Zu sperrendes Signal
 */
void os_signal_lock(os_signal_t * signal);

/**
 * Gibt ein Signal frei
 * \param *signal	Freizugebendes Signal
 */
void os_signal_unlock(os_signal_t * signal);
#endif // MCU

/**
 * Legt einen neuen Thread an und setzt ihn auf runnable.
 * Der zuerst angelegt Thread bekommt die hoechste Prioritaet,
 * je spaeter ein Thread erzeugt wird, desto niedriger ist seine
 * Prioritaet, das laesst sich auch nicht mehr aendern!
 * \param *pStack	Zeiger auf den Stack (Ende!) des neuen Threads
 * \param *pIp		Zeiger auf die Main-Funktion des Threads (Instruction-Pointer)
 * \return			Zeiger auf den TCB des angelegten Threads
 */
Tcb_t * os_create_thread(void * pStack, void (* pIp)(void));

/**
 * Schaltet auf den Thread mit der naechst niedrigeren Prioritaet um, der lauffaehig ist,
 * indem diesem der Rest der Zeitscheibe geschenkt wird.
 */
void os_thread_yield(void);

/**
 * Blockiert den aktuellen Thread, bis ein Signal freigegeben wird
 * \param *signal	Zeiger auf Signal
 */
void os_signal_set(os_signal_t * signal);

/**
 * Sucht die als naechstes auszufuehrende Funktion heraus, wird intern benutzt.
 * @return Naechste auszufuehrende Funktion oder NULL, falls keine Funktion registriert
 */
os_delayed_func_t* os_delayed_func_search_next(void);

/**
 * Registriert eine Funktion zur spaeteren Ausfuehrung.
 * @param p_func Zeiger auf die Funktion
 * @param p_data Zeiger auf Daten fuer die Funktion oder NULL
 * @param delay_ms Zeit in ms, nach der die Funktion (fruehestens) ausgefuehrt werden soll
 * @return 0, falls Funktion korrekt registriert werden konnte, 1 sonst
 */
uint8_t os_delay_func(os_delayed_func_ptr_t p_func, void* p_data, uint32_t delay_ms);

#ifdef OS_DEBUG
/**
 * Maskiert einen Stack, um spaeter ermitteln zu koennen,
 * wieviel Byte ungenutzt bleiben
 * \param *stack	Anfangsadresse des Stacks
 * \param size		Groesse des Stacks in Byte
 */
void os_mask_stack(void * stack, size_t size);

/**
 * Gibt per LOG aus, wieviel Bytes auf den Stacks der Threads noch nie benutzt wurden
 */
void os_print_stackusage(void);

/**
 * Gibt den Inhalt des Stacks eines Threads per LOG aus
 * \param *thread	Zeiger auf den TCB des Threads
 * \param *stack	Zeiger auf die hoechste Adresse des Stacks (Anfang)
 * \param size		Groesse des Stacks in Byte
 */
void os_stack_dump(Tcb_t * thread, void * stack, uint16_t size);
#endif // OS_DEBUG

#else // OS_AVAILABLE

#define os_enterCS() // NOP
#define os_exitCS() // NOP

#endif // OS_AVAILABLE
#endif // _OS_THREAD_H_
