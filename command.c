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
#include "command.h"
#include "tcp.h"
#include "led.h"
#include "log.h"
#include "adc.h"
#include "timer.h"
#include "mouse.h"
#include "display.h"
#include "sensor.h"
#include "motor.h"
#include "rc5.h"
#include "ir-rc5.h"
#include "rc5-codes.h"
#include "bot-logic/bot-logik.h"
#include "bot-2-sim.h"
#include "delay.h"
#include "bot-2-bot.h"
#include "os_thread.h"
#include <string.h>
#define COMMAND_TIMEOUT 	10				/*!< Anzahl an ms, die maximal auf fehlende Daten gewartet wird */
EEPROM uint8_t bot_address = CMD_BROADCAST; /*!< Kommunikations-Adresse des Bots (EEPROM) */

#ifdef COMMAND_AVAILABLE

#define RCVBUFSIZE (sizeof(command_t) * 2)   /*!< Groesse des Empfangspuffers */

command_t received_command; /*!< Puffer fuer Kommandos */
static uint8_t count = 1;	/*!< Zaehler fuer Paket-Sequenznummer */
/*! Puffer fuer zu sendendes Kommando */
static command_t cmd_to_send = {
	CMD_STARTCODE,
	{0, 0, 0},
	0, 0, 0, 0, 0, 0,
	CMD_STOPCODE
};

//#define DEBUG_COMMAND		//Schalter, um auf einmal alle Debugs an oder aus zu machen
//#define DEBUG_COMMAND_NOISY	// nun wird es voll im Log, da jedes Command geschrioeben wird

#ifndef DEBUG_COMMAND
#undef LOG_AVAILABLE
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}	/*!< Log-Dummy */
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

	if (addr == CMD_BROADCAST) {
		/* Adresse anfordern */
		command_write(CMD_ID, SUB_ID_REQUEST, 0, 0, 0);
	}
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
	uint16_t store; // Puffer für die Endian-Konvertierung
#endif
#endif

	uint16_t old_ticks; // alte Systemzeit
	buffer[0] = 0; // Sicherheitshalber mit sauberem Puffer anfangen

	// Daten holen, maximal soviele, wie ein Kommando lang ist
	bytesRcvd = low_read(buffer, sizeof(command_t));

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("%d read", bytesRcvd);
	LOG_DEBUG("%x %x %x", buffer[0], buffer[1], buffer[2]);
#endif
	// Suche nach dem Beginn des Frames
	while ((start < bytesRcvd) && (buffer[start] != CMD_STARTCODE)) {
		LOG_DEBUG("falscher Startcode");
		start++;
	}

	// Wenn keine STARTCODE gefunden ==> Daten verwerfen
	if (buffer[start] != CMD_STARTCODE) {
		LOG_DEBUG("kein Startcode");
		return -1;
	}

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("Start @%d", start);
#endif

	// haben wir noch genug Platz im Puffer, um das Packet ferig zu lesen?
	if ((RCVBUFSIZE-start) < sizeof(command_t)) {
		LOG_DEBUG("not enough space");
		return -1; // nein? ==> verwerfen
	}

	i = sizeof(command_t) - (bytesRcvd - start) - 1;

	if (i > 0) { // Fehlen noch Daten ?
		LOG_DEBUG("command.c: Start @ %d es fehlen %d bytes ", start, i);
		// Systemzeit erfassen
		old_ticks = TIMER_GET_TICKCOUNT_16;

		// So lange Daten lesen, bis das Packet vollstaendig ist, oder der Timeout zuschlaegt
		while (i > 0) {
			// Wenn der Timeout ueberschritten ist
			if (timer_ms_passed_16(&old_ticks, COMMAND_TIMEOUT)) {
				LOG_DEBUG("Timeout beim Nachlesen");
				return -1; //	==> Abbruch
			}
			LOG_DEBUG("%d bytes missing", i);
			i = low_read(buffer + bytesRcvd, i);
			LOG_DEBUG("%d read", i);
			bytesRcvd += i;
			i = sizeof(command_t) - (bytesRcvd - start);
		}
	}

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("%d/%d read/start", bytesRcvd, start);
	LOG_DEBUG("%x %x %x", buffer[start], buffer[start+1], buffer[start+2]);
#endif

	// Cast in command_t
	command = (command_t *) (buffer + start);

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("start: %x ", command->startCode);
	//	command_display(command);
