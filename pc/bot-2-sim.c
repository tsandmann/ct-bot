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

/*! @file 	bot-2-sim.c 
 * @brief 	Verbindung c't-Bot zu c't-Sim
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include "bot-2-sim.h"
#include "tcp.h"
#include "command.h"
#include "display.h"
#include "sensor.h"
#include "bot-logik.h"
#include "motor.h"
#include "command.h"

#include <stdio.h>      // for printf() and fprintf()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for strlen()
#include <sys/time.h>
#include <pthread.h>
#include <time.h>

/* Linux with glibc:
 *   _REENTRANT to grab thread-safe libraries
 *   _POSIX_SOURCE to get POSIX semantics
 */
#ifdef __linux__
#  define _REENTRANT
//#  define _POSIX_SOURCE
#endif

/* Hack for LinuxThreads */
#ifdef __linux__
#  define _P __P
#endif

#define low_init tcp_init	/*!< Low-Funktion zum Initialisieren*/

pthread_t simThread;			/*!< Simuliert den Bot */
pthread_t bot_2_sim_Thread;		/*!< Thread sammelt Sensordaten, uebertraegt Motor-Daten */

pthread_cond_t      command_cond  = PTHREAD_COND_INITIALIZER;	/*!< Schuetzt das Kommando */
pthread_mutex_t     command_cond_mutex = PTHREAD_MUTEX_INITIALIZER;	/*!< Schuetzt das Kommando */

void signal_command_available(void);
int wait_for_command(int timeout_s);

#ifdef WIN32
	 /* These are winbase.h definitions, but to avoid including 
	tons of Windows related stuff, it is reprinted here */
	
	typedef struct _FILETIME {
		unsigned long dwLowDateTime;
		unsigned long dwHighDateTime;
	} FILETIME;
	
	void __stdcall GetSystemTimeAsFileTime(FILETIME*);	
	void gettimeofday(struct timeval* p, void* tz /* IGNORED */);
	
	void gettimeofday(struct timeval* p, void* tz /* IGNORED */){
		union {
			long long ns100; // time since 1 Jan 1601 in 100ns units 
			FILETIME ft;
		} _now;
	
		GetSystemTimeAsFileTime( &(_now.ft) );
		p->tv_usec=(long)((_now.ns100 / 10LL) % 1000000LL );
		p->tv_sec= (long)((_now.ns100-(116444736000000000LL))/10000000LL);
		return;
	}
#endif

/*! 
 * Dieser Thread nimmt die Daten vom PC entgegen
 */
void *bot_2_sim_rcv_isr(void * arg){
	#ifdef DISPLAY_AVAILABLE
		display_cursor(11,1);
	#endif
	printf("bot_2_sim_rcv_isr() comming up\n");
	for (;;){
		// only write if noone reads command
		if (command_read()!=0)
			printf("Error reading command\n");			// read a command
		else {		
//			command_display(&received_command);	// show it
			if (command_evaluate() ==0)			// use data transfered
				signal_command_available();		// tell anyone waiting
		}
	}
	return 0;
}

/*!
 * Ein wenig Initialisierung kann nicht schaden 
 */
void bot_2_sim_init(void){
	low_init();
		
	if (pthread_create(&bot_2_sim_Thread,  // thread struct
		NULL,		      // default thread attributes
		bot_2_sim_rcv_isr,	      // start routine
		NULL)) {              // arg to routine
			printf("Thread Creation failed");
			exit(1);
	}
	
	int j;
	int16 null=0;
	for(j=0;j<5;j++) 
		command_write(CMD_WELCOME, SUB_WELCOME_SIM ,&null,&null);

}


int not_answered_error=1;	/*!< Wurde ein Packet beantwortet */




/*! 
 * Wartet auf die Antwort des PC
 * @param timeout_s Wartezeit in Sekunden
 * @return 0, wenn Ok
 */
int wait_for_command(int timeout_s){
	struct timespec   ts;
	struct timeval    tp;
	int result=0;
	
	pthread_mutex_lock(&command_cond_mutex);
	
	gettimeofday(&tp, NULL);
	// Convert from timeval to timespec
	ts.tv_sec  = tp.tv_sec;
	ts.tv_nsec = tp.tv_usec * 1000;
	ts.tv_sec += timeout_s;

	result= pthread_cond_timedwait(&command_cond, &command_cond_mutex, &ts);
	
	pthread_mutex_unlock(&command_cond_mutex);
	
	return result;
}

/*!
 * Benachrichtigt wartende Threads ueber eingetroffene Kommandos
 */
void signal_command_available(void){
	pthread_mutex_lock(&command_cond_mutex);
	pthread_cond_signal(&command_cond);
	pthread_mutex_unlock(&command_cond_mutex);
}

/*!
 * Schickt einen Thread in die Warteposition
 * @param timeout_us Wartezeit in �s
 */
void wait_for_time(long timeout_us){
	pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
	struct timespec   ts;
	struct timeval    tp;
	
	pthread_mutex_lock(&mutex);
	gettimeofday(&tp, NULL);
	// Convert from timeval to timespec
	
	tp.tv_usec += (timeout_us % 1000000);
	tp.tv_sec += (timeout_us / 1000000);
	
	ts.tv_sec  = tp.tv_sec+ (tp.tv_usec/1000000);
	ts.tv_nsec = (tp.tv_usec % 1000000)* 1000;
	
	pthread_cond_timedwait(&cond, &mutex, &ts);
	pthread_mutex_unlock(&mutex);
}
#endif
