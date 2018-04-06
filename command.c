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
 * \file 	command.c
 * \brief 	Kommando-Management
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#include "ct-Bot.h"

#include "command.h"
EEPROM uint8_t bot_address = CMD_BROADCAST; /**< Kommunikations-Adresse des Bots (EEPROM) */

#ifdef COMMAND_AVAILABLE
#include "tcp.h"
#include "uart.h"
#include "led.h"
#include "log.h"
#include "timer.h"
#include "mouse.h"
#include "sensor.h"
#include "rc5.h"
#include "rc5-codes.h"
#include "bot-logic.h"
#include "bot-2-sim.h"
#include "bot-2-atmega.h"
#include "bot-2-bot.h"
#include "os_thread.h"
#include "map.h"
#include "sdfat_fs.h"
#include "botcontrol.h"
#include "init.h"
#include "motor.h"
#include "display.h"
#include "gui.h"
#include "math_utils.h"
#include "bot-2-linux.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>


//#define CRC_CHECK					/**< Soll die Kommunikation per CRC-Checksumme abgesichert werden? */
#define CHECK_CMD_ADDRESS			/**< soll die Zieladresse der Kommandos ueberprueft werden? */
#define COMMAND_TIMEOUT 		15		/**< Anzahl an ms, die maximal auf fehlende Daten gewartet wird */

#ifdef USB_UART_LINUX
#define BOT_2_RPI_TIMEOUT	30000UL	/**< Timeout fuer ARM-Boards */
#else
#define BOT_2_RPI_TIMEOUT	20000UL	/**< Timeout fuer ARM-Boards */
#endif

/* CRC aktivieren fuer ARM-Boards, Adress-Check deaktivieren */
#ifdef ARM_LINUX_BOARD
#ifndef CRC_CHECK
#define CRC_CHECK
#endif
#endif // ARM_LINUX_BOARD

/* CRC aktivieren fuer ATmega-2-Linux, Adress-Check deaktivieren */
#ifdef BOT_2_RPI_AVAILABLE
#ifndef CRC_CHECK
#define CRC_CHECK
#endif
#undef CHECK_CMD_ADDRESS
#endif // BOT_2_RPI_AVAILABLE

#define RCVBUFSIZE (sizeof(command_t) * 2)	/**< Groesse des Empfangspuffers */

command_t received_command; /**< Puffer fuer Kommandos */
static uint8_t count = 1;	/**< Zaehler fuer Paket-Sequenznummer */
/** Puffer fuer zu sendendes Kommando */
command_t cmd_to_send = {
	CMD_STARTCODE,
	{0, 0, 0},
	0, 0, 0, 0, 0, 0,
	CMD_STOPCODE
};

/** Funktionspointer fuer Kommandoverarbeitung */
cmd_func_t cmd_functions = {
	.write = NULL,
	.read = NULL,
	.crc_check = NULL,
	.crc_calc = NULL,
};


//#define DEBUG_COMMAND       // Schalter, um auf einmal alle Debugs an oder aus zu machen
//#define DEBUG_COMMAND_NOISY // nun wird es voll im Log, da jedes Command geschrieben wird

#ifndef DEBUG_COMMAND
#undef LOG_AVAILABLE
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif

/**
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

#ifdef ARM_LINUX_BOARD
	/* Start-Synchronisation mit ATmega */
	LOG_INFO("Establishing connection to ATmega...");
	fflush(stdout);
	LOG_DEBUG("command_init(): Sending CMD_DONE to ATmega...");
	set_bot_2_atmega();
	command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0);
	LOG_DEBUG("command_init(): Waiting for CMD_DONE from ATmega...");
	const int8_t r = receive_until_frame(CMD_DONE);
	if (r != 0) {
		LOG_ERROR("command_init(): received invalid command: %d", r);
	} else {
		LOG_INFO("Connection with ATmega up.");
	}
#endif // ARM_LINUX_BOARD

#ifdef MCU
	cmd_functions.write = uart_write;
	cmd_functions.read = uart_read;
	cmd_functions.crc_check = uart_check_crc;
	cmd_functions.crc_calc = uart_calc_crc;
#endif // MCU

	LOG_DEBUG("command_init() done.");
}

/**
 * Liest ein Kommando ein, ist blockierend!
 * Greift auf cmd_functions.read() zurueck
 * Achtung, die Payload wird nicht mitgelesen!
 */
