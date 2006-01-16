/*! @file 	tcp.c
 * @brief 	TCP/IP-Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef TCP_H_
#define TCP_H_


#include "bot-2-sim.h"


#define IP "127.0.0.1"		///<  IP to connect with (Normally localhos)
//#define IP "10.10.22.242"		///<  IP to connect with (Normally localhos)
#define PORT 10001			///< Port to connect To

extern int tcp_sock;			///< Unser TCP Socket

/*!
 * Übertrage Daten per tcp/ip
 * @param data Zeiger auf die Daten
 * @param length Anzahl der Bytes
 * @return Anzahl der übertragenen Bytes
*/
int tcp_write(char* data, int length);

/*!
 * Lese Daten von TCP/IP-Verbindung
 * achtung blockierend
 * @param data Zeiger auf die Daten
 * @param length Anzahl der gewünschten Bytes
 * @return Anzahl der übertragenen Bytes
*/
int tcp_read(char* data, int length);

/*! 
 * Initialisiere TCP/IP Verbindung 
 */ 
void tcp_init(void);
#endif
