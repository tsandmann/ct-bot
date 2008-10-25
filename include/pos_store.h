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

#ifndef POS_STACK_H_
#define POS_STACK_H_

#include "ct-Bot.h"
#include "global.h"

#ifdef POS_STORE_AVAILABLE

#define POS_STORE_SIZE	32  /*!< Speichergroesse */

#if POS_STORE_SIZE & (POS_STORE_SIZE-1)
#error "POS_STORE_SIZE ist keine 2er-Potenz!"
#endif

extern uint8_t pos_store_sp;				/*!< Stackpointer */
extern uint8_t pos_store_fp;				/*!< FIFO-Pointer */

/*!
 * Liefert den aktuellen Stackpointer des Positionsspeichers
 * @return	Index des naechsten freien Platzes im Array
 */
static inline uint8_t pos_store_get_sp(void) {
	return pos_store_sp;
}

/*!
 * Liefer den aktuellen FIFO-Pointer des Positionsspeichers
 * @return	Index des ersten belegten Platzes im Array
 */
static inline uint8_t pos_store_get_fp(void) {
	return pos_store_fp;
}

/*!
 * Leert den Positionsspeicher
 */
void pos_store_clear(void);

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes
 * @param *pos	Zeiger auf Rueckgabe-Speicher der Position
 * @return		False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_store_pop(position_t * pos) __attribute__((noinline));

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes, falls Stackpointer until_sp noch nicht erreicht ist
 * @param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * @param until_sp	Stackpointer (per pos_store_get_sp() geholt), bis zu dem ein Pop maximal erfolgen soll
 * @return			False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_store_pop_until(position_t * pos, uint8_t until_sp);

/*!
 * Speichern einer Koordinate auf dem Stack
 * @param pos	X/Y-Koordinaten des zu sichernden Punktes
 * @return		True wenn erfolgreich sonst False wenn Array voll ist
 */
uint8_t pos_store_push(position_t pos);

/*!
 * Erweiterung des Stacks zur Queue; Element wird hinten angefuegt, identisch dem Stack-Push
 * @param pos	X/Y-Koordinaten des zu sichernden Punktes
 * @return 		True wenn erfolgreich sonst False wenn Array voll ist
 */
static inline uint8_t pos_store_queue(position_t pos) {
	return pos_store_push(pos);
}

/*!
 * Erweiterung des Stacks zur Queue; Element wird vorn entnommen
 * @param * pos	Zeiger auf Rueckgabe-Speicher der Position
 * @return True wenn Element erfolgreich entnommen werden konnte sonst False falls kein Element mehr enthalten ist
 */
uint8_t pos_store_dequeue(position_t * pos) __attribute__((noinline));

#ifdef PC
/*!
 * Testet push(), pop() und dequeue()
 */
void pos_store_test(void);
#else	// MCU
#define pos_store_test();	// NOP
#endif	// PC

#endif	// POS_STORE_AVAILABLE
#endif	/*POS_STACK_H_*/
