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
 * @file 	tcp.h
 * @brief 	TCP/IP-Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#ifndef TCP_H_
#define TCP_H_
#ifdef PC

#include "global.h"
#include "bot-2-sim.h"
#include "command.h"


#define IP "localhost"		/*!<  IP, mit der verbunden werden soll (normalerweise localhost) */	
#define PORT 10001			/*!<  Port, mit dem verbunden werden soll  */

extern int tcp_sock;		/*!< Unser TCP-Socket */
extern char *tcp_hostname;	/*!< Hostname, auf dem ct-Sim laeuft */

/*!
 * Sende Kommando per TCP/IP im Little Endian
 * @param cmd Zeiger auf das Kommando
 * @return Anzahl der gesendete Bytes
 */
int tcp_send_cmd(command_t *cmd);


/*!
 * Uebertrage Daten per TCP/IP
 * @param data Zeiger auf die Daten
 * @param length Anzahl der Bytes
 * @return Anzahl der uebertragenen Bytes
 */
int tcp_write(void* data, int length);

/*!
 * Lese Daten von TCP/IP-Verbindung.
 * Achtung: blockierend!
 * @param data Zeiger auf die Daten
 * @param length Anzahl der gewuenschten Bytes
 * @return Anzahl der uebertragenen Bytes
 */
int tcp_read(void* data, int length);

/*!
 * Oeffnet eine TCP-Verbindung zum Server 
 * @param hostname Symbolischer Name des Host, auf dem ct-Sim laeuft
 * @return Der Socket
 */
int tcp_openConnection(const char *hostname);

/*! 
 * Initialisiere TCP/IP Verbindung 
 */ 
void tcp_init(void);

/*! 
 * Schreibt den Sendepuffer auf einen Schlag raus 
 * @return -1 bei Fehlern, sonst zahl der uebertragenen Bytes
 */
int flushSendBuffer(void);

/*!
 * Schliesst eine TCP-Connection
 * @param sock Der Socket
 */
void tcp_closeConnection(int sock);
#endif	// PC
#endif	/* TCP_H_ */
