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
#include "log.h"
#include <stdio.h>

#ifdef POS_STORE_AVAILABLE

//#define DEBUG_POS_STORE		// Schalter fuer Debug-Ausgaben

#ifndef LOG_AVAILABLE
	#undef DEBUG_POS_STORE
#endif

#ifndef DEBUG_POS_STORE
	#undef LOG_DEBUG
	#define LOG_DEBUG(a, ...) {}
#endif

#define SLOT_COUNT	4	/*!< Anzahl der Plaetze im Array */

static pos_store_t pos_stores[SLOT_COUNT];	/*!< Liste der Positionsspeicher */

/*!
 * Erzeugt einen neuen Positionsspeicher
 * @param *owner	Zeiger Verhaltensdatensatz
 * @param *data		NULL oder Zeiger auf Speicher fuer POS_STORE_SIZE * sizeof(position_t) Bytes
 * @return			Zeiger auf neuen Positionsspeicher oder NULL
 */
pos_store_t * pos_store_create(Behaviour_t * owner, void * data) {
	if (owner == NULL) {
		LOG_DEBUG("Fehler: owner==NULL!");
		return NULL;
	}
	LOG_DEBUG("Erzeuge Positionsspeicher fuer 0x%lx", (size_t)owner);
	pos_store_t * store = pos_store_from_beh(owner);
	if (store == NULL) {
		/* Verhalten hat noch keinen Positionsspeicher -> anlegen */
		pos_store_t * ptr;
		for (ptr=pos_stores; ptr<=&pos_stores[SLOT_COUNT-1]; ptr++) {
			if (ptr->owner == NULL) {
				/* freien Platz gefunden */
				store = ptr;
				store->owner = owner;
				LOG_DEBUG("verwende Slot %u", (store - pos_stores));
				if (data == NULL) {
					store->data = malloc(POS_STORE_SIZE * sizeof(position_t));
					store->stat_data = 0;
					LOG_DEBUG("verwende Heap-Speicher @ 0x%lx", (size_t)store->data);
				} else {
					store->data = data;
					store->stat_data = 1;
					LOG_DEBUG("verwende statischen Speicher @ 0x%lx", (size_t)data);
				}
				if (store->data == NULL) {
					LOG_DEBUG("Kein Speicher zur Verfuegung, Abbruch!");
					pos_store_release(store);
					return NULL;
				}
				break;
			}
		}
	}

	pos_store_clear(store);
	LOG_DEBUG("Positionsspeicher @ 0x%lx angelegt", (size_t)store);
	return store;
}

/*!
 * Loescht einen Positionsspeicher
 * @param *store	Zeiger auf Positionsspeicher
 */
void pos_store_release(pos_store_t * store) {
	LOG_DEBUG("Gebe Positionsspeicher 0x%lx frei", (size_t)store);
	if (store == NULL) {
		return;
	}
	store->owner = NULL;	// kein owner == Platz unbelegt
	if (store->stat_data == 0 && store->data != NULL) {
		/* Datenspeicher freigeben */
		free(store->data);
		LOG_DEBUG("Heap-Speicher 0x%lx freigegeben", (size_t)store->data);
		store->data = NULL;
	}
}

/*!
 * Loescht alle Positionsspeicher
 */
void pos_store_release_all(void) {
	uint8_t i;
	for (i=0; i<SLOT_COUNT; i++) {
		pos_stores[i].owner = NULL;
		if (pos_stores[i].stat_data == 0 && pos_stores[i].data != NULL) {
			free(pos_stores[i].data);
			pos_stores[i].data = NULL;
		}
	}
}

/*!
 * Ermittelt den Positionsspeicher, der zu einem Verhalten gehoert
 * @param *owner	Zeiger auf Verhaltensdatensatz
 * @return			Zeiger auf Positionsspeicher oder NULL
 */
pos_store_t * pos_store_from_beh(Behaviour_t * owner) {
	pos_store_t * store;
	for (store=pos_stores; store<=&pos_stores[SLOT_COUNT-1]; store++) {
		if (store->owner == owner) {
			return store;
		}
	}
	return NULL;
}

