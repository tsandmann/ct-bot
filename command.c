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
 * @date 	20.12.05
 */

#include "ct-Bot.h"

#include "led.h"
#include "log.h"

#include "uart.h"
#include "adc.h"
#include "timer.h"
#include "mouse.h"

#include "command.h"
#include "display.h"

#include "sensor.h"
#include "motor.h"
#include "rc5.h"
#include "ir-rc5.h"
#include "rc5-codes.h"
#include "bot-logic/bot-logik.h"
#include "bot-2-pc.h"
#include "uart.h"
#include "delay.h"
#include "bot-2-bot.h"
#include "eeprom.h"
#include "tcp.h"
#include "os_thread.h"

#include <stdio.h>
#include <string.h>

#define COMMAND_TIMEOUT 	10		/*!< Anzahl an ms, die maximal auf fehlende Daten gewartet wird */

EEPROM uint8_t bot_address = CMD_BROADCAST;	/*!< Kommunikations-Adresse des Bots (EEPROM) */

#ifdef COMMAND_AVAILABLE

#define RCVBUFSIZE (sizeof(command_t)*2)   /*!< Groesse des Empfangspuffers */

command_t received_command;	/*!< Puffer fuer Kommandos */
static uint8_t count = 1;	/*!< Zaehler fuer Paket-Sequenznummer */

#ifdef PC
static pthread_mutex_t send_lock = PTHREAD_MUTEX_INITIALIZER;	/*!< Zur Synchronisation von Pufferzugriffen */
#endif

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
	command_write(CMD_WELCOME, SUB_WELCOME_REAL, NULL, NULL, 0);
#else
	command_write(CMD_WELCOME, SUB_WELCOME_SIM, NULL, NULL, 0);
