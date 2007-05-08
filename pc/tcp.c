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
 * @file 	tcp.c
 * @brief 	TCP/IP-Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#include "ct-Bot.h"

#define USE_SEND_BUFFER				/*!< Schalter fuer Sendepuffer an/aus */
#define TCP_SEND_BUFFER_SIZE 2048	/*!< Groesse des Sendepuffers */

#ifdef PC

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
	#include <netdb.h>		// for gethostbyname()
	#include <netinet/tcp.h> 
#endif


#include <stdio.h>      // for printf() and fprintf()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()

#include "tcp.h"
#include "display.h"

int tcp_sock=0;			/*!< Unser TCP-Socket */
char *tcp_hostname = NULL;		/*!< Hostname, auf dem ct-Sim laeuft */



#ifdef PC
	uint8 sendBuffer[TCP_SEND_BUFFER_SIZE];	/*!< Sendepuffer fuer ausgehende Packete */
	int sendBufferPtr=0;	/*!< Index in den Sendepuffer */
#endif

/*!
 * Oeffnet eine TCP-Verbindung zum Server 
 * @param hostname Symbolischer Name des Host, auf dem ct-Sim laeuft
 * @return Der Socket
*/
int tcp_openConnection(const char *hostname){
    struct sockaddr_in servAddr;   // server address
    int sock=0;                        // Socket descriptor
    struct hostent *he = gethostbyname(hostname);

	// Ueberpruefen, ob der Hostname aufgeloest werden konnte
	if (NULL == he) {
		printf("gethostbyname() failed\n");
		exit(1);
	}

    // Create a stream socket for TCP
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        printf("socket() failed");
	exit(1);
    }
    
    int flag = 1;
  	setsockopt(sock,            /* socket affected */
                          IPPROTO_TCP,     /* set option at TCP level */
                          TCP_NODELAY,     /* name of option */
                          (char *) &flag,  /* the cast is historical 
                                                  cruft */
                          sizeof(int));    /* length of option value */

    // Prepare server address structure
    memset(&servAddr, 0, sizeof(servAddr));     // Zero out structure
    servAddr.sin_family      = AF_INET;     // Internet address
    servAddr.sin_port        = htons(PORT);     // Port

    // Die erste Adresse aus der Liste uebernehmen
    memcpy(&servAddr.sin_addr, *(he->h_addr_list), sizeof(servAddr.sin_addr));

    // Open Connection to the server
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
        printf("tcp_openConnection() to %s failed\n", inet_ntoa(servAddr.sin_addr));
	exit(1);
    }

    return sock;
}

/*!
 * Schliesst eine TCP-Connection
 * @param sock Der Socket
*/
void tcp_closeConnection(int sock){
    close(sock);
    #ifdef WIN32
		WSACleanup();
	#endif
}

/*!
 * Sende Kommando per TCP/IP im Little Endian
 * Diese Funktion setzt vorraus, dass die Symbole BYTE_ORDER und BIG_ENDIAN
 * bzw. LITTLE_ENDIAN definiert wurden. Damit dies auf Linux/Unix 
 * funktioniert darf _POSIX_SOURCE nicht definiert werden. Fuer Windows
 * wird dies in der Headerdatei tcp.h erledigt.
 * Getestet wurde dies bisher auf folgenden Systemen:
 *  - MacOSX (PPC, big endian)
 *  - Gentoo Linux (hppa, big endian)
 *  - OpenBSD (i386, little endian)
 *  - Windows 2000 (i386, little endian mit dem MinGW)
 * Sollten in command_t weitere Werte mit mehr bzw. weniger als 8 bit
 * aufgenommen werden muss hier eine entsprechende Anpassung erfolgen.
 * @param cmd Zeiger auf das Kommando
 * @return Anzahl der gesendete Bytes
 */
