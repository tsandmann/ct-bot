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
 * @file 	command.c
 * @brief 	Kommando-Management
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.2005
 */

#include "ct-Bot.h"

#include "command.h"
EEPROM uint8_t bot_address = CMD_BROADCAST; /*!< Kommunikations-Adresse des Bots (EEPROM) */

#ifdef COMMAND_AVAILABLE
#include "tcp.h"
#include "led.h"
#include "log.h"
#include "timer.h"
#include "mouse.h"
#include "sensor.h"
#include "rc5.h"
#include "rc5-codes.h"
#include "bot-logic/bot-logic.h"
#include "bot-2-sim.h"
#include "bot-2-bot.h"
#include "os_thread.h"
#include "map.h"
#include "botfs.h"
#include "init.h"
#include <string.h>

#define CHECK_CMD_ADDRESS					/*!< soll die Zieladresse der Kommandos ueberprueft werden? */
#define RCVBUFSIZE (sizeof(command_t) * 2)	/*!< Groesse des Empfangspuffers */
#define COMMAND_TIMEOUT 15					/*!< Anzahl an ms, die maximal auf fehlende Daten gewartet wird */

command_t received_command; /*!< Puffer fuer Kommandos */
static uint8_t count = 1;	/*!< Zaehler fuer Paket-Sequenznummer */
/*! Puffer fuer zu sendendes Kommando */
static command_t cmd_to_send = {
	CMD_STARTCODE,
	{0, 0, 0},
	0, 0, 0, 0, 0, 0,
	CMD_STOPCODE
};

//#define DEBUG_COMMAND       // Schalter, um auf einmal alle Debugs an oder aus zu machen
//#define DEBUG_COMMAND_NOISY // nun wird es voll im Log, da jedes Command geschrieben wird

#ifndef DEBUG_COMMAND
#undef LOG_AVAILABLE
#undef LOG_DEBUG
#define LOG_DEBUG(...) {} /*!< Log-Dummy */
#endif

/*!
 * Initialisiert die (High-Level-)Kommunikation
 */
void command_init(void) {
	/* eigene Adresse checken */
	uint8_t addr = get_bot_address();
	if (addr != CMD_BROADCAST && addr > 127) {
		/* gespeicherte Adresse ist eine vom Sim Vergebene,
		 * schalte auf Adressevergabemodus um */
		addr = CMD_BROADCAST;
		set_bot_address(addr);
	}

	/* Bot beim Sim anmelden */
#ifdef MCU
	command_write(CMD_WELCOME, SUB_WELCOME_REAL, 0, 0, 0);
#else
	command_write(CMD_WELCOME, SUB_WELCOME_SIM, 0, 0, 0);
#endif
}

/*!
 * Liest ein Kommando ein, ist blockierend!
 * Greift auf low_read() zurueck
 * Achtung, die Payload wird nicht mitgelesen!!!
 * @see low_read()
 */
