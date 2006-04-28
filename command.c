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
#include "log.h"

#include "uart.h"
#include "adc.h"
#include "timer.h"
#include "mouse.h"

#include "command.h"
#include "display.h"

#include "sensor.h"
#include "motor.h"
#include "rc5.h"
#include "ir-rc5.h"
#include "bot-logik.h"
#include "bot-2-pc.h"

#include <stdio.h>
#include <string.h>

#ifdef PC
	#include "tcp.h"
	#include <pthread.h>	
#endif


#ifdef COMMAND_AVAILABLE

#define RCVBUFSIZE (sizeof(command_t)*2)   /*!< Groesse des Empfangspuffers */

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
	
//	uint8 tmp=uart_data_available();
	// Den ganzen Puffer abholen
	bytesRcvd=low_read(buffer,sizeof(command_t));	
//	LOG_DEBUG(("%d/%d read/av",bytesRcvd,tmp));
	
	
//	LOG_DEBUG(("%x %x %x",buffer[0],buffer[1],buffer[2]));
	// Search for frame start
	while ((start<bytesRcvd)&&(buffer[start] != CMD_STARTCODE)) {	
//		printf(".");
		start++;
	}
		
	// if no STARCODE ==> discard
	if (buffer[start] != CMD_STARTCODE){
//		LOG_DEBUG(("start not found"));
		return -1;	
	}
	
//	LOG_DEBUG(("Start @%d",start));
	
	//is any chance, that the packet will still fit to buffer?
	if ((RCVBUFSIZE-start) < sizeof(command_t)){
//		LOG_DEBUG(("not enough space"));
		return -1;	// no ==> discard
	}
	
	i=sizeof(command_t) - (bytesRcvd-start);
	// get rest of package as long as buffer is full
	while (i > 0){
//		LOG_DEBUG(("%d bytes missing",i));
		i= low_read(buffer+bytesRcvd,i);
//		LOG_DEBUG(("%d read",i));
		bytesRcvd+=i;
		i=sizeof(command_t) - (bytesRcvd-start);
	}

//	LOG_DEBUG(("%d/%d read/start",bytesRcvd,start));

//	LOG_DEBUG(("%x %x %x",buffer[start],buffer[start+1],buffer[start+2]));
	// Cast to command_t
	command= (command_t *) ( buffer +start);

//	LOG_DEBUG(("start: %x ",command->startCode));

//	command_display(command);
	
	// validate (startcode is already ok, or we won't be here)
	if (command->CRC==CMD_STOPCODE){
//		LOG_DEBUG(("Command is valid"));
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
//		LOG_DEBUG(("Invalid Command:"));
//		LOG_DEBUG(("%x %x %x",command->startCode,command->request.command,command->CRC));
		return -1;
	}
}

static int count=1;	/*!< Zaehler fuer Paket-Sequenznummer*/


/*!
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort
 * @param command Kennung zum Command
 * @param subcommand Kennung des Subcommand
 * @param data_l Daten fuer den linken Kanal
 * @param data_r Daten fuer den rechten Kanal
 * @param payload Anzahl der Bytes, die diesem Kommando als Payload folgen
 */
void command_write(uint8 command, uint8 subcommand, int16* data_l,int16* data_r,uint8 payload){
	command_t cmd;
	
	cmd.startCode=CMD_STARTCODE;
	cmd.request.direction=DIR_REQUEST;		// Anfrage
	cmd.request.command= command;
	cmd.request.subcommand= subcommand;
	
	cmd.payload=payload;
    cmd.data_l = (data_l ? *data_l : 0);
    cmd.data_r = (data_r ? *data_r : 0);
	cmd.seq=count++;
	cmd.CRC=CMD_STOPCODE;
	
	low_write(&cmd);
}

/*!
 * Uebertraegt ein Kommando und wartet nicht auf eine Antwort
 * @param command Kennung zum Command
 * @param subcommand Kennung des Subcommand
 * @param data_l Daten fuer den linken Kanal
 * @param data_r Daten fuer den rechten Kanal
 */
//void command_write(uint8 command, uint8 subcommand, int16* data_l,int16* data_r){
//}

/*!
 * Gibt dem Simulator Daten mit Anhang und wartet nicht auf Antwort
 * @param command Kennung zum Command
 * @param subcommand Kennung des Subcommand
 * @param data_l Daten fuer den linken Kanal
 * @param data_r Daten fuer den rechten Kanal
 * @param data Datenanhang an das eigentliche Command
 */
void command_write_data(uint8 command, uint8 subcommand, int16* data_l, int16* data_r, const char* data){
    size_t    len;
	uint8 	payload;    
    
    if (data != NULL) {
        len = strlen(data);
        if (len > MAX_PAYLOAD) {
            payload = MAX_PAYLOAD;
        } else {
            payload = len;
        }
    } else {
        payload = 0;
    }

	command_write(command, subcommand, data_l, data_r,payload);   
    low_write_data((uint8 *)data, payload);
}

#ifdef MAUS_AVAILABLE
	/*!
	 * Uebertraegt ein Bild vom Maussensor an den PC
	 */
	void transmit_mouse_picture(void){
		int16 dummy,i;
	
		int16 pixel;
		uint8 data;
		maus_image_prepare();
		
		for (i=0; i<6; i++) {	
			dummy= i*54 +1;
			command_write(CMD_SENS_MOUSE_PICTURE, SUB_CMD_NORM,  &dummy , &dummy,54);
			for (pixel=0; pixel <54; pixel++){
				data= maus_image_read();
				low_write_data((uint8 *)&data,1);
			}
		}
/*		for (pixel=0; pixel <324; pixel++){
			if (pixel ==0)// Kommando vorbereiten
			if (pixel ==162){ // Kommando vorbereiten
				dummy=163;
				command_write(CMD_SENS_MOUSE_PICTURE, SUB_CMD_NORM,  &dummy , &dummy,0);
			}

			data= maus_image_read();
//			low_write(&data);
		}
*/		
	}
#endif


/*!
 * Wertet das Kommando im Puffer aus
 * return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int command_evaluate(void){
	int analyzed = 1;
	switch (received_command.request.command) {
		case CMD_SENS_RC5:
			ir_data=received_command.data_l;
			break;
		case CMD_AKT_LED:	// LED-Steuerung
			LED_set(received_command.data_l & 255);
			break;
		
		#ifdef MAUS_AVAILABLE		
			case CMD_SENS_MOUSE_PICTURE: 	// PC fragt nach dem Bild
//				LOG_DEBUG(("Request for MouseImage received"));
				transmit_mouse_picture();
			break;
		#endif
			
		// Einige Kommandos ergeben nur fuer simulierte Bots Sinn
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
			case CMD_SENS_MOUSE:
				sensMouseDX=received_command.data_l;
				sensMouseDY=received_command.data_r;
				break;
			case CMD_SENS_ERROR:
				sensError=(char)received_command.data_l;
				sensor_update();	/* Error ist der letzte uebertragene Sensorwert, danach koennen wir uns um allgemeine updates kümmern*/
				break;
		#endif
		default:
			analyzed=0;		// Command was not analysed yet
			break;
	}
	return analyzed;
}



#ifdef LOG_AVAILABLE
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
			LOG_DEBUG(("CMD: %c\tData L: %d\tSeq: %d\n",
				(*command).request.command,
				(*command).data_l,
				(*command).seq));
		#else
			LOG_DEBUG(("%x %x %x",
				(*command).request.command,
				(*command).data_l,
				(*command).seq));

/*			char* raw= (char*)command;
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
*/
			#ifdef WIN32
				printf("\n");
			#endif

		#endif
	}
#endif
#endif
