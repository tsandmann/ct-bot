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

/*! @file 	adc.c
 * @brief 	Routinen zum Einlesen der AnalogeingÄnge
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

#include "adc.h"

/*!
 * Initialisert den AD-Umsetzer. 
 * @param channel Für jeden Kanal, den man nutzen möchte, 
 * muss das entsprechende Bit in channel gesetzt sein
 * Bit0 = Kanal 0 usw.
 */
void adc_init(uint8 channel){
	DDRA &= ~ channel;	// Pin als input
	PORTA &= ~ channel;	// Alle Pullups aus.
}

/*!
 * Liest einen analogen Kanal aus
 * @param channel Kanal - hex-Wertigkeit des Pins (0x01 f�r PA0; 0x02 f�r PA1, ..)
 */
int adc_read(uint8 channel){
	int result = 0x00;

	// interne Refernzspannung AVCC, rechts Ausrichtung
	ADMUX= _BV(REFS0) ;//| _BV(REFS1);	 //|(0<<ADLAR);	

	ADMUX |= (channel & 0x07);		// Und jetzt Kanal waehlen, nur single ended
	
	ADCSRA= (1<<ADPS2) | (1<<ADPS1)|	// prescale faktor= 128 ADC laeuft
		(1 <<ADPS0) |			// mit 14,7456MHz/ 128 = 115,2kHz 
		(1 << ADEN)|			// ADC an
		(1 << ADSC);			// Beginne mit der Konvertierung
			
	while ( (ADCSRA & (1<<ADSC)) != 0){asm volatile("nop");} //Warten bis konvertierung beendet
					      // Das sollte 25 ADC-Zyklen dauern!
					      // also 1/4608 s
	result= ADCL; 
	result+=(ADCH <<8);	// Ergebnis zusammenbauen
	
	return result;
}
#endif
