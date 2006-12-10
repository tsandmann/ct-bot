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

/*! @file 	motor-low_pc.c
 * @brief 	Low-Level Routinen fuer die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include <stdlib.h>
#include "command.h"
#include "bot-2-sim.h"	
#include "motor-low.h"
#include "motor.h"


volatile int16 motor_left;	/*!< zuletzt gestellter Wert linker Motor */
volatile int16 motor_right;	/*!< zuletzt gestellter Wert rechter Motor */

/*!
 *  Initilisiert alles fuer die Motosteuerung 
 */
void motor_low_init(){
}

/*!
 * Unmittelbarer Zugriff auf die beiden Motoren,
 * normalerweise NICHT verwenden!
 * @param left PWM links
 * @param right PWM rechts
*/
void bot_motor(int16 left, int16 right){
	command_write(CMD_AKT_MOT, SUB_CMD_NORM ,&left,&right,0);
	
	if (right < 0 )
		direction.right= DIRECTION_BACKWARD;
	if (right > 0 )
		direction.right= DIRECTION_FORWARD;
	if (left < 0 )
		direction.left= DIRECTION_BACKWARD;
	if (left > 0 )
		direction.left= DIRECTION_FORWARD;

	motor_left=left;
	motor_right=right;
		
}

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 * @param servo Nummer des Servos
 * @param servo Zielwert
 */
void servo_low(uint8 servo, uint8 pos){
	int16 p = pos;
	int16 s = servo;
	command_write(CMD_AKT_SERVO, SUB_CMD_NORM ,&s,&p,0);
}

#endif
