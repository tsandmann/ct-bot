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
 * @file 	ir-rc5.c
 * @brief 	Routinen für die Dekodierung von RC5-Fernbedienungs-Codes
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
 */

// Infos ueber RC6: http://www.xs4all.nl/~sbp/knowledge/ir/rc6.htm
// http://www.xs4all.nl/~sbp/knowledge/ir/ir.htm

// ========================================================================
// RC5 Infrarot-Empfaenger
// ========================================================================
#include "ct-Bot.h"
#ifdef MCU
#ifdef IR_AVAILABLE

#include <avr/io.h>
#include "ir-rc5.h"
#include "timer.h"


// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
#define IR_SAMPLES_PER_BIT	10  	/*!< 10 Samples per Bit */
#define IR_SAMPLES_PER_BIT_EARLY 8	/*!< Flanke fruehestens nach 8 Samples */
#define IR_SAMPLES_PER_BIT_LATE 12	/*!< Flanke spaetestens nach 12 Samples */
#define IR_SAMPLES_PER_BIT_MIN	 3 	/*!< Flanke vor 3 Samples -> paket verwerfen */
#define IR_PAUSE_SAMPLES      250	/*!< Startbit ist erst nach 250 Samples ohne */
				    // Pegelaenderung gueltig -- eigentlich muesste
				    // man rund 500 Samples abwarten (50 x
				    // Bitzeit), doch weil der Samplezaehler ein
				    // Byte ist, beschraenken wir uns hier auf ein
				    // Minimum von 250 Samples

#define IR_PORT		PORTB			/*!< Port B */
#define IR_DDR		DDRB			/*!< DDR of Port B */
#define IR_PINR		PINB			/*!< Port B input */
#define IR_PIN		1				/*!< Pin 1 */



static uint8_t ir_lastsample = 0;	/*!< zuletzt gelesenes Sample */
static uint8_t ir_bittimer   = 0;	/*!< zaehlt die Aufrufe von ir_isr() */

static uint16_t ir_data_tmp = 0; /*!< RC5-Bitstream */
static uint8_t ir_bitcount = 0;	/*!< Anzahl gelesener Bits */

volatile uint16_t ir_data	= 0; /*!< letztes komplett gelesenes RC5-Paket */

/*!
 * Interrupt Serviceroutine
 * wird alle 176 us aufgerufen
 */
void ir_isr(void) {
	// sample lesen
	byte sample = 1;

	if ((IR_PINR & (1 << IR_PIN)) != 0) {
		sample = 0;
	}

	// bittimer erhoehen (bleibt bei 255 stehen)
	if (ir_bittimer < 255) {
		ir_bittimer++;
	}

	// flankenerkennung
	if (ir_lastsample != sample) {
		if (ir_bittimer <= IR_SAMPLES_PER_BIT_MIN) {
			// flanke kommt zu frueh: paket verwerfen
			ir_bitcount=0;
		} else {
			// Startbit
			if (ir_bitcount == 0) {
				if ((sample == 1) && (ir_bittimer > IR_PAUSE_SAMPLES)) {
					// Startbit speichern
					ir_data_tmp = 1;
					ir_bitcount++;
				} else {
					// error
					ir_data_tmp = 0;
				}

				// bittimer-reset
				ir_bittimer = 0;

			// Bits 2..14: nur Flanken innerhalb des Bits beruecksichtigen
			} else {
				if (ir_bittimer >= IR_SAMPLES_PER_BIT_EARLY) {
					if (ir_bittimer <= IR_SAMPLES_PER_BIT_LATE) {
						// Bit speichern
						ir_data_tmp = (ir_data_tmp << 1) | sample;
						ir_bitcount++;
					} else {
						// zu spaet: paket verwerfen
						ir_bitcount = 0;
					}

					// bittimer-reset
					ir_bittimer = 0;
				}
			}
		}

	} else {
		// keine Flanke innerhalb bitzeit?
		if (ir_bittimer > IR_SAMPLES_PER_BIT_LATE) {
			// 14 bits gelesen?
			if (ir_bitcount == 14) {
				ir_data = ir_data_tmp;
			}
			// paket verwerfen
			ir_bitcount = 0;
		}
	}

	// sample im samplepuffer ablegen
	ir_lastsample = sample;
}


/*!
 * IR-Daten lesen
 * @return Wert von ir_data, loescht anschliessend ir_data
 */
uint16_t ir_read(void) {
	uint16_t retvalue = ir_data;
	ir_data = 0;
	return retvalue;
}

/*!
 * Init IR-System
 */
void ir_init(void) {
	IR_DDR  &= ~IR_PIN; 	// Pin auf Input
	IR_PORT |= IR_PIN;		// Pullup an
}
#endif	// IR_AVAILABLE
#endif	// MCU