int8_t command_read(void) {
	int8_t bytesRcvd;
	int8_t start = 0; // Start des Kommandos
	int8_t i;
	command_t * command; // Pointer zum Casten der empfangegen Daten
	uint8_t buffer[RCVBUFSIZE]; // Puffer
#ifdef PC
#if BYTE_ORDER == BIG_ENDIAN
	uint16_t store; // Puffer fuer die Endian-Konvertierung
#endif
#endif // PC

	uint16_t old_ticks; // alte Systemzeit
	buffer[0] = 0; // Sicherheitshalber mit sauberem Puffer anfangen

	/* Daten holen, maximal soviele, wie ein Kommando lang ist */
	bytesRcvd = (int8_t) low_read(buffer, sizeof(command_t));

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("%d read", bytesRcvd);
	LOG_DEBUG("%x %x %x", buffer[0], buffer[1], buffer[2]);
#endif
	/* Suche nach dem Beginn des Frames */
	while ((start < bytesRcvd) && (buffer[start] != CMD_STARTCODE)) {
		LOG_DEBUG("falscher Startcode");
		start++;
	}

	/* Wenn keine STARTCODE gefunden ==> Daten verwerfen */
	if (buffer[start] != CMD_STARTCODE) {
		LOG_DEBUG("kein Startcode");
		return -1;
	}

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("Start @%d", start);
#endif

	// haben wir noch genug Platz im Puffer, um das Packet ferig zu lesen?
	if ((RCVBUFSIZE - (uint8_t) (start)) < sizeof(command_t)) {
		LOG_DEBUG("not enough space");
		return -1; // nein? ==> verwerfen
	}

	i = (int8_t) ((int8_t) sizeof(command_t) - (bytesRcvd - start));

	if (i > 0) { // Fehlen noch Daten ?
		LOG_DEBUG("Start @ %d es fehlen %d Bytes ", start, i);
		old_ticks = TIMER_GET_TICKCOUNT_16; // Systemzeit erfassen

		/* So lange Daten lesen, bis das Packet vollstaendig ist, oder der Timeout zuschlaegt */
		while (i > 0) {
			if (timer_ms_passed_16(&old_ticks, COMMAND_TIMEOUT)) {
				/* Timeout ueberschritten */
				LOG_DEBUG("Timeout beim Nachlesen");
				return -1; // ==> Abbruch
			}
			LOG_DEBUG("%d Bytes missing", i);
			i = (int8_t) low_read(buffer + bytesRcvd, (uint8_t) i);
			LOG_DEBUG("%d read", i);
			bytesRcvd = (int8_t) (bytesRcvd + i);
			i = (int8_t) ((int8_t) sizeof(command_t) - (bytesRcvd - start));
		}
	}

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("%d/%d read/start", bytesRcvd, start);
	LOG_DEBUG("%x %x %x", buffer[start], buffer[start+1], buffer[start+2]);
#endif

	command = (command_t *) (buffer + start); // Cast in command_t

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("start: %c ", command->startCode);
	//	command_display(command);
#endif

	// validate (Startcode ist bereits ok, sonst waeren wir nicht hier)
	if (command->CRC == CMD_STOPCODE) {
#ifdef DEBUG_COMMAND_NOISY
		LOG_DEBUG("Command is valid");
#endif

#ifdef CHECK_CMD_ADDRESS
		/* Ist das Paket ueberhaupt fuer uns? */
		if ((command->to != CMD_BROADCAST)
				&& (command->to != get_bot_address())
				&& (command->request.command != CMD_WELCOME)) {
			LOG_DEBUG("Fehler: Paket To= %d statt %u", command->to, get_bot_address());
#ifdef LOG_AVAILABLE
			command_display(command);
#endif
			return -1;
		}
#endif // CHECK_CMD_ADDRESS

		// Transfer
		memcpy(&received_command, buffer + start, sizeof(command_t));
#ifdef PC
#if BYTE_ORDER == BIG_ENDIAN
		/* Umwandeln der 16 bit Werte in Big Endian */
		store = received_command.data_l;
		received_command.data_l = store << 8;
		received_command.data_l |= (store >> 8) & 0xff;

		store = received_command.data_r;
		received_command.data_r = store << 8;
		received_command.data_r |= (store >> 8) & 0xff;
#endif // BYTE_ORDER == BIG_ENDIAN
#endif // PC
		return 0;
	} else { // Command not valid
		LOG_ERROR("Invalid Command:");
		LOG_DEBUG("%x %x %x", command->startCode, command->request.command, command->CRC);
		return -1;
	}
}

