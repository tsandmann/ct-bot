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
#endif
	
#ifdef PC
	#include "bot-2-sim.h"
	#include "tcp.h"
	#include "tcp-server.h"
	#include <pthread.h>
#endif


#include <string.h>
#include <stdio.h>

#include "global.h"
#include "display.h"
#include "led.h"
#include "key.h"
#include "delay.h"
#include "uart.h"
#include "adc.h"
#include "key.h"
#include "tools.h"
#include "timer.h"


#include "bot-mot.h"
#include "bot-sens.h"
#include "bot-logik.h"
#include "mouse.h"

#include "command.h"
#include "ir.h"
#include "rc5.h"

#ifdef TICKER_AVAILABLE
	#define TICKER_LENGTH 27		///< Länge des Tickers in Zeichen

	char ticks;           			//
	char TICKER_TEXT[TICKER_LENGTH] =" c't-Projekt: c't-Roboter"; ///< Der eigentliche Ticker-Text
	char * ticker = TICKER_TEXT;	///< ein Zeiger auf den Ticker
	char ticker_dir=0;				///< die aktuelle Richtung des Tickers
#endif

/*!
 * Der Mikrocontroller und der PC-Simulator brauchen ein paar Einstellungen, 
 * bevor wir loslegen könen.
 */
void init(void){
	
	#ifdef MCU
		PORTA=0; DDRA=0;		//Alles Eingang alles Null
		PORTB=0; DDRB=0;
		PORTC=0; DDRC=0;
		PORTD=0; DDRD=0;
	#endif

	#ifdef DISPLAY_AVAILABLE
		display_init();
		display_update=1;
	#endif
	
	#ifdef PC
		bot_2_sim_init();
	#endif
	
	bot_mot_init();
	bot_sens_init();
	
	#ifdef LED_AVAILABLE
		LED_init();
	#endif

	#ifdef UART_AVAILABLE	
		uart_init();
	#endif
	
	#ifdef MCU
		timer_0_init();
		timer_1_init();
		timer_2_init();
	#endif
	
	// dreh_init();
	#ifdef RC5_AVAILABLE
		ir_init();
	#endif
	
	#ifdef EVENT_AVAILABLE
		event_init();
	#endif
	
	#ifdef MAUS_AVAILABLE
		maus_sens_init();
	#endif
}

#ifdef DISPLAY_AVAILABLE
/*!
 * Zeige ein paar Infos an
 */
	void display(void){
		char hex[20];
		if (display_update >0){
			#ifdef TICKER_AVAILABLE
				display_cursor(1,1);
				display_string(ticker);
			#endif
			
			#ifdef MAUS_AVAILABLE
				display_cursor(1,1);
				sprintf(hex,"y: %4d",maus_y);
				display_string(hex);
			
				display_cursor(1,10);
				sprintf(hex,"x: %4d",maus_x);
				display_string(hex);
			#endif
			
			display_cursor(2,1);
			display_string("L=");
			sprintf(hex,"%5d",encoderL);
			display_string(hex);			

			display_cursor(2,10);
			display_string("R=");
			sprintf(hex,"%5d",encoderR);
			display_string(hex);			

			#ifdef RC5_AVAILABLE
				display_cursor(3,1);
				display_string(" RC=");
				to_hex((char)(RC5_Code>>8),hex);
				display_string(hex);
				to_hex((char)(RC5_Code & 0xFF),hex);
				display_string(hex);
			#endif		

			
			display_cursor(1,1);
			display_string("dl=");
			sprintf(hex,"%5d",sensDistL);
			display_string(hex);			

			display_cursor(1,10);
			display_string("dr=");
			sprintf(hex,"%5d",sensDistR);
			display_string(hex);	
		}
	}
#endif

#ifdef MCU
/*! 
 * Hauptprogramm des Bots. Diese Schleife kümmert sich um seine Intelligenz
 */
	int main (void){
#endif

#ifdef PC
/*! 
 * Hauptprogramm des Bots. Diese Schleife kümmert sich um seine Intelligenz
 */
 	int main (int argc, char *argv[]){

    if (argc ==2 )    // Test for correct number of arguments
    {
    	printf("ARGV[0]= %s\n",argv[1]);
       tcp_server_init();
       tcp_server_run();
    } else {
    	printf("c't-Bot\n");
    }
    
    
#endif
	init();		
	
	#ifdef WELCOME_AVAILABLE
		display_cursor(1,1);
		display_string("c't-Roboter");
	#endif
	
	/// Hauptschleife des Bots
	for(;;){
				
		#ifdef PC
//			wait_for_time(100000);
			wait_for_time(10000);
		#endif
		
		bot_behave();
		
		#ifdef DISPLAY_AVAILABLE
			display();
		#endif
	}
	
	/// Falls wir das je erreichen sollten ;-)
	return 1;	
}
