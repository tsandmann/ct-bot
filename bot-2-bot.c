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

#define DEBUG_BOT2BOT /*!< Schaltet LOG-Ausgaben (z.B. Bot-Liste) ein oder aus */

#include "ct-Bot.h"
#ifdef BOT_2_BOT_AVAILABLE
#include "bot-2-bot.h"
#include "log.h"
#include "tcp.h"
#include "sensor.h"
#include "pos_store.h"
#include <string.h>

#ifndef DEBUG_BOT2BOT
#undef LOG_AVAILABLE
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {} /*!< Log-Dummy */
#endif

bot_list_entry_t * bot_list = NULL; /*!< Liste aller bekannten Bots */

int16_t my_state = BOT_STATE_AVAILABLE; /*!< Der eigene Status */

#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
static volatile int16_t bot_2_bot_payload_size = 0;	/*!< Anzahl der (noch) zu sendenden oder erwarteten Bytes */
static uint8_t * bot_2_bot_data = NULL;				/*!< Zeiger auf die zu sendenden oder empfangenen Daten */
static void (* bot_2_bot_callback)(void) = NULL;	/*!< Callback-Funktion, die nach Abschluss des Empfangs ausgefuehrt wird */

#ifdef BOT_2_BOT_PAYLOAD_TEST_AVAILABLE
static uint8_t payload_test_buffer[255]; /*!< Datenpuffer fuer Bot-2-Bot-Payload-Test */
#endif // BOT_2_BOT_PAYLOAD_TEST_AVAILABLE

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
static char remotecall_buffer[REMOTE_CALL_BUFFER_SIZE]; /*!< Puffer fuer RemoteCall-Empfang von anderem Bot */
#endif
#endif // BOT_2_BOT_PAYLOAD_AVAILABLE

void default_cmd(command_t * cmd);

/*!
 * Dummy, fuer Kommandos, die nicht bearbeitet werden sollen
 * @param *cmd	Zeiger auf ein Kommando
 */
void default_cmd(command_t * cmd) {
	cmd = cmd;
	// NOP
}

/*! Dummy-Eintrag fuer Funktionsliste, falls entsprechender Code inaktiv */
#define BOT_2_BOT_DUMMY	default_cmd

/*!
 * Funktionstabelle fuer alle Bot-2-Bot-Kommandos.
 * Hier muss man die Auswertungs-Funktionen eintragen, wenn man
 * die Bot-2-Bot-Kommunikation fuer weitere Zwecke benutzen moechte.
 * Abschaltbare Teile muessen den Dummy "BOT_2_BOT_DUMMY" per ifdef
 * aktivieren, damit sich in der Liste keine Positionen verschieben,
 * falls bestimmte Code-Teile inaktiv sind!
 */
void (* cmd_functions[])(command_t * cmd) = {
		add_bot_to_list,
		set_received_bot_state,
#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
		bot_2_bot_handle_payload_request,
		bot_2_bot_handle_payload_ack,
		bot_2_bot_handle_payload_data,
#ifdef POS_STORE_AVAILABLE
		bot_2_bot_handle_pos_store,
#else
		BOT_2_BOT_DUMMY,
#endif // POS_STORE_AVAILABLE
#else
		BOT_2_BOT_DUMMY,
		BOT_2_BOT_DUMMY,
		BOT_2_BOT_DUMMY,
		BOT_2_BOT_DUMMY,
#endif // BOT_2_BOT_PAYLOAD_AVAILABLE
	};

#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
/*! Dummy-Eintrag fuer Payload-Mappings, falls entsprechender Code inaktiv */
#define BOT_2_BOT_PAYLOAD_DUMMY	{ (void (*)(void)) default_cmd, NULL, 0 }