/*!
 * Ermittelt den Index eines Positionsspeichers
 * @param *store	Zeiger auf Positionsspeicher
 * @return			Index des Positionsspeichers im Array
 */
uint8_t pos_store_get_index(pos_store_t * store) {
	size_t index = (size_t)store - (size_t)pos_stores;	// offset abziehen
	index /= sizeof(pos_store_t);	// Differenz in Groesse umrechnen
	return index;
}

/*!
 * Ermittelt den Positionsspeicher, der den gegebenen Index im Array hat
 * @param index	Index des Positionsspeichers im Array
 * @return		Zeiger auf Positionsspeicher oder NULL
 */
pos_store_t * pos_store_from_index(uint8_t index) {
	if (index > SLOT_COUNT - 1) {
		return NULL;
	}
	return &pos_stores[index];
}

/*!
 * Leert den Positionsspeicher
 * @param *store	Zeiger auf Positionsspeicher
 */
void pos_store_clear(pos_store_t * store) {
	LOG_DEBUG("Loesche Positionsspeicher 0x%lx", (size_t)store);
	if (store == NULL) {
		return;
	}
	/* Status zuruecksetzen */
	store->sp = 0;
	store->fp = 0;
	store->count = 0;
}

/*!
 * Speicher leer?
 * @return True falls Speicher leer sonst False
 */
static uint8_t is_empty(pos_store_t * store) {
	if (store == NULL) {
		/* Fehler */
		return True;
	}
	return store->count == 0;
}

/*!
 * Speicher voll?
 * @return True falls Speicher voll, sonst False
 */
static uint8_t is_full(pos_store_t * store) {
	if (store == NULL || store->data == NULL) {
		/* Fehler */
		return True;
	}
	return store->count == POS_STORE_SIZE;
}

/*!
 * Speichern einer Koordinate vorne im Speicher
 * @param *store	Zeiger auf Positionsspeicher
 * @param pos		X/Y-Koordinaten des zu sichernden Punktes
 * @return			True wenn erfolgreich sonst False wenn Array voll ist
 */
uint8_t pos_store_insert(pos_store_t * store, position_t pos) {
	if (is_full(store)) {
		return False;
	}
	uint8_t fp = store->fp;
	fp--;
	fp &= POS_STORE_SIZE - 1;
	store->data[fp] = pos;
	store->fp = fp;
	store->count++;
	return True;
}

/*!
 * Speichern einer Koordinate auf dem Stack
 * @param *store	Zeiger auf Positionsspeicher
 * @param pos		X/Y-Koordinaten des zu sichernden Punktes
 * @return			True wenn erfolgreich sonst False wenn Array voll ist
 */
uint8_t pos_store_push(pos_store_t * store, position_t pos) {
	if (is_full(store)) {
		return False;
	}
	uint8_t sp = store->sp;
	store->data[sp] = pos;
	sp++;
	sp &= POS_STORE_SIZE - 1;
	store->sp = sp;
	store->count++;
	return True;
}

/*!
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes
 * @param *store	Zeiger auf Positionsspeicher
 * @param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * @return			False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_store_pop(pos_store_t * store, position_t * pos) {
	if (is_empty(store)) {
		return False;
	}
	store->count--;
	store->sp--;
	store->sp &= POS_STORE_SIZE - 1;
	*pos = store->data[store->sp];
	return True;
}

/*!
 * Erweiterung des Stacks zur Queue; Element wird vorn entnommen
 * @param *store	Zeiger auf Positionsspeicher
 * @param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * @return 			True wenn Element erfolgreich entnommen werden konnte sonst False falls kein Element mehr enthalten ist
 */
uint8_t pos_store_dequeue(pos_store_t * store, position_t * pos) {
	if (is_empty(store)) {
		return False;
	}
	store->count--;
	*pos = store->data[store->fp];
	store->fp++;
	store->fp &= POS_STORE_SIZE - 1;
	return True;
}

#ifdef PC
/*!
 * Gibt alle Eintraege auf stdout aus
 * @param *store	Zeiger auf Positionsspeicher
 */
