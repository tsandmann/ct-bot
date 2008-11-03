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
 * @file 	pos_store.h
 * @brief 	Implementierung eines Positionsspeichers mit den ueblichen Stackbefehlen push(), pop()
 * 			und FIFO-Befehlen queue(), dequeue()
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	13.12.2007
 */

#ifndef POS_STORE_H_
#define POS_STORE_H_

#include "ct-Bot.h"
#include "global.h"
#include "bot-logic/bot-logik.h"
#include <stdlib.h>

#ifdef POS_STORE_AVAILABLE

#define POS_STORE_SIZE	32  /*!< Groesse (pro Platz) */

#if POS_STORE_SIZE & (POS_STORE_SIZE-1)
#error "POS_STORE_SIZE ist keine 2er-Potenz!"
#endif
#if POS_STORE_SIZE > 127
#error "POS_STORE_SIZE ist zu gross!"
#endif

/*! Positionsspeicher-Datentyp */
typedef struct {
	Behaviour_t * owner;	/*!< Besitzer dieses Speicher (ein Verhalten) */
	position_t * data;		/*!< Speicher fuer die Daten */
	uint8_t count;			/*!< Anzahl der Elemente im Speicher */
	uint8_t stat_data;		/*!< 1: Statischer Speicher; 0: Heap-Speicher */
	uint8_t sp;				/*!< Stackpointer */
	uint8_t fp;				/*!< FIFO-Pointer */
} pos_store_t;

/*!
 * Erzeugt einen neuen Positionsspeicher
 * @param *owner	Zeiger Verhaltensdatensatz
 * @param *data		NULL oder Zeiger auf Speicher fuer POS_STORE_SIZE * sizeof(position_t) Bytes
 * @return			Zeiger auf neuen Positionsspeicher oder NULL
 */
pos_store_t * pos_store_create(Behaviour_t * owner, void * data);

/*!
 * Erzeugt einen neuen Positionsspeicher
 * @param *owner	Zeiger Verhaltensdatensatz
 * @return			Zeiger auf neuen Positionsspeicher oder NULL
 */
static inline pos_store_t * pos_store_new(Behaviour_t * owner) {
	return pos_store_create(owner, NULL);
}

/*!
 * Ermittelt den Positionsspeicher, der zu einem Verhalten gehoert
 * @param *owner	Zeiger auf Verhaltensdatensatz
 * @return			Zeiger auf Positionsspeicher oder NULL
 */
pos_store_t * pos_store_from_beh(Behaviour_t * owner);

/*!
 * Ermittelt den Positionsspeicher, der den gegebenen Index im Array hat
 * @param index	Index des Positionsspeichers im Array
 * @return		Zeiger auf Positionsspeicher oder NULL
 */
pos_store_t * pos_store_from_index(uint8_t index);

/*!
 * Ermittelt den Index eines Positionsspeichers
 * @param *store	Zeiger auf Positionsspeicher
 * @return			Index des Positionsspeichers im Array
 */
uint8_t pos_store_get_index(pos_store_t * store);

/*!
 * Leert den Positionsspeicher
 * @param *store	Zeiger auf Positionsspeicher
 */
void pos_store_clear(pos_store_t * store);

/*!
 * Loescht einen Positionsspeicher
 * @param *store	Zeiger auf Positionsspeicher
 */
void pos_store_release(pos_store_t * store);

/*!
 * Loescht alle Positionsspeicher
 */
void pos_store_release_all(void);

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes
 * @param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * @param *store	Zeiger auf Positionsspeicher
 * @return			False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_store_pop(pos_store_t * store, position_t * pos);

/*!
 * Speichern einer Koordinate auf dem Stack
 * @param pos		X/Y-Koordinaten des zu sichernden Punktes
 * @param *store	Zeiger auf Positionsspeicher
 * @return			True wenn erfolgreich sonst False wenn Array voll ist
 */
uint8_t pos_store_push(pos_store_t * store, position_t pos);

/*!
 * Erweiterung des Stacks zur Queue; Element wird hinten angefuegt, identisch dem Stack-Push
 * @param pos		X/Y-Koordinaten des zu sichernden Punktes
 * @param *store	Zeiger auf Positionsspeicher
 * @return 			True wenn erfolgreich sonst False wenn Array voll ist
 */
static inline uint8_t pos_store_queue(pos_store_t * store, position_t pos) {
	return pos_store_push(store, pos);
}

/*!
 * Erweiterung des Stacks zur Queue; Element wird vorn entnommen
 * @param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * @param *store	Zeiger auf Positionsspeicher
 * @return 			True wenn Element erfolgreich entnommen werden konnte sonst False falls kein Element mehr enthalten ist
 */
uint8_t pos_store_dequeue(pos_store_t * store, position_t * pos);

#ifdef PC
/*!
 * Testet push(), pop() und dequeue()
 */
void pos_store_test(void);
#else	// MCU
#define pos_store_test();	// NOP
#endif	// PC

#endif	// POS_STORE_AVAILABLE
#endif	/*POS_STORE_H_*/
