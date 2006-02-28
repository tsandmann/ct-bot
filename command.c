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

/*! @file 	command.c
 * @brief 	Kommando-Management
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#include "ct-Bot.h"

#include "led.h"

#include "uart.h"
#include "adc.h"
#include "timer.h"

#include "command.h"
#include "display.h"

#include "sensor.h"
#include "motor.h"
#include "rc5.h"
#include "ir-rc5.h"
#include "bot-logik.h"

#include <stdio.h>

#ifdef PC
	#include "tcp.h"
	#include <pthread.h>	
	#define low_read tcp_read 	/*!< Which function to use to read data */
#endif

#ifdef MCU
	#define low_read uart_read 	/*!< Which function to use to read data */
#endif


#ifdef COMMAND_AVAILABLE

#define RCVBUFSIZE sizeof(command_t)   /*!< Groesse des Empfangspuffers */

command_t received_command;		/*!< Puffer fuer Kommandos */

#ifdef PC
	// Auf dword alignment bestehen, wird fuer MacOS X benoetigt
	pthread_mutex_t     command_mutex __attribute__ ((aligned (4)))
		= PTHREAD_MUTEX_INITIALIZER;
#endif

/*!
 * Liest ein Kommando ein, ist blockierend!
 * Greift auf low_read() zurueck
 * @see low_read()
 */
int command_read(void){
	int bytesRcvd;
	int start=0;			// start of command sequence
	int i;			
	command_t * command;		// Pointer to Cast rceceived data
	char * ptr;			// helper
	char buffer[RCVBUFSIZE];       // Buffer  
#if BYTE_ORDER == BIG_ENDIAN
	uint16 store;			//store for endian conversion
#endif

	buffer[0]=0;			// Start with clean data 
	
	// get first data
	bytesRcvd=low_read(buffer,RCVBUFSIZE);	
	
	// Search for frame start
	while ((start<bytesRcvd)&&(buffer[start] != CMD_STARTCODE)) {	
		printf(".");
		start++;
	}
	
	// if no STARCODE ==> discard
	if (buffer[start] != CMD_STARTCODE)
		return -1;
	
	//is any chance, that the packet will still fit to buffer?
	if ((RCVBUFSIZE-start) < sizeof(command_t))
		return -1;	// no ==> discard
	
	// get rest of package as long as buffer is full
	while ((bytesRcvd-start)<sizeof(command_t)){
		bytesRcvd+=low_read(buffer+bytesRcvd,sizeof(command_t)-(bytesRcvd-start));
	}

	// Cast to command_t
	command= (command_t *)&buffer+start;
	
	// validate (startcode is already ok, or we won't be here)
	if ((*command).CRC==CMD_STOPCODE){
		// Transfer
		#ifdef PC
			command_lock();		// on PC make storage threadsafe
		#endif
		ptr = (char *) &received_command;
		for (i=0; i<sizeof(command_t);i++){
			*ptr=buffer[i+start];
			ptr++;
		}
		#if BYTE_ORDER == BIG_ENDIAN
			/* Umwandeln der 16 bit Werte in Big Endian */
			store = received_command.data_l;
			received_command.data_l = store << 8;
			received_command.data_l |= (store >> 8) & 0xff;

			store = received_command.data_r;
			received_command.data_r = store << 8;
			received_command.data_r |= (store >> 8) & 0xff;
    
			store = received_command.seq;
			received_command.seq = store << 8;
			received_command.seq |= (store >> 8) & 0xff;

			/* "Umdrehen" des Bitfields */
			store = received_command.request.subcommand;
			received_command.request.subcommand = store << 1;
			received_command.request.direction = store >> 7;
		#endif
		#ifdef PC
			command_unlock();	// on PC make storage threadsafe
		#endif
		return 0;
	} else {	// Command not valid
		return -1;
		printf("Invalid Command!\n");
	}
}

/*!
 * Wertet das Kommando im Puffer aus
 * return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int command_evaluate(void){
	int analyzed = 1;
	
	switch (received_command.request.command) {
		#ifdef LED_AVAILABLE
			case CMD_AKT_LED:	// LED-Steuerung
				LED_set(received_command.data_l & 255);
				break;
		#endif
		#ifdef PC
			case CMD_SENS_IR:
				sensDistL=received_command.data_l;
				sensDistR=received_command.data_r;
				#ifdef TIME_AVAILABLE
					system_time_isr();		/* Einmal pro Update-Zyklus aktualisieren wir die Systemzeit */
				#endif
				break;
			case CMD_SENS_ENC:
				sensEncL+=received_command.data_l;
				sensEncR+=received_command.data_r;
				break;
			case CMD_SENS_BORDER:
				sensBorderL=received_command.data_l;
				sensBorderR=received_command.data_r;
				break;
			case CMD_SENS_LINE:
				sensLineL=received_command.data_l;
				sensLineR=received_command.data_r;
				break;
			case CMD_SENS_LDR:
				sensLDRL=received_command.data_l;
				sensLDRR=received_command.data_r;
				break;
			case CMD_SENS_TRANS:
				sensTrans=(char)received_command.data_l;
				break;
			case CMD_SENS_ERROR:
				sensError=(char)received_command.data_l;
				sensor_update();	/* Error ist der letzte uebertragene Sensorwert, danach koennen wir uns um allgemeine updates kümmern*/
				break;
			case CMD_SENS_RC5:
				ir_data=received_command.data_l;
				break;
		#endif
		default:
			analyzed=0;		// Command was not analysed yet
			break;
	}
	return analyzed;
}

#ifdef DISPLAY_AVAILABLE
/*! 
 * Gibt ein Kommando auf dem Bildschirm aus
 */
	void command_display(command_t * command){
		#ifdef PC
/*			printf("START= %d\nDIR= %d CMD= %d SUBCMD= %d\nPayload= %d\nDATA = %d %d\nSeq= %d\nCRC= %d",
				(*command).startCode,
				(*command).request.direction,
				(*command).request.command,
				(*command).request.subcommand,
				(*command).payload,
				(*command).data_l,
				(*command).data_r,
				(*command).seq,				
				(*command).CRC);
*/			
			printf("CMD: %c\tData L: %d\tSeq: %d\n",
				(*command).request.command,
				(*command).data_l,
				(*command).seq);
		#else
			char* raw= (char*)command;
			unsigned char i=0;
			char hex[6];
			
			display_cursor(4,1);
			display_string("0x");
			to_hex(*raw,hex);
			display_string(hex);
			display_string(" ");
			raw++;
		
			for (i=1; i<sizeof(command_t)-1; i++){
				to_hex(*raw++,hex);
				display_string(hex);				
			}

			display_string(" ");
			to_hex(*raw,hex);
			display_string(hex);
			
			sprintf(hex,"%5d",(*command).seq);
			display_string(" ");
			display_string(hex);

			#ifdef WIN32
				printf("\n");
			#endif

		#endif
	}
#endif
#endif
