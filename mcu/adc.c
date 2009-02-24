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
 * @file 	adc.c
 * @brief 	Routinen zum Einlesen der Analogeingaenge
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#ifdef MCU

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "adc.h"

typedef struct {
	uint8_t channel;
	int16_t * value;
} adc_channel_t;

static int8_t act_channel = -1;
static adc_channel_t channels[8];

/*!
 * Initialisert den AD-Umsetzer.
 * @param channel Fuer jeden Kanal, den man nutzen moechte,
 * muss das entsprechende Bit in channel gesetzt sein
 * Bit0 = Kanal 0 usw.
 */
void adc_init(uint8_t channel) {
	DDRA &= ~channel;	// Pin als input
	PORTA &= ~channel;	// Alle Pullups aus.
#ifdef MCU_ATMEGA644X
	DIDR0 = channel;	// Digital Input Disable
#endif
}

// deprecated
///*!
// * Liest einen analogen Kanal aus
// * @param channel Kanal - hex-Wertigkeit des Pins (0x01 fuer PA0; 0x02 fuer PA1, ..)
// */
//uint16 adc_read(uint8 channel){
//	uint16 result = 0x00;
//
//	// interne Refernzspannung AVCC, rechts Ausrichtung
//	ADMUX= _BV(REFS0) ;//| _BV(REFS1);	 //|(0<<ADLAR);
//
//	ADMUX |= (channel & 0x07);		// Und jetzt Kanal waehlen, nur single ended
//
//	ADCSRA= (1<<ADPS2) | (1<<ADPS1)|	// prescale faktor= 128 ADC laeuft
//		(1 <<ADPS0) |			// mit 14,7456MHz/ 128 = 115,2kHz
//		(1 << ADEN)|			// ADC an
//		(1 << ADSC);			// Beginne mit der Konvertierung
//
//	while ( (ADCSRA & (1<<ADSC)) != 0){asm volatile("nop");} //Warten bis konvertierung beendet
//					      // Das sollte 25 ADC-Zyklen dauern!
//					      // also 1/4608 s
//	result= ADCL;
//	result+=(ADCH <<8);	// Ergebnis zusammenbauen
//
//	return result;
//}

/*!
 * @brief			Fuegt einen analogen Kanal in die ADC-Konvertierungsliste ein und wertet ihn per Interrupt aus
 * @param channel 	Kanal - hex-Wertigkeit des Pins (0x01 fuer PA0; 0x02 fuer PA1, ..)
 * @param p_sens	Zeiger auf den Sensorwert, der das Ergebnis enthalten soll
 */
void adc_read_int(uint8_t channel, int16_t * p_sens) {
	static uint8_t next_channel = 0;
	if (act_channel == -1) next_channel = 0;
	if (next_channel >= 8) return;	// es gibt nur 8 ADC-Channels
	channels[next_channel].value = p_sens;
	channels[next_channel++].channel = channel & 0x7;
	if (act_channel == -1) {
		act_channel = 0;
		// interne Refernzspannung AVCC, rechts Ausrichtung
		ADMUX = _BV(REFS0); //| _BV(REFS1);	 //|(0<<ADLAR);

		ADMUX |= (channel & 0x07);		// Und jetzt Kanal waehlen, nur single ended

		ADCSRA= (1<<ADPS2) | (1<<ADPS1) |	// prescale faktor = 128 => ADC laeuft
			(1 << ADPS0)   |				// mit 16 MHz / 128 = 125 kHz
			(1 << ADEN)    |				// ADC an
			(1 << ADSC)    |				// Beginne mit der Konvertierung
			(1 << ADIE);					// Interrupt an
	}
}

/*!
 * Interrupt-Handler fuer den ADC. Speichert das Ergebnis des aktuellen Channels und
 * schaltet in der Liste der auszuwertenden Sensoren eins weiter.
 */
ISR(SIG_ADC) {
	/* Daten speichern und Pointer im Puffer loeschen */
	*channels[act_channel].value = ADC;
	channels[act_channel].value = NULL;
	/* zum naechsten Sensor weiterschalten */
	act_channel++;
	if (act_channel < 8 && channels[act_channel].value != NULL) {
		ADMUX = _BV(REFS0); //| _BV(REFS1);	//|(0<<ADLAR);	// interne Refernzspannung AVCC, rechts Ausrichtung
		ADMUX |= channels[act_channel].channel;
		ADCSRA = (1<<ADPS2) | (1<<ADPS1) |	// prescale faktor = 128 => ADC laeuft
		(1 << ADPS0)	|					// mit 16 MHz / 128 = 125 kHz
		(1 << ADEN) 	|					// ADC an
		(1 << ADSC) 	|					// Beginne mit der Konvertierung
		(1 << ADIE);						// Interrupt an
	} else {
		ADCSRA = 0;	// ADC aus
		act_channel = -1;
	}
}

/*!
 * Gibt die laufende Nr. des Channels zurueck, der aktuell ausgewertet wird.
 * 0: erste registrierter Channel, 1: zweiter registrierter Channel usw.
 * 255: derzeit wird kein Channel ausgewertet (= Konvertierung fertig)
 */
uint8_t adc_get_active_channel(void) {
	return act_channel;
}

#endif	// MCU
