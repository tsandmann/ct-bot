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
 * @file 	shift.h
 * @brief 	Routinen zur Ansteuerung der Shift-Register
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
 */

#ifndef SHIFT_H_
#define SHIFT_H_


#define SHIFT_LATCH	(1<<1)			/*!< Clock to store Data into shiftregister */

#define SHIFT_REGISTER_DISPLAY	0x04	/*!< Port-Pin for shiftregister latch (display) */
#define SHIFT_REGISTER_LED	0x10	/*!< Port-Pin for shiftregister latch (leds) */
#define SHIFT_REGISTER_ENA	0x08	/*!< Port-Pin for shiftregister latch (enable) */

/*!
 * Initialisert die Shift-Register
 */
void shift_init(void);

/*!
 * Schiebt Daten durch eines der drei 74HC595-Schieberegister
 * Achtung: den Port sollte man danach noch per shift_clear() zuruecksetzen!
 * @param data	Das Datenbyte
 * @param latch_data der Pin, an dem der Daten-latch-Pin des Registers (PIN 11) haengt
 * @param latch_store der Pin, an dem der latch-Pin zum Transfer des Registers (PIN 12) haengt
 */
void shift_data_out(uint8_t data, uint8_t latch_data, uint8_t latch_store);

/*!
 * Schiebt Daten durch eines der drei 74HC595-Schieberegister,
 * vereinfachte Version, braucht kein shift_clear().
 * Funktioniert NICHT fuer das Shift-Register, an dem das Display haengt!!!
 * @param data	Das Datenbyte
 * @param latch_data der Pin, an dem der Daten-latch-Pin des Registers (PIN 11) haengt
 */
void shift_data(uint8_t data, uint8_t latch_data);

#endif // SHIFT_H_