#endif

	// validate (Startcode ist bereits ok, sonst waeren wir nicht hier)
	if (command->CRC == CMD_STOPCODE) {
#ifdef DEBUG_COMMAND_NOISY
		LOG_DEBUG("Command is valid");
#endif

		/* Ist das Paket ueberhaupt fuer uns? */
		if ((command->to != CMD_BROADCAST)
				&& (command->to != get_bot_address())
				&& (command->request.command != CMD_WELCOME)) {
			LOG_DEBUG("Fehler: Paket To= %d statt %d", command->to, get_bot_address());
#ifdef LOG_AVAILABLE
			command_display(command);
#endif
			return -1;
		}

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
#endif	// BYTE_ORDER == BIG_ENDIAN
#endif	// PC
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
static void command_write_to_internal(uint8_t command, uint8_t subcommand,
		uint8_t to, int16_t data_l, int16_t data_r, uint8_t payload) {
	request_t request;
	request.command = command;
	request.subcommand = subcommand;
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
#endif	// PC
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
		int16_t data_l, int16_t data_r, uint8_t payload, void * data) {
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
		int16_t data_l, int16_t data_r, uint8_t payload, void * data) {
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
			payload = len;
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
	uint8_t analyzed = 1;

#ifdef LOG_AVAILABLE
	if (received_command.from != SIM_ID) {
		LOG_DEBUG("Achtung: weitergeleitetes Kommando:");
	}
#ifdef DEBUG_COMMAND_NOISY
		command_display(&received_command);
#endif	// DEBUG_COMMAND_NOISY
#endif	// LOG_AVAILABLE
	/* woher ist das Kommando? */
	if (received_command.from == CMD_SIM_ADDR) {
		/* Daten vom ct-Sim */
		switch (received_command.request.command) {
#ifdef IR_AVAILABLE
		case CMD_SENS_RC5:
			ir_data = received_command.data_l | (RC5_Last_Toggle & RC5_TOGGLE);
			if (received_command.data_l != 0)
				RC5_Last_Toggle = 0xffff ^ (RC5_Last_Toggle & RC5_TOGGLE);
			break;
#endif	// IR_AVAILABLE

			// Einige Kommandos ergeben nur fuer reale Bots Sinn
#ifdef MCU
		case CMD_WELCOME:
			/* mit WELCOME antworten */
			command_init();
#ifdef BOT_2_BOT_AVAILABLE
			/* hello (bot-)world! */
			if (get_bot_address() != CMD_BROADCAST) {
				command_write_to(BOT_CMD_WELCOME, SUB_CMD_NORM, CMD_BROADCAST, 0, 0, 0);
			}
#endif	// BOT_2_BOT_AVAILABLE
			break;
#endif	// MCU
		case CMD_ID:
			if (received_command.request.subcommand == SUB_ID_OFFER) {
#ifdef LOG_AVAILABLE
				LOG_DEBUG("Bekomme eine Adresse angeboten: %u", (uint8_t)received_command.data_l);
#endif	// LOG_AVAILABLE
				set_bot_address(received_command.data_l); // Setze Adresse
				command_write(CMD_ID, SUB_ID_SET, received_command.data_l, 0, 0); // Und bestaetige dem Sim das ganze
#ifdef BOT_2_BOT_AVAILABLE
				/* hello (bot-)world! */
				command_write_to(BOT_CMD_WELCOME, SUB_CMD_NORM, CMD_BROADCAST, 0, 0, 0);
#endif	// BOT_2_BOT_AVAILABLE
			}
			break;

#ifdef MOUSE_AVAILABLE
		case CMD_SENS_MOUSE_PICTURE: // PC fragt nach dem Bild
			mouse_transmit_picture();
			break;
#endif	// MOUSE_AVAILABLE

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
		case CMD_REMOTE_CALL:
			LOG_DEBUG("remote-call-cmd ...");
			switch (received_command.request.subcommand) {
			case SUB_REMOTE_CALL_LIST:
				LOG_DEBUG("... auflisten ");
				remote_call_list();
				break;
			case SUB_REMOTE_CALL_ORDER: {
				LOG_DEBUG("remote-call-Wunsch empfangen. Data= %d bytes",received_command.payload);
				uint8_t buffer[REMOTE_CALL_BUFFER_SIZE];
				uint16_t ticks = TIMER_GET_TICKCOUNT_16;
#ifdef MCU
				while (uart_data_available() < received_command.payload && (uint16_t)(TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT));
#endif
				low_read(buffer, received_command.payload);
				if ((uint16_t)(TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)) {
					bot_remotecall_from_command((char *) &buffer);
				} else {
					int16_t result = SUBFAIL;
					command_write(CMD_REMOTE_CALL, SUB_REMOTE_CALL_DONE, result, result, 0);
				}
				break;
			}
			case SUB_REMOTE_CALL_ABORT: {
				LOG_DEBUG("remote calls werden abgebrochen");
				deactivateCalledBehaviours(bot_remotecall_behaviour);
				break;
			}

			default:
				LOG_DEBUG("unbekanntes Subkommando: %c", received_command.request.subcommand);
				break;
			}
			break;
#endif	// BEHAVIOUR_REMOTECALL_AVAILABLE
#ifdef MAP_2_SIM_AVAILABLE
		case CMD_MAP:
			switch (received_command.request.subcommand) {
			case SUB_MAP_REQUEST:
				map_2_sim_send();
				break;
			}
			break;
#endif	// MAP_2_SIM_AVAILABLE
		// Einige Kommandos ergeben nur fuer simulierte Bots Sinn
#ifdef PC
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
#endif	// MOUSE_AVAILABLE
		case CMD_SENS_ERROR:
			sensError = (uint8_t) received_command.data_l;
			break;
		case CMD_DONE:
			simultime = received_command.data_l;
			system_time_isr(); // Einmal pro Update-Zyklus aktualisieren wir die Systemzeit
			break;
#endif	// PC
		default:
			analyzed = 0; // Command was not analysed yet
			break;
		}
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
#endif	// BOT_2_BOT_AVAILABLE
		analyzed = 1;
	}
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
#else	// MCU
	LOG_DEBUG("CMD: %c\tSub: %c\tData L: %d\tPay: %d\tSeq: %d\n",
		(*command).request.command,
		(*command).request.subcommand,
		(*command).data_l,
		(*command).payload,
		(*command).seq);
#endif	// PC
}
#endif	// LOG_AVAILABLE
#endif	// COMMAND_AVAILABLE