/*!
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort. Interne Version, nicht threadsicher!
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param to			Adresse des Empfaengers
 * @param data_l 		Daten fuer den linken Kanal
 * @param data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write_to_internal(uint8_t command, uint8_t subcommand,
		uint8_t to, int16_t data_l, int16_t data_r, uint8_t payload) {
	request_t request;
	request.command = command;

	union {
		uint8_t byte;
		unsigned bits:7;
	} tmp = {subcommand};
	request.subcommand = tmp.bits;

	request.direction = DIR_REQUEST; // Anfrage
	cmd_to_send.request = request;
	cmd_to_send.from = get_bot_address();
	cmd_to_send.to = to;

	cmd_to_send.payload = payload;

	cmd_to_send.data_l = data_l;
	cmd_to_send.data_r = data_r;

	cmd_to_send.seq = count++;

	low_write(&cmd_to_send);
}

/*!
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param to			Adresse des Empfaengers
 * @param data_l 		Daten fuer den linken Kanal
 * @param data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write_to(uint8_t command, uint8_t subcommand, uint8_t to,
		int16_t data_l, int16_t data_r, uint8_t payload) {
	os_enterCS();
	command_write_to_internal(command, subcommand, to, data_l, data_r, payload);
	os_exitCS();
}

/*!
 * Uebertraegt ein Kommando an den ct-Sim und wartet nicht auf eine Antwort
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param data_l 		Daten fuer den linken Kanal
 * @param data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write(uint8_t command, uint8_t subcommand, int16_t data_l,
		int16_t data_r, uint8_t payload) {
	os_enterCS();
	command_write_to_internal(command, subcommand, CMD_SIM_ADDR, data_l,
			data_r, payload);
#ifdef PC
	if (command == CMD_DONE) {
		flushSendBuffer(); // Flushen hier, bevor das Mutex freigegeben wird!
	}
#endif // PC
	os_exitCS();
}

/*!
 * Versendet Daten mit Anhang und wartet nicht auf Antwort
 * @param command 		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param to			Adresse, an die die Daten gesendet werden sollen
 * @param data_l 		Daten fuer den linken Kanal
 * @param data_r		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes im Anhang
 * @param *data 		Datenanhang an das eigentliche Command
 */
void command_write_rawdata_to(uint8_t command, uint8_t subcommand, uint8_t to,
		int16_t data_l, int16_t data_r, uint8_t payload, const void * data) {
	os_enterCS();
	command_write_to_internal(command, subcommand, to, data_l, data_r, payload);
	low_write_data(data, payload);
	os_exitCS();
}

/*!
 * Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * @param command 		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param data_l 		Daten fuer den linken Kanal
 * @param data_r		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes im Anhang
 * @param *data 		Datenanhang an das eigentliche Command
 */
void command_write_rawdata(uint8_t command, uint8_t subcommand,
		int16_t data_l, int16_t data_r, uint8_t payload, const void * data) {
	command_write_rawdata_to(command, subcommand, CMD_SIM_ADDR, data_l, data_r,
			payload, data);
}

/*!
 * Gibt dem Simulator Daten mit String-Anhang und wartet nicht auf Antwort
 * @param command 		Kennung zum Command
 * @param subcommand 	Kennung des Subcommand
 * @param data_l		Daten fuer den linken Kanal
 * @param data_r		Daten fuer den rechten Kanal
 * @param *data 		Datenanhang an das eigentliche Command, null-terminiert
 */
void command_write_data(uint8_t command, uint8_t subcommand, int16_t data_l,
		int16_t data_r, const char * data) {
	size_t len;
	uint8_t payload;

	if (data != NULL) {
		len = strlen(data);
		if (len > MAX_PAYLOAD) {
			payload = MAX_PAYLOAD;
		} else {
			payload = (uint8_t) len;
		}
	} else {
		payload = 0;
	}

	os_enterCS();
	command_write_to_internal(command, subcommand, CMD_SIM_ADDR, data_l,
			data_r, payload);
	low_write_data((uint8_t *) data, payload);
	os_exitCS();
}

/*!
 * Wertet das Kommando im Puffer aus
 * return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int8_t command_evaluate(void) {
	static uint16_t RC5_Last_Toggle = 0xffff;
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
	static botfs_file_descr_t prog_file;
	static uint16_t prog_size = 0;
#endif // BEHAVIOUR_UBASIC_AVAILABLE
	int8_t analyzed = 1;

#ifdef LOG_AVAILABLE
	if (received_command.from != CMD_SIM_ADDR) {
		LOG_DEBUG("Weitergeleitetes Kommando:");
	}
#ifdef DEBUG_COMMAND_NOISY
		command_display(&received_command);
#endif // DEBUG_COMMAND_NOISY
#endif // LOG_AVAILABLE
	/* woher ist das Kommando? */
