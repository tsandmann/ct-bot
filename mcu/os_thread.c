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
 * Ein Thread besteht aus einem Stack, einem Status (RUNNING, RUNNABLE
 * oder BLOCKED), einem Timestamp (fruehester Zeitpunkt der naechsten 
 * Ausfuehrung), die zusammen den Thread Control Block (TCB) bilden, 
 * und indirekt einer Prioritaet. 
 * Der zuerst angelegte Thread hat die hoechste Prioritaet, der naechste 
 * die Zweithoechste usw. 
 * Es kann somit keine Prioritaet doppelt vergeben werden.
 * Threads koennen zwar zur Laufzeit erzeugt, aber nicht wieder beendet 
 * werden. 
 * 
 * Ein Thread kann eine beliebige Anzahl an ms schlafen und die Kontrolle
 * an einen anderen Thread abgeben. 
 * Wacht ein hoeher prioraer Thread wieder auf (= Schlafzeit zuende), wird 
 * dem Aktuellen die Ausfuehrung entzogen.
 * Identifiziert werden Threads ueber einen Zeiger auf ihren TCB
 * 
 * 
 * Status: unfertig, ungetestet
 */


/* TODO:
 *  - In der Initialisierung einen Dummy-Kontext erzeugen
 * 	- Main-Thread anlegen (hoechste Prioritaet)
 *  - Fehler abfangen
 *  - Kontextsicherung pruefen
 *  - Scheduler jede ms per Timer aufrufen
 *  - yield() implementieren
 *  - sleep(x, NULL) implementieren
 *  - Code optimieren, unnoetiges Zeug rauswerfen
 *  - #define-Abhaengigkeiten einbauen (nur fuer MCU, MAP und MMC an)
 *  - Doku
 *  - testen waer auch net schlecht...
 * 
 */
#include "ct-Bot.h"
#ifdef MCU
#ifdef OS_AVAILABLE

#include "os_thread.h"
#include <stdlib.h>

Tcb_t os_threads[OS_MAX_THREADS];	/*!< Array aller TCBs */
Tcb_t * os_thread_running = NULL;	/*!< Zeiger auf den TCB des Threads, der gerade laeuft */

/*!
 * Legt einen neuen Thread an und setzt ihn auf runnable.
 * Der zuerst angelegt Thread bekommt die hoechste Prioritaet,
 * je spaeter ein Thread erzeugt wird, desto niedriger ist seine
 * Prioritaet, das laesst sich auch nicht mehr aendern!
 * @param *pStack	Zeiger auf den Stack des neuen Threads
 * @param *pIp		Zeiger auf die Main-Funktion des Threads
 * @return			Zeiger auf den TCB des angelegten Threads
 */
Tcb_t * os_create_thread(void * pStack, void * pIp) {
	uint8_t i;
	Tcb_t * ptr = os_threads;
	for (i=0; i<OS_MAX_THREADS; i++, ptr++) {
		if (ptr->stack == NULL) {
			ptr->stack = pStack;
			ptr->state = OS_TS_RUNNABLE;
			if (os_thread_running == NULL) {
				os_thread_running = ptr;
			}
			//TODO:	Der Thread kommt aus dem Nichts, aber wir muessen ihm eine Vergangenheit konstruieren
			return ptr;
		}
	}
	return NULL;	// Fehler :(
}

/*!
 * Schaltet "von aussen" auf einen neuen Thread um. 
 * => kernel threadswitch
 * @param *from	Zeiger auf TCB des aktuell laufenden Threads
 * @param *to	Zeiger auf TCB des Threads, der nun laufen soll
 */
void os_switch_thread(Tcb_t * from, Tcb_t * to) {
	os_enterCS();
	to->state = OS_TS_RUNNING;
	os_thread_running = to;
	from->state = OS_TS_RUNNABLE;
	// r18 bis r27, Z und SREG wurden bereits beim Eintritt in die Timer-ISR auf dem (korrekten) Stack gesichert!
	asm volatile(
		"push r0									\n\t"
		"push r1									\n\t"
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
		"in r0, __SREG__	; save SREG				\n\t"
		"cli				; Interrupts off		\n\t"
		"in r16, __SP_L__	; switch Stacks			\n\t"
		"st Z+, r16									\n\t"	
		"in r16, __SP_H__							\n\t"
		"st Z, r16									\n\t"	
		"ld r16, Y+ 								\n\t"
		"out __SP_L__, r16							\n\t"
		"ld r16, Y 									\n\t"
		"out __SP_H__, r16							\n\t"
		//-- live changed here --//
		"out __SREG__, r0	; restore SREG			\n\t" 
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
		"pop r1										\n\t"			
		"pop r0											"
		::	"y"(&to->stack), "z"(&from->stack)
		:	"memory"
	);
	os_exitCS();
}

#endif	// OS_AVAILABLE
#endif	// MCU
