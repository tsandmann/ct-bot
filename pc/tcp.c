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
 * \file 	tcp.c
 * \brief 	TCP/IP-Kommunikation
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#ifdef PC
#include "ct-Bot.h"
#include "tcp.h"
#include "display.h"
#include "log.h"
#include "command.h"

#define USE_SEND_BUFFER				/**< Schalter fuer Sendepuffer an/aus */
#define TCP_SEND_BUFFER_SIZE 4096	/**< Groesse des Sendepuffers / Byte */

//#define DEBUG_TCP	/**< Schalter fuer Debug-Ausgaben */

#ifndef DEBUG_TCP
#undef LOG_AVAILABLE
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}	/**< Log-Dummy */
#endif

#ifndef WIN32
#define _REENTRANT		/**< to grab thread-safe libraries */
#endif

/* Hack for LinuxThreads */
#ifdef __linux__
#define _P __P
#endif

#include <pthread.h>

#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>		// for gethostbyname()
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#endif // WIN32

#include <stdio.h>      // for printf() and fprintf()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()

int tcp_sock = 0;			/**< Unser TCP-Socket */
char * tcp_hostname = NULL;	/**< Hostname, auf dem ct-Sim laeuft */

static uint8_t sendBuffer[TCP_SEND_BUFFER_SIZE];	/**< Sendepuffer fuer ausgehende Packete */
static int sendBufferPtr = 0;						/**< Index in den Sendepuffer */

#ifndef __WIN32__
static int server; /**< Server-Socket */
static struct sockaddr_in serverAddr; /**< lokale Adresse */
static struct sockaddr_in clientAddr; /**< Client-Adresse  */
static socklen_t clntLen; /**< Laenge der Datenstruktur der Client-Adresse */
#endif // __WIN32__


/**
 * Oeffnet eine TCP-Verbindung zum Server
 * \param *hostname	Symbolischer Name des Host, auf dem ct-Sim laeuft
 * \return			Der Socket
 */
int tcp_openConnection(const char * hostname) {
	struct sockaddr_in servAddr;	// server address
	int sock = 0;	// Socket descriptor
	struct hostent * he = gethostbyname(hostname);

	// Ueberpruefen, ob der Hostname aufgeloest werden konnte
	if (NULL == he) {
		printf("gethostbyname() failed\n");
		exit(1);
	}

	// Create a stream socket for TCP
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("socket() failed\n");
		exit(1);
	}

	int flag = 1;
	setsockopt(sock,		/* socket affected */
			IPPROTO_TCP,	/* set option at TCP level */
			TCP_NODELAY,	/* name of option */
			(char *) &flag,	/* the cast is historical cruft */
			sizeof(int)		/* length of option value */
	);

	// Prepare server address structure
	memset(&servAddr, 0, sizeof(servAddr));	// Zero out structure
	servAddr.sin_family = AF_INET;	// Internet address
	servAddr.sin_port = htons(PORT);	// Port

	// Die erste Adresse aus der Liste uebernehmen
	memcpy(&servAddr.sin_addr, *(he->h_addr_list), sizeof(servAddr.sin_addr));

	// Open Connection to the server
	if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
		printf("tcp_openConnection() to %s failed\n", inet_ntoa(servAddr.sin_addr));
		exit(1);
	}

	return sock;
}

/**
 * Schliesst eine TCP-Connection
 * \param sock	Der Socket
 */
void tcp_closeConnection(int sock) {
	close(sock);
#ifdef WIN32
	WSACleanup();
#endif
}

/**
 * Puffert Daten im Sendepuffer zwischen
 * \param *data		Zeiger auf die Daten
 * \param length	Anzahl der Bytes
 * \return 			Anzahl der kopierten Bytes, -1 wenn Puffer zu klein
 */
static inline int16_t copy2Buffer(const void * data, unsigned length) {
	if ((sendBufferPtr + length) > sizeof(sendBuffer)) {
		LOG_DEBUG("Sendbuffer filled with %u/%u bytes, another %d bytes pending.", sendBufferPtr, (unsigned int)sizeof(sendBuffer), length);
		LOG_DEBUG("  ==> Trying to recover by calling flushSendBuffer()...");
		flushSendBuffer();
		if ((sendBufferPtr + length) > sizeof(sendBuffer)) {
			printf("ERROR - Not enough space in sendbuffer!\n");
			LOG_DEBUG("  ==> Failed, Aborting");
			return -1;
		} else {
			LOG_DEBUG("  ==> Succeeded, data copied");
		}
	}
	LOG_DEBUG("Store %d bytes", length);
	// Auf dem PC kopieren wir nur alles in den Ausgangspuffer
	memcpy(&sendBuffer[sendBufferPtr], data, length);
	sendBufferPtr += length;
	LOG_DEBUG(" %d bytes now in buffer", sendBufferPtr);
	return length;
}

/**
 * Uebertrage Daten per TCP/IP
 * \param *data		Zeiger auf die Daten
 * \param length	Anzahl der Bytes
 * \return 			Anzahl der gesendeten Byte, -1 wenn Fehler
 */
int16_t tcp_write(const void * data, int16_t length) {
#ifdef ARM_LINUX_BOARD
	if (tcp_sock == 0) {
		return 0;
	}
#endif // ARM_LINUX_BOARD
	int16_t bytes_sent;
#ifdef USE_SEND_BUFFER
	bytes_sent = copy2Buffer(data, length);
#else
	bytes_sent = length;
	if (send(tcp_sock, data, length, 0) != length) {
		printf("send() sent a different number of bytes than expected\n");
		bytes_sent = -1;
	}
#endif	// USE_SEND_BUFFER
	LOG_DEBUG("sent %d bytes", bytes_sent);
	return bytes_sent;
}

