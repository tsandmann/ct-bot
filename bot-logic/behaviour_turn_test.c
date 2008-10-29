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
 * @file 	behaviour_turn_test.c
 * @brief 	Fuehrt mehrere Drehungen mit bot_turn() aus und misst die Fehler
 * @author	Timo Sandmann (mail@timosandmann.de)
 * @date 	08.07.2007
 */


#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
#include <math.h>
#include "sensor.h"
#include "log.h"
#include "eeprom.h"

#define FLOAT_PRINTF	/*!< aktiviert float-Ausgaben mit printf ("-Wl,-u,vfprintf -lprintf_flt") */

static uint16_t degrees = 0;
static uint8_t turn_count = 0;
extern uint8_t EEPROM turn_err[3];

/*!
 * @brief		Das eigentliche Verhalten
 * @param data	Zeiger auf den Verhaltensdatensatz
 * @see			bot_turn_test()
 */
void bot_turn_test_behaviour(Behaviour_t *data) {
	static float last_heading = 0.0;
	static float err = 0.0;
	if (degrees > 0) {
		if (turn_count < 10) {
			if (turn_count > 0) {
				err += turn_last_err;
//				LOG_DEBUG("heading=%f Grad", heading);
//				LOG_DEBUG("Fehler=%f Grad", turn_last_err);
			}
			last_heading = heading;
			bot_turn(data, degrees);
			turn_count++;
		} else {
			err += turn_last_err;
//			LOG_DEBUG("heading=%f", heading);
//			LOG_DEBUG("Fehler=%f Grad", turn_last_err);
#ifdef FLOAT_PRINTF
			LOG_DEBUG("durchschn. Fehler=%f Grad", err/10.0);
#else
			LOG_DEBUG("durchschn. Fehler=%d.%u Grad", (int16_t)(err/10.0), (int16_t)((err/10.0 - (int16_t)(err/10.0))*10));
#endif
			LOG_DEBUG("degrees=%d: turn_err[]={%d,%d,%d}", degrees, ctbot_eeprom_read_byte(&turn_err[0]), ctbot_eeprom_read_byte(&turn_err[1]), ctbot_eeprom_read_byte(&turn_err[2]));
			turn_count = 0;
			err = 0.0;
			degrees -= 5;
			if (degrees > 60) degrees -= 15;
			LOG_DEBUG("degress=%d", degrees);
		}
	} else {
		err = 0.0;
		return_from_behaviour(data);
	}
}

/*!
 * @brief			Testet das bot_turn-Verhalten und gibt Informationen ueber die Drehgenauigkeit aus
 * @param caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 */
void bot_turn_test(Behaviour_t* caller) {
	switch_to_behaviour(caller, bot_turn_test_behaviour,NOOVERRIDE);
	degrees = 360;
	turn_count = 0;
}

#endif	// BEHAVIOUR_TURN_TEST_AVAILABLE
