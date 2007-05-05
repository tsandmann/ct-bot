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
#include "rc5-codes.h"
#include "bot-logic/bot-logik.h"
#include "bot-2-pc.h"
#include "uart.h"

#include <stdio.h>
#include <string.h>

#ifdef PC
	#include "tcp.h"
	#include <pthread.h>	
#endif

#ifdef MCU
	#ifdef NEW_AVR_LIB
		#include <util/delay.h>
	#else
		#include <avr/delay.h>
	#endif
#endif

#define COMMAND_TIMEOUT 	10		/*!< Anzahl an ms, die maximal auf fehlende Daten gewartet wird */

#ifdef COMMAND_AVAILABLE

#define RCVBUFSIZE (sizeof(command_t)*2)   /*!< Groesse des Empfangspuffers */

command_t received_command;		/*!< Puffer fuer Kommandos */

#ifdef PC
	// Auf dword alignment bestehen, wird fuer MacOS X benoetigt
	pthread_mutex_t     command_mutex __attribute__ ((aligned (4)))
		= PTHREAD_MUTEX_INITIALIZER;
#endif


//#define DEBUG_COMMAND		//Schalter, um auf einmal alle Debugs an oder aus zu machen

#ifndef DEBUG_COMMAND
	#undef LOG_DEBUG
	#define LOG_DEBUG(a, ...) {}
#endif
/*!
 * Liest ein Kommando ein, ist blockierend!
 * Greift auf low_read() zurueck
 * Achtung, die Payload wird nicht mitgelesen!!!
 * @see low_read()
 */
int8 command_read(void){
	int bytesRcvd;
	int start=0;			// start des Kommandos
	int i;			
	command_t * command;	// Pointer zum Casten der empfangegen Daten
	char * ptr;				// Nur zu Hilfszwecken
	uint8 buffer[RCVBUFSIZE];       // Buffer  
	#ifdef PC
		#if BYTE_ORDER == BIG_ENDIAN
			uint16 store;			//Puffer für die Endian-Konvertierung
		#endif
	#endif
	
	uint16 old_ticks;			// alte Systemzeit

	buffer[0]=0;				// Sicherheitshalber mit sauberem Puffer anfangen
	
	// Daten holen, maximal soviele, wie ein Kommando lang ist
	bytesRcvd=low_read(buffer,sizeof(command_t));	

	LOG_DEBUG("%d read",bytesRcvd);
	LOG_DEBUG("%x %x %x",buffer[0],buffer[1],buffer[2]);

	// Suche nach dem Beginn des Frames
	while ((start<bytesRcvd)&&(buffer[start] != CMD_STARTCODE)) {	
		LOG_DEBUG("falscher Startcode");
//		printf("\nStartzeichen nicht am Anfang des Puffers! (%d)\n",start);
//		printf(".");
		start++;
	}
		
	// Wenn keine STARTCODE gefunden ==> Daten verwerfen
	if (buffer[start] != CMD_STARTCODE){
		LOG_DEBUG("kein Startcode");
		return -1;	
	}
	
	LOG_DEBUG("Start @%d",start);
	
	// haben wir noch genug Platz im Puffer, um das Packet ferig zu lesen?
	if ((RCVBUFSIZE-start) < sizeof(command_t)){
		LOG_DEBUG("not enough space");
//		printf("not enough space");
		return -1;	// nein? ==> verwerfen
	}
	
	i=sizeof(command_t) - (bytesRcvd-start)-1;

	
	if (i> 0) {	// Fehlen noch Daten ?
		LOG_DEBUG("command.c: Start @ %d es fehlen %d bytes ",start,i);	
		// Systemzeit erfassen
		old_ticks = TIMER_GET_TICKCOUNT_16;
				
		// So lange Daten lesen, bis das Packet vollstaendig ist, oder der Timeout zuschlaegt
		while (i > 0){
			// Wenn der Timeout ueberschritten ist
			if (timer_ms_passed(&old_ticks, COMMAND_TIMEOUT)) {
				LOG_DEBUG("Timeout beim nachlesen");
				return -1; //	==> Abbruch
			}
			LOG_DEBUG("%d bytes missing",i);
			i= low_read(buffer+bytesRcvd,i);
			LOG_DEBUG("%d read",i);
			bytesRcvd+=i;
			i=sizeof(command_t) - (bytesRcvd-start);
		}
	}
	
	LOG_DEBUG("%d/%d read/start",bytesRcvd,start);
	LOG_DEBUG("%x %x %x",buffer[start],buffer[start+1],buffer[start+2]);

	// Cast in command_t
	command= (command_t *) ( buffer +start);

	LOG_DEBUG("start: %x ",command->startCode);
	//	command_display(command);
	
	// validate (startcode ist bereits ok, sonst waeren wir nicht hier )
	if (command->CRC==CMD_STOPCODE){
		LOG_DEBUG("Command is valid");
		// Transfer
		#ifdef PC
			command_lock();		// on PC make storage threadsafe
		#endif
		ptr = (char *) &received_command;
		for (i=0; i<sizeof(command_t);i++){
			*ptr=buffer[i+start];
			ptr++;
		}
		#ifdef PC
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
		
			command_unlock();	// on PC make storage threadsafe
		#endif

		return 0;
	} else {	// Command not valid
		LOG_DEBUG("Invalid Command:");
		LOG_DEBUG("%x %x %x",command->startCode,command->request.command,command->CRC);
		return -1;
	}
}

