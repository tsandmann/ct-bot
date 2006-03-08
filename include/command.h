/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	command.h
 * @brief 	Kommando-Management
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#ifndef __command_h_
#define __command_h_ 

#include "global.h"
#include "ct-Bot.h"

#define MAX_PAYLOAD 255  /*!< Max. Anzahl Bytes, die an ein Command angehaengt werden */


/*!
 * Request Teil eines Kommandos
 */
typedef struct {
	unsigned char command:8;	/*!< command */
	unsigned char subcommand:7;	/*!< subcommand */
	unsigned char direction:1;	/*!< 0 ist Anfrage, 1 ist Antwort */
#ifndef DOXYGEN
	} __attribute__ ((packed)) request_t; // Keine Luecken in der Struktur lassen
#else
	} request_t; // Keine Luecken in der Struktur lassen
#endif

/*!
 * Kommando
 */
typedef struct {
	unsigned char startCode;	/*!< Markiert den Beginn eines Commands */
	request_t request; 			/*!< Command-ID */
	unsigned char  payload;		/*!< Bytes, die dem Kommando noch folgen*/
	int16 data_l;				/*!< Daten zum Kommando links*/
	int16 data_r;				/*!< Daten zum Kommando rechts*/
	int16 seq;					/*!< Paket-Sequenznummer*/
	unsigned char CRC;			/*!< Markiert das Ende des Commands*/
#ifndef DOXYGEN
	} __attribute__ ((packed)) command_t;// Keine Luecken in der Struktur lassen
#else
	} command_t;
#endif

#define CMD_STARTCODE	'>'		/*!< Anfang eines Kommandos*/
#define CMD_STOPCODE	'<'		/*!< Ende eines Kommandos*/

//Sensoren
#define CMD_SENS_IR	'I'			/*!< Abstandssensoren*/
#define CMD_SENS_ENC	'E'		/*!< Radencoder*/
#define CMD_SENS_BORDER 'B'		/*!< Abgrundsensoren*/
#define CMD_SENS_LINE 	'L'		/*!< Liniensensoren*/
#define CMD_SENS_LDR 	'H'		/*!< Helligkeitssensoren */
#define CMD_SENS_TRANS	'T'		/*!< Ueberwachung Transportfach */
#define CMD_SENS_DOOR	'D'		/*!< Ueberwachung Klappe */
#define CMD_SENS_MOUSE	'm'		/*!< Maussensor */
#define CMD_SENS_ERROR 	'e'		/*!< Motor- oder Batteriefehler */
#define CMD_SENS_RC5 	'R'		/*!< IR-Fernbedienung */

// Aktuatoren
#define	CMD_AKT_MOT		'M'		/*!< Motorgeschwindigkeit */
#define	CMD_AKT_DOOR	'd'		/*!< Steuerung Klappe */
#define	CMD_AKT_SERVO	'S'		/*!< Steuerung Servo  */
#define CMD_AKT_LED		'l'		/*!< LEDs steuern */
#define CMD_AKT_LCD     'c'     /*!< LCD Anzeige */

#define SUB_CMD_NORM	'N' 	/*!< Standard-Kommando */
#define SUB_CMD_LEFT	'L' 	/*!< Kommmando fuer links */
#define SUB_CMD_RIGHT	'R' 	/*!< Kommando fuer rechts */

// Subcommandos fuer LCD
#define SUB_LCD_CLEAR   'c'     /*!< Subkommando Clear Screen */
#define SUB_LCD_DATA    'D'     /*!< Subkommando Text ohne Cursor */
#define SUB_LCD_CURSOR  'C'     /*!< Subkommando Cursorkoordinaten */    

// Log-Ausgaben
#define CMD_LOG			'O'		/*!< Logausgaben */

//Kommandos fuer die Verbindung zum c't-Sim
#define CMD_WELCOME		'W'		/*!< Kommado zum anmelden an c't-Sim */
#define SUB_WELCOME_REAL	'R'		/*!< Subkommado zum anmelden eine realen Bots an c't-Sim */
#define SUB_WELCOME_SIM	'S'		/*!< Subkommado zum anmelden eines simulierten Bots an c't-Sim */

#define DIR_REQUEST	0			/*!< Richtung fuer Anfragen */
#define DIR_ANSWER		1			/*!< Richtung fuer Antworten */

#ifdef PC	// Auf dem PC muss der Zugriff auf received_command Thread-sicher sein
	#include <pthread.h>
	extern pthread_mutex_t     command_mutex;	/*!< Zugriff auf das Kommando */
	#define	command_lock()		pthread_mutex_lock(&command_mutex) /*!< Zugriff auf das Kommando */
	#define	command_unlock()	pthread_mutex_unlock(&command_mutex) /*!< Zugriff auf das Kommando */
#endif


extern command_t received_command;		/*!< Puffer fuer Kommandos */

/*!
 * Liest ein Kommando ein, ist blockierend!
 * greift auf low_read() zurueck
 * @see low_read()
 */
int command_read(void);	

/*!
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort
 * @param command Kennung zum Command
 * @param subcommand Kennung des Subcommand
 * @param data_l Daten fuer den linken Kanal
 * @param data_r Daten fuer den rechten Kanal
 */
void command_write(uint8 command, uint8 subcommand, int16* data_l,int16* data_r);

/*!
 *  Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * @param command Kennung zum Command
 * @param subcommand Kennung des Subcommand
 * @param data_l Daten fuer den linken Kanal
 * @param data_r Daten fuer den rechten Kanal
 * @param data Datenanhang an das eigentliche Command
 */
void command_write_data(uint8 command, uint8 subcommand, const int16* data_l, const int16* data_r, const char* data);

/*!
 * Wertet das Kommando im Puffer aus
 * return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int command_evaluate(void);

/*! 
 * Gibt ein Kommando auf dem Bildschirm aus
 */
void command_display(command_t * command);

#endif
