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

/*! @file 	ct-Bot.c
 * @brief 	Demo-Hauptprogramm
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include "ct-Bot.h"

#ifdef MCU
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/signal.h>
	#include <avr/wdt.h>
	#include "bot-2-pc.h"
#endif
	
#ifdef PC
	#include "bot-2-sim.h"
	#include "tcp.h"
	#include "tcp-server.h"
	#include <pthread.h>
	#include <unistd.h>
	#include <stdlib.h>
#endif


#include <string.h>
#include <stdio.h>

#include "global.h"
#include "display.h"
#include "led.h"
#include "ena.h"
#include "shift.h"
#include "delay.h"
#include "uart.h"
#include "adc.h"
#include "timer.h"
#include "sensor.h"


#include "motor.h"
#include "sensor-low.h"
#include "bot-logik.h"
#include "mouse.h"

#include "command.h"
#include "ir-rc5.h"
#include "rc5.h"
#include "timer.h"

/* Nimmt den Status von MCUCSR bevor dieses Register auf 0x00 gesetzt wird */
#ifdef DISPLAY_SCREEN_RESETINFO
	unsigned char reset_flag; 
#endif

#ifdef TEST_AVAILABLE_COUNTER
	#include <avr/eeprom.h>
	char resetsEEPROM  __attribute__ ((section (".eeprom")))=0;
	char resetInfoEEPROM  __attribute__ ((section (".eeprom")));
	char resets;
#endif
/*!
 * Der Mikrocontroller und der PC-Simulator brauchen ein paar Einstellungen, 
 * bevor wir loslegen koennen.
 */
void init(void){
	
	#ifdef MCU
		PORTA=0; DDRA=0;		//Alles Eingang alles Null
		PORTB=0; DDRB=0;
		PORTC=0; DDRC=0;
		PORTD=0; DDRD=0;
		
		// Watchdog aus!	
		wdt_disable();
		#ifdef DISPLAY_SCREEN_RESETINFO
			reset_flag = MCUCSR & 0x1F;	//Lese Grund fuer Reset und sichere Wert
			MCUCSR = 0;	//setze Register auf 0x00 (loeschen)
		#endif		
	#endif

	#ifdef PC
		bot_2_sim_init();
	#endif

	#ifdef DISPLAY_AVAILABLE
		display_init();
		display_update=1;
	#endif

	#ifdef LED_AVAILABLE
		LED_init();
	#endif

	motor_init();
	bot_sens_init();
	bot_behave_init();
	
	#ifdef MCU
		#ifdef RC5_AVAILABLE
			ir_init();
		#endif
	#endif

	#ifdef UART_AVAILABLE	
		uart_init();
	#endif
			
	#ifdef MAUS_AVAILABLE
		maus_sens_init();
	#endif

}

#ifdef DISPLAY_AVAILABLE