#ifdef CHECK_CMD_ADDRESS
	if (received_command.from == CMD_SIM_ADDR) {
#endif
		/* Daten vom ct-Sim */
		switch (received_command.request.command) {
#ifdef RC5_AVAILABLE
		case CMD_SENS_RC5:
			rc5_ir_data.ir_data = (uint16_t) received_command.data_l | (RC5_Last_Toggle & RC5_TOGGLE);
			if (received_command.data_l != 0) {
				RC5_Last_Toggle = 0xffff ^ (RC5_Last_Toggle & RC5_TOGGLE);
			}
			break;
#endif // RC5_AVAILABLE

		case CMD_WELCOME:
			/* Bot beim Sim anmelden */
#ifdef MCU
			command_write(CMD_WELCOME, SUB_WELCOME_REAL, 0, 0, 0);
#else
			command_write(CMD_WELCOME, SUB_WELCOME_SIM, 0, 0, 0);
#endif // MCU
			if (get_bot_address() == CMD_BROADCAST) {
				/* Adresse anfordern */
				command_write(CMD_ID, SUB_ID_REQUEST, 0, 0, 0);
			}
#ifdef BOT_2_BOT_AVAILABLE
			/* hello (bot-)world! */
			else {
				command_write_to(BOT_CMD_WELCOME, SUB_CMD_NORM, CMD_BROADCAST, 0, 0, 0);
			}
#endif // BOT_2_BOT_AVAILABLE
			break;
		case CMD_ID:
			if (received_command.request.subcommand == SUB_ID_OFFER) {
#ifdef LOG_AVAILABLE
				LOG_DEBUG("Bekomme eine Adresse angeboten: %u", (uint8_t) received_command.data_l);
#endif // LOG_AVAILABLE
				set_bot_address((uint8_t) received_command.data_l); // Setze Adresse
				command_write(CMD_ID, SUB_ID_SET, received_command.data_l, 0, 0); // Und bestaetige dem Sim das ganze
#ifdef BOT_2_BOT_AVAILABLE
				/* hello (bot-)world! */
				command_write_to(BOT_CMD_WELCOME, SUB_CMD_NORM, CMD_BROADCAST, 0, 0, 0);
#endif // BOT_2_BOT_AVAILABLE
			}
			break;

#ifdef MOUSE_AVAILABLE
		case CMD_SENS_MOUSE_PICTURE: // Sim fragt nach dem Bild
			mouse_transmit_picture();
			break;
#endif // MOUSE_AVAILABLE

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
		case CMD_REMOTE_CALL:
			LOG_DEBUG("RemoteCall-CMD:");
			switch (received_command.request.subcommand) {
			case SUB_REMOTE_CALL_LIST:
				LOG_DEBUG(" Liste");
				bot_remotecall_list();
				break;
			case SUB_REMOTE_CALL_ORDER: {
				LOG_DEBUG("RemoteCall empfangen. Data=%u Bytes", received_command.payload);
				uint8_t buffer[REMOTE_CALL_BUFFER_SIZE];
				uint16_t ticks = TIMER_GET_TICKCOUNT_16;
#ifdef MCU
				while (uart_data_available() < received_command.payload &&
					(uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT));
#endif
				low_read(buffer, received_command.payload);
				if ((uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)) {
					bot_remotecall_from_command((char *) &buffer);
				} else {
					int16_t result = BEHAVIOUR_SUBFAIL;
					command_write(CMD_REMOTE_CALL, SUB_REMOTE_CALL_DONE, result, result, 0);
				}
				break;
			}
			case SUB_REMOTE_CALL_ABORT: {
				LOG_DEBUG("RemoteCalls werden abgebrochen");
				bot_remotecall_cancel();
				break;
			}

			default:
				LOG_DEBUG("unbekanntes Subkommando: %c", received_command.request.subcommand);
				break;
			}
			break;
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE

#ifdef MAP_2_SIM_AVAILABLE
		case CMD_MAP:
			switch (received_command.request.subcommand) {
			case SUB_MAP_REQUEST:
				map_2_sim_send();
				break;
			}
			break;
#endif // MAP_2_SIM_AVAILABLE

		case CMD_SHUTDOWN:
			ctbot_shutdown();
			break;

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
		case CMD_PROGRAM: {
			LOG_DEBUG("Programm-Empfang:");
			switch (received_command.request.subcommand) {
			case SUB_PROGRAM_PREPARE: {
				/* Vorbereitung auf neues Programm */
				const uint8_t type = (uint8_t) received_command.data_l;
				prog_size = (uint16_t) received_command.data_r;
				LOG_DEBUG(" Typ=%u Laenge=%u", type, prog_size);
				const uint8_t len = received_command.payload;
				LOG_DEBUG(" len=%u", len);
				char filename[len + 1];
				uint16_t ticks = TIMER_GET_TICKCOUNT_16;
#ifdef MCU
				while (uart_data_available() < len &&
					(uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT));
#endif
				low_read(filename, len);
				if ((uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)) {
					/* OK */
					filename[len] = 0;
					LOG_DEBUG(" Datei:\"%s\"", filename);
					void * buffer = GET_MMC_BUFFER(ubasic_buffer);
					/* Datei loeschen, falls vorhanden */
					botfs_unlink(filename, buffer);
					/* Datei anlegen */
					const uint16_t size = prog_size / BOTFS_BLOCK_SIZE + (uint16_t) (prog_size % BOTFS_BLOCK_SIZE != 0 ? 1 : 0);
					LOG_DEBUG(" size=%u", size);
					if (botfs_create(filename, size, buffer) != 0 || botfs_open(filename, &prog_file, BOTFS_MODE_W, buffer) != 0) {
						LOG_ERROR("Fehler beim Dateizugriff");
						prog_size = 0;
						break;
					}
					memset(buffer, 0, BOTFS_BLOCK_SIZE);
					/* falls uBasic laeuft, abbrechen */
					Behaviour_t * const beh = get_behaviour(bot_ubasic_behaviour);
					deactivate_called_behaviours(beh);
					deactivate_behaviour(beh);
					/* evtl. hatte uBasic einen RemoteCall gestartet, daher dort aufraeumen */
					activateBehaviour(NULL, bot_remotecall_behaviour);
					/* Datei laden */
					switch (type) {
					case 0:
						/* uBasic */
						bot_ubasic_load_file(filename, &prog_file);
						break;
					}
				} else {
					/* Fehler */
					LOG_ERROR("Timeout beim Programmempfang");
					prog_size = 0;
				}
				break;
			}

			case SUB_PROGRAM_DATA: {
				/* Datenteil eines Programms */
				if (prog_size == 0) {
					LOG_DEBUG(" Datenempfang fehlerhaft");
					break;
				}
				const uint16_t done = (uint16_t) received_command.data_r;
				LOG_DEBUG(" type=%u %u Bytes (%u Bytes insgesamt)", (uint8_t) received_command.data_l, received_command.payload,
					received_command.payload + done);
				void * buffer = GET_MMC_BUFFER(ubasic_buffer);
				const uint16_t index = (uint16_t) done % BOTFS_BLOCK_SIZE;
				buffer += index;
				uint16_t ticks = TIMER_GET_TICKCOUNT_16;
#ifdef MCU
				while (uart_data_available() < received_command.payload &&
					(uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT));
#endif
				const uint8_t n = low_read(buffer, received_command.payload);
				if (n != received_command.payload) {
					LOG_DEBUG(" Datenempfang fehlerhaft");
					prog_size = 0;
					break;
				}
				prog_size -= n;
				LOG_DEBUG(" prog_size=%u", prog_size);
				if ((uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)) {
					/* OK */
//					puts(buffer);
					if (index + n == BOTFS_BLOCK_SIZE || prog_size == 0) {
						/* Puffer in Datei schreiben */
						LOG_DEBUG(" Puffer rausschreiben...");
						if (botfs_write(&prog_file, GET_MMC_BUFFER(ubasic_buffer)) != 0) {
							/* Fehler */
							LOG_ERROR("Fehler beim Dateizugriff");
							prog_size = 0;
							break;
						}
						memset(GET_MMC_BUFFER(ubasic_buffer), 0, BOTFS_BLOCK_SIZE);
						if (prog_size == 0) {
							/* Progamm vollstaendig empfangen */
							botfs_flush_used_blocks(&prog_file, GET_MMC_BUFFER(ubasic_buffer));
							LOG_DEBUG("->fertig");
						}
					}
				} else {
					/* Fehler */
					LOG_ERROR("Timeout beim Programmempfang");
					prog_size = 0;
				}
				break;
			}

			case SUB_PROGRAM_START:
				/* Programm starten */
				switch ((uint8_t) received_command.data_l) {
				case 0:
					/* uBasic */
					bot_ubasic(NULL);
					break;
				}
				break;

			case SUB_PROGRAM_STOP:
				/* Programm abbrechen */
				switch ((uint8_t) received_command.data_l) {
				case 0:
					/* uBasic */
					bot_ubasic_break();
					break;
				}
				break;

			default:
				LOG_DEBUG("unbekanntes Subkommando: %c", received_command.request.subcommand);
				break;
			}
			break;
		}
#endif // BEHAVIOUR_UBASIC_AVAILABLE

#ifdef PC
		/* Einige Kommandos ergeben nur fuer simulierte Bots Sinn */
		case CMD_SENS_IR: {
			(*sensor_update_distance)(&sensDistL, &sensDistLToggle,
					sensDistDataL, received_command.data_l);
			(*sensor_update_distance)(&sensDistR, &sensDistRToggle,
					sensDistDataR, received_command.data_r);
			break;
		}
		case CMD_SENS_ENC:
			sensEncL += received_command.data_l;
			sensEncR += received_command.data_r;
			break;
		case CMD_SENS_BORDER:
			sensBorderL = received_command.data_l;
			sensBorderR = received_command.data_r;
			break;
		case CMD_SENS_LINE:
			sensLineL = received_command.data_l;
			sensLineR = received_command.data_r;
			break;
		case CMD_SENS_LDR:
			sensLDRL = received_command.data_l;
			sensLDRR = received_command.data_r;
			break;
		case CMD_SENS_TRANS:
			sensTrans = (uint8_t) received_command.data_l;
			break;
#ifdef MOUSE_AVAILABLE
		case CMD_SENS_MOUSE:
			sensMouseDX = received_command.data_l;
			sensMouseDY = received_command.data_r;
			break;
#endif // MOUSE_AVAILABLE
		case CMD_SENS_ERROR:
			sensError = (uint8_t) received_command.data_l;
			break;
#ifdef BPS_AVAILABLE
		case CMD_SENS_BPS:
			sensBPS = received_command.data_l;
			break;
#endif // BPS_AVAILABLE
		case CMD_DONE: {
			simultime = received_command.data_l;
			system_time_isr(); // Einmal pro Update-Zyklus aktualisieren wir die Systemzeit
			break;
		}
#endif // PC
		default:
			analyzed = 0; // Command was not analysed yet
			break;
		}
#ifdef CHECK_CMD_ADDRESS
	} else {
#ifdef BOT_2_BOT_AVAILABLE
		/* kein loop-back */
		if (received_command.from != get_bot_address()) {
			/* Kommando kommt von einem anderen Bot */
			if (received_command.request.command >= get_bot2bot_cmds()) {
				/* ungueltig */
				return 0;
			}
			cmd_functions[received_command.request.command](&received_command);
		}
#endif // BOT_2_BOT_AVAILABLE
		analyzed = 1;
	}
