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
 * \file 	command.h
 * \brief 	Kommando-Management
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include "eeprom.h"

#define MAX_PAYLOAD 255  		/**< Max. Anzahl Bytes, die an ein Command angehaengt werden koennen */

#define CMD_STARTCODE	'>'		/**< Anfang eines Kommandos */
#define CMD_STOPCODE	'<'		/**< Ende eines Kommandos */

#define SUB_CMD_NORM	'N' 	/**< Standard-Subkommando */

// Sensoren
#define CMD_SENS_IR	    'I'		/**< Abstandssensoren */
#define CMD_SENS_ENC	'E'		/**< Radencoder */
#define CMD_SENS_BORDER 'B'		/**< Abgrundsensoren */
#define CMD_SENS_LINE 	'L'		/**< Liniensensoren */
#define CMD_SENS_LDR 	'H'		/**< Helligkeitssensoren */
#define CMD_SENS_TRANS	'T'		/**< Ueberwachung Transportfach */
#define CMD_SENS_DOOR	'D'		/**< Ueberwachung Klappe */
#define CMD_SENS_MOUSE	'm'		/**< Maussensor */
#define CMD_SENS_ERROR  'e'		/**< Motor- oder Batteriefehler */
#define CMD_SENS_RC5 	'R'		/**< IR-Fernbedienung */
#define CMD_SENS_BPS	'b'		/**< Bot Positioning System */

#define CMD_SENS_MOUSE_PICTURE	'P'	/**< Bild vom Maussensor in data_l steht, welche Nummer der 1. Pixel hat */

// Aktuatoren
#define CMD_AKT_MOT	    'M'		/**< Motorgeschwindigkeit */
#define CMD_AKT_SERVO	'S'		/**< Steuerung Servo  */
#define CMD_AKT_LED	    'l'		/**< LEDs steuern */
#define CMD_AKT_LCD     'c'     /**< LCD Anzeige */

#define CMD_SETTINGS	's'		/**< Einstellungen uebertragen */
#define SUB_SETTINGS_DISTSENS	'e'	/**< Auswertung der Sensordaten (L: Rohwerte 0 / vorverarbeitet 1) */

#define CMD_DONE        'X'    	/**< Markiert das Ende einer Uebertragung */

#define CMD_ID			'A'		/**< Adressverwaltung */
#define SUB_ID_REQUEST	'R'		/**< Fordere eine Adresse an */
#define SUB_ID_OFFER	'O'		/**< Bot bekommt eine Adresse angeboten */
#define SUB_ID_SET		'S'		/**< Bot Setzt/bestaetigt eine Adresse */

// Subcommandos fuer LCD
#define SUB_LCD_CLEAR   'c'     /**< Subkommando Clear Screen */
#define SUB_LCD_DATA    'D'     /**< Subkommando Text ohne Cursor */
#define SUB_LCD_CURSOR  'C'     /**< Subkommando Cursorkoordinaten */

// Log-Ausgaben
#define CMD_LOG			'O'		/**< Logausgaben */

// Kommandos fuer die Verbindung zum c't-Sim
#define CMD_WELCOME		 'W'	/**< Kommando zum Anmelden an c't-Sim */
#define SUB_WELCOME_REAL 'R'	/**< Subkommando zum Anmelden eines realen Bots an c't-Sim */
#define SUB_WELCOME_SIM	 'S'	/**< Subkommando zum Anmelden eines simulierten Bots an c't-Sim */
#define SUB_WELCOME_BOTS 'B'	/**< Subkommando zu bekanntmachen der eigenen ID bei anderen Bots */

// Kommandos fuer die Remote-Calls
#define CMD_REMOTE_CALL			'r'		/**< Kommando fuer Remote-Calls */
#define SUB_REMOTE_CALL_LIST	'L'		/**< Anforderung an den Bot alle verfuegbaren Kommandos zu listen */
#define SUB_REMOTE_CALL_ENTRY	'E'		/**< Hiermit liefert der Bot ein erfuegbares Kommandos an den PC */
#define SUB_REMOTE_CALL_ORDER	'O'		/**< Hiermit gibt der PC einen Remote-Call in Auftrag */
#define SUB_REMOTE_CALL_DONE	'D'		/**< Hiermit signalisiert der MCU dem PC die Beendigung des Auftrags. Ergebins steht in DataL 0=FAIL 1=SUCCESS */
#define SUB_REMOTE_CALL_ABORT	'A'		/**< Hiermit signalisiert der PC dem MCU die Berarbeitung des laufenden Remote-Calls zu beenden */