/**
 * Lese Daten von TCP/IP-Verbindung.
 * Achtung: blockierend!
 * \param *data		Zeiger auf die Daten
 * \param length	Anzahl der gewuenschten Bytes
 * \return 			Anzahl der uebertragenen Bytes
 */
int16_t tcp_read(void * data, int16_t length) {
	if (tcp_sock == 0 || length == 0) {
		return 0; // NOP, aber auch kein Programmabbruch noetig
	}
	int16_t bytesReceived = 0;

	if ((bytesReceived = recv(tcp_sock, data, length, 0)) <= 0) {
		printf("recv() failed or connection closed prematurely\n");
	    exit(1);
	}

	return bytesReceived;
}

/**
 * Initialisiere TCP/IP Verbindung
 */
void tcp_init(void) {
#ifndef ARM_LINUX_BOARD
	tcp_init_client();
#else
	static pthread_t thread;
	pthread_create(&thread, NULL, tcp_init_server, NULL);
#endif // ARM_LINUX_BOARD
}

#ifndef __WIN32__
/**
 * Initialisiere TCP/IP Verbindung als Server
 * \param *ptr Datenparameter fuer pthread, wird nicht verwendet
 * \return NULL
 */
void * tcp_init_server(void * ptr) {
	(void) ptr;
	/* Create socket for incoming connections */
	if ((server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		LOG_ERROR("socket() failed");
		exit(1);
	}

	int i = 1;
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof(i));

//	fcntl(server, F_SETFL, O_NONBLOCK); // non-blocking

	memset(&serverAddr, 0, sizeof(serverAddr)); // clean up
	serverAddr.sin_family = AF_INET; // internet address family
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // any incoming interface
	serverAddr.sin_port = htons(SERVERPORT); // local port to listen on

	/* bind to the local address */
	if (bind(server, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		LOG_ERROR("bind() failed");
		exit(1);
	}

	/* mark the socket so it will listen for incoming connections */
	if (listen(server, 5) < 0) {
		LOG_ERROR("listen() failed");
		exit(1);
	}

	/* set the size of the in-out parameter */
	clntLen = sizeof(clientAddr);

	/* wait for a client to connect */
	LOG_INFO("Waiting for TCP client (in background)...");
	if ((tcp_sock = accept(server, (struct sockaddr *) &clientAddr, &clntLen)) < 0) {
		LOG_ERROR("accept() failed");
	}
	LOG_INFO("TCP Client %s connected on port %u.", inet_ntoa(clientAddr.sin_addr), SERVERPORT);

#ifndef __WIN32__
	signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE signal
#endif

	return NULL;
}

/**
 * Ermittelt wie viele Bytes auf dem TCP-Server Socket zur Verfuegung stehen
 * \return Bytes verfuegbar
 */
int tcp_data_available(void) {
	int bytes_avail;
	int ret = ioctl(tcp_sock, FIONREAD, &bytes_avail);
	if (ret < 0)	{
		int err = errno;
		LOG_ERROR("tcp_data_available(): ioctl() failed: %d; %d", ret, err);
		return -1;
	}

	return bytes_avail;
}
#endif // __WIN32__

/**
 * Initialisiere TCP/IP Verbindung als Client (Verbindung zum Sim)
 */
void tcp_init_client(void) {
#ifdef WIN32
	WSADATA wsaData;
	WORD wVersionRequested;
	int err;

	wVersionRequested = MAKEWORD(2, 0); // 2.0 and above version of WinSock
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		printf("Couldn't not find a usable WinSock DLL.\n");
		exit(1);
	}
#endif	// WIN32

	if ((tcp_sock = tcp_openConnection(tcp_hostname)) != -1) {
		printf("Connection to %s established on Port: %u\n", tcp_hostname, PORT);
	} else {
		printf("Connection to %s failed on Port: %u\n", tcp_hostname, PORT);
		exit(1);
	}

	sendBufferPtr = 0; // Puffer leeren
}

/**
 * Schreibt den Sendepuffer auf einen Schlag raus
 * \return -1 bei Fehlern, sonst Anzahl der uebertragenen Bytes
 */
int16_t flushSendBuffer(void) {
	int16_t length = 0;
#ifdef USE_SEND_BUFFER
	LOG_DEBUG("Flushing Buffer with %d bytes", sendBufferPtr);

	length = sendBufferPtr;
	if (length == 0) {
		return 0;
	}
	sendBufferPtr = 0; // Puffer auf jedenfall leeren
	const int n = send(tcp_sock, (char *) &sendBuffer, length, 0);
	if (n != length) {
		LOG_ERROR("flushSendBuffer(): send() sent a different number of bytes (%d) than expected (%d)", n, length);
		length = -1;
	}
#endif // USE_SEND_BUFFER
	return length;
}

/**
 * Prueft die CRC Checksumme eines Kommandos
 * \param *cmd Zeiger auf das Kommando
 * \return True oder False
 */
uint8_t tcp_check_crc(command_t * cmd) {
	(void) cmd;
	return True; // wir vertrauen hier auf TCP/IP
}

/**
 * Berechnet die CRC Checksumme eines Kommandos
 * \param *cmd Zeiger auf das Kommando
 */
void tcp_calc_crc(command_t * cmd) {
	(void) cmd;
	// wir vertrauen hier auf TCP/IP
}
#endif // PC
