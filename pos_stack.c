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
 * @file 	stack.c
 * @brief 	Implementierung eines Stacks mit den ueblichen Stackbefehlen push(_pos), pop(_pos),...
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	13.12.2007
 */

#include "ct-Bot.h"
#include "pos_stack.h"

#ifdef POS_STACK_AVAILABLE


#include <stdio.h>
#include <stdlib.h>

/*! bei Verfuegbarkeit des ARRY-Stacks; sonst Stack mit dynamischer Speicherfreigabe */
#ifdef ARRAY_POINT_STACK

static pos_stack_t pos_stack[STACK_SIZE];
static pos_stack_t * pos_sp = pos_stack;
static uint8_t stack_count = 0;

/*!
 * Setzt den Stack auf initial zurueck, d.h. Array-Index wird auf Leerwert rueckgesetzt
 */
void pos_stack_clear(void) {
	pos_sp = pos_stack;
	stack_count = 0;
}

/*!
 * Stack leer?
 * @return True falls Stack leer sonst false
 */
static uint8_t is_empty(void) {
	return stack_count == 0;
}

/*!
 * Stack voll?
 * @return True falls Stack voll, sonst false
 */
static uint8_t is_full(void) {
	return stack_count == STACK_SIZE;
}

/*!
 * Speichern einer Koordinate auf dem Stack
 * @param x	X-Koordinate des zu sichernden Punktes
 * @param y	Y-Koordinate des zu sichernden Punktes
 */
void pos_stack_push(int16_t x, int16_t y) {
	if (is_full()) return;
	pos_sp->posx = x;
	pos_sp->posy = y;
	pos_sp++;
	stack_count++;	
}

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes
 * @param *x	zuletzt gepoppte X-Koordinate
 * @param *y	zuletzt gepoppte Y-Koordinate
 * @return		False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_stack_pop(int16_t * x, int16_t * y) {
	if (is_empty()) return False;
	pos_sp--;
	*x = pos_sp->posx;
	*y = pos_sp->posy;
	stack_count--;
	return True;
}

#else	//  ab hier der dynamische Stack */

static pos_stack_t * Point_Stack; /*!< die eigentlich zu verwendende Stack-Variable */

/*!
 * Initialisierung des Stacks; Routine muss vor der ersten Benutzung des Stacks aufgerufen worden sein
 */
static void create_stack(void) {
	Point_Stack = malloc(sizeof(pos_stack_t));
	if (Point_Stack != NULL)
		Point_Stack->next = NULL;
}

/*!
 * Implementierung der Push-Routine, nur fuer interne Verwendung
 * @param *ele	Variable der Punkt-Element-Struktur
 */
static void push_element(pos_stack_t * ele) {
	if (ele != NULL) {
		ele->next = Point_Stack->next;
		Point_Stack->next = ele;
	}
}

/*!
 * Implementierung der internen Pop-Routine; Rueckgabe nach LIFO des ersten Element-Typs, d.h. des letzten gepoppten Punktes
 * @return	Rueckgabe des letzten Pop-Elemetes (LIFO)
 */
static pos_stack_t * pop_element(void) {
	pos_stack_t * ele = NULL;
	if (Point_Stack->next != NULL) {
		ele = Point_Stack->next;
		Point_Stack->next = Point_Stack->next->next;
	}
	return ele;
}

/*!
 * Speicherfreigabe der noch im Stack befindlichen Elemente und Ruecksetzen des Stacks auf NULL
 */
void pos_stack_clear(void) {
	int16_t x;
	int16_t y;
	while (pos_stack_pop(&x, &y)) {}
	Point_Stack->next = NULL;
}

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes
 * @param *x	zuletzt gepoppte X-Koordinate
 * @param *y	zuletzt gepoppte Y-Koordinate
 * @return		False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_stack_pop(int16_t * x, int16_t * y) {
	pos_stack_t * first_cell = pop_element();
	if (first_cell != NULL) {
		*x = first_cell->element.posx;
		*y = first_cell->element.posy;
		free(first_cell);
		return True;
	}
	return False;
}

/*!
 * Speichern einer Koordinate auf dem Stack
 * @param x	X-Koordinate des zu sichernden Punktes
 * @param y	Y-Koordinate des zu sichernden Punktes
 */
void pos_stack_push(int16_t x, int16_t y) {
	if (Point_Stack == NULL) {
		create_stack();
	}
	pos_stack_t * tmp;
	tmp = malloc(sizeof(pos_stack_t));
	tmp->element.posx = x;
	tmp->element.posy = y;
	push_element(tmp);
}

#endif	// ARRAY_POINT_STACK
#endif	// POS_STACK_AVAILABLE
