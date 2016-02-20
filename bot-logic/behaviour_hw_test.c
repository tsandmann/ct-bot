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
 * \file 	behaviour_hw_test.c
 * \brief 	Testcode fuer die Bot-Hardware (ehemals TEST_AVAILABLE_ANALOG, _DIGITAL, _MOTOR)
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	22.05.2011
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_HW_TEST_AVAILABLE
#include "led.h"

#define TEST_ANALOG		/**< Sollen die LEDs die analoge Sensorwerte anzeigen? */
//#define TEST_DIGITAL	/**< Sollen die LEDs die digitale Sensorwerte anzeigen? */
//#define TEST_MOTOR	/**< Sollen die Motoren ein wenig drehen? */

static uint8_t testmode =
#if defined TEST_ANALOG
	0
#elif defined TEST_DIGITAL
	1
#elif defined TEST_MOTOR
	2
#else
	3
#endif // TEST_X
	; /**< Testmodus, 0: analog, 1: digital, 2: Motor */
static uint16_t calls = 0; /**< Anzahl der Durchlaeufe fuer Motor-Test */

/**
 * Das Testverhalten
 * \param data	Der Verhaltensdatensatz
 */
void bot_hw_test_behaviour(Behaviour_t * data) {
#ifdef LED_AVAILABLE
	led_t status;
	uint8_t led_status;
	bit_t tmp;
#endif // LED_AVAILABLE

	sensor_update_distance = sensor_dist_straight; // Distanzsensordaten 1:1 weiterreichen

	switch (testmode) {
	case 0: // analog
#ifdef LED_AVAILABLE
		tmp.byte = (uint8_t) (sensDistR >> 8);
		status.rechts = tmp.bit;
		tmp.byte = (uint8_t) (sensDistL >> 8);
		status.links = tmp.bit;
		tmp.byte = (uint8_t) (sensLineL >> 9);
		status.rot = tmp.bit;
		tmp.byte = (uint8_t) (sensLineR >> 9);
		status.orange =  tmp.bit;
		tmp.byte = (uint8_t) (sensLDRL >> 8);
		status.gelb = tmp.bit;
		tmp.byte = (uint8_t) (sensLDRR >> 8);
		status.gruen = tmp.bit;
		tmp.byte = (uint8_t) (sensBorderL >> 9);
		status.tuerkis = tmp.bit;
		tmp.byte = (uint8_t) (sensBorderR >> 9);
		status.weiss = tmp.bit;

		memcpy(&led_status, &status, sizeof(led_status));
		LED_set(led_status);
#endif // LED_AVAILABLE
		break;

	case 1: // digital
#ifdef LED_AVAILABLE
		tmp.byte = (uint8_t) sensEncR;
		status.rechts = tmp.bit;
		tmp.byte = (uint8_t) sensEncL;
		status.links = tmp.bit;
		tmp.byte = sensTrans;
		status.rot = tmp.bit;
		tmp.byte = sensError;
		status.orange = tmp.bit;
		tmp.byte = sensDoor;
		status.gelb = tmp.bit;
#ifdef MOUSE_AVAILABLE
		tmp.byte = (uint8_t) (sensMouseDX >> 1);
		status.gruen = tmp.bit;
		tmp.byte = (uint8_t) (sensMouseDY >> 1);
		status.tuerkis = tmp.bit;
#endif // MOUSE_AVAILABLE
#ifdef RC5_AVAILABLE
		tmp.byte = (uint8_t) rc5_ir_data.ir_data;
		status.weiss = tmp.bit;
#endif // RC5_AVAILABLE

		memcpy(&led_status, &status, sizeof(led_status));
		LED_set(led_status);
#endif // LED_AVAILABLE
		break;

	case 2: // Motor
		if (calls < 350) {
			speedWishLeft  =  BOT_SPEED_SLOW;
			speedWishRight = -BOT_SPEED_SLOW;
		} else if (calls < 700) {
			speedWishLeft  = -BOT_SPEED_SLOW;
			speedWishRight =  BOT_SPEED_SLOW;
		} else {
			speedWishLeft  = BOT_SPEED_STOP;
			speedWishRight = BOT_SPEED_STOP;
			return_from_behaviour(data);
		}
		++calls;
		break;
	}
}

/**
 * Veraendert den Testmode
 * \param *caller Zeiger auf Verhaltensdatensatz des Aufrufers
 * \param mode neuer Mode (0: analog, 1: digital, 2: motor)
 */
void bot_hw_test(Behaviour_t * caller, uint8_t mode) {
	testmode = mode;
	calls = 0;
	switch_to_behaviour(caller, bot_hw_test_behaviour, BEHAVIOUR_OVERRIDE | BEHAVIOUR_BACKGROUND);
}

#endif // BEHAVIOUR_HW_TEST_AVAILABLE