/*!
 * Zeigt ein paar Informationen an
 */
	void display(void){
		#ifdef TEST_AVAILABLE_COUNTER
			static int counter=0;
		#endif
 		if (display_update >0)
 			#ifdef DISPLAY_SCREENS_AVAILABLE
			switch (display_screen) {
				case 0:
			#endif
					display_cursor(1,1);
					sprintf(display_buf,"P=%03X %03X D=%03d %03d ",sensLDRL,sensLDRR,sensDistL,sensDistR);
					display_buffer();
		
					display_cursor(2,1);
					sprintf(display_buf,"B=%03X %03X L=%03X %03X ",sensBorderL,sensBorderR,sensLineL,sensLineR);
					display_buffer();
		
					display_cursor(3,1);
					sprintf(display_buf,"R=%2d %2d F=%d K=%d T=%d ",sensEncL % 10,sensEncR %10,sensError,sensDoor,sensTrans);
					display_buffer();
		
					display_cursor(4,1);
					sprintf(display_buf,"I=%04X M=%05d %05d",RC5_Code,sensMouseX,sensMouseY);
					display_buffer();				
			#ifdef 	DISPLAY_SCREENS_AVAILABLE					
					break;
				case 1:
					#ifdef TIME_AVAILABLE
						display_cursor(1,1);
						sprintf(display_buf,"Zeit: %04d:%03d",time_s,time_ms);
						display_buffer();
					#endif
		
					display_cursor(2,1);
					sprintf(display_buf,"TS=%+4d %+4d",target_speed_l,target_speed_r);
					display_buffer();
		
					display_cursor(3,1);
					sprintf(display_buf,"RC=%+4d %+4d",sensEncL,sensEncR);
					display_buffer();
		
					display_cursor(4,1);
					sprintf(display_buf,"Speed= %04d %04d",v_left,v_right);
					display_buffer();				

					break;

				case 2:
					display_cursor(1,1);
					sprintf(display_buf,"Screen 3");
					display_buffer();

					#ifdef TEST_AVAILABLE_COUNTER						
						display_cursor(2,1);
						sprintf(display_buf,"count %d",counter++);
						display_buffer();

						display_cursor(3,1);
						sprintf(display_buf,"Reset-Counter %d",resets);
						display_buffer();
					#endif
					break;

				case 3:
					display_cursor(1,1);
					#ifdef DISPLAY_SCREEN_RESETINFO
						/* Zeige den Grund für Resets an */
						sprintf(display_buf,"MCUCSR - Register");
						display_buffer();			
												
						display_cursor(2,1);
						sprintf(display_buf,"PORF :%d  WDRF :%d",binary(reset_flag,0),binary(reset_flag,3)); 
						display_buffer();				

						display_cursor(3,1);
						sprintf(display_buf,"EXTRF:%d  JTRF :%d",binary(reset_flag,1),binary(reset_flag,4)); 
						display_buffer();
									
						display_cursor(4,1);
						sprintf(display_buf,"BORF :%d",binary(reset_flag,2)); 
					#else
						sprintf(display_buf,"Screen 4");
					#endif
					display_buffer();
					break;
			}
			#endif	
	}
#endif

#ifdef TEST_AVAILABLE
	/*! Zeigt den internen Status der Sensoren mit den LEDs an */
	void show_sensors(void){
		char led=0x00;
		led_t * status = (led_t *)&led;
		#ifdef TEST_AVAILABLE_ANALOG
			(*status).rechts	= (sensDistR >> 9) & 0x01;
			(*status).links		= (sensDistL >> 9) & 0x01;
			(*status).rot		= (sensLineL >> 9) & 0x01;
			(*status).orange	= (sensLineR >> 9) & 0x01;
			(*status).gelb		= (sensLDRL >> 9) & 0x01;
			(*status).gruen		= (sensLDRR >> 9) & 0x01;
			(*status).tuerkis	= (sensBorderL >> 9) & 0x01;
			(*status).weiss		= (sensBorderR >> 9) & 0x01;
		#endif
		#ifdef TEST_AVAILABLE_DIGITAL
			(*status).rechts	= sensEncR  & 0x01;
			(*status).links		= sensEncL  & 0x01;
			(*status).rot		= sensTrans & 0x01;
			(*status).orange	= sensError & 0x01;
			(*status).gelb		= sensDoor  & 0x01;
			(*status).gruen		= (sensMouseDX >>1)  & 0x01;
			(*status).tuerkis	= (sensMouseDY >>1) & 0x01;
			(*status).weiss		= RC5_Code & 0x01;
		#endif
				
		LED_set(led);
	}
#endif

#ifdef PC
	/*!
	 * Zeigt Informationen zu den moeglichen Kommandozeilenargumenten an.
	 * Das Programm wird nach Anzeige des Hilfetextes per exit() beendet.
	 */
	void usage(void){
		puts("USAGE: ct-Bot [-t host] [-h] [-s]");
		puts("\t-t\tHostname oder IP Adresse zu der Verbunden werden soll");
		puts("\t-s\tServermodus");
		puts("\t-h\tZeigt diese Hilfe an");
		exit(1);
	}
#endif

