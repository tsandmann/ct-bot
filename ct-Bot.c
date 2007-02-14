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
 * @file 	ct-Bot.c
 * @brief 	Bot-Hauptprogramm
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#include "ct-Bot.h"

#ifdef MCU
	#include <avr/io.h>
	#include <avr/interrupt.h>
//	#include <avr/signal.h>
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
	#include <time.h>
	#include <sys/time.h>
#endif

#ifdef TWI_AVAILABLE
	#include "TWI_driver.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
#include "log.h"

#include "motor.h"
#include "sensor-low.h"
#include "bot-logic/bot-logik.h"
#include "mouse.h"

#include "command.h"
#include "ir-rc5.h"
#include "rc5.h"
#include "timer.h"
#include "mmc.h"
#include "map.h"
#include "mmc-emu.h"
#include "mini-fat.h"
#include "mmc-vm.h"
#include "gui.h"
#include "ui/available_screens.h"

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
			
		wdt_disable();	// Watchdog aus!
		timer_2_init();
		
		/* Ist das ein Power on Reset? */
		#ifdef __AVR_ATmega644__
			if ((MCUSR & 1) == 1) {
				MCUSR &= ~1;	// Bit loeschen
		#else
			if ((MCUCSR & 1) == 1) {
				MCUCSR &= ~1;	// Bit loeschen
		#endif
			delay(100);
			asm volatile("jmp 0");
		}
		
		delay(100);	
		#ifdef RESET_INFO_DISPLAY_AVAILABLE
			#ifdef __AVR_ATmega644__
				reset_flag = MCUSR & 0x1F;	//Lese Grund fuer Reset und sichere Wert
				MCUSR = 0;	//setze Register auf 0x00 (loeschen)
			#else
				reset_flag = MCUCSR & 0x1F;	//Lese Grund fuer Reset und sichere Wert
				MCUCSR = 0;	//setze Register auf 0x00 (loeschen)
			#endif
		#endif		
	#endif

	#ifdef UART_AVAILABLE
		uart_init();
	#endif

	#ifdef BOT_2_PC_AVAILABLE
		bot_2_pc_init();
	#endif

	#ifdef PC
		bot_2_sim_init();
	#endif

	#ifdef DISPLAY_AVAILABLE
		display_init();
//		display_update=1;	// wird nie ausgewertet?!?
	#endif

	#ifdef LED_AVAILABLE
		LED_init();
	#endif

	motor_init();
	bot_sens_init();
	#ifdef BEHAVIOUR_AVAILABLE
		bot_behave_init();
	#endif
	
	#ifdef MCU
		#ifdef RC5_AVAILABLE
			ir_init();
		#endif
	#endif

	#ifdef MMC_AVAILABLE
		mmc_init();
	#endif
			
	#ifdef MAUS_AVAILABLE
		maus_sens_init();
	#endif
	
	#ifdef MAP_AVAILABLE
		map_init();
	#endif
	
	#ifdef LOG_MMC_AVAILABLE
		log_mmc_init();
	#endif

	#ifdef TWI_AVAILABLE
		Init_TWI();
		Close_TWI();
	#endif
	
	#ifdef DISPLAY_AVAILABLE
		gui_init();
	#endif	
}

#ifdef TEST_AVAILABLE
	/*! Zeigt den internen Status der Sensoren mit den LEDs an */
	void show_sensors(void){
		uint8 led=0x00;
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
			#ifdef MAUS_AVAILABLE
				(*status).gruen		= (sensMouseDX >>1)  & 0x01;
				(*status).tuerkis	= (sensMouseDY >>1) & 0x01;
			#endif
			#ifdef RC5_AVAILABLE
				(*status).weiss		= RC5_Code & 0x01;
			#endif
		#endif
				
		LED_set(led);
	}
#endif	// TEST_AVAILABLE

#ifdef PC
	/*!
	 * Zeigt Informationen zu den moeglichen Kommandozeilenargumenten an.
	 * Das Programm wird nach Anzeige des Hilfetextes per exit() beendet.
	 */
	void usage(void){
		puts("USAGE: ct-Bot [-t host] [-T] [-h] [-s] [-M from] [-c FILE ID SIZE]");
		puts("\t-t\tHostname oder IP Adresse zu der Verbunden werden soll");
		puts("\t-T\tTestClient");
		puts("\t-s\tServermodus");
		puts("\t-M from\tKonvertiert eine Bot-map in eine PGM-Datei");
		puts("\t-c \tErzeugt eine Mini-Fat-Datei fuer den Bot.");		
		puts("\t   FILE\tDateiname");
		puts("\t   ID  \tDie ID aus ASCII-Zeichen");
		puts("\t   SIZE\tDie Nutzgroesse der Datei in KByte");
		puts("\t-l \tKonvertiert eine SpeedLog-Datei in eine txt-Datei");
		puts("\t-h\tZeigt diese Hilfe an");
		exit(1);
	}