// Kommandos fuer Map
#define CMD_MAP				'Q'	/**< Kommando fuer Map */
#define SUB_MAP_DATA_1		'D'	/**< Daten eines Map-Blocks (bzw. die ersten 128 Byte davon, es sind also 4 Kommandos noetig fuer einen kompletten Block) */
#define SUB_MAP_DATA_2		'E'	/**< Map-Daten Teil 2 */
#define SUB_MAP_DATA_3		'F'	/**< Map-Daten Teil 3 */
#define SUB_MAP_DATA_4		'G'	/**< Map-Daten Teil 4 */
#define SUB_MAP_REQUEST		'R'	/**< Aufforderung die komplette Karte (neu) zu uebertragen */
#define SUB_MAP_LINE		'L'	/**< Linie zeichnen */
#define SUB_MAP_CIRCLE		'C'	/**< Kreis zeichnen */
#define SUB_MAP_CLEAR_LINES	'X'	/**< Linien loeschen */
#define SUB_MAP_CLEAR_CIRCLES	'Y' /**< Kreise loeschen */

#define CMD_SHUTDOWN		'q' /**< Kommando zum Herunterfahren */

// Kommandos fuer Programmtransfer
#define CMD_PROGRAM			'p' /**< Programmdaten (Basic oder ABL) */
#define SUB_PROGRAM_PREPARE	'P' /**< Bereitet den Programm-Versand vor */
#define SUB_PROGRAM_DATA	'D' /**< Schickt ein Skript-Programm an den Bot */
#define SUB_PROGRAM_START	'S' /**< Startet ein uebertragenes Programm auf dem Bot */
#define SUB_PROGRAM_STOP	'Q' /**< Bricht ein laufendes Programm ab */

#define CMD_BOT_2_BOT		'C' /**< Bot-2-Bot Kommunikation */


#define DIR_REQUEST	0			/**< Richtung fuer Anfragen */
#define DIR_ANSWER	1			/**< Richtung fuer Antworten */

#define CMD_BROADCAST	0xFF	/**< Broadcast-Adresse, Daten gehen an alle Bots */
#define CMD_SIM_ADDR	0xFE	/**< "Bot"-Adresse des Sim */
#define CMD_IGNORE_ADDR	0xFD	/**< ignoriere Bot-Adresse */

/** Kommunikations-Adresse des Bots (EEPROM) */
extern uint8_t bot_address;

#ifdef COMMAND_AVAILABLE
/** Request Teil eines Kommandos */
typedef struct {
#if (defined PC) && (BYTE_ORDER == BIG_ENDIAN)
	/* Bitfeld im big-endian-Fall umdrehen */
	uint8_t command;		/**< Kommando */
	unsigned direction:1;	/**< 0 ist Anfrage, 1 ist Antwort */
	unsigned subcommand:7;	/**< Subkommando */
#else
	uint8_t command;		/**< Kommando */
	unsigned subcommand:7;	/**< Subkommando */
	unsigned direction:1;	/**< 0 ist Anfrage, 1 ist Antwort */
#endif
} PACKED_FORCE request_t;

/** Kommando */
typedef struct {
	uint8_t startCode;	/**< Markiert den Beginn eines Commands */
	request_t request; 	/**< Command-ID */
	uint8_t payload;	/**< Bytes, die dem Kommando noch folgen */
	int16_t data_l;		/**< Daten zum Kommando link s*/
	int16_t data_r;		/**< Daten zum Kommando rechts */
	uint8_t seq;		/**< Paket-Sequenznummer */
	uint8_t from;		/**< Absender-Adresse */
	uint8_t to;			/**< Empfaenger-Adresse */
	uint8_t CRC;		/**< Markiert das Ende des Commands */
} PACKED_FORCE command_t;

extern command_t received_command; /**< Puffer fuer empfangenes Kommando */
extern command_t cmd_to_send; /**< Puffer fuer zu sendendes Kommando */

typedef int16_t (* read_func_t)(void * data, int16_t length); /**< Funktion zum Lesen von Daten einer Verbindung */
typedef int16_t (* write_func_t)(const void * data, int16_t length); /**< Funktion zum Schreiben von Daten einer Verbindung */
typedef uint8_t (* check_crc_func_t)(command_t * cmd); /**< Funktion zum Ueberpruefen der CRC Checksumme */
typedef void (* calc_crc_func_t)(command_t * cmd); /**< Funktion zum Berechnen der CRC Checksumme */