#endif // CHECK_CMD_ADDRESS
	return analyzed;
}

#ifdef LOG_AVAILABLE
/*!
 * Gibt ein Kommando auf dem Bildschirm aus
 */
void command_display(command_t * command) {
#ifdef PC
//	printf("START= %d\nDIR= %d CMD= %d SUBCMD= %d\nPayload= %d\nDATA = %d %d\nSeq= %d\nCRC= %d\n",
//		(*command).startCode,
//		(*command).request.direction,
//		(*command).request.command,
//		(*command).request.subcommand,
//		(*command).payload,
//		(*command).data_l,
//		(*command).data_r,
//		(*command).seq,
//		(*command).CRC);
	LOG_DEBUG("CMD: %c\tSub: 0x%x\tData L: %d\tSeq: %d\t From: %d\tTo:%d\tCRC: %d\n",
		(*command).request.command,
		(*command).request.subcommand,
		(*command).data_l,
		(*command).seq,
		(*command).from,
		(*command).to,
		(*command).CRC);
#else // MCU
	LOG_DEBUG("CMD: %c\tSub: %c\tData L: %d\tPay: %d\tSeq: %d\n",
		(*command).request.command,
		(*command).request.subcommand,
		(*command).data_l,
		(*command).payload,
		(*command).seq);
#endif // PC
}
#endif // LOG_AVAILABLE
#endif // COMMAND_AVAILABLE
