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

/**
 * \file 	shift.c
 * \brief 	Routinen zur Ansteuerung der Shift-Register
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef SHIFT_AVAILABLE
#include "shift.h"
#include "i2c.h"
#include <avr/io.h>

#define SHIFT_OUT				(_BV(PINC0) | _BV(PINC1) | _BV(PINC2) | _BV(PINC3) | _BV(PINC4)) /**< Alle Pins die Ausgaenge sind */
#define SHIFT_PORT				PORTC	/**< Port an dem die Register haengen */
#define SHIFT_DDR				DDRC	/**< DDR des Ports an dem die Register haengen */

/**
 * Setzt die Shift-Register wieder zurueck
 */
static void shift_clear(void) {
	SHIFT_PORT = (uint8_t) (SHIFT_PORT & ~SHIFT_OUT); // clear
}

/**
 * Initialisiert die Shift-Register
 */
void shift_init(void) {
	SHIFT_DDR |= SHIFT_OUT; // Ausgaenge Schalten
	shift_clear(); // Und auf Null
}

/**
 * Schiebt Daten durch eines der drei 74HC595 Schieberegister
 * Achtung den Port sollte man danach noch per shift_clear() zuruecksetzen
 * \param data			Das Datenbyte
 * \param latch_data 	Der Pin an dem der Daten-latch-Pin des Registers (PIN 11) haengt
 * \param latch_store 	Der Pin an dem der latch-Pin zum transfer des Registers (PIN 12) haengt
 */
void shift_data_out(uint8_t data, uint8_t latch_data, uint8_t latch_store) {
#ifdef I2C_AVAILABLE
	i2c_wait(); // I2C-Transfer muss beendet sein (benutzt PC0 und PC1)
#endif

	SHIFT_PORT = (uint8_t) (SHIFT_PORT & ~SHIFT_OUT); // und wieder clear
	int8_t i;
	for (i = 8; i > 0; i--) {
		SHIFT_PORT |= (uint8_t) ((data >> 7) & 0x01); // Das oberste Bit von data auf PC0.0 (Datenleitung Schieberegister)
		SHIFT_PORT |= latch_data; // und ins jeweilige Storageregister latchen
		data = (uint8_t) (data << 1); // data links schieben
		shift_clear(); // und wieder clear
	}

	SHIFT_PORT |= latch_store; // alles vom Storage ins Output register latchen
}

/**
 * Schiebt Daten durch eines der drei 74HC595 Schieberegister
 * vereinfachte Version, braucht kein shift_clear()
 * geht NICHT fuer das Shift-register, an dem das Display-haengt!!!
 * \param data			Das Datenbyte
 * \param latch_data 	Der Pin an dem der Daten-latch-Pin des Registers (PIN 11) haengt
 */
void shift_data(uint8_t data, uint8_t latch_data) {
	shift_data_out(data, latch_data, SHIFT_LATCH);
	shift_clear();
}
#endif // SHIFT_AVAILABLE
#endif // MCU
