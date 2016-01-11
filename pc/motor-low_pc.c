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
 * \file 	motor-low_pc.c
 * \brief 	Low-Level Routinen fuer die Motorsteuerung des c't-Bots
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	01.12.2005
 */

#ifdef PC

#include "ct-Bot.h"
#include <stdlib.h>
#include "command.h"
#include "bot-2-sim.h"
#include "motor-low.h"
#include "motor.h"
#include "log.h"

int16_t motor_left; /**< zuletzt gestellter Wert linker Motor */
int16_t motor_right; /**< zuletzt gestellter Wert rechter Motor */

/**
 *  Initilisiert alles fuer die Motosteuerung
 */
void motor_low_init() {
}

/**
 * Unmittelbarer Zugriff auf die beiden Motoren,
 * normalerweise NICHT verwenden!
 * \param left PWM links
 * \param right PWM rechts
*/
void bot_motor(int16_t left, int16_t right){
	command_write(CMD_AKT_MOT, SUB_CMD_NORM, left, right, 0);

	if (right < 0) {
		direction.right = DIRECTION_BACKWARD;
	} else if (right > 0) {
		direction.right = DIRECTION_FORWARD;
	}
	if (left < 0) {
		direction.left = DIRECTION_BACKWARD;
	} else if (left > 0) {
		direction.left = DIRECTION_FORWARD;
	}

	motor_left = left;
	motor_right = right;
}

/**
 * Stellt die Servos
 * \param servo Nummer des Servos (1 oder 2)
 * \param pos Zielwert, sinnvolle Werte liegen zwischen 7 und 16, oder 0 fuer Servo aus
 */
void servo_low(uint8_t servo, uint8_t pos) {
	static uint8_t values[2] = {0, 0};

	if (servo == 0 || servo > sizeof(values) / sizeof(values[0])) {
		LOG_ERROR("invalid servo addressed!");
		return;
	}

	values[servo - 1] = pos;
//	LOG_DEBUG("servo_low(): command_write(CMD_AKT_SERVO, SUB_CMD_NORM, %u, %u, 0)", values[0], values[1]);
	command_write(CMD_AKT_SERVO, SUB_CMD_NORM, values[0], values[1], 0);
}

#endif // PC
