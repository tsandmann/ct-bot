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

/*! @file 	display.c
 * @brief 	Routinen zur Displaysteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#include "global.h"
#include "ct-Bot.h"

#ifdef MCU
#ifdef DISPLAY_AVAILABLE

#include <avr/io.h>
#ifdef NEW_AVR_LIB
	#include <util/delay.h>
#else
	#include <avr/delay.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include "command.h"

#include "display.h"
#include "led.h"
#include "delay.h"
#include "shift.h"
#include "display.h"

/*! Puffergroesse fuer eine Zeile in bytes */
#define DISPLAY_BUFFER_SIZE	(DISPLAY_LENGTH + 1)

//uint8 display_update=0;	/*!< Muss das Display aktualisiert werden? */
uint8 display_screen=0;	/*!< zurzeit aktiver Displayscreen */
static char display_buf[DISPLAY_BUFFER_SIZE];	/*!< Pufferstring fuer Displayausgaben */

#define DISPLAY_CLEAR 0x01		/*!< Kommando zum Loeschen */
#define DISPLAY_CURSORHOME 0x02	/*!< Kommando fuer den Cursor */

#define DISPLAY_OUT 			0x07		/*!< Output-Pins Display */
#define DISPLAY_IN 			(1<<5)		/*!< Input-Pins Display */

#define DISPLAY_PORT			PORTC		/*!< Port an dem das Display haengt */
#define DISPLAY_DDR				DDRC		/*!< Port an dem das Display haengt */
#define DPC (DISPLAY_PORT & ~DISPLAY_OUT)	/*!< Port des Displays */
//#define DRC (DDRC & ~DISPLAY_PINS)

//#define DISPLAY_READY_PINR		PINC		/*!< Port an dem das Ready-Flag des Display haengt */
#define DISPLAY_READY_DDR		DDRC		/*!< Port an dem das Ready-Flag des Display haengt */
#define DISPLAY_READY_PIN		(1<<5)		/*!< Pin  an dem das Ready-Flag des Display haengt */

/*! RS-Leitung 
 * legt fest, ob die Daten an das Display in den Textpuffer (RS=1) kommen
 * oder als Steuercode interpretiert werden (RS=0)
 */
#define DISPLAY_RS				(1<<0)		/*!< Pin an dem die RS-Leitung des Displays haengt */

/*! RW-Leitung
 * legt fest, ob zum Display geschrieben wird (RW=0)
 * oder davon gelesen wird (RW=1)
 */
#define DISPLAY_RW				(1<<1)		/*!< Pin an dem die RW-Leitung des Displays haengt */

/*! Enable Leitung 
 * schaltet das Interface ein (E=1). 
 * Nur wenn Enable auf High-Pegel liegt, laesst sich das Display ansprechen
 */
#define DISPLAY_EN				(1<<2)		/*!< Pin an dem die EN-Leitung des Displays haengt */

/*
 * Im Moment der Low-High-Flanke von ENABLE liest das Dislplay 
 * die Werte von RS und R/W ein. Ist zu diesem Zeitpunkt R/W=0, 
 * dann liest das Display mit der folgenden High-Low-Flanke von ENABLE 
 * den Datenbus ein (Schreibzyklus). 
 * War aber R/W=1, dann legt das Display ein Datenword auf den 
 * Datenbus (Lese-Zyklus), solange bis die High-Low-Flanke von ENABLE 
 * das Interface wieder deaktiviert.
 */

#ifdef DISPLAY_REMOTE_AVAILABLE
	static uint8 remote_column = 0;
	static uint8 remote_row = 0;
#endif

/*! 
 * Übertrage Kommando an das Display
 * @param cmd Kommando
 */
void display_cmd(uint8 cmd){		//ein Kommando cmd an das Display senden
	uint8 i;
	shift_data_out(cmd,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY);
	// Enable muss für mind. 450 ns High bleiben, bevor es fallen darf!
	// ==> Also mind. 8 Zyklen warten
	for (i=0; i<150; i++){
	        asm volatile("nop");
	}
	DISPLAY_PORT=DPC;	// Alles zurück setzen ==> Fallende Flanke von Enable
}


/*! 
 * Ein Zeichen auf das Display schreiben
 * @param data Das Zeichen
 */
void display_data(char data){ //ein Zeichen aus data in den Displayspeicher schreiben
        uint8 i;
		shift_data_out(data,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY|DISPLAY_RS);
		
		// Enable muss für mind. 450 ns High bleiben, bevor es fallen darf!
		// ==> Also mind. 8 Zyklen warten
        for (i=0; i<150; i++){
                asm volatile("nop");
        }
      DISPLAY_PORT=DPC;	// Alles zurueck setzen ==> Fallende Flanke von Enable
}

