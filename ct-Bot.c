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

/* Nimmt den Status von MCUCSR bevor dieses Register auf 0x00 gesetzt wird */
#ifdef DISPLAY_SCREEN_RESETINFO
	unsigned char reset_flag; 
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
	
	

/*	#ifdef UART_AVAILABLE	
		uart_init();
	#endif
*/			
	#ifdef MAUS_AVAILABLE
		maus_sens_init();
	#endif

}

#ifdef DISPLAY_AVAILABLE

/*!
 * Zeigt ein paar Informationen an
 */
	void display(void){
 		if (display_update >0)
 			#ifdef DISPLAY_SCREENS_AVAILABLE
			switch (display_screen) {
				case 0:
			#endif
					display_cursor(1,1);
					sprintf(display_buf,"P=%03X %03X D=%03X %03X ",sensLDRL,sensLDRR,sensDistL,sensDistR);
					display_buffer();
		
					display_cursor(2,1);
					sprintf(display_buf,"B=%03X %03X L=%3X %03X ",sensBorderL,sensBorderR,sensLineL,sensLineR);
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
					display_cursor(1,1);
					sprintf(display_buf,"Screen 2");
					display_buffer();
		
					display_cursor(2,1);
					sprintf(display_buf,"TS=%+4d %+4d",target_speed_l,target_speed_r);
					display_buffer();
		
					display_cursor(3,1);
					sprintf(display_buf,"RC=%+4d %+4d",sensEncL,sensEncR);
					display_buffer();
		
					display_cursor(4,1);
					sprintf(display_buf,"I=%04X",RC5_Code);
					display_buffer();				

					break;

				case 2:
					display_cursor(1,1);
					sprintf(display_buf,"Screen 3");
					display_buffer();

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

    if (argc ==2 )    // Test for correct number of arguments
    {
    	printf("ARGV[0]= %s\n",argv[1]);
       tcp_server_init();
       tcp_server_run();
    } else {
    	printf("c't-Bot\n");
    }
    
    
#endif
	#ifdef  TEST_AVAILABLE_MOTOR
		uint16 calls=0;	/*!< Im Testfall zaehle die Durchlaeufe */
	#endif

	init();		
	
	#ifdef WELCOME_AVAILABLE
		display_cursor(1,1);
		display_string("c't-Roboter");
		LED_set(0x00);
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
		bot_behave();
		
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
