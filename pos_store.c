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

/**
 * \file 	pos_store.c
 * \brief 	Implementierung eines Positionsspeichers mit den ueblichen Stackbefehlen push(), pop()
 * 			und FIFO-Befehlen queue(), dequeue()
 * \author 	Frank Menzel (Menzelfr@gmx.net)
 * \date 	13.12.2007
 */

#include "ct-Bot.h"

#ifdef POS_STORE_AVAILABLE
#include "bot-logic.h"
#include "pos_store.h"
#include "bot-2-bot.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_POS_STORE		// Schalter fuer Debug-Ausgaben

#ifndef LOG_AVAILABLE
#undef DEBUG_POS_STORE
#endif

#ifndef DEBUG_POS_STORE
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif

#define SLOT_COUNT	4	/**< Anzahl der Plaetze im Array */

static pos_store_t pos_stores[SLOT_COUNT];	/**< Liste der Positionsspeicher */

/**
 * Erzeugt einen neuen Positionsspeicher angegebener Groesse
 * \param *owner	Zeiger Verhaltensdatensatz
 * \param *data		NULL oder Zeiger auf Speicher fuer size * sizeof(position_t) Bytes
 * \param size		Groesse des Speichers, <= POS_STORE_SIZE
 * \return			Zeiger auf neuen Positionsspeicher oder NULL
 */
pos_store_t * pos_store_create_size(Behaviour_t * owner, void * data, pos_store_size_t size) {
	if (owner == NULL) {
		LOG_ERROR("Fehler: owner==NULL!");
		return NULL;
	}
	if (size > POS_STORE_SIZE) {
		LOG_ERROR("Fehler: size > POS_STORE_SIZE");
		return NULL;
	}
	if (size & (size - 1)) {
		LOG_ERROR("Fehler: size keine 2er Potenz");
		return NULL;
	}

	LOG_DEBUG("Erzeuge Positionsspeicher (%u) fuer 0x%zx", size, (uintptr_t) owner);
	pos_store_t * store = pos_store_from_beh(owner);
	if (store && store->mask != size - 1) {
		/* Positionsspeicher existiert bereits, aber mit anderer Groesse */
		pos_store_release(store);
		store = NULL;
	}
	if (store == NULL) {
		/* Verhalten hat noch keinen Positionsspeicher -> anlegen */
		pos_store_t * ptr;
		for (ptr=pos_stores; ptr<=&pos_stores[SLOT_COUNT - 1]; ptr++) {
			if (ptr->owner == NULL) {
				/* freien Platz gefunden */
				store = ptr;
				store->owner = owner;
				store->mask = (pos_store_size_t) (size - 1);
				LOG_DEBUG("verwende Slot %zu", (store - pos_stores));
				if (data == NULL) {
					store->data = malloc(size * sizeof(position_t));
					store->stat_data = 0;
					LOG_DEBUG("verwende Heap-Speicher @ 0x%zx", (uintptr_t) store->data);
				} else {
					store->data = data;
					store->stat_data = 1;
					LOG_DEBUG("verwende statischen Speicher @ 0x%zx", (uintptr_t) data);
				}
				if (store->data == NULL) {
					LOG_ERROR("Kein Speicher zur Verfuegung, Abbruch!");
					pos_store_release(store);
					return NULL;
				}
				break;
			}
		}
	}

	pos_store_clear(store);
	LOG_DEBUG("Positionsspeicher @ 0x%zx angelegt", (uintptr_t) store);
	return store;
}

/**
 * Loescht einen Positionsspeicher
 * \param *store	Zeiger auf Positionsspeicher
 */
void pos_store_release(pos_store_t * store) {
	LOG_DEBUG("Gebe Positionsspeicher 0x%zx frei", (uintptr_t) store);
	if (store == NULL) {
		return;
	}
	store->owner = NULL;	// kein owner == Platz unbelegt
	if (store->stat_data == 0 && store->data != NULL) {
		/* Datenspeicher freigeben */
		free(store->data);
		LOG_DEBUG("Heap-Speicher 0x%zx freigegeben", (uintptr_t) store->data);
		store->data = NULL;
	}
}

