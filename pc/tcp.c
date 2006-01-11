/*! @file 	tcp.c
 * @brief 	TCP/IP-Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include "ct-Bot.h"

#ifdef PC

/* Linux with glibc:
 *   _REENTRANT to grab thread-safe libraries
 *   _POSIX_SOURCE to get POSIX semantics
 */
#ifdef __linux__
#  define _REENTRANT
#  define _POSIX_SOURCE
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
#endif


#include <stdio.h>      // for printf() and fprintf()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()

#include "tcp.h"
#include "display.h"

int tcp_sock=0;			///< Unser TCP Socket

/*!
 * Öffnet eime TCP-Connection zum Server 
 * @return Der Socket
*/
int tcp_openConnection(void){
    struct sockaddr_in servAddr;   // server address
    int sock=0;                        // Socket descriptor


    // Create a stream socket for TCP
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        printf("socket() failed");
	exit(1);
    }

    // Prepare server address structure
    memset(&servAddr, 0, sizeof(servAddr));     // Zero out structure
    servAddr.sin_family      = AF_INET;     // Internet address
    servAddr.sin_port        = htons(PORT);     // Port
    servAddr.sin_addr.s_addr = inet_addr(IP);   // IP address 

    // Open Connection to the server
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
        printf("tcp_openConnection() failed\n");
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
 * Übertrage Daten per tcp/ip
 * @param data Zeiger auf die Daten
 * @param length Anzahl der Bytes
 * @return Anzahl der übertragenen Bytes
*/
int tcp_write(char* data, int length){
	if (send(tcp_sock,data,length,0) != length){
		printf("send() sent a different number of bytes than expected");
		return -1;
	}
	return 0;
}

/*!
 * Lese Daten von TCP/IP-Verbindung
 * achtung blockierend
 * @param data Zeiger auf die Daten
 * @param length Anzahl der gewünschten Bytes
 * @return Anzahl der übertragenen Bytes
*/
int tcp_read(char* data, int length){
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
	
	
    if ((tcp_sock=tcp_openConnection()) != -1)
        printf ("connection to %s established on Port: %d\n",IP,PORT);
    else {
	printf ("connection to %s failed on Port: %d\n",IP,PORT);
    	exit(1);
    }

}
#endif
