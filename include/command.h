/*! @file 	command.h
 * @brief 	kommando Management
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#ifndef __command_h_
#define __command_h_ 

#include "global.h"

/*!
 * Request Teil eines Kommandos
 */
typedef struct {
	unsigned char command:8;	///< command
	unsigned char subcommand:7;	///< subcommand
	unsigned char direction:1;	///< 0 ist anfrage, 1 ist antwort
} __attribute__ ((packed)) request_t; ///< Keine Lücken in der Struktur lassen

/*!
 * Kommando
 */
typedef struct {
	unsigned char startCode;	///< Markiert den Beginn eines Commands
	request_t request; 			///< Command-ID
	unsigned char  payload;		///< Bytes, die dem Kommando noch folgen
	int16 data_l;				///< Daten zum Kommando links
	int16 data_r;				///< Daten zum Kommando rechts
	int16 seq;					///< Packet-Sequenznummer
	unsigned char CRC;			///< Markiert das Ende des Commands
} __attribute__ ((packed)) command_t;

#define CMD_STARTCODE	'>'		///< Anfang eines Kommandos
#define CMD_STOPCODE	'<'		///< Ende eines Kommandos

//Sensoren
#define CMD_SENS_IR	'I'			///< Abstandssensoren
#define CMD_SENS_ENC	'E'		///< Radencoder
#define CMD_SENS_BORDER 'B'		///< Abgrundsensoren
#define CMD_SENS_LINE 	'L'		///< Liniensensoren
#define CMD_SENS_LDR 	'H'		///< Helligkeitssensoren
#define CMD_SENS_TRANS	'T'		///< Überwachung Transportfach
#define CMD_SENS_DOOR	'D'		///< Überwachung Klappe
#define CMD_SENS_MOUSE	'm'		///< Maussensor
#define CMD_SENS_ERROR 	'e'		///< Motor- oder Batteriefehler
#define CMD_SENS_RC5 	'R'		///< IR-Fernbedienung

// Aktuatoren
#define	CMD_AKT_MOT		'M'		///< Motorgeschwindigkeit
#define	CMD_AKT_DOOR	'd'		///< Steuerung Klappe
#define	CMD_AKT_SERVO	'S'		///< Steuerung Servo 
#define CMD_AKT_LED		'l'		///< LEDs steuern

#define SUB_CMD_NORM	'N' 	///< Standard-Kommando
#define SUB_CMD_LEFT	'L' 	///< Kommmando für links
#define SUB_CMD_RIGHT	'R' 	///< Kommando für rechts

#define DIR_REQUEST	0			///< Richtung für Anfragen
#define DIR_ANSWER	1			///< Richtung für Antworten

#ifdef PC	// On PC we have to make access to received_command threadsafe
	#include <pthread.h>
	extern pthread_mutex_t     command_mutex;	///< Zugriff auf das Kommando
	#define	command_lock()		pthread_mutex_lock(&command_mutex) ///< Zugriff auf das Kommando
	#define	command_unlock()	pthread_mutex_unlock(&command_mutex) ///< Zugriff auf das Kommando
#endif


extern command_t received_command;		///< Puffer fÜr Kommandos

/*!
 * Liest ein Kommando ein, ist blockierend!
 * greift auf low_read() zurück
 * @see low_read()
 */
int command_read(void);	

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