static void dump(pos_store_t * store) {
	int i;
	for (i=0; i<store->count; i++) {
		int x = store->data[(store->fp + i) & (POS_STORE_SIZE - 1)].x;
		int y = store->data[(store->fp + i) & (POS_STORE_SIZE - 1)].y;
		printf("%d:\tx=%d\ty=%d\n", i + 1, x, y);
	}
	printf("fp=%u\tsp=%u\tcount=%u\t\n\n", store->fp, store->sp, store->count);
}

/*!
 * Testet push(), pop() und dequeue()
 */
void pos_store_test(void) {
	pos_store_t * store = pos_store_new((Behaviour_t *)0x42);
	if (store == NULL) {
		printf("ERROR\n\n");
		return;
	}
	pos_store_t * store2 = pos_store_from_beh((Behaviour_t *)0x42);
	if (store2 != store) {
		printf("ERROR\n\n");
		return;
	}
	uint8_t index = pos_store_get_index(store);
	printf("index=%u\n", index);
	store2 = pos_store_from_index(index);
	if (store2 != store) {
		printf("ERROR\n\n");
		return;
	}
	store2 = pos_store_new((Behaviour_t *)0x42);
	if (store2 != store) {
		printf("ERROR\n\n");
		return;
	}
	store2 = pos_store_new((Behaviour_t *)42);
	if (store2 == store) {
		printf("ERROR\n\n");
		return;
	}
	index = pos_store_get_index(store2);
	printf("index2=%u\n\n", index);
	pos_store_release(store2);
	if (store2->owner != NULL || store2->data != NULL) {
		printf("ERROR\n\n");
		return;
	}
	dump(store);
	int i;
	for (i=0; i<POS_STORE_SIZE+1; i++) {
		uint8_t result = pos_store_push(store, (position_t) {i, i + 50});
		printf("push(%d, %d)=%u\n", i, i + 50, result);
		dump(store);
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
		uint8_t result = pos_store_pop(store, &pos);
		printf("pop()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		dump(store);
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR\n\n");
			return;
		}
		exspected_x--;
		exspected_y--;
	}
	for (i=0; i<5; i++) {
		uint8_t result = pos_store_push(store, (position_t) {i - 100, i});
		printf("push(%d, %d)=%u\n", i - 100, i, result);
		dump(store);
		if (result != 1) {
			printf("ERROR\n\n");
			return;
		}
	}
	exspected_x = 0;
	exspected_y = 50;
	for (i=0; i<10; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_dequeue(store, &pos);
		printf("dequeue()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		dump(store);
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR\n\n");
			return;
		}
		exspected_x++;
		exspected_y++;
	}

	exspected_x = -96;
	exspected_y = 4;
	for (i=0; i<10; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_pop(store, &pos);
		printf("pop()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		dump(store);
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR\n\n");
			return;
		}
		exspected_x--;
		exspected_y--;
		if (exspected_x == -101 && exspected_y == -1) {
			exspected_x = 26;
			exspected_y = 76;
		}
	}
	for (i=0; i<15; i++) {
		uint8_t result = pos_store_insert(store, (position_t) {i - 300, i});
		printf("insert(%d, %d)=%u\n", i - 300, i, result);
		dump(store);
		if (result != 1) {
			printf("ERROR\n\n");
			return;
		}
	}
	exspected_x = -286;
	exspected_y = 14;
	for (i=0; i<15; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_dequeue(store, &pos);
		printf("dequeue()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		dump(store);
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR\n\n");
			return;
		}
		exspected_x--;
		exspected_y--;
	}

	for (i=0; i<20; i++) {
		uint8_t result = pos_store_push(store, (position_t) {i - 200, i});
		printf("push(%d, %d)=%u\n", i - 200, i, result);
		dump(store);
		if (result != 1) {
			printf("ERROR\n\n");
			return;
		}
	}
	exspected_x = -181;
	exspected_y = 19;
	for (i=0; i<POS_STORE_SIZE+1; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_pop(store, &pos);
		printf("pop()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		dump(store);
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
			exspected_x = 21;
			exspected_y = 71;
		}
	}
	pos_store_clear(store);
	dump(store);
	pos_store_release(store);
	printf("Test PASSED\n\n");
}
#endif	// PC
#endif	// POS_STORE_AVAILABLE