/**
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

/**
 * Ermittelt den Positionsspeicher, der zu einem Verhalten gehoert
 * \param *owner	Zeiger auf Verhaltensdatensatz
 * \return			Zeiger auf Positionsspeicher oder NULL
 */
pos_store_t * pos_store_from_beh(Behaviour_t * owner) {
	pos_store_t * store;
	for (store=pos_stores; store<=&pos_stores[SLOT_COUNT - 1]; store++) {
		if (store->owner == owner) {
			return store;
		}
	}
	return NULL;
}

/**
 * Ermittelt den Index eines Positionsspeichers
 * \param *store	Zeiger auf Positionsspeicher
 * \return			Index des Positionsspeichers im Array
 */
uint8_t pos_store_get_index(pos_store_t * store) {
	size_t index = (size_t) store - (size_t) pos_stores; // Offset abziehen
	index /= sizeof(pos_store_t); // Differenz in Groesse umrechnen
	return (uint8_t) index;
}

/**
 * Ermittelt den Positionsspeicher, der den gegebenen Index im Array hat
 * \param index	Index des Positionsspeichers im Array
 * \return		Zeiger auf Positionsspeicher oder NULL
 */
pos_store_t * pos_store_from_index(uint8_t index) {
	if (index > SLOT_COUNT - 1) {
		return NULL;
	}
	return &pos_stores[index];
}

/**
 * Leert den Positionsspeicher
 * \param *store	Zeiger auf Positionsspeicher
 */
void pos_store_clear(pos_store_t * store) {
	LOG_DEBUG("Loesche Positionsspeicher 0x%zx", (uintptr_t) store);
	if (store == NULL) {
		return;
	}
	/* Status zuruecksetzen */
	store->sp = 0;
	store->fp = 0;
	store->count = 0;
}

/**
 * Speicher leer?
 * \return True falls Speicher leer sonst False
 */
static uint8_t is_empty(pos_store_t * store) {
	if (store == NULL) {
		/* Fehler */
		return True;
	}
	return (uint8_t) (store->count == 0);
}

/**
 * Speicher voll?
 * \return True falls Speicher voll, sonst False
 */
static uint8_t is_full(pos_store_t * store) {
	if (store == NULL || store->data == NULL) {
		/* Fehler */
		return True;
	}
	return (uint8_t) (store->count > store->mask);
}

/**
 * Speichern einer Koordinate vorne im Speicher
 * \param *store	Zeiger auf Positionsspeicher
 * \param pos		X/Y-Koordinaten des zu sichernden Punktes
 * \return			True wenn erfolgreich sonst False wenn Array voll ist
 */
uint8_t pos_store_insert(pos_store_t * store, position_t pos) {
	if (is_full(store)) {
		LOG_INFO("Pos-Store 0x%zx voll, kein push moeglich", (uintptr_t) store);
		LOG_DEBUG(" count=%u", store->count);
		return False;
	}
	pos_store_pointer_t fp = store->fp;
	fp--;
	fp = (pos_store_pointer_t) (fp & store->mask);
	store->data[fp] = pos;
	store->fp = fp;
	store->count++;
	return True;
}

/**
 * Speichern einer Koordinate auf dem Stack
 * \param *store	Zeiger auf Positionsspeicher
 * \param pos		X/Y-Koordinaten des zu sichernden Punktes
 * \return			True wenn erfolgreich sonst False wenn Array voll ist
 */
uint8_t pos_store_push(pos_store_t * store, position_t pos) {
	if (is_full(store)) {
		LOG_INFO("Pos-Store 0x%zx voll, kein push moeglich", (uintptr_t) store);
		LOG_DEBUG(" count=%u", store->count);
		return False;
	}
	pos_store_pointer_t sp = store->sp;
	store->data[sp] = pos;
	sp++;
	sp = (pos_store_pointer_t) (sp & store->mask);
	store->sp = sp;
	store->count++;
	return True;
}

/**
 * Pop-Routine zur Rueckgabe des letzten auf dem Stack gepushten Punktes
 * \param *store	Zeiger auf Positionsspeicher
 * \param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * \return			False falls Pop nicht erfolgreich, d.h. kein Punkt mehr auf dem Stack, sonst True nach erfolgreichem Pop
 */