#endif

	if (addr == CMD_BROADCAST) {
		/* Adresse anfordern */
		command_write(CMD_ID, SUB_ID_REQUEST, NULL, NULL, 0);
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
	uint8_t buffer[RCVBUFSIZE]; // Buffer
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
	LOG_DEBUG("%d read",bytesRcvd);
	LOG_DEBUG("%x %x %x",buffer[0],buffer[1],buffer[2]);
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
	LOG_DEBUG("Start @%d",start);
#endif

	// haben wir noch genug Platz im Puffer, um das Packet ferig zu lesen?
	if ((RCVBUFSIZE-start) < sizeof(command_t)) {
		LOG_DEBUG("not enough space");
		return -1; // nein? ==> verwerfen
	}

	i = sizeof(command_t) - (bytesRcvd - start) - 1;

	if (i > 0) { // Fehlen noch Daten ?
		LOG_DEBUG("command.c: Start @ %d es fehlen %d bytes ",start,i);
		// Systemzeit erfassen
		old_ticks = TIMER_GET_TICKCOUNT_16;

		// So lange Daten lesen, bis das Packet vollstaendig ist, oder der Timeout zuschlaegt
		while (i > 0) {
			// Wenn der Timeout ueberschritten ist
			if (timer_ms_passed_16(&old_ticks, COMMAND_TIMEOUT)) {
				LOG_DEBUG("Timeout beim Nachlesen");
				return -1; //	==> Abbruch
			}
			LOG_DEBUG("%d bytes missing",i);
			i = low_read(buffer + bytesRcvd, i);
			LOG_DEBUG("%d read",i);
			bytesRcvd += i;
			i = sizeof(command_t) - (bytesRcvd - start);
		}
	}

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("%d/%d read/start",bytesRcvd,start);
	LOG_DEBUG("%x %x %x",buffer[start],buffer[start+1],buffer[start+2]);
#endif

	// Cast in command_t
	command = (command_t *) (buffer + start);

#ifdef DEBUG_COMMAND_NOISY
	LOG_DEBUG("start: %x ",command->startCode);
	//	command_display(command);
#endif

	// validate (startcode ist bereits ok, sonst waeren wir nicht hier )
	if (command->CRC == CMD_STOPCODE) {
#ifdef DEBUG_COMMAND_NOISY
		LOG_DEBUG("Command is valid");
#endif

		/* Ist das Paket ueberhaupt fuer uns? */
		if ((command->to != CMD_BROADCAST)
				&& (command->to != get_bot_address())
				&& (command->request.command != CMD_WELCOME)) {
			LOG_DEBUG("Fehler: Paket To= %d statt %d",command->to, get_bot_address());
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
static void command_write_to_internal(uint8_t command, uint8_t subcommand, uint8_t to, int16_t * data_l, int16_t * data_r, uint8_t payload) {
	command_t cmd;

	cmd.startCode = CMD_STARTCODE;
	cmd.request.direction = DIR_REQUEST; // Anfrage
	cmd.request.command = command;
	cmd.request.subcommand = subcommand;
	cmd.from = get_bot_address();
	cmd.to = to;

	cmd.payload = payload;
	if (data_l != NULL) {
		cmd.data_l = *data_l;
	} else {
		cmd.data_l = 0;
	}

	if (data_r != NULL) {
    	cmd.data_r = *data_r;
	} else {
		cmd.data_r = 0;
	}

	cmd.seq = count++;
	cmd.CRC = CMD_STOPCODE;

	low_write(&cmd);

	/*
	printf("\nSTART= %d\nDIR= %d CMD= %d SUBCMD= %d\nPayload= %d\nDATA = %d %d\nSeq= %d\nFrom= %d\nTo= %d\nCRC= %d\n",
				cmd.startCode,
				cmd.request.direction,
				cmd.request.command,
				cmd.request.subcommand,
				cmd.payload,
				cmd.data_l,
				cmd.data_r,
				cmd.seq,
				cmd.from,
				cmd.to,
				cmd.CRC);
	 */
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
void command_write_to(uint8_t command, uint8_t subcommand, uint8_t to, int16_t * data_l, int16_t * data_r, uint8_t payload) {
//TODO:	Allgemeine Loesung fuer PC und MCU
#ifdef PC
   	pthread_mutex_lock(&send_lock);
#else
   	os_enterCS();
#endif
	command_write_to_internal(command, subcommand, to, data_l, data_r, payload);
#ifdef PC
	pthread_mutex_unlock(&send_lock);
#else
 	os_exitCS();
#endif
}

/*!
 * Uebertraegt ein Kommando an den ct-Sim und wartet nicht auf eine Antwort
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param data_l 		Daten fuer den linken Kanal
 * @param data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write(uint8_t command, uint8_t subcommand, int16_t * data_l, int16_t * data_r, uint8_t payload) {
//TODO:	Allgemeine Loesung fuer PC und MCU
#ifdef PC
   	pthread_mutex_lock(&send_lock);
#else
   	os_enterCS();
#endif
	command_write_to_internal(command, subcommand, CMD_SIM_ADDR, data_l, data_r, payload);
#ifdef PC
	if (command == CMD_DONE) {
		flushSendBuffer();	// Flushen hier, bevor das Mutex freigegeben wird!
	}
	pthread_mutex_unlock(&send_lock);
#else
	os_exitCS();
#endif
}

/*!
 * Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * @param command 		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param *data_l 		Daten fuer den linken Kanal
 * @param *data_r		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes im Anhang
 * @param *data 		Datenanhang an das eigentliche Command
 */
void command_write_rawdata(uint8_t command, uint8_t subcommand, int16_t * data_l, int16_t * data_r, uint8_t payload, uint8_t * data) {
//TODO:	Allgemeine Loesung fuer PC und MCU
#ifdef PC
	pthread_mutex_lock(&send_lock);
#else
	os_enterCS();
#endif
	command_write_to_internal(command, subcommand, CMD_SIM_ADDR, data_l, data_r, payload);
    low_write_data(data, payload);
#ifdef PC
	pthread_mutex_unlock(&send_lock);
#else
	os_exitCS();
#endif
}


/*!
 * Gibt dem Simulator Daten mit String-Anhang und wartet nicht auf Antwort
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param *data_l		Daten fuer den linken Kanal
 * @param *data_r		Daten fuer den rechten Kanal
 * @param *data			Datenanhang an das eigentliche Command
 */
void command_write_data(uint8_t command, uint8_t subcommand, int16_t * data_l, int16_t * data_r, const char * data) {
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

//TODO:	Allgemeine Loesung fuer PC und MCU
#ifdef PC
   	pthread_mutex_lock(&send_lock);
#else
  	os_enterCS();
#endif
	command_write_to_internal(command, subcommand, CMD_SIM_ADDR, data_l, data_r, payload);
    low_write_data((uint8_t *)data, payload);
#ifdef PC
	pthread_mutex_unlock(&send_lock);
#else
	os_exitCS();
#endif
}

#ifdef MAUS_AVAILABLE
/*!
 * Uebertraegt ein Bild vom Maussensor an den PC
 */
void transmit_mouse_picture(void) {
	int16_t dummy;
	int16_t pixel;
	uint8_t data, i;
	maus_image_prepare();

	for (i=0; i<6; i++) {
		dummy = i * 54 + 1;
		command_write(CMD_SENS_MOUSE_PICTURE, SUB_CMD_NORM, &dummy, &dummy, 54);
		for (pixel=0; pixel<54; pixel++) {
			data = maus_image_read();
			low_write_data((uint8_t *)&data, 1);
#ifdef MCU
#if BAUDRATE > 17777	// Grenzwert: 8 Bit / 450 us = 17778 Baud
//			_delay_loop_2(1800);	// warten, weil Sendezeit < Maussensordelay (450 us)
			_delay_loop_2(3600);	// warten, weil Sendezeit < Maussensordelay (450 us)
#endif
#endif	// MCU
		}
	}
}
#endif	// MAUS_AVAILABLE

/*!
 * Wertet das Kommando im Puffer aus
 * return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int8_t command_evaluate(void) {
	static uint16_t RC5_Last_Toggle = 0xffff;
	uint8_t analyzed = 1;

	#ifdef LOG_AVAILABLE
		if (received_command.from != SIM_ID)
			LOG_DEBUG("Achtung: weitergeleitetes Kommando:");
			#ifdef DEBUG_COMMAND_NOISY
				command_display(&received_command);
			#endif
	#endif	// LOG_AVAILABLE

	/* woher ist das Kommando? */
	if (received_command.from == CMD_SIM_ADDR) {
		/* Daten vom ct-Sim */
		switch (received_command.request.command) {
			#ifdef IR_AVAILABLE
				case CMD_SENS_RC5:
					ir_data=received_command.data_l | (RC5_Last_Toggle & RC5_TOGGLE);
					if (received_command.data_l != 0)
						RC5_Last_Toggle = 0xffff ^ (RC5_Last_Toggle & RC5_TOGGLE);
					break;
			#endif

			// Einige Kommandos ergeben nur fuer reale Bots Sinn
			#ifdef MCU
				case CMD_WELCOME:
					/* mit WELCOME antworten */
					command_init();
					#ifdef BOT_2_BOT_AVAILABLE
						/* hello (bot-)world! */
						if (get_bot_address() != CMD_BROADCAST) {
							command_write_to(BOT_CMD_WELCOME, SUB_CMD_NORM, CMD_BROADCAST, NULL, NULL, 0);
						}
					#endif	// BOT_2_BOT_AVAILABLE
					break;
			#endif

			case CMD_ID:
				if (received_command.request.subcommand == SUB_ID_OFFER) {
					#ifdef LOG_AVAILABLE
						LOG_DEBUG("Bekomme eine Adresse angeboten: %u", (uint8_t)received_command.data_l);
					#endif	// LOG_AVAILABLE
					set_bot_address(received_command.data_l);	// Setze Adresse
					command_write(CMD_ID, SUB_ID_SET, &(received_command.data_l), NULL, 0); // Und bestaetige dem Sim das ganze
					#ifdef BOT_2_BOT_AVAILABLE
						/* hello (bot-)world! */
						command_write_to(BOT_CMD_WELCOME, SUB_CMD_NORM, CMD_BROADCAST, NULL, NULL, 0);
					#endif
				}
				break;

			#ifdef MAUS_AVAILABLE
				case CMD_SENS_MOUSE_PICTURE: 	// PC fragt nach dem Bild
					transmit_mouse_picture();
				break;
			#endif

			#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
				case CMD_REMOTE_CALL:
						LOG_DEBUG("remote-call-cmd ...");
						switch (received_command.request.subcommand) {
							case SUB_REMOTE_CALL_LIST:
								LOG_DEBUG("... auflisten ");
								remote_call_list();
								break;
							case SUB_REMOTE_CALL_ORDER:
							{
								LOG_DEBUG("remote-call-Wunsch empfangen. Data= %d bytes",received_command.payload);
								uint8 buffer[REMOTE_CALL_BUFFER_SIZE];
								uint16 ticks = TIMER_GET_TICKCOUNT_16;
								#ifdef MCU
									while (uart_data_available() < received_command.payload && (uint16)(TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT));
								#endif
								low_read(buffer,received_command.payload);
								if ((uint16)(TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)){
									bot_remotecall_from_command((char *)&buffer);
								} else{
									int16 result = SUBFAIL;
									command_write_data(CMD_REMOTE_CALL,SUB_REMOTE_CALL_DONE,&result,&result,NULL);
								}
								break;
							}
							case SUB_REMOTE_CALL_ABORT: {
								LOG_DEBUG("remote calls werden abgebrochen");
								deactivateCalledBehaviours(bot_remotecall_behaviour);
								break;
							}

							default:
								LOG_DEBUG("unbekanntes Subkommando: %c",received_command.request.subcommand);
								break;
						}
					break;
			#endif	// BEHAVIOUR_REMOTECALL_AVAILABLE


			// Einige Kommandos ergeben nur fuer simulierte Bots Sinn
			#ifdef PC
				case CMD_SENS_IR: {
					(*sensor_update_distance)(&sensDistL, &sensDistLToggle, sensDistDataL, received_command.data_l);
					(*sensor_update_distance)(&sensDistR, &sensDistRToggle, sensDistDataR, received_command.data_r);
					break;
				}
				case CMD_SENS_ENC:
					sensEncL+=received_command.data_l;
					sensEncR+=received_command.data_r;
					break;
				case CMD_SENS_BORDER:
					sensBorderL=received_command.data_l;
					sensBorderR=received_command.data_r;
					break;
				case CMD_SENS_LINE:
					sensLineL=received_command.data_l;
					sensLineR=received_command.data_r;
					break;
				case CMD_SENS_LDR:
					sensLDRL=received_command.data_l;
					sensLDRR=received_command.data_r;
					break;
				case CMD_SENS_TRANS:
					sensTrans=(uint8_t)received_command.data_l;
					break;
				#ifdef MAUS_AVAILABLE
					case CMD_SENS_MOUSE:
						sensMouseDX=received_command.data_l;
						sensMouseDY=received_command.data_r;
						break;
				#endif
				case CMD_SENS_ERROR:
					sensError=(uint8_t)received_command.data_l;
					sensor_update();	/* Error ist der letzte uebertragene Sensorwert, danach koennen wir uns um allgemeine updates kuemmern*/
					led_update();
					break;
				case CMD_DONE:
					simultime = received_command.data_l;
					system_time_isr();	// Einmal pro Update-Zyklus aktualisieren wir die Systemzeit
					break;
			#endif	// PC
			default:
				analyzed=0;		// Command was not analysed yet
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
		#endif
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
//		printf("START= %d\nDIR= %d CMD= %d SUBCMD= %d\nPayload= %d\nDATA = %d %d\nSeq= %d\nCRC= %d\n",
//			(*command).startCode,
//			(*command).request.direction,
//			(*command).request.command,
//			(*command).request.subcommand,
//			(*command).payload,
//			(*command).data_l,
//			(*command).data_r,
//			(*command).seq,
//			(*command).CRC);
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
