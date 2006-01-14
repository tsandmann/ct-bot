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
#include "shift.h"

volatile char display_update=0;	///< Muss das Display aktualisiert werden?


#define DISPLAY_CLEAR 0x01		///< Kommando zum Löschen
#define DISPLAY_CURSORHOME 0x02	///< Kommando für den Cursor

#define DISPLAY_OUT 			0x07		///< Output-Pins Display
#define DISPLAY_IN 			(1<<5)		///< Input-Pins Display

#define DISPLAY_PORT			PORTC		///< Port an dem das Display hängt
#define DISPLAY_DDR			DDRC		///< Port an dem das Display hängt
#define DPC (DISPLAY_PORT && ~DISPLAY_OUT)	///< Port des Displays
//#define DRC (DDRC & ~DISPLAY_PINS)

#define DISPLAY_READY_PORT		PINC		///< Port an dem das Ready-Flag des Display hängt
#define DISPLAY_READY_DDR		DDRC		///< Port an dem das Ready-Flag des Display hängt
#define DISPLAY_READY_PIN		(1<<5)		///< Pin  an dem das Ready-Flag des Display hängt

/*! RS-Leitung 
 * legt fest, ob die Daten an das Display in den Textpuffer (RS=1) kommen
 * oder als Steuercode interpretiert werden (RS=0)
 */
#define DISPLAY_RS				(1<<0)		///< Pin an dem die RS-Leitung des Displays hängt

/*! RW-Leitung
 * legt fest, ob zum Display geschrieben wird (RW=0)
 * oder davon gelesen wird (RW=1)
 */
#define DISPLAY_RW				(1<<1)		///< Pin an dem die RW-Leitung des Displays hängt

/*! Enable Leitung 
 * schaltet das Interface ein (E=1). 
 * Nur wenn Enable auf High-Pegel liegt, läßt sich das Display ansprechen
 */
#define DISPLAY_EN				(1<<2)		///< Pin an dem die EN-Leitung des Displays hängt

/*
 * Im Moment der Low-High-Flanke von ENABLE liest das Dislplay 
 * die Werte von RS und R/W ein. Ist zu diesem Zeitpunkt R/W=0, 
 * dann liest das Display mit der folgenden High-Low-Flanke von ENABLE 
 * den Datenbus ein (Schreibzyklus). 
 * War aber R/W=1, dann legt das Display ein Datenword auf den 
 * Datenbus (Lese-Zyklus), solange bis die High-Low-Flanke von ENABLE 
 * das Interface wieder deaktiviert.
 */


/*!
 * Warte bis Display fertig
 */
void wait_busy(void){ //warten bis Busy-Flag vom Display aus
	char data=DISPLAY_READY_PIN;

	while (data==DISPLAY_READY_PIN){
		DISPLAY_PORT = DPC |  DISPLAY_RW ; // RS=0, RW=1, E=0
		asm("nop");  asm("nop");
		DISPLAY_PORT= DPC |  DISPLAY_RW | DISPLAY_EN	;  // RS=0, RW=1, E=1
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); //warten bis Busy-Flag bereit
		data= (DISPLAY_READY_PORT & DISPLAY_READY_PIN);	// Flag lesen
		asm("nop"); asm("nop");	
		DISPLAY_PORT= DPC |  DISPLAY_RW ; // RS=0, RW=1, E=0 zurücksetzen
	}	
    DISPLAY_PORT=DPC;	// Alles zurück setzen 
}

/*! 
 * Übertrage Kommando an das Display
 * @param cmd Kommando
 */
void display_cmd(char cmd){		//ein Kommando cmd an das Display senden
       int i;
		shift_data_out(cmd,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY);
        for (i=0; i<2000; i++){
                asm("nop");
        }
        DISPLAY_PORT=DPC;	// Alles zurück setzen ==> Fallende Flanke von Enable
        wait_busy();
}


/*! 
 * Ein Zeichen auf das Display schreiben
 * @param data Das Zeichen
 */
void display_data(char data){ //ein Zeichen aus data in den Displayspeicher schreiben
        int i;
		shift_data_out(data,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY|DISPLAY_RS);
		
		// Enable muss für mind. 450 ns High bleiben, bevor es fallen darf!
		// ==> Also mind. 8 Zyklen warten
        for (i=0; i<2000; i++){
                asm("nop");
        }
        PORTC=DPC;	// Alles zurück setzen ==> Fallende Flanke von Enable
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
 * Init Display
 */
void display_init(void){
	shift_init();
	DISPLAY_DDR |= DISPLAY_OUT;		// Ausgänge
	DISPLAY_DDR &= ~DISPLAY_IN;		// Eingänge

	display_cmd(0x38);  		//Display auf 8 Bit Betrieb
	display_cmd(0x0f);  		//Display On, Cursor On, Cursor Blink
	
	display_cmd(DISPLAY_CLEAR); // Display l�schen, Cursor Home
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

/*
void display_test(){
	shift_init();	

	shift_data(0xAA,SHIFT_LATCH,SHIFT_REGISTER_DISPLAY);
	
	display_cmd(0x38);  		//Display auf 8 Bit Betrieb
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
