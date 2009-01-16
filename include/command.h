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
 * @file 	command.h
 * @brief 	Kommando-Management
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
 */

#ifndef __command_h_
#define __command_h_

#include "ct-Bot.h"
#include "eeprom.h"
#include "uart.h"
#include <stdio.h>

#define MAX_PAYLOAD 255  /*!< Max. Anzahl Bytes, die an ein Command angehaengt werden koennen */

#ifdef PC
#define low_read tcp_read			/*!< Which function to use to read data */
#define low_write tcp_send_cmd		/*!< Which function to use to write data */
#define low_write_data tcp_write	/*!< Which function to use to write data */
#endif	// PC

#ifdef MCU
#define low_read uart_read 			/*!< Which function to use to read data */
#define low_write uart_send_cmd		/*!< Which function to use to write data */
#define low_write_data uart_write	/*!< Which function to use to write data */
#endif	// MCU


/*! Request Teil eines Kommandos */
typedef struct {
#if (defined PC) && (BYTE_ORDER == BIG_ENDIAN)
	/* Bitfeld im big-endian-Fall umdrehen */
	uint8_t command:8;		/*!< Kommando */
	uint8_t direction:1;	/*!< 0 ist Anfrage, 1 ist Antwort */
	uint8_t subcommand:7;	/*!< Subkommando */
#else
	uint8_t command:8;		/*!< Kommando */
	uint8_t subcommand:7;	/*!< Subkommando */
	uint8_t direction:1;	/*!< 0 ist Anfrage, 1 ist Antwort */
#endif
} __attribute__ ((packed)) request_t; // Keine Luecken in der Struktur lassen


/*! Kommando */
typedef struct {
	uint8_t startCode;	/*!< Markiert den Beginn eines Commands */
	request_t request; 	/*!< Command-ID */
	uint8_t  payload;	/*!< Bytes, die dem Kommando noch folgen */
	int16_t data_l;		/*!< Daten zum Kommando link s*/
	int16_t data_r;		/*!< Daten zum Kommando rechts */
	uint8_t seq;		/*!< Paket-Sequenznummer */
	uint8_t from;		/*!< Absender-Adresse */
	uint8_t to;			/*!< Empfaenger-Adresse */
	uint8_t CRC;		/*!< Markiert das Ende des Commands */
} __attribute__ ((packed)) command_t; // Keine Luecken in der Struktur lassen

#define CMD_STARTCODE	'>'		/*!< Anfang eines Kommandos */
#define CMD_STOPCODE	'<'		/*!< Ende eines Kommandos */

#define SUB_CMD_NORM	'N' 	/*!< Standard-Subkommando */

// Sensoren
#define CMD_SENS_IR	    'I'		/*!< Abstandssensoren */
#define CMD_SENS_ENC	'E'		/*!< Radencoder */
#define CMD_SENS_BORDER 'B'		/*!< Abgrundsensoren */
#define CMD_SENS_LINE 	'L'		/*!< Liniensensoren */
#define CMD_SENS_LDR 	'H'		/*!< Helligkeitssensoren */
#define CMD_SENS_TRANS	'T'		/*!< Ueberwachung Transportfach */
#define CMD_SENS_DOOR	'D'		/*!< Ueberwachung Klappe */
#define CMD_SENS_MOUSE	'm'		/*!< Maussensor */
#define CMD_SENS_ERROR  'e'		/*!< Motor- oder Batteriefehler */
#define CMD_SENS_RC5 	'R'		/*!< IR-Fernbedienung */

#define CMD_SENS_MOUSE_PICTURE	'P'		/*!< Bild vom Maussensor in data_l steht, welche Nummer der 1. Pixel hat */

// Aktuatoren
#define CMD_AKT_MOT	    'M'		/*!< Motorgeschwindigkeit */
#define CMD_AKT_SERVO	'S'		/*!< Steuerung Servo  */
#define CMD_AKT_LED	    'l'		/*!< LEDs steuern */
#define CMD_AKT_LCD     'c'     /*!< LCD Anzeige */

#define CMD_DONE        'X'    	/*!< Markiert das Ende einer Uebertragung */

#define CMD_ID			'A'		/*!< Adressverwaltung */
#define SUB_ID_REQUEST	'R'		/*!< Fordere eine Adresse an */
#define SUB_ID_OFFER	'O'		/*!< Bot bekommt eine Adresse angeboten */
#define SUB_ID_SET		'S'		/*!< Bot Setzt/bestaetigt eine Adresse an */

// Subcommandos fuer LCD
#define SUB_LCD_CLEAR   'c'     /*!< Subkommando Clear Screen */
#define SUB_LCD_DATA    'D'     /*!< Subkommando Text ohne Cursor */
#define SUB_LCD_CURSOR  'C'     /*!< Subkommando Cursorkoordinaten */

// Log-Ausgaben
#define CMD_LOG			'O'		/*!< Logausgaben */

// Kommandos fuer die Verbindung zum c't-Sim
#define CMD_WELCOME		 'W'	/*!< Kommando zum Anmelden an c't-Sim */
#define SUB_WELCOME_REAL 'R'	/*!< Subkommando zum Anmelden eines realen Bots an c't-Sim */
#define SUB_WELCOME_SIM	 'S'	/*!< Subkommando zum Anmelden eines simulierten Bots an c't-Sim */
#define SUB_WELCOME_BOTS 'B'	/*!< Subkommando zu bekanntmachen der eigenen ID bei anderen Bots */

