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
void adc_init(char channel){
	DDRA &= ~ channel;	// Pin als input
	PORTA &= ~ channel;	// Alle Pullups aus.
}

/*!
 * Liest einen analogen Kanal aus
 * @param channel Kanal - hex-Wertigkeit des Pins (0x01 f�r PA0; 0x02 f�r PA1, ..)
 */
int adc_read(char channel){
	int result = 0x00;

	// interne Refernzspannung AVCC, rechts Ausrichtung
	ADMUX= _BV(REFS0) ;//| _BV(REFS1);	 //|(0<<ADLAR);	

	ADMUX |= (channel & 0x07);		// Und jetzt Kanal waehlen, nur single ended
	
	ADCSRA= (1<<ADPS2) | (1<<ADPS1)|	// prescale faktor= 128 ADC l�uft
		(1 <<ADPS0) |			// mit 14,7456MHz/ 128 = 115,2kHz 
		(1 << ADEN)|			// ADC an
		(1 << ADSC);			// Beginne mit der Konvertierung
			
	while ( (ADCSRA & (1<<ADSC)) != 0){} //Warten bis konvertierung beendet
					      // Das sollte 25 ADC-Zyklen dauern!
					      // also 1/4608 s
	result= ADCL; 
	result+=(ADCH <<8);	// Ergebnis zusammenbauen
	
	return result;
}
#endif
