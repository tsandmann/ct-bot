/*! @file 	tcp-server.h
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
 */
int tcp_server_run (void);
#endif
