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
 * Die ausfuehrenden Einheiten heissen "Thread" (denn sie leben ja alle im selben
 * Adressraum).
 * Es gibt nur einen Scheduler, daher wird nicht weiter zwischen Kernel-Level-Threads 
 * und User-Level-Threads unterschieden.
 * Ein Thread besteht aus einem Stack, einem Timestamp "fruehester Zeitpunkt 
 * der naechsten Ausfuehrung" und der letzten Laufzeit, die zusammen den Thread 
 * Control Block (TCB) bilden, und indirekt einer Prioritaet. 
 * Der zuerst angelegte Thread hat die hoechste Prioritaet, der naechste die 
 * Zweithoechste usw. 
 * Es kann somit keine Prioritaet doppelt vergeben werden.
 * Threads koennen zwar zur Laufzeit erzeugt, aber nicht wieder beendet werden. 
 * 
 * Ein Thread kann eine beliebige Anzahl an ms schlafen und somit die Kontrolle
 * an einen anderen Thread abgeben. 
 * Ruft ein Thread os_thread_yield() auf, schlaeft er (mindestens) fuer die Dauer 
 * einer Zeitscheibe minus seiner bisherigen Laufzeit. Man muss yield() also periodisch 
 * aufrufen, damit das Ganze Sinn macht. Die Berechnung ist nur dann korrekt, wenn
 * der Thread, der yield() benutzt, nicht laenger als 44 ms blockiert wird, weil hier
 * zum Teil in 8 Bit gerechnet wird (Effizienzvorteil). Der Gedanke dahinter ist, dass
 * ein Thread, der nur selten laueft, keine so exakte Zeitangabe der naechsten Ausfuehrung
 * benoetigt und deshalb einfach sleep() benutzen kann (s.o.).
 * 
 * Eine Zeitscheibe dauert derzeit 10 ms, einstellen laesst sich das in os_scheduler.h
 * mit dem #define OS_TIME_SLICE in ms-Schritten.
 * Die Scheduling-Aufloesung betraegt ca. 1 ms.
 * 
 * Wacht ein hoeher prioraer Thread wieder auf (= Schlafzeit zuende), wird dem 
 * Aktuellen die Ausfuehrung entzogen.
 * 
 * Identifiziert werden Threads durch die (eindeutige) Adresse ihres TCBs. 
 * 
 * Die Anzahl der moeglichen Threads ist durch OS_MAX_THREADS in os_thread.h festgelegt.
 * Hier sollte man nicht mehr als noetig einstellen, um RAM zu sparen (es gibt ein statisches
 * Array fuer alle TCBs der Groesse OS_MAX_THREADS).
 * 
 * 
 * Interna:
 * Dieser Abschnitt ist nur dann wichtig, wenn man etwas am Scheduler und / oder an der
 * Timer-ISR veraendern oder erweitern moechte.
 * 
 * os_switch_thread() darf ausschliesslich vom Scheduler aus aufgerufen werden!
 * os_switch_helper() kann nur von os_switch_thread() verwendet werden!
 * 
 * Man kann den Teil der Timer-ISR, der den Scheduler aufruft und den Scheduler selbst
 * als eine Art Kernel ansehen. Es gibt jedoch (aus Effizienzgruenden) KEINEN Kernel-Stack, 
 * weder pro Thread, noch einen Allgemeinen. Das hat zur Folge, dass ein vom Scheduler 
 * angestossener Threadwechsel SOFORT ausgefuehrt wird. 
 * Falls also ein Threadwechsel von A nach B ansteht, wird die Scheduler-Funktion (und die 
 * Timer-ISR ebenfalls) von Thread A aufgerufen und von Thread B(!) wieder verlassen. Erfolgt 
 * spaeter umgekehrt ein Wechsel von B zurueck nach A, wird der beim ersten Wechsel 
 * "aufgeschobene" Ruecksprung vom Scheduler zurueck nach Thread A zu diesem Zeitpunkt 
 * ausgefuehrt / "nachgeholt". 
 * Es muss also gewaehrleistet sein, dass der Code (im Scheduler) NACH dem Threadwechsel fuer 
 * ALLE Threads identisch ist.
 * Da es zwei unterschiedliche Gruende fuer einen Threadwechsel geben kann ("hart" per 
 * Timer-ISR, weil der aktuelle Thread nicht mehr weiterlaufen darf, und "kooperativ" vom 
 * Thread selbst gewuenscht, weil dieser sleep() oder yield() aufgerufen hat), erfolgt der 
 * Ruecksprung aus dem Scheduler nicht zwangsweise an die Codestelle, die (direkt) vor dem
 * Scheduler-Aufruf ausgefuehrt wurde. Daher darf NACH dem Scheduler-Aufruf in der Timer-ISR
 * kein Code mehr folgen, der bei jedem Timer-Tick ausgefuehrt werden soll. Solcher Code
 * muss unbedingt VOR dem Scheduler-Aufruf stehen. 
 * 
 * 
 * Weitere Hinweise:
 * Aufpassen muss man bei moeglichen Ressourcen-Konflikten: Z.B. Darf nicht waehrend eines
 * Display- oder ENA-Zugriffs auf einen anderen Thread umgeschaltet werden, der dann evtl.
 * auch auf das Display oder eine ENA-Leitung zugreifen moechte, denn die Schieberegister-
 * Operationen sind nicht atomar. Aehnlich verhaelt es sich mit LOG- oder UART-Zugriffen. 
 * Fuer MMC und Maussensor ist ein entsprechender Schutz (mit os_enterCS() und os_exitCS()) 
 * bereits im MMC- und ENA-Code vorhanden, andere Ressourcen sind derzeit nicht geschuetzt, 
 * da es neben dem Main-Thread bisher nur einen weiteren zum Map-Update gibt (und das Map-
 * Update nicht auf Display oder andere ENA-Leitungen zugreift). 
 * Moechte man weitere Threads fuer andere Zwecke hinzufuegen, muss man waehrend des
 * Zugriffs auf gemeinsam genutzte Ressourcen entweder das Scheduling temporaer deaktivieren
 * (mit os_enterCS() und os_exitCS() entweder im Thread-Code oder im Display- / ENA-Code), 
 * oder die jeweiligen Ressourcen z.B. per Mutex sperren.
 * Bei letzterem muesste man aber aufpassen, keine sich ueberkreuzenden Zugriffe zu erzeugen,
 * um Dead- oder Livelocks zu vermeiden. 
 * Um diesen Problemen aus dem Weg zu gehen, ist derzeit nur der MMC-Zugriff in einen extra 
 * Thread ausgelagert. 
 * 
 * Ebenfalls will die Stack-Groesse eines Threads sorgfaeltig ueberlegt sein, da auf dem Stack
 * beim Threadwechsel der komplette Kontext (die 32 GP-Register, das Statusregister und der 
 * Instruction-Pointer) gesichert wird. Nicht-statische, lokale Variablen legt der Compiler
 * ebenfalls auf dem Stack ab, sobald sie nicht mehr in Registern untergebracht werden 
 * koennen oder eine Adresseberechnung gebraucht wird (z.B. bei Arrays). Man sollte daher
 * (groessere) lokale Arrays als statisch deklarieren. 
 * Ausserdem sollten viele verschachtelte Funktionsaufrufe moeglichst vermieden werden, da
 * fuer jeden Funktionsaufruf die Ruecksprungadresse sowie evtl. gesicherte Register ebenfalls
 * auf dem Stack Platz finden muessen.
 * 
 * Fuer den PC-Code gibt es bisher keine Thread-Implementierung, d.h. man muss mindestens die
 * while-Schleife im Thread-Code per #ifdef auf den MCU-Fall beschraenken und den 
 * Threadwechsel im PC-Fall durch den Funktionsaufruf der Thread-Main-Funktion ersetzen.
 * 
 */

