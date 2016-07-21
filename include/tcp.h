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
 * \file 	tcp.h
 * \brief 	TCP/IP-Kommunikation
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#ifndef TCP_H_
#define TCP_H_

#ifdef PC

#include "command.h"

#define IP "localhost"		/**< IP, mit der verbunden werden soll (normalerweise localhost) */
#define PORT 10003			/**< Port, mit dem verbunden werden soll */
#define SERVERPORT 10002	/**< Port fuer TCP-Server */

extern int tcp_sock;		/**< Unser TCP-Socket */
extern char * tcp_hostname;	/**< Hostname, auf dem ct-Sim laeuft */

/**
 * Uebertrage Daten per TCP/IP
 * \param *data		Zeiger auf die Daten
 * \param length	Anzahl der Bytes
 * \return 			Anzahl der gesendeten Byte, -1 wenn Fehler
 */
int16_t tcp_write(const void * data, int16_t length);

/**
 * Lese Daten von TCP/IP-Verbindung.
 * Achtung: blockierend!
 * \param *data		Zeiger auf die Daten
 * \param length	Anzahl der gewuenschten Bytes
 * \return 			Anzahl der uebertragenen Bytes
 */
int16_t tcp_read(void * data, int16_t length);

/**
 * Oeffnet eine TCP-Verbindung zum Server
 * \param *hostname	Symbolischer Name des Host, auf dem ct-Sim laeuft
 * \return 			Der Socket
 */
int tcp_openConnection(const char * hostname);

/**
 * Initialisiere TCP/IP Verbindung
 */
void tcp_init(void);

/**
 * Initialisiere TCP/IP Verbindung als Client (Verbindung zum Sim)
 */
void tcp_init_client(void);

#ifndef __WIN32__
/**
 * Initialisiere TCP/IP Verbindung als Server
 * \param *ptr Datenparameter fuer pthread, wird nicht verwendet
 * \return NULL
 */
void * tcp_init_server(void * ptr);

/**
 * Ermittelt wie viele Bytes auf dem TCP-Server Socket zur Verfuegung stehen
 * \return Bytes verfuegbar
 */
int tcp_data_available(void);
#endif // __WIN32__
/**
 * Gibt an, ob derzeit ein TCP-Client verbunden ist
 * \return True oder False
 */
static inline uint8_t tcp_client_connected(void) {
	return tcp_sock != 0;
}

/**
 * Schreibt den Sendepuffer auf einen Schlag raus
 * \return -1 bei Fehlern, sonst Anzahl der uebertragenen Bytes
 */
int16_t flushSendBuffer(void);

/**
 * Schliesst eine TCP-Connection
 * \param sock Der Socket
 */
void tcp_closeConnection(int sock);

/**
 * Prueft die CRC Checksumme eines Kommandos
 * \param *cmd Zeiger auf das Kommando
 * \return True oder False
 */
uint8_t tcp_check_crc(command_t * cmd);

/**
 * Berechnet die CRC Checksumme eines Kommandos
 * \param *cmd Zeiger auf das Kommando
 */
void tcp_calc_crc(command_t * cmd);
#endif // PC
#endif // TCP_H_
