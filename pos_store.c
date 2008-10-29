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
 * @file 	pos_store.c
 * @brief 	Implementierung eines Positionsspeichers mit den ueblichen Stackbefehlen push(), pop()
 * 			und FIFO-Befehlen queue(), dequeue()
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	13.12.2007
 */

#include "ct-Bot.h"
#include "pos_store.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef POS_STORE_AVAILABLE

static position_t pos_stack[POS_STORE_SIZE];	/*!< Datenspeicher */
static uint8_t count = 0;						/*!< Anzahl der gespeicherten Elemente */
uint8_t pos_store_sp = 0;				/*!< Stackpointer */
uint8_t pos_store_fp = 0;				/*!< FIFO-Pointer */

/*!
 * Leert den Positionsspeicher
 */
void pos_store_clear(void) {
	pos_store_sp = 0;
	pos_store_fp = 0;
	count = 0;
}

/*!
 * Speicher leer?
 * @return True falls Speicher leer sonst False
 */
static uint8_t is_empty(void) {
	return count == 0;
}

/*!
 * Speicher voll?
 * @return True falls Speicher voll, sonst False
 */
static uint8_t is_full(void) {
	return count == POS_STORE_SIZE;
}

/*!
 * Speichern einer Koordinate auf dem Stack
 * @param pos	X/Y-Koordinaten des zu sichernden Punktes
 * @return		True wenn erfolgreich sonst False wenn Array voll ist
 */
uint8_t pos_store_push(position_t pos) {
	if (is_full()) {
		return False;
	}
	pos_stack[pos_store_sp] = pos;
	pos_store_sp++;
	pos_store_sp &= POS_STORE_SIZE - 1;
	count++;
	return True;
}

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes
 * @param *pos	Zeiger auf Rueckgabe-Speicher der Position
 * @return		False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_store_pop(position_t * pos) {
	if (is_empty()) {
		return False;
	}
	count--;
	pos_store_sp--;
	pos_store_sp &= POS_STORE_SIZE - 1;
	*pos = pos_stack[pos_store_sp];

	return True;
}

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes, falls Stackpointer until_sp noch nicht erreicht ist
 * @param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * @param until_sp	Stackpointer (per pos_store_get_sp() geholt), bis zu dem ein Pop maximal erfolgen soll
 * @return			False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_store_pop_until(position_t * pos, uint8_t until_sp) {
	if (pos_store_sp == until_sp) {
		return False;
	}
	return pos_store_pop(pos);
}

/*!
 * Erweiterung des Stacks zur Queue; Element wird vorn entnommen
 * @param *pos	Zeiger auf Rueckgabe-Speicher der Position
 * @return 		True wenn Element erfolgreich entnommen werden konnte sonst False falls kein Element mehr enthalten ist
 */
uint8_t pos_store_dequeue(position_t * pos) {
	if (is_empty()) {
		return False;
	}
	count--;
	*pos = pos_stack[pos_store_fp];
	pos_store_fp++;
	pos_store_fp &= POS_STORE_SIZE - 1;

	return True;
}

#ifdef PC
/*!
 * Gibt alle Eintraege auf stdout aus
 */
static void dump(void) {
	int i;
	for (i=0; i<count; i++) {
		int x = pos_stack[(pos_store_fp + i) & (POS_STORE_SIZE - 1)].x;
		int y = pos_stack[(pos_store_fp + i) & (POS_STORE_SIZE - 1)].y;
		printf("%d:\tx=%d\ty=%d\n", i + 1, x, y);
	}
	printf("fp=%u\tsp=%u\tcount=%u\t\n\n", pos_store_fp, pos_store_sp, count);
}

/*!
 * Testet push(), pop() und dequeue()
 */
void pos_store_test(void) {
	pos_store_clear();
	dump();
	int i;
	for (i=0; i<POS_STORE_SIZE+1; i++) {
		uint8_t result = pos_store_push((position_t) {i, i + 50});
		printf("push(%d, %d)=%u\n", i, i + 50, result);
		dump();
		if (result != 1 && i < POS_STORE_SIZE) {
			printf("ERROR\n\n");
			return;
		}
		if (result != 0 && i >= POS_STORE_SIZE) {
			printf("ERROR\n\n");
			return;
		}
	}
	int16_t exspected_x = 31;
	int16_t exspected_y = 81;
	for (i=0; i<5; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_pop(&pos);
		printf("pop()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		dump();
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR\n\n");
			return;
		}
		exspected_x--;
		exspected_y--;
	}
	for (i=0; i<5; i++) {
		uint8_t result = pos_store_push((position_t) {i - 100, i});
		printf("push(%d, %d)=%u\n", i - 100, i, result);
		dump();
		if (result != 1) {
			printf("ERROR\n\n");
			return;
		}
	}
	exspected_x = 0;
	exspected_y = 50;
	for (i=0; i<10; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_dequeue(&pos);
		printf("dequeue()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		dump();
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR\n\n");
			return;
		}
		exspected_x++;
		exspected_y++;
	}
	for (i=0; i<10; i++) {
		uint8_t result = pos_store_push((position_t) {i - 200, i});
		printf("push(%d, %d)=%u\n", i - 200, i, result);
		dump();
		if (result != 1) {
			printf("ERROR\n\n");
			return;
		}
	}
	exspected_x = -191;
	exspected_y = 9;
	for (i=0; i<POS_STORE_SIZE+1; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_pop(&pos);
		printf("pop()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		dump();
		if (result != 1 && i < POS_STORE_SIZE) {
			printf("ERROR\n\n");
			return;
		}
		if (result != 0 && i >= POS_STORE_SIZE) {
			printf("ERROR!\n\n");
			return;
		}
		if (result == 1 && (pos.x != exspected_x || pos.y != exspected_y)) {
			printf("ERROR\n\n");
			return;
		}
		exspected_x--;
		exspected_y--;
		if (exspected_x == -201) {
			exspected_x = -96;
			exspected_y = 4;
		} else if (exspected_x == -101) {
			exspected_x = 26;
			exspected_y = 76;
		}
	}
	pos_store_clear();
	dump();
	printf("Test PASSED\n\n");
}
#endif	// PC
#endif	// POS_STORE_AVAILABLE
