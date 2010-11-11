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
 * @file 	tcp-server.h
 * @brief 	Demo-TCP-Server
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

/*!
 * Init TCP-Server
 */
void tcp_server_init(void);

/*!
 * Hauptschleife des TCP-Servers
 * @param runs	Anzahl der Durchlaeufe
 */
void tcp_server_run(int runs);

/*!
 * Init TCP-test-Client
 */
void tcp_test_client_init(void);

/*!
 * Hauptschleife des TCP-Test-Clients
 * @param runs	Anzahl der Durchlaeufe, 0 fuer unendlich
 */
void tcp_test_client_run(int runs);
#endif // TCP_SERVER_H_