uint8_t pos_store_pop(pos_store_t * store, position_t * pos) {
	if (is_empty(store)) {
		return False;
	}
	store->count--;
	store->sp--;
	store->sp = (pos_store_pointer_t) (store->sp & store->mask);
	*pos = store->data[store->sp];
	return True;
}

/**
 * Erweiterung des Stacks zur Queue; Element wird vorn entnommen
 * \param *store	Zeiger auf Positionsspeicher
 * \param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * \return 			True wenn Element erfolgreich entnommen werden konnte sonst False falls kein Element mehr enthalten ist
 */
uint8_t pos_store_dequeue(pos_store_t * store, position_t * pos) {
	if (is_empty(store)) {
		return False;
	}
	store->count--;
	*pos = store->data[store->fp];
	store->fp++;
	store->fp = (pos_store_pointer_t) (store->fp & store->mask);
	return True;
}

/**
 * Gibt das n-letzte Element des Stacks / der Queue zurueck, entfernt es aber nicht.
 * pos_store_top(&store,  &pos, 2) gibt z.B. das vorletzte Element zurueck
 * \param *store	Zeiger auf Positionsspeicher
 * \param *pos		Zeiger auf Rueckgabe-Speicher der Position
 * \param index		Index des gewuenschten Elements vom Ende aus gezaehlt, 1-based
 * \return			True, wenn ein Element im Speicher ist, sonst False
 */
uint8_t pos_store_top(pos_store_t * store, position_t * pos, uint8_t index) {
	if (store == NULL) {
		return False;
	}
	if (index > store->count) {
		return False;
	}
	pos_store_pointer_t sp = (pos_store_pointer_t) (store->sp - index);
	sp = (pos_store_pointer_t) (sp & store->mask);
	*pos = store->data[sp];
	return True;
}

#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
pos_store_t * bot_2_bot_pos_store;

/**
 * Uebertraegt einen Positionsspeicher an einen anderen Bot
 * \param *store	Zeiger auf den zu uebertragenden Positionsspeicher
 * \param bot		Adresse des Zielbots
 * \return			Fehlercode (0: alles ok)
 */
int8_t pos_store_send_to_bot(pos_store_t * store, uint8_t bot) {
	if (store == NULL || store->owner == NULL) {
		return -1;
	}
	command_write_to(CMD_BOT_2_BOT, BOT_CMD_POS_STORE, bot, 0, store->owner->priority, 0);
	command_write_to(CMD_BOT_2_BOT, BOT_CMD_POS_STORE, bot, 1, (store->mask + 1) * (int16_t) sizeof(position_t), 0);
	command_write_to(CMD_BOT_2_BOT, BOT_CMD_POS_STORE, bot, 2, store->count, 0);
	command_write_to(CMD_BOT_2_BOT, BOT_CMD_POS_STORE, bot, 3, store->fp, 0);
	command_write_to(CMD_BOT_2_BOT, BOT_CMD_POS_STORE, bot, 4,store->sp, 0);
	return bot_2_bot_send_payload_request(bot, BOT_2_BOT_POS_STORE, store->data, (store->mask + 1) * (int16_t) sizeof(position_t));
}

/**
 * Verarbeitet eine Positionsspeicher-Empfang-Anfrage
 */
void bot_2_bot_handle_pos_store(command_t * cmd) {
	static uint8_t owner = 0;
	switch (cmd->data_l) {
	case 0:
		owner = (uint8_t) cmd->data_r;
		break;

	case 1:
		bot_2_bot_pos_store = pos_store_new_size(get_behaviour_from_prio(owner), (pos_store_size_t) cmd->data_r);
		break;

	case 2:
		if (bot_2_bot_pos_store->owner != NULL) {
			bot_2_bot_pos_store->count = (pos_store_size_t) cmd->data_r;
		}
		break;

	case 3:
		if (bot_2_bot_pos_store->owner != NULL) {
			bot_2_bot_pos_store->fp = (pos_store_size_t) cmd->data_r;
		}
		break;

	case 4:
		if (bot_2_bot_pos_store->owner != NULL) {
			bot_2_bot_pos_store->sp = (pos_store_pointer_t) cmd->data_r;
			if (bot_2_bot_pos_store->data != NULL) {
				uint8_t index = BOT_2_BOT_POS_STORE;
				bot_2_bot_payload_mappings[index].size = (bot_2_bot_pos_store->mask + 1) * (int16_t) sizeof(position_t);
				bot_2_bot_payload_mappings[index].data = bot_2_bot_pos_store->data;
			} else {
				pos_store_release(bot_2_bot_pos_store);
				LOG_DEBUG("Fehler, Positionsspeicher hat keinen Datenpuffer zugewiesen");
			}
		}
		break;
	}
}