/*!
 * Tabelle fuer alle Bot-2-Bot-Payload-Zuordnungen.
 * Hier muss man die Callback-Funktionen und Datenpuffer eintragen,
 * wenn man die Bot-2-Bot-Kommunikation mit Payload-Versand fuer
 * weitere Zwecke benutzen moechte.
 * Abschaltbare Teile muessen den Dummy "BOT_2_BOT_PAYLOAD_DUMMY"
 * per ifdef aktivieren, damit sich in der Liste keine Positionen
 * verschieben, falls bestimmte Code-Teile inaktiv sind!
 */
bot_2_bot_payload_mappings_t bot_2_bot_payload_mappings[] = {
#ifdef BOT_2_BOT_PAYLOAD_TEST_AVAILABLE
	{ bot_2_bot_payload_test_verify, payload_test_buffer, sizeof(payload_test_buffer) },
#else
	BOT_2_BOT_PAYLOAD_DUMMY,
#endif // BOT_2_BOT_PAYLOAD_TEST_AVAILABLE
#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
	{ bot_2_bot_handle_remotecall, remotecall_buffer, sizeof(remotecall_buffer) },
#else
	BOT_2_BOT_PAYLOAD_DUMMY,
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE
#ifdef POS_STORE_AVAILABLE
	{ bot_2_bot_handle_pos_store_data, NULL, 0 },
#else
	BOT_2_BOT_PAYLOAD_DUMMY,
#endif // POS_STORE_AVAILABLE
};
#endif // BOT_2_BOT_PAYLOAD_AVAILABLE

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
 * @return		Kommando-ID, oder 255, falls Funktion nicht vorhanden
 */
uint8_t get_command_of_function(void(* func)(command_t * cmd)) {
	uint8_t i;
	for (i = 0; i < sizeof(cmd_functions) / sizeof(void *); ++i) {
		if (cmd_functions[i] == func)
			return i;
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
	command_write_to(BOT_CMD_STATE, 0, addr, my_state, 0, 0);

	/* und ihn in die eigene Liste eintragen */
	bot_list_entry_t * ptr = bot_list;
	if (ptr == NULL) {
		/* Liste noch ganz leer */
		ptr = bot_list = malloc(sizeof(bot_list_entry_t));
		if (ptr == NULL)
			return; // Fehler
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
			return; // fertig
		}
		if (ptr->next == NULL)
			break; // Ende, ptr zeigt auf letzten Eintrag
		ptr = ptr->next; // auf zum Naechsten
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
		if (ptr->next == NULL)
			break;
		ptr = ptr->next;
	}

	/* neuen Eintrag anhaengen */
	ptr->next = malloc(sizeof(bot_list_entry_t));
	ptr = ptr->next;
	if (ptr == NULL)
		return; // Fehler

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
			ptr->state = BOT_STATE_GONE; // Eintrag deaktivieren
		}
		ptr = ptr->next;
	}
}
#endif // DELETE_BOTS
/*!
 * Sucht den naechsten verfuegbaren Bot in der Botliste
 * @param *ptr	Zeiger auf letzten Eintrag oder NULL (=Anfang)
 * @return		Zeiger auf verfuegbaren Bot oder NULL (=alle busy)
 */
