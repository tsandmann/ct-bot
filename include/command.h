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

#include "global.h"
#include "ct-Bot.h"
#include <stdio.h>

#define MAX_PAYLOAD 255  /*!< Max. Anzahl Bytes, die an ein Command angehaengt werden */

#ifdef PC
	#define low_read tcp_read 	/*!< Which function to use to read data */
	#define low_write tcp_send_cmd /*!< Which function to use to write data */
	#define low_write_data tcp_write /*!< Which function to use to write data */
	#include "eeprom-emu.h"
#endif

#ifdef MCU
	#define low_read uart_read 	/*!< Which function to use to read data */
	#define low_write uart_send_cmd /*!< Which function to use to write data */
	#define low_write_data uart_write /*!< Which function to use to write data */
	#include <avr/eeprom.h>
#endif


/*! Request Teil eines Kommandos */
typedef struct {
#if (defined PC) && (BYTE_ORDER == BIG_ENDIAN)
	/* Bitfeld im big-endian-Fall umdrehen */
	uint8 command:8;	/*!< command */
	uint8 direction:1;	/*!< 0 ist Anfrage, 1 ist Antwort */
	uint8 subcommand:7;	/*!< subcommand */
#else
	uint8 command:8;	/*!< command */
	uint8 subcommand:7;	/*!< subcommand */
	uint8 direction:1;	/*!< 0 ist Anfrage, 1 ist Antwort */	
#endif
#ifndef DOXYGEN
	} __attribute__ ((packed)) request_t; // Keine Luecken in der Struktur lassen
#else
	} request_t; // Keine Luecken in der Struktur lassen
#endif

/*! Kommando */
typedef struct {
	uint8 startCode;	/*!< Markiert den Beginn eines Commands */
	request_t request; 	/*!< Command-ID */
	uint8  payload;		/*!< Bytes, die dem Kommando noch folgen*/
	int16 data_l;		/*!< Daten zum Kommando links*/
	int16 data_r;		/*!< Daten zum Kommando rechts*/
	uint8 seq;			/*!< Paket-Sequenznummer*/
	uint8 from;			/*!< Absender-Adresse */	
	uint8 to;			/*!< Empfaenger-Adresse */
	uint8 CRC;			/*!< Markiert das Ende des Commands*/
#ifndef DOXYGEN
	} __attribute__ ((packed)) command_t;// Keine Luecken in der Struktur lassen
#else
	} command_t;
#endif

#define CMD_STARTCODE	'>'		/*!< Anfang eines Kommandos*/
#define CMD_STOPCODE	'<'		/*!< Ende eines Kommandos*/

//Sensoren
#define CMD_SENS_IR	    'I'		/*!< Abstandssensoren*/
#define CMD_SENS_ENC	'E'		/*!< Radencoder*/
#define CMD_SENS_BORDER 'B'		/*!< Abgrundsensoren*/
#define CMD_SENS_LINE 	'L'		/*!< Liniensensoren*/
#define CMD_SENS_LDR 	'H'		/*!< Helligkeitssensoren */
#define CMD_SENS_TRANS	'T'		/*!< Ueberwachung Transportfach */
#define CMD_SENS_DOOR	'D'		/*!< Ueberwachung Klappe */
#define CMD_SENS_MOUSE	'm'		/*!< Maussensor */
#define CMD_SENS_ERROR  'e'		/*!< Motor- oder Batteriefehler */
#define CMD_SENS_RC5 	'R'		/*!< IR-Fernbedienung */

#define CMD_SENS_MOUSE_PICTURE	'P'		/*!< Bild vom Maussensor in data_l steht, welche Nummer da 1. Pixel hat*/


// Aktuatoren
#define CMD_AKT_MOT	    'M'		/*!< Motorgeschwindigkeit */
#define CMD_AKT_DOOR	'd'		/*!< Steuerung Klappe */
#define CMD_AKT_SERVO	'S'		/*!< Steuerung Servo  */
#define CMD_AKT_LED	    'l'		/*!< LEDs steuern */
#define CMD_AKT_LCD     'c'     /*!< LCD Anzeige */

#define CMD_DONE        'X'    	/*!< Markiert das Ende einer Uebertragung */

#define CMD_ID			'A'		/*!< Adressverwaltung */
#define SUB_ID_REQUEST	'R'		/*!< Fordere eine Adresse an */
#define SUB_ID_OFFER	'O'		/*!< Bot bekommt eine Adresse angeboten */
#define SUB_ID_SET		'S'		/*!< Bot Setzt/bestätigt eine Adresse an */

	
#define SUB_CMD_NORM	'N' 		/*!< Standard-Kommando */
#define SUB_CMD_LEFT	'L' 		/*!< Kommmando fuer links */
#define SUB_CMD_RIGHT	'R' 		/*!< Kommando fuer rechts */


