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
#include <stdlib.h>
#include <pthread.h>

Tcb_t os_threads[OS_MAX_THREADS];	/*!< Array aller Threads */
Tcb_t * os_thread_running = NULL;	/*!< Zeiger auf den Thread, der gerade laeuft */

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
	for (i=0; i<OS_MAX_THREADS; i++) {	
		if (os_threads[i] == NULL) {
			pthread_create(&os_threads[i], NULL, pIp, NULL);
			/* Thread zurueckgeben */
			return &os_threads[i];
		}
	}
	return NULL;	// Fehler :(
}

/*!
 * Schaltet auf den Thread mit der naechst niedrigeren Prioritaet um, der lauffaehig ist,
 * indem diesem der Rest der Zeitscheibe geschenkt wird.
 */
void os_thread_yield(void) {
	// NOP
	//pthread_yield();
}

/*!
 * Weckt einen wartenden Thread auf, falls dieser eine hoehere Prioritaet hat
 * @param *thread	Zeiger auf TCB des zu weckenden Threads
 */
void os_thread_wakeup(Tcb_t * thread) {
	// NOP
}

#endif	// OS_AVAILABLE
#endif	// PC
