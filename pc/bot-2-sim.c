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
 * @file 	bot-2-sim.c 
 * @brief 	Verbindung c't-Bot zu c't-Sim
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#include "ct-Bot.h"

#ifdef PC

#include <stdio.h>      // for printf() and fprintf()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for strlen()
#include <sys/time.h>
#include <time.h>

#include "bot-2-sim.h"
#include "tcp.h"
#include "command.h"
#include "display.h"
#include "sensor.h"
#include "bot-logic/bot-logik.h"
#include "motor.h"
#include "command.h"



/* Linux with glibc:
 *   _REENTRANT to grab thread-safe libraries
 *   _POSIX_SOURCE to get POSIX semantics
 */
#ifdef __linux__
#define _REENTRANT
//#define _POSIX_SOURCE
#endif

/* Hack for LinuxThreads */
#ifdef __linux__
#define _P __P
#endif

#define low_init tcp_init	/*!< Low-Funktion zum Initialisieren */

#ifdef WIN32
	 /* These are winbase.h definitions, but to avoid including 
	tons of Windows related stuff, it is reprinted here */
	
	typedef struct _FILETIME {
		unsigned long dwLowDateTime;
		unsigned long dwHighDateTime;
	} FILETIME;
	
	void __stdcall GetSystemTimeAsFileTime(FILETIME*);	
	
	void gettimeofday_win(struct timeval* p, void* tz /* IGNORED */){
		union {
			long long ns100; // time since 1 Jan 1601 in 100ns units 
			FILETIME ft;
		} _now;
	
		GetSystemTimeAsFileTime( &(_now.ft) );
		p->tv_usec=(long)((_now.ns100 / 10LL) % 1000000LL );
		p->tv_sec= (long)((_now.ns100-(116444736000000000LL))/10000000LL);
		return;
	}
#endif	// WIN32

/*! 
 * Schleife, die Kommandos empfaengt und bearbeitet, bis ein Kommando vom Typ Frame kommt 
 * @param frame	Kommando zum Abbruch
 * @return		Fehlercode
 */
int8 receive_until_Frame(int8 frame) {
	int8 result = 0;
	for(;;) {
		result=command_read();
		if (result != 0) {
			/* Fehler werden in command_read() ausgegeben
			 * -1 kommt auch, wenn das Paket nicht fuer unsere Adresse war */ 
			return result;
		} else {		
			command_evaluate();
		}
		
		if (received_command.request.command == frame)
			return 0;
	}
}


/*!
 * Ein wenig Initialisierung kann nicht schaden 
 */
void bot_2_sim_init(void) {
	low_init();

	uint8_t addr = get_bot_address();
	if (addr > 127) {
		/* gespeicherte Adresse ist eine vom Sim Vergebene,
		 * schalte auf Adressevergabemodus um */
		addr = CMD_BROADCAST;
		set_bot_address(addr);
	}
	int j;
	for (j=0; j<5; j++) { 
		command_write(CMD_WELCOME, SUB_WELCOME_SIM, NULL, NULL, 0);
	}

	if (addr == CMD_BROADCAST) {
		// Fordere eine Adresse an
		command_write(CMD_ID, SUB_ID_REQUEST, NULL, NULL, 0);
	}

	flushSendBuffer();
}

#endif	// PC