/**
 * Verarbeitet einen Positionsspeicher-Empfang
 */
void bot_2_bot_handle_pos_store_data(void) {
	if (bot_2_bot_pos_store && bot_2_bot_pos_store->owner) {
		LOG_DEBUG("Pos-Store fuer Verhalten %u empfangen", bot_2_bot_pos_store->owner->priority);
		LOG_DEBUG(" Groesse:%u\tfp=%u\tsp=%u\tcount=%u", bot_2_bot_pos_store->mask + 1, bot_2_bot_pos_store->fp, bot_2_bot_pos_store->sp, bot_2_bot_pos_store->count);
		LOG_DEBUG(" data=0x%zx", (uintptr_t) (bot_2_bot_pos_store->data));
#ifdef PC
		pos_store_dump(bot_2_bot_pos_store);
#endif // PC
		uint8_t index = BOT_2_BOT_POS_STORE;
		bot_2_bot_payload_mappings[index].size = 0;
		bot_2_bot_payload_mappings[index].data = NULL;
	}
}
#endif // BOT_2_BOT_PAYLOAD_AVAILABLE

#ifdef PC
/**
 * Gibt alle Eintraege auf stdout aus
 * \param *store Zeiger auf Positionsspeicher
 */
void pos_store_dump(pos_store_t * store) {
	int16_t i;
	for (i=0; i<store->count; i++) {
		int16_t x = store->data[(store->fp + i) & store->mask].x;
		int16_t y = store->data[(store->fp + i) & store->mask].y;
		printf("%d:\tx=%d\ty=%d\n", i + 1, x, y);
	}
	printf("fp=%u\tsp=%u\tcount=%u\tsize=%u\t\n\n", store->fp, store->sp, store->count, store->mask + 1);
}

/**
 * Testet push(), pop() und dequeue()
 */
