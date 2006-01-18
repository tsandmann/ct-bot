/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	bot-mot.h 
 * @brief 	Low-Level Routinen fuer die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#ifndef motor_low_H_
#define motor_low_H_

#include "global.h"

#define SERVO_LEFT 	8
#define SERVO_RIGHT	16
#define SERVO_MIDDLE   ((SERVO_RIGHT- SERVO-LEFT)/2)

#define SERVO1 1
#define SERVO2 2


/*!
 *  Initialisiert alles fuer die Motosteuerung 
 */
void motor_low_init(void);

/*!
 * Unmittelbarer Zugriff auf die beiden Motoren
 * normalerweise NICHT verwenden!!!!!
 * @param left speed links
 * @param right speed rechts
*/
void bot_motor(int16 left, int16 right);

/*!
 * Stellt die Servos.
 * Sinnvolle Werte liegen zwischen 8 und 16
 */
void servo_set(char servo, char pos);
#endif