bot_list_entry_t * get_next_available_bot(bot_list_entry_t * ptr) {
	while (1) {
		ptr = get_next_bot(ptr);
		if (ptr == NULL || ptr->state == BOT_STATE_AVAILABLE)
			return ptr;
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
		if (ptr == NULL)
			break;
		if (ptr->address == cmd->from) {
			ptr->state = (uint8_t) cmd->data_l;
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
 * @param state	Neuer Status
 */
void publish_bot_state(int16_t state) {
	my_state = state;
	command_write_to(BOT_CMD_STATE, 0, CMD_BROADCAST, my_state, 0, 0);
}

#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
/*!
 * Liefert den Payload-Typ (Subkommando) zu einer Auswertungsfunktion
 * @param *func	Name der auswertenden Funktion
 * @return		Subcommand oder 255, falls Funktion nicht vorhanden
 */
uint8_t get_type_of_payload_function(void(* func)(void)) {
	uint8_t i;
	for (i = 0; i < sizeof(bot_2_bot_payload_mappings) / sizeof(bot_2_bot_payload_mappings[0]); ++i) {
		if (bot_2_bot_payload_mappings[i].function == func)
			return i;
	}
	return 255;
}

/*! @todo Bot-Adressen ueberpruefen */

/*!
 * Sendet eine Payload-Transferanfrage an einen anderen Bot
 * @param to			Empfaengeradresse
 * @param type			Typ der Daten fuer den anderen Bot
 * @param *data			Zeiger auf zu sendende Daten
 * @param size			Anzahl der Bytes, die zum anderen Bot uebertragen werden sollen
 * @return				0, falls die Daten korrekt uebertragen wurden, sonst Fehlercode
 */
int8_t bot_2_bot_send_payload_request(uint8_t to, uint8_t type,
		void * data, int16_t size) {
	if (to == get_bot_address()) {
		/* kein loop-back */
		return -1;
	}
//	if (size > BOT_2_BOT_MAX_PAYLOAD) {
//		/* Datenmenge zu gross */
//		return -2;
//	}
	LOG_DEBUG("Fordere Payload-Senderecht (%u) vom Typ %u bei Bot %u an", BOT_CMD_REQ, type, to);
	LOG_DEBUG(" zu sendende Daten umfassen %d Bytes @ 0x%lx", size, (size_t) data);
	command_write_to(BOT_CMD_REQ, 0, to, size, type, 0);
#ifdef PC
/*! @todo etwas unschoene Loesung */
	command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0);
#endif
	bot_2_bot_data = data;
	bot_2_bot_payload_size = size;
/*! @todo Timeout */
	/* warten auf ACK */
	while (bot_2_bot_payload_size >= 0) {
#ifdef MCU
/*! @todo receive_until_Frame() fuer MCU => einheitlicher Code hier fuer MCU und PC */
		while (uart_data_available() < sizeof(command_t)) {}
		if (command_read() == 0) {
			command_evaluate();
		}
#else	// PC
		LOG_DEBUG(" Warte auf (naechstes) ACK von Bot %u", to);
		if (receive_until_Frame(BOT_CMD_ACK) != 0) {
			LOG_DEBUG(" receive_until_Frame() meldet Fehler, Abbruch");
			bot_2_bot_data = NULL;
			bot_2_bot_callback = NULL;
			bot_2_bot_payload_size = 0;
			return -3;
		}
#endif // MCU
	}
	if (bot_2_bot_payload_size == -2) {
		LOG_DEBUG(" Alle Daten fehlerfrei zu Bot %u uebertragen", to);
		return 0;
	}
	LOG_DEBUG(" Sendeversuch mit Fehler abgebrochen, bot_2_bot_payload_size=%d", bot_2_bot_payload_size);
	return -4;
}

/*!
 * Behandelt eine Payload-Sende-Anfrage
 * @param *cmd	Zeiger auf das empfangene Kommando
 */
void bot_2_bot_handle_payload_request(command_t * cmd) {
/*! @todo Nur wenn Bot steht? */
	LOG_DEBUG("Payload-Sendeanfrage von Bot %u erhalten", cmd->from);
	LOG_DEBUG(" werte Payload-Sendeanfrage aus...");
	int16_t size = cmd->data_l;
	bot_2_bot_payload_size = size;
	LOG_DEBUG("  Anfrage umfasst %d Bytes", size);
	uint8_t type = (uint8_t) cmd->data_r;
	LOG_DEBUG("  und ist vom Typ %u", type);
	uint8_t error = 0;
	if (type > sizeof(bot_2_bot_payload_mappings) / sizeof(bot_2_bot_payload_mappings[0])) {
		LOG_DEBUG("  Typ %u ist ungueltig, Abbruch", type);
		error = 1;
	}
//	if (bot_2_bot_payload_mappings[type].data == NULL) {
//		LOG_DEBUG("  Typ %u ist nicht aktiv", type);
//		error++;
//	}
//	if (size > BOT_2_BOT_MAX_PAYLOAD) {
//		LOG_DEBUG("  Datenumfang ist zu gross, max. %u Bytes", BOT_2_BOT_MAX_PAYLOAD);
//		error++;
//	}
	if (size > bot_2_bot_payload_mappings[type].size) {
		LOG_DEBUG("  Datenumfang ist fuer Typ %u zu gross, max. %u Bytes", type, bot_2_bot_payload_mappings[type].size);
		error++;
	}

	if (error != 0) {
		/* Anfrage ablehnen */
		LOG_DEBUG(" Lehne Anfrage ab, error=%u", error);
		int16_t result = 1;
		command_write_to(BOT_CMD_ACK, 0, cmd->from, 0, result, 0);
		bot_2_bot_payload_size = 0;
	} else {
		/* Anfrage ok, ACK senden */
		int16_t window_size = BOT_2_BOT_PAYLOAD_WINDOW_SIZE;
		int16_t result = 0;
		LOG_DEBUG(" Anfrage akzeptiert, setze window_size auf %d", window_size);
		command_write_to(BOT_CMD_ACK, 0, cmd->from, window_size, result, 0);

		/* Typ setzen */
		bot_2_bot_callback = bot_2_bot_payload_mappings[type].function;
		LOG_DEBUG("  Callback-Funktion = 0x%lx", (size_t)bot_2_bot_callback);
		bot_2_bot_data = bot_2_bot_payload_mappings[type].data;
		LOG_DEBUG("  Datenpuffer @ 0x%lx", (size_t)bot_2_bot_data);
#ifdef MCU
/*! @todo Timeout */
		/* warten auf Kommando, dem die Payload-Daten folgen */
		while (42) {
			while (uart_data_available() < sizeof(command_t)) {}
			if (command_read() == 0) {
				command_evaluate();
				if (received_command.from == cmd->from && received_command.request.command == BOT_CMD_PAYLOAD) {
					break;
				}
			}
		}
#endif // MCU
	}
}

/*!
 * Behandelt eine Payload-Sende-Bestaetigung
 * @param *cmd	Zeiger auf das empfangene Kommando
 */
void bot_2_bot_handle_payload_ack(command_t * cmd) {
	if (bot_2_bot_data == NULL) {
		LOG_DEBUG("ACK von Bot %u empfangen, aber gar kein Transfer aktiv!", cmd->from);
		return;
	}
	switch (cmd->data_r) {
	case 0: {
			/* Der andere Bot hat unsere Anfrage akzeptiert */
			uint8_t window_size = (uint8_t) cmd->data_l;
			int16_t to_send = bot_2_bot_payload_size;
			LOG_DEBUG(" ACK von Bot %u, habe noch %d Bytes zu senden", cmd->from, to_send);
			LOG_DEBUG("  Bot %u hat window_size=%d festgelegt", cmd->from, window_size);
			to_send -= window_size;
			int16_t last_packet = 0;
			if (to_send < 0) {
				/* Rest */
				window_size = (uint8_t) (window_size + to_send);
				to_send = 0;
				last_packet = 1;
			}
			bot_2_bot_payload_size = to_send;
			LOG_DEBUG(" Sende %d Bytes zu Bot %u", window_size, cmd->from);
			command_write_rawdata_to(BOT_CMD_PAYLOAD, 0, cmd->from, last_packet, 0,
					window_size, bot_2_bot_data);
#ifdef PC
/*! @todo etwas unschoene Loesung */
			command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0);
#endif
			bot_2_bot_data += window_size;
			break;
		}

	case 1:
		/* Abbruch */
		LOG_DEBUG(" Bot %u hat Anfrage abgelehnt", cmd->from);
		bot_2_bot_callback = NULL;
		bot_2_bot_data = NULL;
		bot_2_bot_payload_size = -1;
		break;

	case 2:
		/* fertig */
		bot_2_bot_callback = NULL;
		bot_2_bot_data = NULL;
		if (bot_2_bot_payload_size != 0) {
			bot_2_bot_payload_size = -1;
			LOG_DEBUG(" Bot %u hat Abschluss gemeldet, habe aber noch %d Bytes zu senden", cmd->from, bot_2_bot_payload_size);
		} else {
			bot_2_bot_payload_size = -2;
			LOG_DEBUG(" Bot %u hat Abschluss gemeldet", cmd->from);
		}
		break;
	}
}

/*!
 * Behandelt eingehende Payload-Daten, die von einem anderen Bot kommen
 * @param *cmd	Zeiger auf das empfangene Kommando
 */
void bot_2_bot_handle_payload_data(command_t * cmd) {
	uint8_t size = cmd->payload;
	LOG_DEBUG(" Payload mit %u Bytes angekuendigt", size);
#ifdef MCU
/*! @todo Timeout */
	/* warten, bis Payload-Daten im Empfangspuffer */
	while (uart_data_available() < size) {}
#endif // MCU
	uint8_t n = low_read(bot_2_bot_data, size);
	LOG_DEBUG(" %u Bytes der Payload gelesen", n);
	if (size != n) {
		int16_t result = 1;
		command_write_to(BOT_CMD_ACK, 0, cmd->from, 0, result, 0);
		bot_2_bot_data = NULL;
		bot_2_bot_callback = NULL;
		bot_2_bot_payload_size = 0;
		LOG_DEBUG(" Fehler beim Lesen, Abbruch");
		return;
	}
	bot_2_bot_data += n;
	if (cmd->data_l == 1) {
		/* fertig */
		LOG_DEBUG(" Daten komplett empfangen");
		LOG_DEBUG(" fuehre Callback 0x%lx aus", (size_t) bot_2_bot_callback);
		bot_2_bot_callback();
		bot_2_bot_data = NULL;
		bot_2_bot_callback = NULL;
		bot_2_bot_payload_size = 0;
		/* letztes ACK senden */
		LOG_DEBUG(" bestaetige Bot %u den Abschluss der Uebertragung", cmd->from);
		int16_t result = 2;
		command_write_to(BOT_CMD_ACK, 0, cmd->from, 0, result, 0);
	} else {
		/* ACK senden */
		int16_t window_size = BOT_2_BOT_PAYLOAD_WINDOW_SIZE;
		int16_t result = 0;
		LOG_DEBUG(" bestaetige Bot %u den Empfang, window_size=%d", cmd->from, window_size);
		command_write_to(BOT_CMD_ACK, 0, cmd->from, window_size, result, 0);
	}
}

/*!
 * Gibt die empfangenen Daten auf stdout aus (nur PC)
 */
void bot_2_bot_print_recv_data(void) {
#if defined PC && defined DEBUG_BOT2BOT
	int16_t i;
	int16_t size = bot_2_bot_payload_size;
	printf("bot_2_bot_data @ 0x%lx\n", (size_t) bot_2_bot_data);
	printf("size = 0x%x\n", size);
	uint8_t * ptr = bot_2_bot_data;
	ptr -= size;
	printf("data: \n");
	for (i=0; i<size; i++) {
		printf("%02x ", *ptr);
		ptr++;
		if (i % 4 == 3) {
			printf(" ");
		}
		if (i % 16 == 15) {
			printf("\n");
		}
	}
	printf("\n");
#endif // DEBUG_BOT2BOT
}

#ifdef BOT_2_BOT_PAYLOAD_TEST_AVAILABLE
static const char * test_string = "Hey Bot!"; /*!< Testdaten fuer Payload-Test */

/*!
 * Testet den Payload-Empfang
 */
void bot_2_bot_payload_test_verify(void) {
	uint8_t i;
	uint8_t errors = 0;
	for (i=strlen(test_string)+1; i<sizeof(payload_test_buffer); i++) {
		if (payload_test_buffer[i] != i) {
			LOG_DEBUG("  Payload-Testdaten @ %u wurden fehlerhaft empfangen!", i);
			LOG_DEBUG("  soll: %u\tist: %u", i, payload_test_buffer[i]);
			errors++;
		}
	}
	if (strncmp(test_string, (char *) payload_test_buffer, sizeof(test_string)) == 0 && errors == 0) {
		LOG_DEBUG("  Payload-Empfangstest verlief fehlerfrei! :-)");
		LOG_DEBUG("   \"%s\"", payload_test_buffer);
	} else {
		LOG_DEBUG("  Payload-Empfangstest verlief mit %u Fehlern!", errors);
	}
	bot_2_bot_print_recv_data();
	memset(payload_test_buffer, 0, sizeof(payload_test_buffer));
}

/*!
 * Testet den Payload-Versand
 * @param *caller	Dummy-Zeiger, damit per RemoteCall verwendbar
 * @param to		Adresse des Empfangsbots
 * @return			0, falls kein Fehler, sonst Fehlercode
 */
int8_t bot_2_bot_pl_test(Behaviour_t * caller, uint8_t to) {
	uint16_t i;
	const char * tmp = "Hey Bot!";
	memset(payload_test_buffer, 0, sizeof(payload_test_buffer));
	strncpy((char *) payload_test_buffer, tmp, sizeof(payload_test_buffer) - 1);
	for (i = strlen((char *) payload_test_buffer) + 1;
			i < sizeof(payload_test_buffer); i++) {
		payload_test_buffer[i] = i;
	}
	int8_t result = bot_2_bot_send_payload_request(to,
			BOT_2_BOT_PAYLOAD_TEST, payload_test_buffer,
			sizeof(payload_test_buffer));
	LOG_DEBUG("bot_2_bot_payload_test(%u) abgeschlossen mit %d", to, result);
	memset(payload_test_buffer, 0, sizeof(payload_test_buffer));
	if (caller != NULL) {
		caller->subResult = result == 0 ? SUBSUCCESS : SUBFAIL;
	}
	return result;
}
#endif // BOT_2_BOT_PAYLOAD_TEST_AVAILABLE

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
/*!
 * Fuehrt einen RemoteCall aus, der von einem
 * anderen Bot kam
 */
void bot_2_bot_handle_remotecall(void) {
	bot_remotecall_from_command(remotecall_buffer);
}

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
		remote_call_data_t par2, remote_call_data_t par3) {

	/* Funktionsnamen auf Gueltigkeit / Laenge pruefen */
	uint8_t len = (uint8_t) strlen(function);
	if (len == 0 || len > REMOTE_CALL_FUNCTION_NAME_LEN) {
		return -1;
	}

	LOG_DEBUG("Starte RemoteCall auf Bot mit Adresse 0x%02x (%u)", bot_addr, bot_addr);

	/* alle Parameter in den Puffer kopieren */
	remote_call_data_t * pPar = (void *)&remotecall_buffer[len + 1];
	*pPar = par1;
	LOG_DEBUG("Parameter 1: 0x%x", pPar->u32);
	pPar++;
	*pPar = par2;
	LOG_DEBUG("Parameter 2: 0x%x", pPar->u32);
	pPar++;
	*pPar = par3;
	LOG_DEBUG("Parameter 3: 0x%x", pPar->u32);

	/* Funktionsnamen in den Puffer kopieren */
	strcpy(remotecall_buffer, function);
	LOG_DEBUG("Funktionsname: \"%s\"", remotecall_buffer);

	/* Payloaduebertragung starten */
	return bot_2_bot_send_payload_request(bot_addr, BOT_2_BOT_REMOTECALL, remotecall_buffer, REMOTE_CALL_BUFFER_SIZE);
}
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE
#endif // BOT_2_BOT_PAYLOAD_AVAILABLE

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
#endif // LOG_AVAILABLE
#endif // BOT_2_BOT_AVAILABLE