/*!
 * Loescht das ganze Display
 */
void display_clear(void){
	display_cmd(DISPLAY_CLEAR); // Display loeschen, Cursor Home
   #ifdef DISPLAY_REMOTE_AVAILABLE
		command_write(CMD_AKT_LCD, SUB_LCD_CLEAR, NULL, NULL,0);
	#endif
}

/*!
 * Positioniert den Cursor
 * @param row Zeile
 * @param column Spalte
 */
void display_cursor (uint8 row, uint8 column) {
   switch (row) {
    case 1:
		display_cmd (0x80 + column - 1);
		break;
    case 2:
		display_cmd (0xc0 + column - 1);
		break;
    case 3:
		display_cmd (0x94 + column - 1);
		break;
    case 4:
		display_cmd (0xd4 + column - 1);
		break;
    default: break;
   }
  
   #ifdef DISPLAY_REMOTE_AVAILABLE
	  int16 r=row-1;
  	  int16 c=column-1;
  	  remote_column = c;
  	  remote_row = r;
//	  command_write(CMD_AKT_LCD, SUB_LCD_CURSOR, &c,&r,0);
	#endif   
   
}

/*! 
 * Init Display
 */
void display_init(void){
	shift_init();

	DISPLAY_DDR |= DISPLAY_OUT;		// Ausgaenge
	DISPLAY_DDR &= ~DISPLAY_IN;		// Eingaenge

	delay(12);		// Display steht erst 10ms nach dem Booten bereit
	
	// Register in 8-Bit-Modus 3x uebertragen, dazwischen warten
	shift_data_out(0x38,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY);
	DISPLAY_PORT= DPC;
	delay(5);		
	shift_data_out(0x38,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY);
	DISPLAY_PORT= DPC;
	delay(5);		
	shift_data_out(0x38,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY);
	DISPLAY_PORT= DPC;
	delay(5);		
	
	display_cmd(0x0f);  		//Display On, Cursor On, Cursor Blink
	
	display_cmd(DISPLAY_CLEAR); // Display loeschen, Cursor Home
	
	display_data('i');
}

/*! 
 * Zeigt einen String an 
 * @return -1 falls string zuende 0 falls Zeile (20 zeichen) zuende
 */
/*int display_string(char data[20]){
	int i=0;
	
	while ((i<20) && (data[i] != 0x00)){ 	// Abbruch, sobald ein Nullstring erreicht wird
						// oder 20 Zeichen gesendet sind
		display_data(data[i++]);	// einzelnes Zeichen schicken
	}
	
	// return -1 falls string zuende, 0 falls zeile (20 zeichen) zuende
	if (data[i]==0x00)	return -1;	else return 0;
} 
*/

/*!
 * Schreibt einen String auf das Display.
 * @param format Format, wie beim printf
 * @param ... Variable Argumentenliste, wie beim printf
 */
void display_printf(char *format, ...) {
	uint8 run = 0;
	va_list	args;
	
	/* Sicher gehen, das der zur Verfuegung stehende Puffer nicht
	 * ueberschrieben wird.
	 */
	va_start(args, format);
	vsnprintf(display_buf, DISPLAY_BUFFER_SIZE, format, args);
	va_end(args);
	
	/* Ausgeben bis Puffer leer ist */
	while(display_buf[run] != '\0') {
		display_data(display_buf[run]);
		run++;
	}
	
	#ifdef DISPLAY_REMOTE_AVAILABLE
		int16 c = remote_column;
		int16 r = remote_row;
		command_write(CMD_AKT_LCD, SUB_LCD_CURSOR,&c,&r,0);
		command_write_data(CMD_AKT_LCD, SUB_LCD_DATA, NULL, NULL, display_buf);
		remote_column += run;
	#endif
	
	return;
}

/*
void display_test(){
	shift_init();	

//	shift_data_out(0xAA,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY);
	
	display_cmd(0x38);  		//Display auf 8 Bit Betrieb
	for(;;){}
	display_cmd(0x0f);  		//Display On, Cursor On, Cursor Blink
	
	display_cmd(DISPLAY_CLEAR); // Display l�schen, Cursor Home
	display_cursor(2,2);
	
	display_string("Hallo");
	for(;;){
	}
}
*/

#endif

#endif
