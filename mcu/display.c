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
#include <avr/delay.h>


#include "display.h"
#include "led.h"
#include "delay.h"

volatile char display_update=0;	///< Muss das Display aktualisiert werden?


#define DISPLAY_CLEAR 0x01		///< Kommando zum Löschen
#define DISPLAY_CURSORHOME 0x02	///< Kommando für den Cursor

#define DISPLAY_OUT 0x07		///< Kommando für das Display

#define DPC (PORTC & ~DISPLAY_OUT)	///< Port des Displays
//#define DRC (DDRC & ~DISPLAY_PINS)

/*!
 * Warte bis Display fertig
 */
void wait_busy(void){ //warten bis Busy-Flag vom Display aus
	char data=0x08;

	while (data==0x08){
		
		PORTC= DPC | 0x02; // RS=0, RW=1, E=0
		
		asm("nop");  asm("nop");
		PORTC= DPC | 0x06 ;  // RS=0, RW=1, E=1
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); //warten bis Busy-Flag bereit
		data= (PINC && 0x08);	// Flag lesen
		asm("nop"); asm("nop");	
		PORTC= DPC | 0x02; // E zuruecksetzen
	}	
}

/*! 
 * Übertrage Kommando an das Display
 * @param cmd Kommando
 */
void display_cmd(char cmd){		//ein Kommando cmd an das Display senden
	int i;
	PORTC =DPC |0;
	
	for (i=8; i>0; i--){
		PORTC = DPC |(cmd >> 7);      // Das oberste Bit von cmd auf PC0
		asm("nop");
		PORTC |= DPC | 0x02;	  		  // und PC1 takten
		asm("nop");
		cmd= cmd << 1;		      // cmd links schieben
		asm("nop"); asm("nop");
		PORTC= DPC | 0x00;
	}
	for (i=0; i<100; i++){
		asm("nop"); 
	}
	asm("nop"); asm("nop"); asm("nop"); asm("nop");
	PORTC=DPC | 0x04; // RS=0, RW=0, E setzen, damit wird gleichzeitig Inhalt 
				// des Schieberegisters auf dieparallelen Displayleitungen gegeben
				// das Display ist LAHM, daher warten
	
	for (i=0; i<2500; i++){
		asm("nop"); 
	}
	
	PORTC=DPC |0x00;
	
	wait_busy();
}

/*!
 * Löscht das ganze Display
 */
void display_clear(void){
	 display_cmd(DISPLAY_CLEAR); // Display l�schen, Cursor Home
}


/*!
 * Positioniert den Cursor
 * @param row Zeile
 * @param column Spalte
 */
void display_cursor (int row, int column) {
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
}

/*! 
 * Ein Zeichen auf das Display schreiben
 * @üaram data Das Zeichen
 */
void display_data(char data){ //ein Zeichen aus data in den Displayspeicher schreiben
	int i;
	PORTC=DPC | 0;
	for (i=8; i>0; i--){
		PORTC = DPC | (data >> 7);    // Das oberste Bit von cmd auf PC0
		asm("nop");
		PORTC |= 0x02;	  		  // und PC1 takten
		asm("nop");
		data= data << 1;		      // cmd links schieben
		asm("nop"); asm("nop");
		PORTC=DPC | 0x00;
	}
	PORTC= DPC | 5 ; //nur mit RS=1 statt RS=0
	for (i=0; i<2500; i++){
		asm("nop"); 
	}
	PORTC=DPC |0x01;
	wait_busy();	
}


/*! 
 * Init Display
 */
void display_init(void){
	DDRC= (DDRC &~DISPLAY_OUT) | 0x07; 		// Display Ports PC0-2
							//PC3 Eingang
	display_cmd(0x38);  		//Display auf 8 Bit Betrieb
	display_cmd(0x0f);  		//Display On, Cursor On, Cursor Blink
	
	display_cmd(DISPLAY_CLEAR); // Display l�schen, Cursor Home
	display_data('i'); 			// ein i zur Begruessung ausgeben
}

/*! 
 * Zeigt einen String an 
 * @return -1 falls string zuende 0 falls Zeile (20 zeichen) zuende
 */
int display_string(char data[20]){
	int i=0;
	
	while ((i<20) && (data[i] != 0x00)){ 	// Abbruch, sobald ein Nullstring erreicht wird
						// oder 20 Zeichen gesendet sind
		display_data(data[i++]);	// einzelnes Zeichen schicken
	}
	
	// return -1 falls string zuende, 0 falls zeile (20 zeichen) zuende
	if (data[i]==0x00)	return -1;	else return 0;
}
#endif

#endif