int8_t command_read(void) {
	int16_t bytesRcvd;
	int8_t start = 0; // Start des Kommandos
	int8_t i;
	command_t * command; // Pointer zum Casten der empfangegen Daten
	uint8_t buffer[RCVBUFSIZE]; // Puffer
#ifdef PC
#if BYTE_ORDER == BIG_ENDIAN
	uint16_t store; // Puffer fuer die Endian-Konvertierung
#endif
#endif // PC

#ifdef MCU
	uint16_t old_ticks; // alte Systemzeit
#endif
	buffer[0] = 0; // Sicherheitshalber mit sauberem Puffer anfangen

	/* Daten holen, maximal soviele, wie ein Kommando lang ist */
	const uint8_t n = sizeof(command_t);
	bytesRcvd = cmd_functions.read(buffer, n);
	if (bytesRcvd < 0) {
		LOG_ERROR("command_read(): read() returned %d", bytesRcvd);
		return -1;
	}

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("%d read", bytesRcvd);
	LOG_DEBUG("%x %x %x", buffer[0], buffer[1], buffer[2]);
#endif
	/* Suche nach dem Beginn des Frames */
	while ((start < bytesRcvd) && (buffer[start] != CMD_STARTCODE)) {
		LOG_DEBUG("falscher Startcode: 0x%02x (should be 0x%02x), start=%d bytesRcvd=%d", buffer[start], CMD_STARTCODE, start, bytesRcvd);
		start++;
	}

	/* Wenn keine STARTCODE gefunden ==> Daten verwerfen */
	if (buffer[start] != CMD_STARTCODE) {
#ifdef DEBUG_COMMAND_NOISY
		LOG_DEBUG("kein Startcode");
#endif
		return -2;
	}

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("Start @%d", start);
#endif

	// haben wir noch genug Platz im Puffer, um das Packet ferig zu lesen?
	if ((RCVBUFSIZE - (uint8_t) (start)) < n) {
		LOG_DEBUG("not enough space");
		return -3; // nein? ==> verwerfen
	}

	i = (int8_t) ((int8_t) n - (bytesRcvd - start));

	if (i > 0) { // Fehlen noch Daten ?
		LOG_DEBUG("Start @ %d es fehlen %d Bytes ", start, i);
#ifdef MCU
		old_ticks = TIMER_GET_TICKCOUNT_16; // Systemzeit erfassen
#endif

		/* So lange Daten lesen, bis das Packet vollstaendig ist, oder der Timeout zuschlaegt */
		while (i > 0) {
#ifdef MCU
			if (timer_ms_passed_16(&old_ticks, COMMAND_TIMEOUT)) {
				/* Timeout ueberschritten */
				LOG_DEBUG("Timeout beim Nachlesen");
				return -4; // ==> Abbruch
			}
#endif // MCU
			LOG_DEBUG("%d Bytes missing", i);
			i = (int8_t) cmd_functions.read(buffer + bytesRcvd, i);
			LOG_DEBUG("%d read", i);
			bytesRcvd = bytesRcvd + i;
			i = (int8_t) ((int8_t) n - (bytesRcvd - start));
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

#ifdef CRC_CHECK
		if (! cmd_functions.crc_check(command)) {
			LOG_ERROR("CRC ungueltig:");
#ifdef DEBUG_COMMAND
			command_display(command);
#endif
			memcpy(&received_command, command, sizeof(command_t));
			return -20;
		} else {
#ifdef DEBUG_COMMAND_NOISY
			LOG_DEBUG("CRC korrekt");
#endif // DEBUG_COMMAND_NOISY
		}
#endif // CRC_CHECK

#ifdef CHECK_CMD_ADDRESS
		/* Ist das Paket ueberhaupt fuer uns? */
		if ((command->to != CMD_BROADCAST) && (command->to != CMD_IGNORE_ADDR) && (command->to != get_bot_address()) && (command->request.command != CMD_WELCOME)) {
			LOG_DEBUG("Fehler: Paket To= %d statt %u", command->to, get_bot_address());
#ifdef LOG_AVAILABLE
			command_display(command);
#endif
			return -10;
		}
#endif // CHECK_CMD_ADDRESS

		// Transfer
		memcpy(&received_command, command, sizeof(command_t));
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

#ifdef DEBUG_COMMAND_NOISY
		LOG_DEBUG("Command received:");
		command_display(&received_command);
#endif
		return 0;
	} else { // Command not valid
		LOG_ERROR("Invalid Command:");
		command_display(command);
		return -6;
	}
}

/**
 * Schleife, die Kommandos empfaengt und bearbeitet, bis ein Kommando vom Typ frame kommt
 * \param frame	Kommando zum Abbruch
 * \return		Fehlercode
 */
int8_t receive_until_frame(uint8_t frame) {
	int8_t result;
	for (;;) {
		result = command_read();
		if (result != 0) {
			/* Fehler werden in command_read() ausgegeben
			 * -10 kommt, wenn das Paket nicht fuer unsere Adresse war */
			if (result == -20) {
				// CRC falsch
			} else {
				return result;
			}
		} else {
#ifdef DEBUG_COMMAND
			LOG_DEBUG("receive_until_frame(): received valid command:");
			command_display(&received_command);
#endif // DEBUG_COMMAND
			command_evaluate();
		}

		if (received_command.request.command == frame) {
			return result;
		}
	}

	return 0;
}

/**
 * Sendet ein Kommando im Little Endian Format.
 * Diese Funktion setzt vorraus, dass die Symbole BYTE_ORDER und BIG_ENDIAN
 * bzw. LITTLE_ENDIAN definiert wurden. Damit dies auf Linux/Unix
 * funktioniert darf _POSIX_SOURCE nicht definiert werden. Fuer Windows
 * wird dies in der Headerdatei tcp.h erledigt.
 * Getestet wurde dies bisher auf folgenden Systemen:
 *  - MacOSX (PPC, big endian, i386 / x86-64 little endian)
 *  - Linux (hppa, big endian, i386 / x86-64 little endian)
 *  - OpenBSD (i386, little endian)
 *  - Windows 2000 - Vista (i386, little endian mit MinGW)
 * Sollten in command_t weitere Werte mit mehr bzw. weniger als 8 Bit
 * aufgenommen werden muss hier eine entsprechende Anpassung erfolgen.
 *
 * \param *cmd	Zeiger auf das Kommando
 * \return		Anzahl der gesendete Bytes
 */
static int16_t send_cmd(command_t * cmd) {
	if (! cmd_functions.write) {
		return 0;
	}
#if defined PC && BYTE_ORDER == BIG_ENDIAN
	command_t le_cmd;

	/* Kopieren des Kommandos und auf Little Endian wandeln */
	memcpy(&le_cmd, cmd, sizeof(command_t));

	/* Alle 16 Bit Werte in Little Endian wandeln */
	le_cmd.data_l = cmd->data_l << 8;
	le_cmd.data_l |= (cmd->data_l >> 8) & 0xff;
	le_cmd.data_r = cmd->data_r << 8;
	le_cmd.data_r |= (cmd->data_r >> 8) & 0xff;
	le_cmd.seq = cmd->seq << 8;
	le_cmd.seq |= (cmd->seq >> 8) & 0xff;

	return cmd_functions.write(&le_cmd, sizeof(command_t));
#else // LITTLE_ENDIAN
	return cmd_functions.write(cmd, sizeof(command_t));
#endif // BIG_ENDIAN
}

/**
 * Uebertraegt ein KomSDFAT_AVAILABLE
 * mando und wartet nicht auf eine Antwort. Interne Version, nicht threadsicher!
 * \param command		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param to			Adresse des Empfaengers
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r 		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 * \return				Fehlercode, 0 falls alles ok
 */
uint8_t command_write_to_internal(uint8_t command, uint8_t subcommand, uint8_t to, int16_t data_l, int16_t data_r, uint8_t payload) {
	request_t request;
	request.command = command;

	union {
		uint8_t byte;
		unsigned bits:7;
	} tmp = {subcommand};
	request.subcommand = tmp.bits;

	request.direction = DIR_REQUEST; // Anfrage
	cmd_to_send.startCode = CMD_STARTCODE;
	cmd_to_send.CRC = CMD_STOPCODE;
	cmd_to_send.request = request;
	cmd_to_send.from = get_bot_address();
	cmd_to_send.to = to;
	cmd_to_send.payload = payload;
	cmd_to_send.data_l = data_l;
	cmd_to_send.data_r = data_r;
	cmd_to_send.seq = count++;

#ifdef DEBUG_COMMAND
	LOG_DEBUG("Command written:");
	command_display(&cmd_to_send);
#endif

#ifdef CRC_CHECK
	cmd_functions.crc_calc(&cmd_to_send);
#endif // CRC_CHECK

	if (send_cmd(&cmd_to_send) != sizeof(command_t)) {
		return 1;
	}

	return 0;
}

/**
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort
 * \param command		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param to			Adresse des Empfaengers
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r 		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write_to(uint8_t command, uint8_t subcommand, uint8_t to, int16_t data_l, int16_t data_r, uint8_t payload) {
	os_enterCS();
#ifdef ARM_LINUX_BOARD
	cmd_func_t old_func = cmd_functions;
	if (to != CMD_IGNORE_ADDR) {
		set_bot_2_sim();
	}
#endif // ARM_LINUX_BOARD
	command_write_to_internal(command, subcommand, to, data_l, data_r, payload);
#ifdef PC
	if (command == CMD_BOT_2_BOT) {
		flushSendBuffer();
	}
#endif // PC
#ifdef ARM_LINUX_BOARD
	cmd_functions = old_func;
#endif // ARM_LINUX_BOARD
	os_exitCS();
}

/**
 * Uebertraegt ein Kommando an den ct-Sim und wartet nicht auf eine Antwort
 * \param command		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r 		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write(uint8_t command, uint8_t subcommand, int16_t data_l, int16_t data_r, uint8_t payload) {
	os_enterCS();
#ifdef ARM_LINUX_BOARD
	cmd_func_t old_func = cmd_functions;
	if (command == CMD_MAP || command == CMD_REMOTE_CALL) {
		set_bot_2_sim();
	}
#endif // ARM_LINUX_BOARD
	command_write_to_internal(command, subcommand, CMD_SIM_ADDR, data_l, data_r, payload);
#ifdef ARM_LINUX_BOARD
	cmd_functions = old_func;
#endif // ARM_LINUX_BOARD
#ifdef PC
	if (command == CMD_DONE) {
		flushSendBuffer(); // Flushen hier, bevor das Mutex freigegeben wird!
	}
#endif // PC
	os_exitCS();
}

/**
 * Versendet Daten mit Anhang und wartet nicht auf Antwort
 * \param command 		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param to			Adresse, an die die Daten gesendet werden sollen
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes im Anhang
 * \param *data 		Datenanhang an das eigentliche Command
 */
void command_write_rawdata_to(uint8_t command, uint8_t subcommand, uint8_t to, int16_t data_l, int16_t data_r, uint8_t payload, const void * data) {
	if (! cmd_functions.write) {
		return;
	}
	os_enterCS();
#ifdef ARM_LINUX_BOARD
	cmd_func_t old_func = cmd_functions;
	if (command == CMD_MAP || command == CMD_REMOTE_CALL || command == CMD_LOG || command == CMD_BOT_2_BOT) {
		set_bot_2_sim();
	}
#endif // ARM_LINUX_BOARD
	if (! command_write_to_internal(command, subcommand, to, data_l, data_r, payload)) {
		cmd_functions.write(data, payload);
	}
#ifdef PC
	if (command == CMD_BOT_2_BOT) {
		flushSendBuffer();
	}
#endif // PC
#ifdef ARM_LINUX_BOARD
	if (command == CMD_MAP || command == CMD_REMOTE_CALL) {
		cmd_functions = old_func;
	}
#endif // ARM_LINUX_BOARD
	os_exitCS();
}

/**
 * Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * \param command 		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes im Anhang
 * \param *data 		Datenanhang an das eigentliche Command
 */
void command_write_rawdata(uint8_t command, uint8_t subcommand, int16_t data_l, int16_t data_r, uint8_t payload, const void * data) {
	command_write_rawdata_to(command, subcommand, CMD_SIM_ADDR, data_l, data_r, payload, data);
}

/**
 * Gibt dem Simulator Daten mit String-Anhang und wartet nicht auf Antwort
 * \param command 		Kennung zum Command
 * \param subcommand 	Kennung des Subcommand
 * \param data_l		Daten fuer den linken Kanal
 * \param data_r		Daten fuer den rechten Kanal
 * \param *data 		Datenanhang an das eigentliche Command, null-terminiert
 */
void command_write_data(uint8_t command, uint8_t subcommand, int16_t data_l, int16_t data_r, const char * data) {
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
	command_write_rawdata_to(command, subcommand, CMD_SIM_ADDR, data_l, data_r, payload, data);
}

#ifdef BOT_2_SIM_AVAILABLE
/**
 * Registriert den Bot beim Sim und teilt diesem dabei mit, welche
 * Features aktiviert sind
 */
void register_bot(void) {
	set_bot_2_sim();
	/* aktivierte Komponenten als Integer codieren */
	union {
		struct { // siehe ctSim.model.bots.components.WelcomeReceiver
			unsigned log:1;			// | 1
			unsigned rc5:1;			// | 2
			unsigned abl:1;			// | 4
			unsigned basic:1;		// | 8
			unsigned map:1;			// | 16
			unsigned remotecall:1;	// | 32
		} PACKED_FORCE data;
		int16_t raw;
	} features = {
		{
#ifdef LOG_CTSIM_AVAILABLE
			1,
#else
			0,
#endif
#ifdef RC5_AVAILABLE
			1,
#else
			0,
#endif
#if defined BEHAVIOUR_ABL_AVAILABLE
			1,
#else
			0,
#endif
#if defined BEHAVIOUR_UBASIC_AVAILABLE
			1,
#else
			0,
#endif
#if defined MAP_AVAILABLE && defined MAP_2_SIM_AVAILABLE
			1,
#else
			0,
#endif
#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
			1,
#else
			0,
#endif
		}
	};

	/* Bot beim Sim anmelden */
#if defined MCU || defined ARM_LINUX_BOARD
	command_write(CMD_WELCOME, SUB_WELCOME_REAL, features.raw, 0, 0);
#else
	command_write(CMD_WELCOME, SUB_WELCOME_SIM, features.raw, 0, 0);
#endif // ARM_LINUX_BOARD
}
#endif // BOT_2_SIM_AVAILABLE

/**
 * Wertet das Kommando im Puffer aus
 * \return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int8_t command_evaluate(void) {
#ifdef RC5_AVAILABLE
	static uint16_t RC5_Last_Toggle = 0xffff;
#endif
#if defined BEHAVIOUR_UBASIC_AVAILABLE || defined BEHAVIOUR_ABL_AVAILABLE
#ifdef SDFAT_AVAILABLE
	static pFatFile prog_file;
#endif // SDFAT_AVAILABLE
	static uint16_t prog_size = 0;
#endif // BEHAVIOUR_UBASIC_AVAILABLE || BEHAVIOUR_ABL_AVAILABLE
	int8_t analyzed = 1;

#if defined LOG_AVAILABLE && defined CHECK_CMD_ADDRESS
	if (received_command.from != CMD_SIM_ADDR && received_command.from != CMD_IGNORE_ADDR) {
		LOG_DEBUG("Weitergeleitetes Kommando:");
	}
#ifdef DEBUG_COMMAND_NOISY
		command_display(&received_command);
#endif // DEBUG_COMMAND_NOISY
#endif // LOG_AVAILABLE
	/* woher ist das Kommando? */
#ifdef CHECK_CMD_ADDRESS
	if (received_command.from == CMD_SIM_ADDR || received_command.from == CMD_IGNORE_ADDR) {
#endif
		/* Daten vom ct-Sim */
		switch (received_command.request.command) {
#ifdef RC5_AVAILABLE
		case CMD_SENS_RC5:
			rc5_ir_data.ir_data = (uint16_t) received_command.data_l
#ifndef ARM_LINUX_BOARD
				| (RC5_Last_Toggle & RC5_TOGGLE)
#endif
				;
			if (received_command.data_l != 0) {
				RC5_Last_Toggle = 0xffff ^ (RC5_Last_Toggle & RC5_TOGGLE);
			}
			break;
#endif // RC5_AVAILABLE

		case CMD_WELCOME:
#ifdef BOT_2_SIM_AVAILABLE
			/* Bot beim Sim anmelden */
			register_bot();

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
#endif // BOT_2_SIM_AVAILABLE
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

#if defined MOUSE_AVAILABLE && defined BOT_2_SIM_AVAILABLE
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
				while (uart_data_available() < received_command.payload && (uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT));
#endif
				cmd_functions.read(buffer, received_command.payload);
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

#if defined BEHAVIOUR_UBASIC_AVAILABLE || defined BEHAVIOUR_ABL_AVAILABLE
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
				cmd_functions.read(filename, len);
				if ((uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)) {
					/* OK */
					filename[len] = 0;
					LOG_DEBUG(" Datei:\"%s\"", filename);
					void* buffer = type == 0 ? GET_MMC_BUFFER(ubasic_buffer) : GET_MMC_BUFFER(abl_buffer);
#ifdef SDFAT_AVAILABLE
					/* Datei anlegen */
					LOG_DEBUG(" prog_size=%u", prog_size);
					if (sdfat_open(filename, &prog_file, SDFAT_O_RDWR | SDFAT_O_TRUNC | SDFAT_O_CREAT)) {
						LOG_ERROR("Fehler beim Dateizugriff");
						prog_size = 0;
						break;
					}
#endif // SDFAT_AVAILABLE
					memset(buffer, 0, SD_BLOCK_SIZE);
					/* falls uBasic / ABL laeuft, abbrechen */
#if defined BEHAVIOUR_UBASIC_AVAILABLE && defined BEHAVIOUR_ABL_AVAILABLE
					Behaviour_t* const beh = type == 0 ? get_behaviour(bot_ubasic_behaviour) : get_behaviour(bot_abl_behaviour);
#elif defined BEHAVIOUR_UBASIC_AVAILABLE
					Behaviour_t* const beh = type == 0 ? get_behaviour(bot_ubasic_behaviour) : NULL;
#elif defined BEHAVIOUR_ABL_AVAILABLE
					Behaviour_t* const beh = type == 0 ? NULL : get_behaviour(bot_abl_behaviour);
#endif
					deactivate_called_behaviours(beh);
					deactivate_behaviour(beh);
					/* evtl. hatte uBasic / ABL einen RemoteCall gestartet, daher dort aufraeumen */
					activateBehaviour(NULL, bot_remotecall_behaviour);
					/* Datei laden */
					switch (type) {
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
					case 0:
						/* uBasic */
						bot_ubasic_load_file(filename, &prog_file);
						break;
#endif // BEHAVIOUR_UBASIC_AVAILABLE
#ifdef BEHAVIOUR_ABL_AVAILABLE
					case 1:
						/* ABL */
						abl_load(filename);
						break;
#endif // BEHAVIOUR_ABL_AVAILABLE
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
				const uint8_t type = (uint8_t) received_command.data_l;
				LOG_DEBUG(" type=%u %u Bytes (%u Bytes insgesamt)", type, received_command.payload, received_command.payload + done);
				void* buffer = type == 0 ? GET_MMC_BUFFER(ubasic_buffer) : GET_MMC_BUFFER(abl_buffer);
				const uint16_t index = (uint16_t) done % SD_BLOCK_SIZE;
				buffer += index;
				uint16_t ticks = TIMER_GET_TICKCOUNT_16;
#ifdef MCU
				while (uart_data_available() < received_command.payload && (uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT));
#endif
				const int16_t n = cmd_functions.read(buffer, received_command.payload);
				if (n != received_command.payload) {
					LOG_DEBUG(" Datenempfang fehlerhaft");
					prog_size = 0;
					break;
				}
				prog_size -= (uint16_t) n;
				LOG_DEBUG(" prog_size=%u", prog_size);
				if ((uint16_t) (TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)) {
					/* OK */
//					puts(buffer);
					if (index + (uint16_t) n == SD_BLOCK_SIZE || prog_size == 0) {
						/* Puffer in Datei schreiben */
						LOG_DEBUG(" Puffer rausschreiben...");
#ifdef SDFAT_AVAILABLE
						if (sdfat_write(prog_file, type == 0 ? GET_MMC_BUFFER(ubasic_buffer) : GET_MMC_BUFFER(abl_buffer), SD_BLOCK_SIZE) != SD_BLOCK_SIZE) {
							/* Fehler */
							LOG_ERROR("Fehler beim Dateizugriff");
							prog_size = 0;
							break;
						}
#else // EEPROM
						const uint16_t block = (uint16_t) done / SD_BLOCK_SIZE;
#if defined __AVR_ATmega1284P__ || defined PC
						if (block > 6) {
#elif defined MCU_ATMEGA644X
						if (block > 2) {
#else // ATmega32
						if (block > 0) {
#endif // MCU-Typ
							break;
						}
#ifdef LED_AVAILABLE
						LED_on(LED_ROT);
#endif
						ctbot_eeprom_write_block(&abl_eeprom_data[block << 9], GET_MMC_BUFFER(abl_buffer), 512);
#ifdef LED_AVAILABLE
						LED_off(LED_ROT);
#endif
#endif // SDFAT_AVAILABLE
						memset(type == 0 ? GET_MMC_BUFFER(ubasic_buffer) : GET_MMC_BUFFER(abl_buffer), 0, SD_BLOCK_SIZE);
						if (prog_size == 0) {
							/* Progamm vollstaendig empfangen */
							sdfat_flush(prog_file);
							if (type == 1) { // ABL
								sdfat_close(prog_file);
							}
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
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
				case 0:
					/* uBasic */
					bot_ubasic(NULL);
					break;
#endif // BEHAVIOUR_UBASIC_AVAILABLE
#ifdef BEHAVIOUR_ABL_AVAILABLE
				case 1:
					/* ABL */
					bot_abl(NULL, NULL);
					break;
#endif // BEHAVIOUR_ABL_AVAILABLE
				}
				break;

			case SUB_PROGRAM_STOP:
				/* Programm abbrechen */
				switch ((uint8_t) received_command.data_l) {
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
				case 0:
					/* uBasic */
					bot_ubasic_break();
					break;
#endif // BEHAVIOUR_UBASIC_AVAILABLE
#ifdef BEHAVIOUR_ABL_AVAILABLE
				case 1: {
					/* ABL */
					abl_cancel();
					break;
				}
#endif // BEHAVIOUR_ABL_AVAILABLE
				}
				break;

			default:
				LOG_DEBUG("unbekanntes Subkommando: %c", received_command.request.subcommand);
				break;
			}
			break;
		}
#endif // BEHAVIOUR_UBASIC_AVAILABLE || BEHAVIOUR_ABL_AVAILABLE

#ifdef PC
		/* Einige Kommandos ergeben nur fuer simulierte Bots Sinn */
		case CMD_SENS_IR: {
			(*sensor_update_distance)(&sensDistL, &sensDistLToggle, sensDistDataL, received_command.data_l);
			(*sensor_update_distance)(&sensDistR, &sensDistRToggle,	sensDistDataR, received_command.data_r);
			break;
		}
		case CMD_SENS_ENC:
#ifdef ARM_LINUX_BOARD
			sensEncL = received_command.data_l;
			sensEncR = received_command.data_r;
#else
			sensEncL += received_command.data_l;
			sensEncR += received_command.data_r;
#endif // ARM_LINUX_BOARD
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
		case CMD_SENS_DOOR:
			sensDoor = (uint8_t) received_command.data_l;
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
#ifdef ARM_LINUX_BOARD
			static uint32_t last = 0;
			static uint32_t sum = 0;
			static uint16_t cnt = 0;
			union {
				int16_t s16;
				uint16_t u16;
			} low;
			low.s16 = received_command.data_l;
			union {
				int16_t s16;
				uint16_t u16;
			} high;
			high.s16 = received_command.data_r;
			const uint32_t tick = (uint32_t) low.u16 | ((uint32_t) high.u16 << 16);
			LOG_DEBUG("time_diff=%zu us", (uint32_t) (tick - last) * 176);
			const uint32_t diff = (tick - last) * 176;
			if (diff > BOT_2_RPI_TIMEOUT && last != 0) {
				LOG_ERROR(" diff=%zu", diff);
				LOG_DEBUG(" tick=%zu\tlast=%zu", tick, last);
				LOG_DEBUG(" received_command.data_l=%d\treceived_command.data_r=%d", received_command.data_l, received_command.data_r);
			}
			last = tick;
			tickCount = tick;
			sum += diff;
			cnt++;
			if (cnt == 1024) {
				LOG_INFO("time_diff avg=%zu us", sum / cnt);
				fflush(stdout);
				cnt = 0;
				sum = 0;
			}
#else
			simultime = received_command.data_l;
			system_time_isr(); // Einmal pro Update-Zyklus aktualisieren wir die Systemzeit
#endif // ARM_LINUX_BOARD
			break;
		}
#endif // PC

		case CMD_AKT_LCD: {
#if defined DISPLAY_MCU_AVAILABLE || defined ARM_LINUX_BOARD
			void (* display_func)(void) = NULL;
#ifdef BOT_2_RPI_AVAILABLE
			display_func = linux_display;
#elif defined ARM_LINUX_BOARD
			display_func = atmega_display;
#endif
			switch (received_command.request.subcommand) {
			case SUB_LCD_CLEAR:
				if (screen_functions[display_screen] == display_func) {
					display_clear();
					LOG_DEBUG("SUB_LCD_CLEAR: display_clear() ATmega");
				}
				break;
			case SUB_LCD_CURSOR:
				if (screen_functions[display_screen] == display_func) {
					display_cursor((uint8_t) (received_command.data_r + 1), (uint8_t) (received_command.data_l + 1));
					LOG_DEBUG("SUB_LCD_CURSOR: display_cursor(%d, %d) ATmega", received_command.data_r + 1, received_command.data_l + 1);
				}
				break;
			case SUB_LCD_DATA: {
				if (screen_functions[display_screen] == display_func) {
					display_cursor((uint8_t) (received_command.data_r + 1), (uint8_t) (received_command.data_l + 1));
					LOG_DEBUG("SUB_LCD_DATA: display_cursor(%d, %d) ATmega", received_command.data_r + 1, received_command.data_l + 1);
					LOG_DEBUG("SUB_LCD_DATA: payload=%u", received_command.payload);
				}
#ifdef MCU
				uint16_t ticks = TIMER_GET_TICKCOUNT_16;
#endif
#if defined ARM_LINUX_BOARD && defined DEBUG_COMMAND
				char debug_buf[21];
				memset(debug_buf, 0, sizeof(debug_buf));
				struct timeval start, now;
				gettimeofday(&start, NULL);
#endif // ARM_LINUX_BOARD && DEBUG_COMMAND
				uint8_t i;
				for (i = 0; i < received_command.payload; ++i) {
#ifdef MCU
					while (uart_data_available() < 1 && (uint16_t)(TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)) {}
#endif
					uint8_t n;
					char buffer;
					if ((n = (uint8_t) (cmd_functions.read(&buffer, 1))) != 1) {
						LOG_ERROR("SUB_LCD_DATA: error while receiving display data, n=%d i=%u", n, i);
#ifdef MCU
						LOG_ERROR("uart_data_available()=%d", uart_data_available());
#endif
						i = 0;
						break;
					}
					if (i < 20 && screen_functions[display_screen] == display_func) {
						display_data(buffer);
//						LOG_DEBUG("SUB_LCD_DATA: i=%u buffer='%c'", i, buffer);
#if defined ARM_LINUX_BOARD && defined DEBUG_COMMAND
						debug_buf[i] = buffer;
#endif
					}
				}
#if defined ARM_LINUX_BOARD && defined DEBUG_COMMAND
				gettimeofday(&now, NULL);
				const uint64_t t = (now.tv_sec - start.tv_sec) * 1000000UL + now.tv_usec - start.tv_usec;
				LOG_DEBUG("command_evaluate(): SUB_LCD_DATA: receive took %llu us", t);
				if (i) {
					LOG_DEBUG("command_evaluate(): SUB_LCD_DATA: display_data() %u bytes from ATmega", i);
					LOG_DEBUG(" \"%s\"", debug_buf);
				}
#endif // ARM_LINUX_BOARD && DEBUG_COMMAND
				break;
			}
			} // switch subcommand
#endif // DISPLAY_MCU_AVAILABLE || ARM_LINUX_BOARD
			break;
		}

#ifdef ARM_LINUX_BOARD
		case CMD_LOG: {
			char log_buf[received_command.payload + 1];
			log_buf[received_command.payload] = 0;
			uint8_t i, n;
			for (i = 0; i < received_command.payload; ++i) {
				if ((n = cmd_functions.read(&log_buf[i], 1)) != 1) {
					LOG_ERROR("command_evaluate(): CMD_LOG: error while receiving log data, n=%d i=%u", n, i);
					LOG_ERROR(" uart_data_available()=%d", uart_data_available());
					break;
				}
			}
			LOG_RAW("MCU - %s", log_buf);
			break;
		}
#endif // ARM_LINUX_BOARD

#ifdef BOT_2_RPI_AVAILABLE
		case CMD_AKT_MOT:
			motor_set(received_command.data_l, received_command.data_r);
			break;
		case CMD_AKT_SERVO:
			servo_set(SERVO1, (uint8_t) received_command.data_l);
			servo_set(SERVO2, (uint8_t) received_command.data_r);
			break;
		case CMD_AKT_LED: {
				uint8_t led_ = LED_get() & LED_TUERKIS;
				led_ = (uint8_t) (led_ | (received_command.data_l & ~LED_TUERKIS)); // LED_TUERKIS wird fuer Fehleranzeige auf ATmega-Seite verwendet
				LED_set(led_);
				break;
		}
		case CMD_SETTINGS:
			switch (received_command.request.subcommand) {
			case SUB_SETTINGS_DISTSENS:
				if (received_command.data_l == 0) {
					sensor_update_distance = sensor_dist_straight;
				} else if (received_command.data_l == 1) {
					sensor_update_distance = sensor_dist_lookup;
				}
				break;
			}
			break;
#endif // BOT_2_RPI_AVAILABLE

		default:
			analyzed = 0; // Command was not analysed yet
			break;
		}
#ifdef CHECK_CMD_ADDRESS
	} else { // woher ist das Kommando?
#ifdef BOT_2_BOT_AVAILABLE
		if (received_command.request.command == CMD_BOT_2_BOT) {
			/* kein loop-back */
			if (received_command.from != get_bot_address()) {
				/* Kommando kommt von einem anderen Bot */
				if (received_command.request.subcommand >= get_bot2bot_cmds()) {
					/* ungueltig */
					return 0;
				}
				b2b_cmd_functions[received_command.request.subcommand](&received_command);
			}
		} else {
			return 0;
		}
#endif // BOT_2_BOT_AVAILABLE
		analyzed = 1;
	}
#endif // CHECK_CMD_ADDRESS
	return analyzed;
}

/**
 * Gibt ein Kommando auf dem Bildschirm aus
 * \param *command Zeiger auf das anzuzeigende Kommando
 */
void command_display(command_t * command) {
	(void) command;
#ifdef DEBUG_COMMAND
#ifdef PC
	LOG_DEBUG("Start: %c (0x%x)  CMD: %c (0x%x)  Sub: %c (0x%x)  Direction: %u (0x%x)  Data_L: %d (0x%x)  Data_R: %d (0x%x)  Payload: %u  Seq: %u  From: %d"
			"  To:%d  CRC: %c (0x%x)",
		(char) command->startCode,
		(uint8_t) command->startCode,
		(char) command->request.command,
		(uint8_t) command->request.command,
		(char) command->request.subcommand & 0x7f,
		(uint8_t) command->request.subcommand & 0x7f,
		(uint8_t) command->request.direction & 0x80,
		(uint8_t) command->request.direction & 0x80,
		command->data_l,
		(uint16_t) command->data_l,
		command->data_r,
		(uint16_t) command->data_r,
		command->payload,
		command->seq,
		command->from,
		command->to,
		command->CRC,
		(uint8_t) command->CRC);
#else // MCU
	LOG_DEBUG("CMD: %c\tSub: %c\tData L: %d\tPay: %d\tSeq: %d",
		(*command).request.command,
		(*command).request.subcommand,
		(*command).data_l,
		(*command).payload,
		(*command).seq);
#endif // PC
#ifdef DEBUG_COMMAND_NOISY
	LOG_RAW("raw data:");
	uint8_t * ptr = (uint8_t *) command;
	uint8_t i;
	for (i = 0; i < sizeof(*command) / sizeof(*ptr); ++i) {
		LOG_RAW("0x%02x ", *ptr++);
	}
#endif // DEBUG_COMMAND_NOISY
#endif // DEBUG_COMMAND
}

/**
 * Prueft die CRC Checksumme eines Kommandos
 * \param *cmd Zeiger auf das Kommando
 * \return True oder False
 */
uint8_t uart_check_crc(command_t * cmd) {
	uint16_t crc = CRC_INITIALIZER;
	uint8_t i = 0;
	uint8_t * ptr = (uint8_t *) cmd + offsetof(command_t, request);
	for (; ptr <= (uint8_t *) cmd + offsetof(command_t, seq) ; ++ptr) {
		crc = calc_crc_update(crc, *ptr);
//		LOG_DEBUG("uart_check_crc(): %u: CRC=0x%x", i, crc);
		++i;
	}
	const uint16_t crc_from_cmd = ((uint16_t) cmd->from << 8) | cmd->to;

	cmd->from = CMD_IGNORE_ADDR;
	cmd->to = CMD_IGNORE_ADDR;

	if (crc == crc_from_cmd) {
//		LOG_DEBUG("uart_check_crc(): CRC korrekt: 0x%x", crc);
		return True;
	} else {
		LOG_ERROR("uart_check_crc(): CRC falsch: soll=0x%x, ist=0x%x", crc_from_cmd, crc);
		return False;
	}
}

/**
 * Berechnet die CRC Checksumme eines Kommandos
 * \param *cmd Zeiger auf das Kommando
 */
void uart_calc_crc(command_t * cmd) {
	uint16_t crc = CRC_INITIALIZER;
	uint8_t i = 0;
	uint8_t * ptr = (uint8_t *) cmd + offsetof(command_t, request);
	for (; ptr <= (uint8_t *) cmd + offsetof(command_t, seq) ; ++ptr) {
		crc = calc_crc_update(crc, *ptr);
//		LOG_DEBUG("uart_calc_crc(): %u: CRC=0x%x", i, crc);
		++i;
	}

	cmd->from = (uint8_t) (crc >> 8);
	cmd->to = crc & 0xff;
#if defined PC && 0
	/* CRC Test */
	int r = rand();	// ** int ** da aus <cstdlib>
	if (r > RAND_MAX * 0.999) {
		cmd->to++;
	}
#endif // PC
}
#endif // COMMAND_AVAILABLE
