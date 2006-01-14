/*! @file 	shift.c
 * @brief 	Routinen zur Ansteuerung der Shift-Register
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#include "global.h"
#include "ct-Bot.h"

#include "shift.h"

#ifdef MCU
#ifdef SHIFT_AVAILABLE

#include <avr/io.h>
#include <avr/delay.h>


#define SHIFT_OUT				0x1F
#define SHIFT_PORT				PORTC
#define SHIFT_DDR				DDRC

/*!
 * Initialisert die Shift-Register
 */
void shift_init(){
	SHIFT_DDR |= SHIFT_OUT;		// Ausgänge Schalten
	SHIFT_PORT &= ~SHIFT_OUT; 	// Und auf Null
}

/*!
 * Schiebt Daten durch eines der drei 74HC595 Schieberegister
 * Achtung den Port sollte man danach noch per shift_clear() zurücksetzen
 * @param data	Das Datenbyte
 * @param latch_data der Pin an dem der Daten-latch-Pin des Registers (PIN 11) hängt
 * @param latchtore der Pin an dem der latch-Pin zum transfer des Registers (PIN 12) hängt
 */
void shift_data_out(char data, char latch_data, char latch_store){
	char i;

	SHIFT_PORT &= ~SHIFT_OUT;		// und wieder clear	
	for (i=8; i>0; i--){
		SHIFT_PORT |= (data >> 7)&& 0x01;      // Das oberste Bit von data auf PC0.0 (Datenleitung Schieberegister)
		SHIFT_PORT |= latch_data ;	    		// und ins jeweilige Storageregister latchen
		data= data << 1;		      	// data links schieben
		SHIFT_PORT &= ~SHIFT_OUT;		// und wieder clear
	}
	
	SHIFT_PORT |= latch_store;			// alles vom Storage ins Output register latchen
}

/*!
 * Schiebt Daten durch eines der drei 74HC595 Schieberegister
 * vereinfachte Version, braucht kein shift_clear()
 * geht NICHT für das Shift-register, an dem das Display-hängt!!!
 * @param data	Das Datenbyte
 * @param latch_data der Pin an dem der Daten-latch-Pin des Registers (PIN 11) hängt
 */
void shift_data(char data, char latch_data){
	shift_data_out(data, latch_data, SHIFT_LATCH);
	shift_clear();
}

/*!
 * Setzt die Shift-Register wieder zurück
 */ 
void shift_clear(){
	SHIFT_PORT &= ~SHIFT_OUT;		// und wieder clear	
}
#endif
#endif