/** Verbindungsabhaengige Funktionen zur Kommandoverarbeitung */
typedef struct {
	read_func_t read; /**< Daten von der Verbindung lesen */
	write_func_t write; /**< Daten auf die Verbindung schreiben */
	check_crc_func_t crc_check; /**< CRC Checksumme ueberpruefen */
	calc_crc_func_t crc_calc; /**< CRC Checksumme berechnen und ins Kommando schreiben */
} cmd_func_t;

extern cmd_func_t cmd_functions; /**< Funktionspointer fuer Kommandoverarbeitung */


/**
 * Initialisiert die (High-Level-)Kommunikation
 */
void command_init(void);

/**
 * Liest ein Kommando ein, ist blockierend!
 * greift auf cmd_functions.read() zurueck
 */
int8_t command_read(void);

/**
 * Schleife, die Kommandos empfaengt und bearbeitet, bis ein Kommando vom Typ frame kommt
 * \param frame Kommando zum Abbruch
 * \return		Fehlercode
 */
int8_t receive_until_frame(uint8_t frame);

/**
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort. Interne Version, nicht threadsicher!
 * \param command		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param to			Adresse des Empfaengers
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r 		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write_to_internal(uint8_t command, uint8_t subcommand, uint8_t to, int16_t data_l, int16_t data_r, uint8_t payload);

/**
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort
 * \param command		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param to			Adresse des Empfaengers
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r 		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write_to(uint8_t command, uint8_t subcommand, uint8_t to, int16_t data_l, int16_t data_r, uint8_t payload);

/**
 * Uebertraegt ein Kommando an den ct-Sim und wartet nicht auf eine Antwort
 * \param command		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r 		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write(uint8_t command, uint8_t subcommand, int16_t data_l, int16_t data_r, uint8_t payload);

/**
 * Gibt dem Simulator Daten mit String-Anhang und wartet nicht auf Antwort
 * \param command 		Kennung zum Command
 * \param subcommand 	Kennung des Subcommand
 * \param data_l		Daten fuer den linken Kanal
 * \param data_r		Daten fuer den rechten Kanal
 * \param *data 		Datenanhang an das eigentliche Command, null-terminiert
 */
void command_write_data(uint8_t command, uint8_t subcommand, int16_t data_l, int16_t data_r, const char * data);

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
void command_write_rawdata_to(uint8_t command, uint8_t subcommand, uint8_t to, int16_t data_l, int16_t data_r, uint8_t payload, const void * data);

/**
 * Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * \param command 		Kennung zum Command
 * \param subcommand	Kennung des Subcommand
 * \param data_l 		Daten fuer den linken Kanal
 * \param data_r 		Daten fuer den rechten Kanal
 * \param payload 		Anzahl der Bytes im Anhang
 * \param *data 		Datenanhang an das eigentliche Command
 */
void command_write_rawdata(uint8_t command, uint8_t subcommand, int16_t data_l, int16_t data_r, uint8_t payload, const void * data);

/**
 * Wertet das Kommando im Puffer aus
 * \return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int8_t command_evaluate(void);

/**
 * Gibt ein Kommando auf dem Bildschirm aus
 * \param *command Zeiger auf das anzuzeigende Kommando
 */
void command_display(command_t * command);

#ifdef BOT_2_SIM_AVAILABLE
/**
 * Registriert den Bot beim Sim und teilt diesem dabei mit, welche
 * Features aktiviert sind
 */
void register_bot(void);
#endif // BOT_2_SIM_AVAILABLE

/**
 * Gibt die Kommunikations-Adresse des Bots zurueck
 * \return	Bot-Adresse
 */
static inline uint8_t get_bot_address(void) {
	return ctbot_eeprom_read_byte(&bot_address);
}

/**
 * Setzt die Bot-Adresse auf einen neuen Wert und speichert diesen im EEPROM
 * \param bot_addr	neue Bot-Adresse
 */
static inline void set_bot_address(uint8_t bot_addr) {
	ctbot_eeprom_update_byte(&bot_address, bot_addr);
}

/**
 * Prueft die CRC Checksumme eines Kommandos
 * \param *cmd Zeiger auf das Kommando
 * \return True oder False
 */
uint8_t uart_check_crc(command_t * cmd);

/**
 * Berechnet die CRC Checksumme eines Kommandos
 * \param *cmd Zeiger auf das Kommando
 */
void uart_calc_crc(command_t * cmd);

#endif // COMMAND_AVAILABLE
#endif // COMMAND_H_