#ifdef MCU
/*! 
 * Hauptprogramm des Bots. Diese Schleife kuemmert sich um seine Steuerung.
 */
	int main (void){

#endif

#ifdef PC

/*! 
 * Hauptprogramm des Bots. Diese Schleife kuemmert sich um seine Steuerung.
 */
 	int main (int argc, char *argv[]){

		int ch;	
		int start_server = 0;	/*!< Wird auf 1 gesetzt, falls -s angegeben wurde */
		char *hostname = NULL;	/*!< Speichert den per -t uebergebenen Hostnamen zwischen */

		// Die Kommandozeilenargumente komplett verarbeiten
		while ((ch = getopt(argc, argv, "hst:")) != -1) {
			switch (ch) {
			case 's':
				// Servermodus [-s] wird verlangt
				start_server = 1;
				break;
			case 't':
				// Hostname, auf dem ct-Sim laeuft wurde 
				// uebergeben. Der String wird in hostname
				// gesichert.
				{
					const int len = strlen(optarg);
					hostname = malloc(len + 1);
					if (NULL == hostname)
						exit(1);
					strcpy(hostname, optarg);
				}
				break;
			case 'h':
			default:
				// -h oder falscher Parameter, Usage anzeigen
				usage();
			}
		}
		argc -= optind;
		argv += optind;
		
	if (start_server != 0)    // Soll der TCP-Server gestartet werden?
    {
    	printf("ARGV[0]= %s\n",argv[1]);
       tcp_server_init();
       tcp_server_run();
    } else {
    	printf("c't-Bot\n");
        if (hostname)
            // Hostname wurde per Kommandozeile uebergeben
            tcp_hostname = hostname;
        else {
            // Der Zielhost wird per default durch das Macro IP definiert und
            // tcp_hostname mit einer Kopie des Strings initialisiert.
            tcp_hostname = malloc(strlen(IP) + 1);
            if (NULL == tcp_hostname)
                exit(1);
            strcpy(tcp_hostname, IP);
        }
    }
    
    
#endif
	#ifdef  TEST_AVAILABLE_MOTOR
		uint16 calls=0;	/*!< Im Testfall zaehle die Durchlaeufe */
	#endif

	init();		
	
	#ifdef WELCOME_AVAILABLE
		display_cursor(1,1);
		sprintf(display_buf,"c't-Roboter");
		display_buffer();			
		LED_set(0x00);
		
		#ifdef BOT_2_PC_AVAILABLE		
			uart_write(display_buf,11);
		#endif
	#endif
	
	#ifdef TEST_AVAILABLE_COUNTER
		display_screen=2;

	 	resets=eeprom_read_byte(&resetsEEPROM)+1;
	    eeprom_write_byte(&resetsEEPROM,resets);
	    /* Lege den Grund für jeden Reset im EEPROM ab */	
	    eeprom_write_byte(&resetInfoEEPROM+resets,reset_flag);
	#endif	
	/*! Hauptschleife des Bot */
	for(;;){
		#ifdef MCU
			bot_sens_isr();
			#ifdef TEST_AVAILABLE
				show_sensors();
			#endif
		#endif

		// Testprogramm, dass den Bot erst links, dann rechtsrum dreht
		#ifdef  TEST_AVAILABLE_MOTOR
			calls++;
			if (calls == 1)
				motor_set(BOT_SPEED_SLOW,-BOT_SPEED_SLOW);
			else if (calls == 501)
				motor_set(-BOT_SPEED_SLOW,BOT_SPEED_SLOW);
			else if (calls== 1001)
				motor_set(BOT_SPEED_STOP,BOT_SPEED_STOP);
			else
		#endif
		// hier drin steckt der Verhaltenscode
		#ifdef BEHAVIOUR_AVAILABLE
			bot_behave();
		#endif
			
		#ifdef MCU
			#ifdef BOT_2_PC_AVAILABLE
				static int16 lastTimeCom =0;
	
				if (time_s != lastTimeCom) {	// sollte genau 1x pro Sekunde zutreffen
					lastTimeCom = time_s;		
					bot_2_pc_listen();				// Kommandos vom PC empfangen
					bot_2_pc_inform();				// Den PC ueber Sensorern und aktuatoren informieren
				}
			#endif
		#endif
		// Alles Anzeigen
		#ifdef DISPLAY_AVAILABLE
			display();
		#endif
		
		#ifdef PC
			wait_for_time(100000);
		#endif
		#ifdef MCU
//			delay(10);
		#endif
	}
	
	/*! Falls wir das je erreichen sollten ;-) */
	return 1;	
}
