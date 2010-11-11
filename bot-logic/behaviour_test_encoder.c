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
 * @file 	behaviour_test_encoder.c
 * @brief 	Verhalten, das die Genauigkeit der Encoder-Auswertung testet. Nur zu Debugging-Zwecken.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	08.02.2010
 */

#include "bot-logic/bot-logic.h"
#ifdef BEHAVIOUR_TEST_ENCODER_AVAILABLE

#include <stdlib.h>

static uint8_t state = 0;	/*!< Status des Verhaltens */

/*!
 * Encoder-Test-Verhalten
 * @param *data der Verhaltensdatensatz
 */
void bot_test_encoder_behaviour(Behaviour_t * data) {
	static int16_t cycles = 0;

	speedWishLeft = BOT_SPEED_STOP;
	speedWishRight = BOT_SPEED_STOP;

	switch (state) {
	case 0:
		cycles = (int16_t) random() % 1000;
		state = 1;
		break;

	case 1:
		cycles--;
		if (cycles > 0) {
			speedWishLeft = BOT_SPEED_FOLLOW;
			speedWishRight = BOT_SPEED_FOLLOW + 6;
		} else {
			state = 2;
		}
		break;

	case 2:
		cycles = (int16_t) random() % 500;
		state = 3;
		break;

	case 3:
		cycles--;
		if (cycles > 0) {
			speedWishLeft = - (BOT_SPEED_SLOW + 4);
			speedWishRight = - BOT_SPEED_SLOW;
		} else {
			state = 4;
		}
		break;

	case 4:
		bot_goto_pos(data, 0, 0, 0);
		state = 0;
		break;

	default:
		return_from_behaviour(data);
	}
}

/*!
 * Botenfunktion des Encoder-Test-Verhaltens
 * @param *caller Der Verhaltensdatensatz des Aufrufers
 */
void bot_test_encoder(Behaviour_t * caller) {
	state = 0;
	switch_to_behaviour(caller, bot_test_encoder_behaviour, OVERRIDE);
}
#endif // BEHAVIOUR_TEST_ENCODER_AVAILABLE