#endif	// PC

#ifdef MCU
	/*! 
 	 * Hauptprogramm des Bots. Diese Schleife kuemmert sich um seine Steuerung.
 	 */
	int main (void){
#endif	// MCU

#ifdef PC
	/*! 
 	 * Hauptprogramm des Bots. Diese Schleife kuemmert sich um seine Steuerung.
 	 */
 	int main (int argc, char *argv[]){
		/* zum Debuggen der Zeiten: */	
		#ifdef DEBUG_TIMES
			struct timeval    start, stop;
		#endif

		int ch;	
		int start_server = 0;	/*!< Wird auf 1 gesetzt, falls -s angegeben wurde */
		int start_test_client =0; /*!< Wird auf 1 gesetzt, falls -T angegeben wurde */
		char *hostname = NULL;	/*!< Speichert den per -t uebergebenen Hostnamen zwischen */

		int convert =0; /*!< Wird auf 1 gesetzt, wenn die Karte konvertiert werden soll */
		int create  =0;  /*!< Wird auf 1 gesetzt, wenn eine neue Datei fuer Bot-mini-fat erzeugt werden soll */
		int slog	=0;
		char *from = NULL;	/*!< Speichert den per -M uebergebenen Quellnamen zwischen */


		/* Die Kommandozeilenargumente komplett verarbeiten */
		while ((ch = getopt(argc, argv, "hsTt:M:c:l:")) != -1) {
			switch (ch) {
			case 's':
				/* Servermodus [-s] wird verlangt */
				start_server = 1;
				break;
			case 'T': 	
				start_test_client=1;
				break;		
			case 't':
				/* Hostname, auf dem ct-Sim laeuft wurde uebergeben. Der String wird in hostname gesichert. */
				{
					const int len = strlen(optarg);
					hostname = malloc(len + 1);
					if (NULL == hostname)
						exit(1);
					strcpy(hostname, optarg);
				}
				break;
			case 'M':
				/* Dateiname fuer die Map wurde uebergeben. Der String wird in from gesichert. */
				{
					int len = strlen(optarg);
					from = malloc(len + 1);
					if (NULL == from)
						exit(1);
					strcpy(from, optarg);
					
					convert=1;					
				}
				break;
			case 'c':
				/* Datei fuer den Bot (mini-fat soll erzeugt werden. */
				{
					int len = strlen(optarg);
					from = malloc(len + 1);
					if (NULL == from)
						exit(1);
					strcpy(from, optarg);
					create=1;					
				}
				break;	
			case 'l':
				/* Speedlog-Datei soll in txt konvertiert werden */
				{
					int len = strlen(optarg);
					from = malloc(len + 1);
					if (NULL == from)
						exit(1);
					strcpy(from, optarg);
					slog=1;					
				}				
				break;			
			case 'h':
			default:
				/* -h oder falscher Parameter, Usage anzeigen */
				usage();
			}
		}
		argc -= optind;
		argv += optind;
		
	if (start_server != 0) {   // Soll der TCP-Server gestartet werden?
		printf("ARGV[0]= %s\n",argv[0]);
		tcp_server_init();
		tcp_server_run(100);
    } else {
 		#ifdef MAP_AVAILABLE
	 		/* Karte in pgm konvertieren */
	    	if (convert !=0) {
				printf("Konvertiere Karte %s in PGM %s\n",from,"map.pgm");
	   		 	read_map(from);
	   		 	map_to_pgm("map.pgm");
	   		 	exit(0);
	       	}     	
		#endif	// MAP_AVAILABLE    	
		if (slog != 0){
			convert_slog_file(from);
			exit(0);	
		}
    	if (create !=0) {
    			printf("optind= %d argc=%d\n",optind, argc);
			if (argc != 2){
				usage();
				exit(1);
		    	}

    			char * id;
    			id = malloc(strlen(argv[0]));	
    			strcpy(id,argv[0]);

    			char * s;
    			s = malloc(strlen(argv[1]));	
    			strcpy(s,argv[1]);
		
			int size = atoi(s);

   		 	printf("Erstelle eine Mini-Fat-Datei (%s) mit %d kByte fuer den Bot. ID=%s \n",from,size,id);
   		 	create_mini_fat_file(from,id,size);
//	   		read_map(from);
//	   		map_to_pgm("map.pgm");
	   		exit(0);
		}

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
        
        if (start_test_client !=0) {
	       tcp_test_client_init();
	       tcp_test_client_run(100);
        }
    }
#endif	// PC
#ifdef  TEST_AVAILABLE_MOTOR
	uint16 calls=0;	/*!< Im Testfall zaehle die Durchlaeufe */
#endif

init();	// alles initialisieren

#ifdef WELCOME_AVAILABLE
	display_cursor(1,1);
	display_printf("c't-Roboter");
	LED_set(0x00);
	#ifdef LOG_AVAILABLE
		LOG_DEBUG(("Hallo Welt!"));
	#endif	
#endif

//TODO: what's that?!?
#ifdef TEST_AVAILABLE_COUNTER
//	display_screen=2;
 	resets=eeprom_read_byte(&resetsEEPROM)+1;
    eeprom_write_byte(&resetsEEPROM,resets);
    /* Lege den Grund fuer jeden Reset im EEPROM ab */	
    eeprom_write_byte(&resetInfoEEPROM+resets,reset_flag);
#endif	
	
/* Hauptschleife des Bots */
for(;;){
	#ifdef PC
		receive_until_Frame(CMD_DONE);
		#ifdef DEBUG_TIMES
			//Zum debuggen der Zeiten:	
	 		GETTIMEOFDAY(&start, NULL);
			int t1=(start.tv_sec - stop.tv_sec)*1000000 + start.tv_usec - stop.tv_usec;
			printf("Done-Token (%d) in nach %d usec ",received_command.data_l,t1);
		#endif	// DEBUG_TIMES
	#endif	// PC
		
	#ifdef MCU
		bot_sens_isr();
	#endif
	#ifdef TEST_AVAILABLE
		show_sensors();
	#endif

	/* Testprogramm, das den Bot erst links, dann rechtsrum dreht */
	#ifdef TEST_AVAILABLE_MOTOR
		calls++;
		if (calls == 1)
			motor_set(BOT_SPEED_SLOW,-BOT_SPEED_SLOW);
		else if (calls == 501)
			motor_set(-BOT_SPEED_SLOW,BOT_SPEED_SLOW);
		else if (calls== 1001)
			motor_set(BOT_SPEED_STOP,BOT_SPEED_STOP);
		else
	#endif	// TEST_AVAILABLE_MOTOR
	
	/* hier drin steckt der Verhaltenscode */
	#ifdef BEHAVIOUR_AVAILABLE
		if (sensors_initialized == 1)
			bot_behave();
//		#ifdef LOG_AVAILABLE
//		else
//			LOG_DEBUG(("sens not init"));
//		#endif
	#endif	// BEHAVIOUR_AVAILABLE
			
	#ifdef MCU
		#ifdef BOT_2_PC_AVAILABLE
//			static int16 lastTimeCom =0;
			bot_2_pc_inform();				// Den PC ueber Sensorern und aktuatoren informieren
			bot_2_pc_listen();				// Kommandos vom PC empfangen
				
//			if (timer_get_s() != lastTimeCom) {	// sollte genau 1x pro Sekunde zutreffen
//				lastTimeCom = timer_get_s();		
//			}
		#endif	// BOT_2_PC_AVAILABLE
	#endif	// MCU
		
//	#ifdef LOG_AVAILABLE
//		LOG_DEBUG(("BOT TIME %d s", timer_get_s()));
//	#endif	
		
	/* GUI-Behandlung starten */
	#ifdef DISPLAY_AVAILABLE
		gui_display(display_screen);
	#endif
	
	#ifdef PC
		command_write(CMD_DONE, SUB_CMD_NORM ,(int16*)&simultime,0,0);
		flushSendBuffer();
		/* Zum debuggen der Zeiten: */	
		#ifdef DEBUG_TIMES
			GETTIMEOFDAY(&stop, NULL);
 			int t2=(stop.tv_sec - start.tv_sec)*1000000 +stop.tv_usec - start.tv_usec;
			printf("Done-Token (%d) out after %d usec\n",simultime,t2);
		#endif	// DEBUG_TIMES
	#endif	// PC	
	}
	
	/*! Falls wir das je erreichen sollten ;-) */
	return 1;	
}
