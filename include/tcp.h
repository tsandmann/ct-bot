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

/*! @file 	tcp.c
 * @brief 	TCP/IP-Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef TCP_H_
#define TCP_H_


#include "bot-2-sim.h"


#define IP "127.0.0.1"		///<  IP, mit der verbunden werden soll (normalerweise localhost)
//#define IP "10.10.22.242"		
#define PORT 10001			///<  Port, mit dem verbunden werden soll 

extern int tcp_sock;			///< Unser TCP-Socket

/*!
 * Uebertrage Daten per TCP/IP
 * @param data Zeiger auf die Daten
 * @param length Anzahl der Bytes
 * @return Anzahl der uebertragenen Bytes
*/
int tcp_write(char* data, int length);

/*!
 * Lese Daten von TCP/IP-Verbindung.
 * Achtung: blockierend!
 * @param data Zeiger auf die Daten
 * @param length Anzahl der gewuenschten Bytes
 * @return Anzahl der uebertragenen Bytes
*/
int tcp_read(char* data, int length);

/*! 
 * Initialisiere TCP/IP Verbindung 
 */ 
void tcp_init(void);
#endif
