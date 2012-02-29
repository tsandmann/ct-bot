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
 * \file 	behaviour_servo.c
 * \brief 	kontrolliert die Servos
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	07.12.2006
 */


#include "bot-logic.h"
#ifdef BEHAVIOUR_SERVO_AVAILABLE

#include "motor.h"
#include "timer.h"
static uint8_t servo_nr;	/**< Nr. des aktiven Servos */
uint8_t servo_active = 0;	/**< 0, wenn kein Servo aktiv, sonst Bit der gerade aktiven Servos gesetzt */

/**
 * Dieses Verhalten fuehrt ein Servo-Kommando aus und schaltet danach den Servo wieder ab
 * \param *data der Verhaltensdatensatz
 */
void bot_servo_behaviour(Behaviour_t * data) {
	BLOCK_BEHAVIOUR(data, 1000); // 1 s warten

	return_from_behaviour(data); // Verhalten aus
	servo_set(servo_nr, SERVO_OFF); // Servo aus
	servo_active &= (uint8_t) (~servo_nr);
}

/**
 * Fahre den Servo an eine Position
 * \param *caller	Der Aufrufer
 * \param servo		Nummer des Servos
 * \param pos		Zielposition des Servos
 */
void bot_servo(Behaviour_t * caller, uint8_t servo, uint8_t pos) {
	if (pos == DOOR_CLOSE && sensDoor == 0) {
		return;	// Klappe ist bereits geschlossen
	}
	switch_to_behaviour(caller, bot_servo_behaviour, BEHAVIOUR_OVERRIDE); // Warte-Verhalten an

	servo_active |= servo;
	servo_set(servo, pos); // Servo-PWM einstellen
	servo_nr = servo; // Servo-Nr speichern
}
#endif // BEHAVIOUR_SERVO_AVAILABLE
