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



/*! @file 	behaviour_servo.h
 * @brief 	kontrolliert die Servos
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	07.12.06
*/

#ifndef BEHAVIOUR_SERVO_H_
#define BEHAVIOUR_SERVO_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_SERVO_AVAILABLE

uint8 servo_active;	/*!< 0, wenn kein Servo aktiv, sonst Bit der gerade aktiven Servos gesetzt */

/*! 
 * Dieses Verhalten fuehrt ein Servo-Kommando aus und schaltet danach den Servo wieder ab
 * 
 * @param *data der Verhaltensdatensatz
 */
void bot_servo_behaviour(Behaviour_t *data);

/*!
 * Fahre den Servo an eine Position
 * @param servo Nummer des Servos
 * @param pos Zielposition des Servos
 */
void bot_servo(Behaviour_t * caller, uint8 servo, uint8 pos);

#endif

#endif /*BEHAVIOUR_SIMPLE_H_*/
