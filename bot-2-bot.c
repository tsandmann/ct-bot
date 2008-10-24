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
 * @file 	bot-2-bot.c
 * @brief 	Bot-2-Bot-Kommunikation
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	19.03.2008
 */

//TODO:	regelmaessig Pings senden, um inaktive Bots aus der Liste entfernen zu koennen?

//#define DEBUG_BOT2BOT		/*!< Schaltet LOG-Ausgaben (z.B. Bot-Liste) ein oder aus */

#include "ct-Bot.h"
#ifdef BOT_2_BOT_AVAILABLE
#include "bot-2-bot.h"
#include "log.h"

#ifndef DEBUG_BOT2BOT
#undef LOG_AVAILABLE
#endif

bot_list_entry_t * bot_list = NULL;		/*!< Liste aller bekannten Bots */

int16_t my_state = BOT_STATE_AVAILABLE;	/*!< Der eigene Status */

/*!
 * Dummy, fuer Kommandos, die nicht bearbeitet werden sollen
 * @param *cmd	Zeiger auf ein Kommando
 */
void default_cmd(command_t * cmd) {
	// NOP
}

/*!
 * Funktionstabelle fuer alle Bot-2-Bot-Kommandos
 */
void (* cmd_functions[])(command_t * cmd) = {
	add_bot_to_list,
	set_received_bot_state,
};

/*!
 * Gibt die Anzahl der Kommando-Funktionen zurueck
 * @return	Anzahl der Funktionen in cmd_functions
 */
uint8_t get_bot2bot_cmds(void) {
	return sizeof(cmd_functions) / sizeof(cmd_functions[0]);
}

/*!
 * Liefert die Kommando-Nummer zu einer Kommando-Funktion
 * @param *func	Name der auswertenden Funktion
 * @return		Kommando-ID
 */
uint8_t get_command_of_function(void (* func)(command_t * cmd)) {
	uint8_t i;
	for (i=0; i<sizeof(cmd_functions)/sizeof(void *); i++) {
		if (cmd_functions[i] == func) return i;
	}
	return 255;
}

/*!
 * Fuegt der Bot-Liste einen Bot hinzu.
 * Neue Bots sind per Definition erstmal AVAILABLE
 * @param *cmd	Zeiger auf empfangenes Kommando
 */
void add_bot_to_list(command_t * cmd) {
	uint8_t addr = cmd->from;
	/* dem neuen Bot Hallo sagen */
	command_write_to(BOT_CMD_STATE, 0, addr, &my_state, NULL, 0);

	/* und ihn in die eigene Liste eintragen */
	bot_list_entry_t * ptr = bot_list;
	if (ptr == NULL) {
		/* Liste noch ganz leer */
		ptr = bot_list = malloc(sizeof(bot_list_entry_t));
		if (ptr == NULL) return;	// Fehler
		ptr->address = addr;
		ptr->state = BOT_STATE_AVAILABLE;
		ptr->next = NULL;
		#ifdef LOG_AVAILABLE
			print_bot_list();
		#endif
		return;
	}

	/* Adresse in der Liste suchen */
	while (1) {
		if (ptr->address == addr) {
			/* Eintrag ist ja schon vorhanden */
			ptr->state = BOT_STATE_AVAILABLE;
			#ifdef LOG_AVAILABLE
				print_bot_list();
			#endif
			return;	// fertig
		}
		if (ptr->next == NULL) break;	// Ende, ptr zeigt auf letzten Eintrag
		ptr = ptr->next;		// auf zum Naechsten
	}

	/* Zombies suchen */
	ptr = bot_list;
	while (1) {
		if (ptr->state == BOT_STATE_GONE) {
			/* updaten */
			ptr->address = addr;
			ptr->state = BOT_STATE_AVAILABLE;
			#ifdef LOG_AVAILABLE
				print_bot_list();
			#endif
			return;
		}
		if (ptr->next == NULL) break;
		ptr = ptr->next;
	}

	/* neuen Eintrag anhaengen */
	ptr->next = malloc(sizeof(bot_list_entry_t));
	ptr = ptr->next;
	if (ptr == NULL) return;	// Fehler

	/* und initialisieren */
	ptr->address = addr;
	ptr->state = BOT_STATE_AVAILABLE;
	ptr->next = NULL;
	#ifdef LOG_AVAILABLE
		print_bot_list();
	#endif
}

#ifdef DELETE_BOTS
/*!
 * Setzt einen Bot in der Liste auf inaktiv / verschwunden
 * @param address	Bot-Adresse
 */
void delete_bot_from_list(uint8_t address) {
	bot_list_entry_t * ptr = bot_list;
	/* Bot zur Adresse suchen */
	while (ptr != NULL) {
		if (ptr->address == address) {
			ptr->state = BOT_STATE_GONE;	// Eintrag deaktivieren
		}
		ptr = ptr->next;
	}
}
#endif	// DELETE_BOTS

/*!
 * Sucht den naechsten verfuegbaren Bot in der Botliste
 * @param *ptr	Zeiger auf letzten Eintrag oder NULL (=Anfang)
 * @return		Zeiger auf verfuegbaren Bot oder NULL (=alle busy)
 */
bot_list_entry_t * get_next_available_bot(bot_list_entry_t * ptr) {
	while(1) {
		ptr = get_next_bot(ptr);
		if (ptr == NULL || ptr->state == BOT_STATE_AVAILABLE) return ptr;
	}
}

/*!
 * Setzt einen empfangenen Status eines Bots
 * @param *cmd	Zeiger auf Kommando mit Daten
 */
void set_received_bot_state(command_t * cmd) {
	bot_list_entry_t * ptr = NULL;
	while (1) {
		ptr = get_next_bot(ptr);
		if (ptr == NULL) break;
		if (ptr->address == cmd->from) {
			ptr->state = cmd->data_l;
			#ifdef LOG_AVAILABLE
				print_bot_list();
			#endif
			return;
		}
	}
	add_bot_to_list(cmd);
}

/*!
 * Setzt den eigenen Status auf einen neuen Wert und schickt ihn an
 * alle Bots
 * @param *state	Zeiger auf Status
 */
void publish_bot_state(void * state) {
	command_write_to(BOT_CMD_STATE, 0, CMD_BROADCAST, state, NULL, 0);
}

#ifdef LOG_AVAILABLE
/*!
 * Gibt die Liste der Bots inkl. Adresse und Status aus
 */
void print_bot_list(void) {
	bot_list_entry_t * ptr = NULL;
	while (1) {
		ptr = get_next_bot(ptr);
		if (ptr == NULL) return;
		LOG_DEBUG("Bot %u ist", ptr->address);
		switch (ptr->state) {
		case BOT_STATE_AVAILABLE:
			LOG_DEBUG("\tavailable");
			break;
		case BOT_STATE_BUSY:
			LOG_DEBUG("\tbusy");
			break;
		case BOT_STATE_GONE:
			LOG_DEBUG("\tweg");
			break;
		}
	}
}
#endif	// LOG_AVAILABLE

#endif	// BOT_2_BOT_AVAILABLE