// Subcommandos fuer LCD
#define SUB_LCD_CLEAR   'c'     /*!< Subkommando Clear Screen */
#define SUB_LCD_DATA    'D'     /*!< Subkommando Text ohne Cursor */
#define SUB_LCD_CURSOR  'C'     /*!< Subkommando Cursorkoordinaten */    

// Log-Ausgaben
#define CMD_LOG			'O'		/*!< Logausgaben */

//Kommandos fuer die Verbindung zum c't-Sim
#define CMD_WELCOME		 'W'	/*!< Kommando zum Anmelden an c't-Sim */
#define SUB_WELCOME_REAL 'R'	/*!< Subkommando zum Anmelden eines realen Bots an c't-Sim */
#define SUB_WELCOME_SIM	 'S'	/*!< Subkommando zum Anmelden eines simulierten Bots an c't-Sim */
#define SUB_WELCOME_BOTS 'B'	/*!< Subkommando zu bekanntmachen der eigenen ID bei anderen Bots */	

//Kommandos fuer die Remote-Calls
#define CMD_REMOTE_CALL			'r'		/*!< Kommado fuer Remote-Calls */
#define SUB_REMOTE_CALL_LIST	'L'		/*!< Anforderung an den Bot alle verfuegbaren Kommandos zu listen */
#define SUB_REMOTE_CALL_ENTRY	'E'		/*!< Hiermit leifert der Bot ein erfuegbares Kommandos an den PC */
#define SUB_REMOTE_CALL_ORDER	'O'		/*!< Hiermit gibt der PC einen Remote-call in Auftrag */
#define SUB_REMOTE_CALL_DONE	'D'		/*!< Hiermit signalisiert der MCU dem PC die beendigung des Auftrags. Ergebins steht in DataL 0=FAIL 1=SUCCESS */
#define SUB_REMOTE_CALL_ABORT	'A'		/*!< Hiermit signalisiert der PC dem MCU die berarbeitung des laufenden remote-calls zu beenden */



#define DIR_REQUEST	0			/*!< Richtung fuer Anfragen */
#define DIR_ANSWER		1			/*!< Richtung fuer Antworten */

#define CMD_BROADCAST	0xFF
#define CMD_SIM_ADDR	0xFE
	
extern command_t received_command;		/*!< Puffer fuer Kommandos */

/*!
 * Initialisiert die (High-Level-)Kommunikation
 */
void command_init(void);

/*!
 * Liest ein Kommando ein, ist blockierend!
 * greift auf low_read() zurueck
 * @see low_read()
 */
int8 command_read(void);	

/*!
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param to			Adresse des Empfaengers
 * @param data_l 		Daten fuer den linken Kanal
 * @param data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write_to(uint8_t command, uint8_t subcommand, uint8_t to, int16_t * data_l, int16_t * data_r, uint8_t payload);

/*!
 * Uebertraegt ein Kommando an den ct-Sim und wartet nicht auf eine Antwort
 * @param command		Kennung zum Command
 * @param subcommand	Kennung des Subcommand
 * @param data_l 		Daten fuer den linken Kanal
 * @param data_r 		Daten fuer den rechten Kanal
 * @param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write(uint8_t command, uint8_t subcommand, int16_t * data_l, int16_t * data_r, uint8_t payload);

/*!
 *  Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * @param command Kennung zum Command
 * @param subcommand Kennung des Subcommand
 * @param data_l Daten fuer den linken Kanal
 * @param data_r Daten fuer den rechten Kanal
 * @param data Datenanhang an das eigentliche Command
 */
void command_write_data(uint8_t command, uint8_t subcommand, int16_t* data_l, int16_t* data_r, const char* data);

/*!
 * Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * @param command Kennung zum Command
 * @param subcommand Kennung des Subcommand
 * @param data_l Daten fuer den linken Kanal
 * @param data_r Daten fuer den rechten Kanal
 * @param payload Anzahl der Bytes im Anhang
 * @param data Datenanhang an das eigentliche Command
 */
void command_write_rawdata(uint8_t command, uint8_t subcommand, int16_t* data_l, int16_t* data_r, uint8_t payload, uint8_t* data);

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
	return eeprom_read_byte(&bot_address);
}

/*!
 * Setzt die Bot-Adresse auf einen neuen Wert und speichert diesen im EEPROM
 * @param bot_addr	neue Bot-Adresse
 */
static inline void set_bot_address(uint8_t bot_addr) {
	eeprom_write_byte(&bot_address, bot_addr);
}

#endif	/*__command_h_*/
