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
#define IR_SAMPLES_PER_BIT_MIN		2	/*!< Flanke vor 3 Samples -> Paket verwerfen */

//TODO:	Das gehoert eigentlich nicht hierhin
#ifdef RC5_AVAILABLE
ir_data_t rc5_ir_data = {
	0, 0, 0, 0, 0, 0
};
#endif
#ifdef BPS_AVAILABLE
ir_data_t bps_ir_data = {
	0, 0, 0, 0, 0, 1023
};
#endif

/*!
 * Interrupt Serviceroutine,
 * wird alle 176 us aufgerufen
 * @param *data Zeiger auf Arbeitsdaten
 * @param pin_r Input-Port
 * @param pin Input-Pin
 * @param pause_samples Anzahl der Samples, bevor ein Startbit erkannt wird
 * @param samples_per_bit Anzahl der Samples / Bit
 * @param bits Anzahl der Bits, die Empfangen werden sollen (inkl. Startbit)
 */
void ir_isr(ir_data_t * data, volatile uint8_t * pin_r, const uint8_t pin, const uint8_t pause_samples,
		const uint8_t samples_per_bit, const uint8_t bits) {

	const uint8_t samples_per_bit_early = (uint8_t)(samples_per_bit - 2); /*!< Flanke fruehestens nach X Samples */
	const uint8_t samples_per_bit_late = (uint8_t)(samples_per_bit + 2); /*!< Flanke spaetestens nach X Samples */

	/* Sample lesen */
	uint8_t sample = 1;

	if ((*pin_r & (1 << pin)) != 0) {
		sample = 0;
	}

	/* Bittimer erhoehen (bleibt bei 255 stehen) */
	if (data->ir_bittimer < 255) {
		data->ir_bittimer++;
	}

	/* Flankenerkennung */
	if (data->ir_lastsample != sample) {
		if (data->ir_bittimer <= IR_SAMPLES_PER_BIT_MIN) {
			data->ir_bitcount = 0; // Flanke kommt zu frueh: Paket verwerfen
		} else {
			/* Startbit */
			if (data->ir_bitcount == 0) {
				if ((sample == 1) && (data->ir_bittimer > pause_samples)) {
					// Startbit speichern
					data->ir_data_tmp = 1;
					data->ir_bitcount++;
				} else {
					// Error
					data->ir_data_tmp = 0;
				}

				data->ir_bittimer = 0; // Bittimer-Reset

			/* Bits 2..14: nur Flanken innerhalb des Bits beruecksichtigen */
			} else {
				if (data->ir_bittimer >= samples_per_bit_early) {
					if (data->ir_bittimer <= samples_per_bit_late) {
						// Bit speichern
						data->ir_data_tmp = (data->ir_data_tmp << 1) | sample;
						data->ir_bitcount++;
					} else {
						data->ir_bitcount = 0; // zu spaet: Paket verwerfen
						//data->ir_data = data->no_data;
					}

					data->ir_bittimer = 0; // Bittimer-Reset
				}
			}
		}
	} else {
		/* keine Flanke innerhalb Bitzeit? */
		if (data->ir_bittimer > samples_per_bit_late) {
			/* alle Bits gelesen? */
			if (data->ir_bitcount == bits) {
				data->ir_data = data->ir_data_tmp;
			} else {
				/* Paket verwerfen */
				//data->ir_data = data->no_data;
			}
			data->ir_bitcount = 0;
		}
	}

	data->ir_lastsample = sample; // Sample im Samplepuffer ablegen
}

/*!
 * IR-Daten lesen
 * @param *data Zeiger auf Arbeitsdaten
 * @return Wert von ir_data, loescht anschliessend ir_data
 */
uint16_t ir_read(ir_data_t * data) {
	uint16_t retvalue = data->ir_data;
	data->ir_data = data->no_data;
	return retvalue;
}

#endif	// IR_AVAILABLE
#endif	// MCU
