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

/*! @file 	tcp-server.c
 * @brief 	Demo-TCP-Server
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/
#include "ct-Bot.h"

#ifdef PC

#include "bot-2-sim.h"
#include "tcp.h"
#include "display.h"
#include "command.h"

#include <time.h>
#include <sys/time.h>


/* Linux with glibc:
 *   _REENTRANT to grab thread-safe libraries
 *   _POSIX_SOURCE to get POSIX semantics
 */
#ifndef WIN32
#  define _REENTRANT
//#  define _POSIX_SOURCE
#else
//	#define WIN32
#endif

/* Hack for LinuxThreads */
#ifdef __linux__
#  define _P __P
#endif

#include <pthread.h>

#ifdef WIN32
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

#include <stdio.h>      // for printf() and fprintf()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()

#include "global.h"

int server;                    /*!< Server-Socket */

struct sockaddr_in serverAddr; /*!< Lokale Adresse  */
struct sockaddr_in clientAddr; /*!< Client-Adresse  */
unsigned int clntLen;          /*!< Laenge der Datenstruktur der Client-Adresse */

/*!
 * Init TCP-Server
 */
void tcp_server_init(void){
	#ifdef DISPLAY_AVAILABLE
//		display_init();
	#endif
	
	#ifdef WIN32
	    WSADATA wsaData;
	    WORD wVersionRequested;
	    int err;
		
	    wVersionRequested = MAKEWORD( 2, 0 ); // 2.0 and above version of WinSock
	    err = WSAStartup( wVersionRequested, &wsaData );
	    if ( err != 0 ) {
	        fprintf(stderr, "Couldn't not find a usable WinSock DLL.\n");
	        exit(1); 
	    }
	#endif	
	
	
	// Create socket for incoming connections
	if ((server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		printf("socket() failed\n");
		exit(1);
	}
	
	memset(&serverAddr, 0, sizeof(serverAddr));   // Clean up
	serverAddr.sin_family = AF_INET;              // Internet address family
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
	serverAddr.sin_port = htons(PORT);      // Local port to listen on
	
	// Bind to the local address
	if (bind(server, (struct sockaddr *) &serverAddr, sizeof(serverAddr))	<0	){
		printf("bind() failed\n");
		exit(1);		
	}
	
	// Mark the socket so it will listen for incoming connections 
	if (listen(server, 5) < 0){
		printf("listen() failed\n");
		exit(1);
	}
}

/*!
 * Hauptschleife des TCP-Servers
 */
int tcp_server_run (int runs){
	char buffer[MAX_PAYLOAD];       // Buffer  
	struct timeval    start, stop;
	printf("TCP-Server alive\n");
	
	int seq=0;
//	tcp_server_init();		

//	printf("Initialized\n");
	
	for(;;){
	        /* Set the size of the in-out parameter */
		clntLen = sizeof(clientAddr);
		
		printf("Waiting for client\n");
		// Wait for a client to connect 
		if ((tcp_sock = accept(server, (struct sockaddr *) &clientAddr, &clntLen)) < 0)
			printf("accept() failed");
		
		printf("Connected to %s on Port: %d\n", inet_ntoa(clientAddr.sin_addr),PORT);

		int16 simultime=0;		
		int i;
		for(i=0;i<runs;i++){
			simultime+=10;
			
			command_write(CMD_DONE, SUB_CMD_NORM ,(int16*)&simultime,0,0);


			gettimeofday(&stop, NULL);
			int t2= (stop.tv_sec - start.tv_sec)*1000000 + stop.tv_usec - start.tv_usec;
			printf("X-Token (%d) out after %d usec ",simultime,t2);


			received_command.request.command =0;
			while(received_command.request.command != CMD_DONE ){
				if (command_read() != 0){
					// Fehler
					printf("Probleme beim Lesen eines Kommandos\n");
				} else {
					// Alles ok, evtl. muessen wir aber eine Payload abholen
					if (received_command.payload != 0) {					
				//		printf ("fetching payload (%d bytes)\n",received_command.payload);
						low_read(buffer,received_command.payload);	
					}
					if (received_command.seq != seq){
						printf("Sequenzzaehler falsch! Erwartet: %d Empfangen %d \n",seq,received_command.seq);
					}
				}
				seq=received_command.seq+1;
			}
			gettimeofday(&start, NULL);


			int t= (start.tv_sec - stop.tv_sec)*1000000 + start.tv_usec - stop.tv_usec;
			printf("X-Token (%d) back after %d usec\n",received_command.data_l,t);


			if (received_command.data_l != simultime){
				printf("Falschen X-Frame erhalten ==> Exit\n");
				exit(0);
			}
			
/*			
			printf("Rechenzeit: %d usec\n",(stop.tv_sec - start.tv_sec)*1000000 +stop.tv_usec - start.tv_usec);
			
			command_read();
			if 
			#ifdef LOG_AVAILABLE
				command_display(&received_command);
			#endif		
			
			received_command.request.direction=DIR_ANSWER;
			tcp_send_cmd(&received_command);
		*/
		}
		
		
		#ifdef WIN32
			WSACleanup();
		#endif
		exit(0);
	}
	
	return 1;
}
#endif
