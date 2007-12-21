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
 * @file 	pos_stack.h
 * @brief 	Implementierung eines Stacks mit den ueblichen Stackbefehlen push(_pos), pop(_pos),...
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	13.12.2007
 */

#ifndef POS_STACK_H_
#define POS_STACK_H_

#include "ct-Bot.h"
#include "global.h"

#ifdef POS_STACK_AVAILABLE

/*! Der ARRAY-Stack ist verfuegbar; im anderen Fall mit dynamischem Speicherholen und Freigabe */
#define ARRAY_POINT_STACK


/*! ab hier der statische Stack mittels Array */
#ifdef ARRAY_POINT_STACK

#define STACK_SIZE	20  // Stackgroesse ist beim Array begrenzt

/*! Datenstruktur eines Koordinatenpunktes */
typedef struct {
  int16_t posx;
  int16_t posy;
} pos_stack_t;

#else	// ab hier der dynamische Stack

/*! Datenstruktur eines Koordinatenpunktes */
typedef struct {
	int16_t posx;
	int16_t posy;
} pos_stack_element_t;

/*! Stack-Datenstruktur zur Speicherung aller Koordinatenpunkte nach LIFO */
typedef struct _pos_stack_t {
	pos_stack_element_t element;
	struct _pos_stack_t * next;
} pos_stack_t;

#endif	// ARRAY_POINT_STACK

/*!
 * Speicherfreigabe der noch im Stack befindlichen Elemente und Ruecksetzen des Stacks auf NULL
 */
void pos_stack_clear(void);

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes
 * @param *x	zuletzt gepoppte X-Koordinate
 * @param *y	zuletzt gepoppte Y-Koordinate
 * @return		False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_stack_pop(int16_t * x, int16_t * y);

/*!
 * Speichern einer Koordinate auf dem Stack
 * @param x	X-Koordinate des zu sichernden Punktes
 * @param y	Y-Koordinate des zu sichernden Punktes
 */
void pos_stack_push(int16_t x, int16_t y);
#endif	// POS_STACK_AVAILABLE

#endif	/*POS_STACK_H_*/