void pos_store_test(void) {
	pos_store_t * store = pos_store_new((Behaviour_t *)0x42);
	if (store == NULL) {
		printf("ERROR 1\n\n");
		return;
	}
	pos_store_t * store2 = pos_store_from_beh((Behaviour_t *)0x42);
	if (store2 != store) {
		printf("ERROR 2\n\n");
		return;
	}
	uint8_t index = pos_store_get_index(store);
	printf("index=%u\n", index);
	store2 = pos_store_from_index(index);
	if (store2 != store) {
		printf("ERROR 3\n\n");
		return;
	}
	store2 = pos_store_new((Behaviour_t *)0x42);
	if (store2 != store) {
		printf("ERROR 4\n\n");
		return;
	}
	store2 = pos_store_new((Behaviour_t *)42);
	if (store2 == store) {
		printf("ERROR 5\n\n");
		return;
	}
	index = pos_store_get_index(store2);
	printf("index2=%u\n\n", index);
	pos_store_release(store2);
	if (store2->owner != NULL || store2->data != NULL) {
		printf("ERROR 6\n\n");
		return;
	}
	pos_store_dump(store);
	int16_t i;
	for (i=0; i<=store->mask+1; i++) {
		uint8_t result = pos_store_push(store, (position_t) {i, i + 50});
		printf("push(%d, %d)=%u\n", i, i + 50, result);
		pos_store_dump(store);
		if (result != 1 && i < store->mask + 1) {
			printf("ERROR 7\ti=%d\n\n", i);
			return;
		}
		if (result != 0 && i >= store->mask + 1) {
			printf("ERROR 8\ti=%d\n\n", i);
			return;
		}
	}
	int16_t exspected_x = store->mask;
	int16_t exspected_y = exspected_x + 50;
	for (i=0; i<5; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_pop(store, &pos);
		printf("pop()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		pos_store_dump(store);
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR 9\ti=%d\n\n", i);
			printf("pos.x=%d exspected_x=%d\tpos.y=%d exspected_y=%d\n\n", pos.x, exspected_x, pos.y, exspected_y);
			return;
		}
		exspected_x--;
		exspected_y--;
	}
	for (i=0; i<5; i++) {
		uint8_t result = pos_store_push(store, (position_t) {i - 100, i});
		printf("push(%d, %d)=%u\n", i - 100, i, result);
		pos_store_dump(store);
		if (result != 1) {
			printf("ERROR 10\n\n");
			return;
		}
	}
	exspected_x = 0;
	exspected_y = 50;
	for (i=0; i<10; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_dequeue(store, &pos);
		printf("dequeue()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		pos_store_dump(store);
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR 11\ti=%d\n\n", i);
			printf("pos.x=%d exspected_x=%d\tpos.y=%d exspected_y=%d\n\n", pos.x, exspected_x, pos.y, exspected_y);
			return;
		}
		exspected_x++;
		exspected_y++;
	}

	if (store->mask < 31) {
		pos_store_clear(store);
		pos_store_dump(store);
		pos_store_release(store);
		printf("Test PASSED\n\n");
		return;
	}

	exspected_x = -96;
	exspected_y = 4;
	for (i=0; i<10; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_pop(store, &pos);
		printf("pop()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		pos_store_dump(store);
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR 12\ti=%d\n\n", i);
			printf("pos.x=%d exspected_x=%d\tpos.y=%d exspected_y=%d\n\n", pos.x, exspected_x, pos.y, exspected_y);
			return;
		}
		exspected_x--;
		exspected_y--;
		if (exspected_x == -101 && exspected_y == -1) {
			exspected_x = store->mask - 5;
			exspected_y = exspected_x + 50;
		}
	}
	for (i=0; i<15; i++) {
		uint8_t result = pos_store_insert(store, (position_t) {i - 300, i});
		printf("insert(%d, %d)=%u\n", i - 300, i, result);
		pos_store_dump(store);
		if (result != 1) {
			printf("ERROR 13\ti=%d\n\n", i);
			return;
		}
	}
	exspected_x = -286;
	exspected_y = 14;
	for (i=0; i<15; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_dequeue(store, &pos);
		printf("dequeue()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		pos_store_dump(store);
		if (pos.x != exspected_x || pos.y != exspected_y) {
			printf("ERROR 14\ti=%d\n\n", i);
			printf("pos.x=%d exspected_x=%d\tpos.y=%d exspected_y=%d\n\n", pos.x, exspected_x, pos.y, exspected_y);
			return;
		}
		exspected_x--;
		exspected_y--;
	}

	for (i=0; i<20; i++) {
		uint8_t result = pos_store_push(store, (position_t) {i - 200, i});
		printf("push(%d, %d)=%u\n", i - 200, i, result);
		pos_store_dump(store);
		if (result != 1) {
			printf("ERROR 15\ti=%d\n\n", i);
			return;
		}
	}
	exspected_x = -181;
	exspected_y = 19;
	for (i=0; i<=store->mask+1; i++) {
		position_t pos = {0, 0};
		uint8_t result = pos_store_pop(store, &pos);
		printf("pop()=%u\tx=%d\ty=%d\n", result, pos.x, pos.y);
		pos_store_dump(store);
		if (result != 1 && i < store->mask + 1) {
			printf("ERROR 16\ti=%d\n\n", i);
			return;
		}
		if (result != 0 && i >= store->mask + 1) {
			printf("ERROR 17\ti=%d\n\n", i);
			return;
		}
		if (result == 1 && (pos.x != exspected_x || pos.y != exspected_y)) {
			printf("ERROR 18\ti=%d\n\n", i);
			printf("pos.x=%d exspected_x=%d\tpos.y=%d exspected_y=%d\n\n", pos.x, exspected_x, pos.y, exspected_y);
			return;
		}
		exspected_x--;
		exspected_y--;
		if (exspected_x == -201) {
			exspected_x = store->mask - 10;
			exspected_y = exspected_x + 50;
		}
	}
	pos_store_clear(store);
	pos_store_dump(store);
	pos_store_release(store);
	printf("Test PASSED\n\n");
}
#endif // PC
#endif // POS_STORE_AVAILABLE