int tcp_send_cmd(command_t *cmd){
#if BYTE_ORDER == BIG_ENDIAN
	command_t le_cmd;

	/* Kopieren des Kommandos und auf Little Endian wandeln */
	memcpy(&le_cmd, cmd, sizeof(command_t));

	/* Alle 16bit Werte in Little Endian wandeln */
	le_cmd.data_l = cmd->data_l << 8;
	le_cmd.data_l |= (cmd->data_l >> 8) & 0xff;
	le_cmd.data_r = cmd->data_r << 8;
	le_cmd.data_r |= (cmd->data_r >> 8) & 0xff;
	le_cmd.seq = cmd->seq <<8;
	le_cmd.seq |= (cmd->seq >> 8) & 0xff;

	/* "Umdrehen" des Bitfields */
	le_cmd.request.subcommand = cmd->request.subcommand >> 1;
	le_cmd.request.direction = (cmd->request.subcommand & 1) << 7;

	return tcp_write(&le_cmd, sizeof(command_t));
#else
	return tcp_write(cmd, sizeof(command_t));
#endif
}

/*! Puffert daten im Sendepuffer zwischen
 * @param data zeiger auf die daten
 * @param length Anzahl der Bytes
 * @return -1 wenn kein Platz mehr im Puffer ist, 0 wenn alles ok ist
 */
int copy2Buffer(uint8* data, int length){
	int i;
	uint8 * ptr = data;
	
	if ((sendBufferPtr + length) > sizeof(sendBuffer)){
		printf("%s() %s:%u: sendBuffer filled with %u/%lu Bytes, another %u bytes pending. Full! Aborting copy!\n",__FUNCTION__,__FILE__, __LINE__,sendBufferPtr,sizeof(sendBuffer),length);

		printf("  ==> Trying to recover by calling flushSendBuffer()\n");
		flushSendBuffer(); 
		if ((sendBufferPtr + length) > sizeof(sendBuffer)) {
			printf("  ==> Still not enough Space\n");
				
			return -1;
		}
	}
//	printf("Store %d bytes",length);
	// Auf dem PC kopieren wir nur alles in den Ausgangspuffer
	for (i=0; i< length ; i++){
		sendBuffer[sendBufferPtr++]= *ptr++ ;
	}
//	printf(" %d Bytes now in buffer\n",sendBufferPtr);
	return 0;
}

/*!
 * Uebertrage Daten per TCP/IP
 * @param data Zeiger auf die Daten
 * @param length Anzahl der Bytes
 * @return 0 wenn alles ok, -1 wenn puffer voll
*/
int tcp_write(void* data, int length){
	#ifdef USE_SEND_BUFFER
		return copy2Buffer(data, length);
	#else
		if (send(tcp_sock,data,length,0) != length){
			printf("send() sent a different number of bytes than expected");
			return -1;
		}
		return length;
	#endif
}

/*!
 * Lese Daten von TCP/IP-Verbindung.
 * Achtung: blockierend!
 * @param data Zeiger auf die Daten
 * @param length Anzahl der gewuenschten Bytes
 * @return Anzahl der uebertragenen Bytes
*/
int tcp_read(void* data, int length){
	int bytesReceived=0;

	if ((bytesReceived = recv(tcp_sock, data, length, 0)) <= 0){
            printf("recv() failed or connection closed prematurely\n");
	    exit(1);
	    return -1;
	}
	
	return bytesReceived;
}

/*! 
 * Initialisiere TCP/IP Verbindung 
 */ 
void tcp_init(void){
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
	
	
    if ((tcp_sock=tcp_openConnection(tcp_hostname)) != -1)
        printf ("connection to %s established on Port: %d\n", tcp_hostname, PORT);
    else {
		printf ("connection to %s failed on Port: %d\n", tcp_hostname, PORT);
    	exit(1);
    }

}

/*! 
 * Schreibt den Sendepuffer auf einen Schlag raus 
 * @return -1 bei Fehlern, sonst zahl der uebertragenen Bytes
 */
int flushSendBuffer(void){
	#ifdef USE_SEND_BUFFER
	//	printf("Flushing Buffer with %d bytes\n",sendBufferPtr);
	
		int length=sendBufferPtr;
		sendBufferPtr=0;	// Puffer auf jedenfall leeren
		if (send(tcp_sock,(char*)&sendBuffer,length,0) != length){
			printf("send() sent a different number of bytes than expected");
			return -1;
		}
		return length;
	#else
		return 0;
	#endif
}

#endif