#include "ct-Bot.h"
#ifdef MCU
#ifdef OS_AVAILABLE

#include "os_thread.h"
#include "os_utils.h"
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
	static volatile uint8_t j = 0;	// haesslich, aber sonst rollt der gcc die Schleife aus :-/
	uint8_t i;
	Tcb_t * ptr = os_threads;
	for (i=j; i<OS_MAX_THREADS; i++, ptr++) {	
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
				tmp.ip = &os_exitCS;
				*(pStack-2) = tmp.lo8;
				*(pStack-3) = tmp.hi8;
				ptr->stack = pStack-4;	// 4x push => Stack-Pointer - 4
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
	if (test_and_set((uint8_t *)&os_scheduling_allowed, 1) == 2) {
		os_schedule(TIMER_GET_TICKCOUNT_32);
	}
}

/*!
 * Schaltet auf den Thread mit der naechst niedrigeren Prioritaet um, der lauffaehig ist,
 * indem diesem der Rest der Zeitscheibe geschenkt wird.
 */
void os_thread_yield(void) {
	uint32_t now = TIMER_GET_TICKCOUNT_32;
	/* Zeitpunkt der letzten Ausfuehrung nur in 8 Bit, da yield() nur Sinn macht, wenn es
	 * regelmaessig aufgerufen wird und eine Aufloesung im ms-Bereich gebraucht wird. 
	 * Sonst sleep() benutzen! */
	uint8_t diff = (uint8_t)now - os_thread_running->lastSchedule;
	if (diff > MS_TO_TICKS(OS_TIME_SLICE)) {
		/* Zeitscheibe wurde ueberschritten */
		os_thread_running->nextSchedule = now;
	} else {
		/* Wir haben noch Zeit frei, die wir dem naechsten Thread schenken koennen */
		os_thread_running->nextSchedule = now - (uint8_t)(diff + (uint8_t)MS_TO_TICKS(OS_TIME_SLICE));
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
	/* r0, r1, r18 bis r27, Z und SREG werden hier nach folgender Ueberlegung NICHT gesichert:
	 * r0, r18 bis r27 und Z wurden bereits vor dem Eintritt in diese Funktion auf dem (korrekten) Stack gesichert!
	 * Falls wir aus der Timer-ISR kommen, wurden auch r1 und das Statusregister bereits gerettet. Falls nicht, 
	 * duerfen wir das SREG ruhig ueberschreiben, weil der (noch) aktuelle Thread in diesem Fall den Scheduler explizit 
	 * aufrufen wollte, der Compiler also weiss, dass sich das Statusregister aendern kann (kooperativer Threadswitch).
	 * In r1 muss vor jedem Funktionsaufruf bereits 0 stehen.  
	 */
	asm volatile(
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
	//-- hier ist noch "from" (Z) der aktive Thread 	--//
		"call os_switch_helper	; switch stacks		\n\t"
	//-- jetzt ist schon "to" (Y) der aktive Thread 	--//
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
		"pop r2											"			
		::	"y" (&to->stack), "z" (&from->stack)	// Stackpointer
		:	"memory"
	);
}

/*
 * Hilfsfunktion fuer Thread-Switch, nicht beliebig aufrufbar!
 * (Erwartet Zeiger auf TCBs in Z und Y).
 * Als extra Funktion implementiert, um die Return-Adresse auf dem Stack zu haben.
 */
void os_switch_helper(void) {
	//-- coming in as "Z" --//
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
		::: "memory"
	);
	//-- continue as "Y" --//
}

#endif	// OS_AVAILABLE
#endif	// MCU
