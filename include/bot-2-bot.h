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

#ifdef BOT_2_BOT_AVAILABLE

//#define DELETE_BOTS		/*!< wollen wir Bots aus der Liste loeschen koennen? */
//#define BOT_2_BOT_PAYLOAD_TEST_AVAILABLE	/*!< Aktiviert Test-Code fuer Bot-2-Bot Kommunikation mit Payload */

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
#define BOT_CMD_REQ			get_command_of_function(bot_2_bot_handle_payload_request)
#define BOT_CMD_ACK			get_command_of_function(bot_2_bot_handle_payload_ack)
#define BOT_CMD_PAYLOAD		get_command_of_function(bot_2_bot_handle_payload_data)
#define BOT_CMD_POS_STORE	get_command_of_function(bot_2_bot_handle_pos_store)

#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
#define BOT_2_BOT_PAYLOAD_TEST	get_type_of_payload_function(bot_2_bot_payload_test_verify)
#define BOT_2_BOT_REMOTECALL	get_type_of_payload_function(bot_2_bot_handle_remotecall)
#define BOT_2_BOT_POS_STORE		get_type_of_payload_function(bot_2_bot_handle_pos_store_data)

#define BOT_2_BOT_PAYLOAD_WINDOW_SIZE	32

#if defined MCU && BOT_2_BOT_PAYLOAD_WINDOW_SIZE > UART_BUFSIZE_IN
#error "BOT_2_BOT_PAYLOAD_WINDOW_SIZE zu gross"
#endif
#endif	// BOT_2_BOT_PAYLOAD_AVAILABLE

extern bot_list_entry_t * bot_list;					/*!< Liste aller bekannten Bots */
extern void (* cmd_functions[])(command_t * cmd);	/*!< Funktionstabelle fuer alle Bot-2-Bot-Kommandos */
/*! Datentyp der Bot-2-Bot-Payload-Zuordnungen */
typedef struct {
	void (* function)(void);	/*!< Callback-Funktion, die nach Abschluss des Empfangs ausgefuehrt wird */
	void * data;				/*!< Zeiger auf Datenpuffer */
	int16_t size;				/*!< Maximale Groesse des Datenpuffers in Byte */
} bot_2_bot_payload_mappings_t;

extern bot_2_bot_payload_mappings_t bot_2_bot_payload_mappings[];	/*!< Tabelle fuer alle Bot-2-Bot-Payload-Zuordnungen */

/*!
 * Gibt die Anzahl der Kommando-Funktionen zurueck
 * @return	Anzahl der Funktionen in cmd_functions
 */
uint8_t get_bot2bot_cmds(void);

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
 * @return		Kommando-ID, oder 255, falls Funktion nicht vorhanden
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
 * @param state	Neuer Status
 */
void publish_bot_state(int16_t state);

#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
/*!
 * Liefert den Payload-Typ (Subkommando) zu einer Auswertungsfunktion
 * @param *func	Name der auswertenden Funktion
 * @return		Subcommand oder 255, falls Funktion nicht vorhanden
 */
uint8_t get_type_of_payload_function(void(* func)(void));

/*!
 * Sendet eine Payload-Transferanfrage an einen anderen Bot
 * @param to			Empfaengeradresse
 * @param type			Typ der Daten fuer den anderen Bot
 * @param *data			Zeiger auf zu sendende Daten
 * @param size			Anzahl der Bytes, die zum anderen Bot uebertragen werden sollen
 * @return				0, falls die Daten korrekt uebertragen wurden, sonst Fehlercode
 */
int8_t bot_2_bot_send_payload_request(uint8_t to, uint8_t type,
		void * data, int16_t size);

/*!
 * Behandelt eine Payload-Sende-Anfrage
 * @param *cmd	Zeiger auf der empfangene Kommand
 */
void bot_2_bot_handle_payload_request(command_t * cmd);

/*!
 * Behandelt eine Payload-Sende-Bestaetigung
 * @param *cmd	Zeiger auf das empfangene Kommando
 */
void bot_2_bot_handle_payload_ack(command_t * cmd);

/*!
 * Behandelt eingehende Payload-Daten, die von einem anderen Bot kommen
 * @param *cmd	Zeiger auf das empfangene Kommando
 */
void bot_2_bot_handle_payload_data(command_t * cmd);

/*!
 * Gibt die empfangenen Daten auf stdout aus (nur PC)
 */
void bot_2_bot_print_recv_data(void);

#ifdef BOT_2_BOT_PAYLOAD_TEST_AVAILABLE
/*!
 * Testet den Payload-Empfang
 */
void bot_2_bot_payload_test_verify(void);

/*!
 * Testet den Payload-Versand
 * @param *caller	Dummy-Zeiger, damit per RemoteCall verwendbar
 * @param to		Adresse des Empfangsbots
 * @return			0, falls kein Fehler, sonst Fehlercode
 */
int8_t bot_2_bot_pl_test(Behaviour_t * caller, uint8_t to);
#endif	// BOT_2_BOT_PAYLOAD_TEST_AVAILABLE

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
/*!
 * Fuehrt einen RemoteCall aus, der von einem
 * anderen Bot kam
 */
void bot_2_bot_handle_remotecall(void);

/*!
 * Startet einen RemoteCall auf einem anderen Bot
 * @param bot_addr	Adresse des anderen Bots
 * @param *function	Name der Botenfunktion des zu startenden Verhaltens
 * @param par1		Erster Parameter des zu startenden Verhaltens
 * @param par2		Zweiter Parameter des zu startenden Verhaltens
 * @param par3		Dritter Parameter des zu startenden Verhaltens
 * @return			Fehlercode (0, falls alles OK)
 */
int8_t bot_2_bot_start_remotecall(uint8_t bot_addr, char * function, remote_call_data_t par1,
		remote_call_data_t par2, remote_call_data_t par3);
#endif	// BEHAVIOUR_REMOTECALL_AVAILABLE
#endif	// BOT_2_BOT_PAYLOAD_AVAILABLE

#ifdef LOG_AVAILABLE
/*!
 * Gibt die Liste der Bots inkl. Adresse und Status aus
 */
void print_bot_list(void);
#endif	// LOG_AVAILABLE

#endif // BOT_2_BOT_AVAILABLE
#endif // BOT2BOT_H_