static uint16 count=1;	/*!< Zaehler fuer Paket-Sequenznummer*/

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
	if (data_l != NULL)
		cmd.data_l = *data_l;
	else
		cmd.data_l = 0;
	 
	if (data_r != NULL)
    	cmd.data_r = *data_r;
    else
		cmd.data_r = 0;
    
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
 * @param payload Anzahl der Bytes im Anhang
 * @param data Datenanhang an das eigentliche Command
 */
void command_write_rawdata(uint8 command, uint8 subcommand, int16* data_l, int16* data_r, uint8 payload, uint8* data){
	command_write(command, subcommand, data_l, data_r,payload);   
    low_write_data(data, payload);
}


/*!
 * Gibt dem Simulator Daten mit String-Anhang und wartet nicht auf Antwort
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
				#ifdef MCU
					#if BAUDRATE > 17777	// Grenzwert: 8 Bit / 450 us = 17778 Baud
//						_delay_loop_2(1800);	// warten, weil Sendezeit < Maussensordelay (450 us)
						_delay_loop_2(3600);	// warten, weil Sendezeit < Maussensordelay (450 us)
					#endif
				#endif
			}
		}
	}
#endif


/*!
 * Wertet das Kommando im Puffer aus
 * return 1, wenn Kommando schon bearbeitet wurde, 0 sonst
 */
int command_evaluate(void){
	static uint16 RC5_Last_Toggle = 0xffff;
	uint8 analyzed = 1;
	
	#ifdef LOG_AVAILABLE	
		command_display(&received_command);
	#endif	// LOG_AVAILABLE
	
	switch (received_command.request.command) {
		#ifdef IR_AVAILABLE
			case CMD_SENS_RC5:
				ir_data=received_command.data_l | (RC5_Last_Toggle & RC5_TOGGLE);
				if (received_command.data_l != 0)
					RC5_Last_Toggle = 0xffff ^ (RC5_Last_Toggle & RC5_TOGGLE);
				break;
		#endif
		case CMD_AKT_LED:	// LED-Steuerung
			#ifdef LED_AVAILABLE
				LED_set(received_command.data_l & 255);
			#endif
			break;
			
		// Einige Kommandos ergeben nur fuer reale Bots Sinn
			case CMD_WELCOME:
				#ifdef MCU
					command_write(CMD_WELCOME, SUB_WELCOME_REAL,0,0,0);
				#else
					command_write(CMD_WELCOME, SUB_WELCOME_SIM,0,0,0);
				#endif					
				break;

		
		#ifdef MAUS_AVAILABLE		
			case CMD_SENS_MOUSE_PICTURE: 	// PC fragt nach dem Bild
				transmit_mouse_picture();
			break;
		#endif

		#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
			case CMD_REMOTE_CALL:
					LOG_DEBUG("remote-call-cmd ...");					
					switch (received_command.request.subcommand) {
						case SUB_REMOTE_CALL_LIST:
							LOG_DEBUG("... auflisten ");					
							remote_call_list();
							break;
						case SUB_REMOTE_CALL_ORDER:
						{	
							LOG_DEBUG("remote-call-Wunsch empfangen. Data= %d bytes",received_command.payload);					
							uint8 buffer[REMOTE_CALL_BUFFER_SIZE];
							uint16 ticks = TIMER_GET_TICKCOUNT_16;
							#ifdef MCU
								while (uart_data_available() < received_command.payload && (uint16)(TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT));
							#endif
							low_read(buffer,received_command.payload);
							if ((uint16)(TIMER_GET_TICKCOUNT_16 - ticks) < MS_TO_TICKS(COMMAND_TIMEOUT)){ 	
								bot_remotecall_from_command((uint8 *)&buffer);
							} else{
								int16 result = SUBFAIL;
								command_write_data(CMD_REMOTE_CALL,SUB_REMOTE_CALL_DONE,&result,&result,NULL);
							}
							break;
						}
						case SUB_REMOTE_CALL_ABORT: {
							LOG_DEBUG("remote calls werden abgebrochen");
							deactivateCalledBehaviours(bot_remotecall_behaviour);
							break;
						}
						
						
						default:
							LOG_DEBUG("unbekanntes Subkommando: %c",received_command.request.subcommand);
							break;
					}
				break;
		#endif

			
		// Einige Kommandos ergeben nur fuer simulierte Bots Sinn
		#ifdef PC
			case CMD_SENS_IR: {
				(*sensor_update_distance)(&sensDistL, &sensDistLToggle, sensDistDataL, received_command.data_l*4);
				(*sensor_update_distance)(&sensDistR, &sensDistRToggle, sensDistDataR, received_command.data_r*4);
				break;
			}
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
			#ifdef MAUS_AVAILABLE
				case CMD_SENS_MOUSE:
					sensMouseDX=received_command.data_l;
					sensMouseDY=received_command.data_r;
					break;
			#endif
			case CMD_SENS_ERROR:
				sensError=(char)received_command.data_l;
				sensor_update();	/* Error ist der letzte uebertragene Sensorwert, danach koennen wir uns um allgemeine updates kümmern*/
				break;
			case CMD_DONE:
				simultime=received_command.data_l;
				system_time_isr();		/* Einmal pro Update-Zyklus aktualisieren wir die Systemzeit */
			//	printf("X-Frame for Simultime = %d received ",simultime);
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
			LOG_DEBUG("CMD: %c\tData L: %d\tSeq: %d\n",
				(*command).request.command,
				(*command).data_l,
				(*command).seq);
		#else
			LOG_DEBUG("CMD: %c\tSub: %c\tData L: %d\tPay: %d\tSeq: %d\n",
				(*command).request.command,
				(*command).request.subcommand,
				(*command).data_l,
				(*command).payload,
				(*command).seq);

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
