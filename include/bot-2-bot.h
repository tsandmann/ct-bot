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
 * @file 	bot-2-bot.h
 * @brief 	Bot-2-Bot-Kommunikation
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	19.03.2008
 */

#ifndef BOT2BOT_H_
#define BOT2BOT_H_

#include "ct-Bot.h"
#ifdef BOT_2_BOT_AVAILABLE
#include "command.h"
#include <stdlib.h>

//#define DELETE_BOTS		/*!< wollen wir Bots aus der Liste loeschen koennen? */

//TODO:	Vielleicht noch weitere Infos pro Bot speichern?
/*! Datentyp fuer Bot-Liste */
typedef struct _bot_list_entry_t {
	uint8_t address;					/*!< Bot-Adresse */
	uint8_t state;						/*!< letzter Status des Bots */
	struct _bot_list_entry_t * next;	/*!< Zeiger auf naechsten Bot */
} bot_list_entry_t;

#define BOT_STATE_AVAILABLE	0
#define BOT_STATE_BUSY		1
#define BOT_STATE_GONE		2

#define BOT_CMD_WELCOME		get_command_of_function(add_bot_to_list)
#define BOT_CMD_STATE		get_command_of_function(set_received_bot_state)

extern bot_list_entry_t * bot_list;	/*!< Liste aller bekannten Bots */
extern void (* cmd_functions[])(command_t * cmd);

/*!
 * Fuegt der Bot-Liste einen Bot hinzu
 * @param *cmd	Zeiger auf Kommando mit Bot-Adresse
 */
void add_bot_to_list(command_t * cmd);

#ifdef DELETE_BOTS
/*!
 * Setzt einen Bot in der Liste auf inaktiv / verschwunden
 * @param address	Bot-Adresse
 */
void delete_bot_from_list(uint8_t address);
#endif	// DELETE_BOTS

/*!
 * Sucht den naechsten verfuegbaren Bot in der Botliste
 * @param *ptr	Zeiger auf letzten Eintrag oder NULL (=Anfang)
 * @return		Zeiger auf verfuegbaren Bot oder NULL (=alle busy)
 */
bot_list_entry_t * get_next_available_bot(bot_list_entry_t * ptr);

/*!
 * Iteriert durch die Bot-Liste (zur einfachen Verwendung)
 * @param *ptr	Zeiger auf letzten Eintrag oder NULL (= Listenanfang)
 * @return		Naechster Listeneintrag
 */
static inline bot_list_entry_t * get_next_bot(bot_list_entry_t * ptr) {
	if (ptr == NULL) {
		return bot_list;
	}
	return ptr->next;
}

/*!
 * Liefert die Kommando-Nummer zu einer Kommando-Funktion
 * @param *func	Name der auswertenden Funktion
 * @return		Kommando-ID
 */
uint8_t get_command_of_function(void (* func)(command_t * cmd));

/*!
 * Setzt einen empfangenen Status eines Bots
 * @param *cmd	Zeiger auf Kommando mit Daten
 */
void set_received_bot_state(command_t * cmd);

/*!
 * Setzt den eigenen Status auf einen neuen Wert und schickt ihn an
 * alle Bots
 * @param *state	Zeiger auf Status
 */
void publish_bot_state(void * state);

#ifdef PC
/*!
 * Gibt die Liste der Bots inkl. Adresse und Status aus
 */
void print_bot_list(void);
#endif	// PC

#endif	// BOT_2_BOT_AVAILABLE
#endif	// BOT2BOT_H_
