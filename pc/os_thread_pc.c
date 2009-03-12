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
 * @file 	os_thread_pc.c
 * @brief 	Threadmanagement fuer BotOS (PC)
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	15.04.2008
 */

#include "ct-Bot.h"
#ifdef PC
#ifdef OS_AVAILABLE

#include "os_thread.h"
#include "os_utils.h"
#include "log.h"
#include <pthread.h>

//#define DEBUG_THREADING	/*!< Schalter fuer Debug-Ausgaben */
#define DEBUG_THREAD_N	-1	/*!< Thread, dessen Vorgaenge debuggt werden sollen (0-based), -1 fuer alle */

#ifndef LOG_AVAILABLE
#undef DEBUG_THREADING
#endif
#ifndef DEBUG_THREADING
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif


Tcb_t os_threads[OS_MAX_THREADS];	/*!< Array aller Threads */
Tcb_t * os_thread_running = NULL;	/*!< Zeiger auf den Thread, der gerade laeuft */

/*!
 * Gibt einen Zeiger auf den TCB des aktuellen Threads zurueck
 * @return	Zeiger auf TCB aus os_threads[]
 */
static Tcb_t * get_this_thread(void) {
	uint8_t i;
	for (i=0; i<OS_MAX_THREADS-1; i++) {
		if (os_threads[i] == pthread_self()) {
			return &os_threads[i];
		}
	}
	return NULL;
}

/*!
 * Legt einen neuen Thread an.
 * Der zuerst angelegt Thread bekommt die hoechste Prioritaet,
 * je spaeter ein Thread erzeugt wird, desto niedriger ist seine
 * Prioritaet, das laesst sich auch nicht mehr aendern!
 * @param *pStack	Zeiger auf den Stack (Ende!) des neuen Threads
 * @param *pIp		Zeiger auf die Main-Funktion des Threads (Instruction-Pointer)
 * @return			Zeiger auf den TCB des angelegten Threads
 */
Tcb_t * os_create_thread(void * pStack, void * pIp) {
	static uint8_t thread_count = 0;
	if (thread_count == OS_MAX_THREADS - 1) {	// Main-Thread existiert fuer PC nicht im Array
		/* kein Thread mehr moeglich */
		LOG_DEBUG("Thread konnte nicht angelegt werden");
		return NULL;
	}
	uint8_t i = thread_count;
	thread_count++;
	pthread_create(&os_threads[i], NULL, pIp, NULL);
	LOG_DEBUG("Thread 0x%08x als Thread Nr. %u angelegt", &os_threads[i], i);
	/* Zeiger auf TCB des Threads zurueckgeben */
	return &os_threads[i];
}

/*!
 * Schaltet auf den Thread mit der naechst niedrigeren Prioritaet um, der lauffaehig ist,
 * indem diesem der Rest der Zeitscheibe geschenkt wird.
 * Auf PC ueberlassen wir diese Aufgabe dem Scheduler
 */
void os_thread_yield(void) {
#ifndef __APPLE__
	pthread_yield();
#else
	sched_yield();
#endif	// __APPLE__
}

/*!
 * Blockiert den aktuellten Thread fuer die angegebene Zeit und schaltet
 * auf einen anderen Thread um
 * => coorporative threadswitch
 * @param ms	Zeit in ms, die der aktuelle Thread blockiert wird
 */
void os_thread_sleep(uint32_t ms) {
	// NOP
	Tcb_t * thread = get_this_thread();
	if (DEBUG_THREAD_N == -1 || thread == &os_threads[DEBUG_THREAD_N]) {
		LOG_DEBUG("Thread 0x%08x soll %u ms schlafen, os_thread_sleep() ist aber nicht implementiert", thread, ms);
	}
}

///*!
// * Weckt einen wartenden Thread auf, falls dieser eine hoehere Prioritaet hat
// * @param *thread	Zeiger auf TCB des zu weckenden Threads
// */
//void os_thread_wakeup(Tcb_t * thread) {
//	// NOP
//	if (DEBUG_THREAD_N == -1 || thread == &os_threads[DEBUG_THREAD_N]) {
//		LOG_DEBUG("Soll 0x%08x aufwecken, os_thread_wakeup() ist aber nicht implementiert", thread);
//	}
//}

/*!
 * Blockiert den aktuellen Thread, bis ein Signal freigegeben wird
 * @param *signal	Zeiger auf Signal
 */
void os_signal_set(os_signal_t * signal) {
	Tcb_t * thread = get_this_thread();
	if (DEBUG_THREAD_N == -1 || thread == &os_threads[DEBUG_THREAD_N]) {
		LOG_DEBUG("Setze Signal 0x%08x fuer Thread 0x%08x", signal, thread);
	}
	pthread_mutex_lock(&signal->mutex);
	if (signal->value == 1) {
		if (DEBUG_THREAD_N == -1 || thread == &os_threads[DEBUG_THREAD_N]) {
			LOG_DEBUG("Thread 0x%08x wird blockiert", thread);
		}
		pthread_cond_wait(&signal->cond, &signal->mutex);
		if (DEBUG_THREAD_N == -1 || thread == &os_threads[DEBUG_THREAD_N]) {
			LOG_DEBUG("Thread 0x%08x laeuft weiter", thread);
		}
	}
}

/*!
 * Entfernt ein Signal vom aktuellen Thread
 * @param *signal	Signal, das entfernt werden soll
 */
void os_signal_release(os_signal_t * signal) {
	pthread_mutex_unlock(&signal->mutex);
	Tcb_t * thread = get_this_thread();
	if (DEBUG_THREAD_N == -1 || thread == &os_threads[DEBUG_THREAD_N]) {
		LOG_DEBUG("Entferne Signal 0x%08x von Thread 0x%08x", signal, thread);
	}
}

/*!
 * Sperrt ein Signal
 * @param *signal	Zu sperrendes Signal
 */
void os_signal_lock(os_signal_t * signal) {
	pthread_mutex_lock(&signal->mutex);
	signal->value = 1;
	pthread_mutex_unlock(&signal->mutex);
	Tcb_t * thread = get_this_thread();
	if (DEBUG_THREAD_N == -1 || thread == &os_threads[DEBUG_THREAD_N]) {
		LOG_DEBUG("Signal 0x%08x gesperrt", signal);
	}
}

/*!
 * Gibt ein Signal frei
 * @param *signal	Freizugebendes Signal
 */
void os_signal_unlock(os_signal_t * signal) {
	pthread_mutex_lock(&signal->mutex);
	signal->value = 0;
	pthread_cond_broadcast(&signal->cond);
	pthread_mutex_unlock(&signal->mutex);
	Tcb_t * thread = get_this_thread();
	if (DEBUG_THREAD_N == -1 || thread == &os_threads[DEBUG_THREAD_N]) {
		LOG_DEBUG("Signal 0x%08x freigegeben", signal);
	}
}

#endif	// OS_AVAILABLE
#endif	// PC
