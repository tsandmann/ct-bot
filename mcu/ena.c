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

/*! @file 	ena.c 
 * @brief 	Routinen zur Steuerung der Enable-Leitungen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU 

#include <avr/io.h>
#include "ena.h"
#include "ct-Bot.h"
#include "shift.h"
#include "mouse.h"

#ifdef ENA_AVAILABLE


volatile uint8 ena =0;	/*!< Sichert den Zustand der Enable-Leitungen */

/*!
 * Initialisiert die Enable-Leitungen
 */
void ENA_init(){
	DDRD |= 4;
	shift_init();
	ENA_set(0x00);
}


//void maus_sens_write(int8 adr, uint8 data);
/*! 
 * Schaltet einzelne Enable-Transistoren an
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_on schaltet einen Transistor durch
 * Daher zieht es die entsprechende ENA_XXX-Leitung (mit Transistor) auf Low und NICHT auf High
 * @param enable Bitmaske der anzuschaltenden LEDs
 */
void ENA_on(uint8 enable){
//	uint8 i;
	// Maussensor und MMC-Karte haengen zusammen
	if (enable == ENA_MOUSE_SENSOR){		// Maus sensor an, MMC aus
		
//		if ((ena & ENA_MMC) ==0){ // War die MMC-Karte an?
			ena |= ENA_MMC;			// MMC aus
	
//	        PORTD |= 4;	// Fliplops takten
//	        PORTD &= ~4;
//	
//			for (i=0; i<200; i++){	// Ein paar flanken schocken, damit auch sicher danach ruhe ist
//				PORTB &= ~(1<<7);
//				PORTB |= (1<<7);
//			}			
//		}
		
		// Und dann den Maussensor an
		ena &= ~ENA_MOUSE_SENSOR;		
	} else	if (enable == ENA_MMC) {		// Maus sensor aus, MMC aan
		#ifdef MOUSE_AVAILABLE
			if ((ena & ENA_MOUSE_SENSOR) ==0){ // War der Maussensor an?
				maus_sens_highZ();	// Der Maussensor muss die Datenleitung freigeben
				ena |= ENA_MOUSE_SENSOR;	// Maus aus
			}
		#endif
		ena &= ~enable;
	} else {
		ena |= enable;
	}
	
	ENA_set(ena);
	
	if ( (enable & (ENA_MOUSE_SENSOR | ENA_MMC)) != 0 ){
        PORTD |= 4;	// Fliplops takten
        PORTD &= ~4;
	}
}

/*! 
 * Schaltet einzelne Enable-Transistoren aus
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_off schaltet einen Transistor ab
 * Daher zieht es die entsprechende ENA_XXX-Leitung (mit Transistor) auf High und NICHT auf Low
 * @param enable Bitmaske der anzuschaltenden LEDs
 */
void ENA_off(uint8 enable){
	ena &= ~enable;
	ENA_set(ena);
	
	if ( (enable & (ENA_MOUSE_SENSOR | ENA_MMC)) != 0 ){
        PORTD |= 4;	// Fliplops takten
        PORTD &= ~4;
	}
}

/*!
 * Schaltet die Enable-Transistoren
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_set bezieht sich auf die Transistor
 * Daher zieht es die entsprechende ENA_XXX-Leitung auf ~enable
 * @param LED Wert der gezeigt werden soll
 */
void ENA_set(uint8 enable){
	ena=enable;
	shift_data(~enable,SHIFT_REGISTER_ENA); 
}

#endif
#endif
