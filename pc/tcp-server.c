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
 * @file 	tcp-server.c
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
#define _REENTRANT
//#define _POSIX_SOURCE
#else
//#define WIN32
#endif

/* Hack for LinuxThreads */
#ifdef __linux__
#  define _P __P
#endif

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

static int server;                    /*!< Server-Socket */

static struct sockaddr_in serverAddr; /*!< Lokale Adresse  */
static struct sockaddr_in clientAddr; /*!< Client-Adresse  */
static unsigned int clntLen;          /*!< Laenge der Datenstruktur der Client-Adresse */

/*!
 * Init TCP-Server
 */
void tcp_server_init(void) {
#ifdef WIN32
    WSADATA wsaData;
    WORD wVersionRequested;
    int err;
	
    wVersionRequested = MAKEWORD(2, 0);	// 2.0 and above version of WinSock
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("Couldn't not find a usable WinSock DLL.\n");
        exit(1); 
    }
#endif	

    /* Wir geben uns als Sim aus, weil die Pakete mit derselben Zieladresse 
     * wieder zurueckkommen */
	set_bot_address(CMD_SIM_ADDR);
	
	/* Create socket for incoming connections */
	if ((server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("socket() failed\n");
		exit(1);
	}
	
	int i = 1;
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&i, sizeof(i));
	
	memset(&serverAddr, 0, sizeof(serverAddr));   // Clean up
	serverAddr.sin_family = AF_INET;              // Internet address family
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
	serverAddr.sin_port = htons(PORT);      // Local port to listen on
	
	/* Bind to the local address */
	if (bind(server, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		printf("bind() failed\n");
		exit(1);		
	}
	
	/* Mark the socket so it will listen for incoming connections */ 
	if (listen(server, 5) < 0) {
		printf("listen() failed\n");
		exit(1);
	}
}

/*!
 * Hauptschleife des TCP-Servers
 * @param runs	Anzahl der Durchlaeufe
 */
void tcp_server_run(int runs) {
	char buffer[MAX_PAYLOAD + sizeof(command_t)];
	struct timeval start, stop;
	printf("TCP-Server alive\n");

	uint8_t seq = 1;
	unsigned long t_sum = 0, t2_sum = 0;
	
	/* Set the size of the in-out parameter */
	clntLen = sizeof(clientAddr);

	/* Wait for a client to connect */
	printf("Waiting for client\n");
	if ((tcp_sock = accept(server, (struct sockaddr *) &clientAddr, &clntLen)) < 0) {
		printf("accept() failed");
	}

	printf("Connected to %s on Port: %u\n", inet_ntoa(clientAddr.sin_addr), PORT);

	int16_t simultime = 0;
	int i;
	for (i=0; i<runs; i++) {
		simultime += 10;
		//printf("i=%d\n", i);
		GETTIMEOFDAY(&start, NULL);
		command_write(CMD_SENS_IR, SUB_CMD_NORM, &simultime, NULL, 0);
		command_write(CMD_SENS_ENC, SUB_CMD_NORM, &simultime, &simultime, 0);
		command_write(CMD_SENS_BORDER, SUB_CMD_NORM, &simultime, &simultime, 0);
		command_write(CMD_SENS_LINE, SUB_CMD_NORM, &simultime, &simultime, 0);
		command_write(CMD_SENS_LDR, SUB_CMD_NORM, &simultime, &simultime, 0);
		command_write(CMD_SENS_TRANS, SUB_CMD_NORM, &simultime, NULL, 0);
		command_write(CMD_SENS_DOOR, SUB_CMD_NORM, &simultime, NULL, 0);
		command_write(CMD_SENS_MOUSE, SUB_CMD_NORM, &simultime, &simultime, 0);
		command_write(CMD_SENS_ERROR, SUB_CMD_NORM, &simultime, NULL, 0);
		command_write(CMD_SENS_RC5, SUB_CMD_NORM, &simultime, NULL, 0);

		command_write(CMD_DONE, SUB_CMD_NORM, &simultime, NULL, 0);
		flushSendBuffer();

		GETTIMEOFDAY(&stop, NULL);
		int t2 = (stop.tv_sec - start.tv_sec)*1000000 + stop.tv_usec
				- start.tv_usec;
		t2_sum += t2;
		printf("X-Token (%u) out after %u usec\n", simultime, t2);

		received_command.request.command = 0;
		GETTIMEOFDAY(&start, NULL);
		while (received_command.request.command != CMD_DONE) {
			int8_t res = command_read();
			//printf("Command read, result=%d\n", res);
			if (res != 0) {
				/* Fehler */
				printf("Probleme beim Lesen eines Kommandos\n");
			} else {
				/* Alles ok, evtl. muessen wir aber eine Payload abholen */
				if (received_command.payload != 0) {
					printf("fetching payload (%u bytes)\n", received_command.payload);
					low_read(buffer, received_command.payload);
				}
				if (received_command.seq != seq) {
					printf("Sequenzzaehler falsch! Erwartet: %u Empfangen %u\n", seq, received_command.seq);
				}
			}
			seq = received_command.seq + 1;
			//printf("seq=%u\n", seq);
		}
		GETTIMEOFDAY(&stop, NULL);

		int t = (stop.tv_sec - start.tv_sec)*1000000 + stop.tv_usec - start.tv_usec;
		t_sum += t;
		printf("X-Token (%u) back after %u usec\n", received_command.data_l, t);

		if (received_command.data_l != simultime) {
			printf("Falschen X-Frame erhalten ==> Exit\n");
			exit(0);
		}
	}

	/* Statistik */
	unsigned int mean_send = (double)t2_sum / (double)i;
	unsigned int mean_xfer = (double)t_sum / (2.0 * i);
	printf("\nAverage sendtime: %u usec\nAverage transfertime (1-way): %u usec\n", mean_send, mean_xfer);

	printf("\nTCP-Server hat seine %d runs durch und beendet sich.\nSo long and thanks for all the fish!\n", runs);

#ifdef WIN32
	WSACleanup();
#endif
	tcp_closeConnection(tcp_sock);
	exit(0);
}


/*!
 * Init TCP-Test-Client
 */
void tcp_test_client_init(void) {
	printf("Connecting Testclient to %s on Port: %d...\n", tcp_hostname, 10001);
	tcp_init();
}

/*!
 * Hauptschleife des TCP-Test-Clients
 * @param runs	Anzahl der Durchlaeufe, 0 fuer unendlich
 */
void tcp_test_client_run(int runs) {
	char buffer[sizeof(command_t)];
	
	if (runs > 0) {
		printf("Answering %d frames\n", runs);
	} else {
		printf("Answering all frames\n");
	}
	
	int i;
	for(i=0; runs==0 || i<runs; i++) {
		//printf("i=%d\n", i);
		int j;
		/* 11 Commands lesen und zurueckschicken */
		for (j=0; j<11; j++) {
			int len = tcp_read(buffer, sizeof(command_t));
			//printf("len=%d\n", len);
			tcp_write(buffer, len);
		}
		flushSendBuffer();
		printf(".");
		fflush(stdout);
	}
	printf("\nFinished %d frames\n", runs);
	tcp_closeConnection(tcp_sock);
	exit(0);
}

#endif	// PC