// Kommandos fuer die Remote-Calls
#define CMD_REMOTE_CALL			'r'		/*!< Kommado fuer Remote-Calls */
#define SUB_REMOTE_CALL_LIST	'L'		/*!< Anforderung an den Bot alle verfuegbaren Kommandos zu listen */
#define SUB_REMOTE_CALL_ENTRY	'E'		/*!< Hiermit liefert der Bot ein erfuegbares Kommandos an den PC */
#define SUB_REMOTE_CALL_ORDER	'O'		/*!< Hiermit gibt der PC einen Remote-Call in Auftrag */
#define SUB_REMOTE_CALL_DONE	'D'		/*!< Hiermit signalisiert der MCU dem PC die Beendigung des Auftrags. Ergebins steht in DataL 0=FAIL 1=SUCCESS */
#define SUB_REMOTE_CALL_ABORT	'A'		/*!< Hiermit signalisiert der PC dem MCU die Berarbeitung des laufenden Remote-Calls zu beenden */

// Kommandos fuer Map
#define CMD_MAP			'Q'	/*!< Kommando fuer Map */
#define SUB_MAP_DATA_1	'D'	/*!< Daten eines Map-Blocks (bzw. die ersten 128 Byte davon, es sind also 4 Kommandos noetig fuer einen kompletten Block) */
#define SUB_MAP_DATA_2	'E'	/*!< Map-Daten Teil 2 */
#define SUB_MAP_DATA_3	'F'	/*!< Map-Daten Teil 3 */
#define SUB_MAP_DATA_4	'G'	/*!< Map-Daten Teil 4 */
#define SUB_MAP_FLUSH	'U'	/*!< Aufforderung alle ausstehenden Updates zu uebertragen */


#define DIR_REQUEST	0			/*!< Richtung fuer Anfragen */
#define DIR_ANSWER	1			/*!< Richtung fuer Antworten */

#define CMD_BROADCAST	0xFF	/*!< Broadcast-Adresse, Daten gehen an alle Bot */
#define CMD_SIM_ADDR	0xFE	/*!< "Bot"-Adresse des Sim */

extern command_t received_command;	/*!< Puffer fuer Kommandos */

/*!
 * Initialisiert die (High-Level-)Kommunikation
 */
void command_init(void);

/*!
 * Liest ein Kommando ein, ist blockierend!
 * greift auf low_read() zurueck
 * @see low_read()
 */
int8_t command_read(void);

/*!
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param to			Adresse des Empfaengers
 * @param *data_l 		Daten fuer den linken Kanal
 * @param *data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write_to(uint8_t command, uint8_t subcommand, uint8_t to, int16_t * data_l, int16_t * data_r, uint8_t payload);

/*!
 * Uebertraegt ein Kommando an den ct-Sim und wartet nicht auf eine Antwort
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param *data_l 		Daten fuer den linken Kanal
 * @param *data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write(uint8_t command, uint8_t subcommand, int16_t * data_l, int16_t * data_r, uint8_t payload);

/*!
 * Gibt dem Simulator Daten mit String-Anhang und wartet nicht auf Antwort
 * @param command 		Kennung zum Command
 * @param subcommand 	Kennung des Subcommand
 * @param *data_l		Daten fuer den linken Kanal
 * @param *data_r		Daten fuer den rechten Kanal
 * @param *data 		Datenanhang an das eigentliche Command, null-terminiert
 */
void command_write_data(uint8_t command, uint8_t subcommand, int16_t * data_l, int16_t * data_r, const char * data);

/*!
 * Versendet Daten mit Anhang und wartet nicht auf Antwort
 * @param command 		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param to			Adresse, an die die Daten gesendet werden sollen
 * @param *data_l 		Daten fuer den linken Kanal
 * @param *data_r		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes im Anhang
 * @param *data 		Datenanhang an das eigentliche Command
 */
void command_write_rawdata_to(uint8_t command, uint8_t subcommand, uint8_t to, int16_t * data_l, int16_t * data_r, uint8_t payload, uint8_t * data);

/*!
 * Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * @param command 		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param *data_l 		Daten fuer den linken Kanal
 * @param *data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes im Anhang
 * @param *data 			Datenanhang an das eigentliche Command
 */
void command_write_rawdata(uint8_t command, uint8_t subcommand, int16_t * data_l, int16_t * data_r, uint8_t payload, uint8_t * data);

/*!
 * Wertet das Kommando im Puffer aus
 * return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int8_t command_evaluate(void);

/*!
 * Gibt ein Kommando auf dem Bildschirm aus
 */
void command_display(command_t * command);

/*! Kommunikations-Adresse des Bots (EEPROM) */
extern uint8_t bot_address;

/*!
 * Gibt die Kommunikations-Adresse des Bots zurueck
 * @return	Bot-Adresse
 */
static inline uint8_t get_bot_address(void) {
	return ctbot_eeprom_read_byte(&bot_address);
}

/*!
 * Setzt die Bot-Adresse auf einen neuen Wert und speichert diesen im EEPROM
 * @param bot_addr	neue Bot-Adresse
 */
static inline void set_bot_address(uint8_t bot_addr) {
	ctbot_eeprom_write_byte(&bot_address, bot_addr);
}

#endif	/*__command_h_*/
