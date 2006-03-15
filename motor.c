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

/*! @file 	motor.c
 * @brief 	High-Level-Routinen fuer die Motorsteuerung des c't-Bot
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
*/
#include <stdlib.h>
#include "global.h"
#include "ct-Bot.h"
#include "motor.h"
#include "motor-low.h"

volatile int16 speed_l=0;	/*!< Geschwindigkeit linker Motor */
volatile int16 speed_r=0;	/*!< Geschwindigkeit rechter Motor */

direction_t direction;		/*!< Drehrichtung der Motoren */

/*!
 * Direkter Zugriff auf den Motor
 * @param left	Geschwindigkeit fuer den linken Motor
 * @param right Geschwindigkeit fuer den linken Motor
 * Geschwindigkeit liegt zwischen -255 und +255.
 * 0 bedeutet Stillstand, 255 volle Kraft voraus, -255 volle Kraft zurueck.
 * Sinnvoll ist die Verwendung der Konstanten: BOT_SPEED_XXX, 
 * also z.B. motor_set(BOT_SPEED_LOW,-BOT_SPEED_LOW);
 * fuer eine langsame Drehung
*/
void motor_set(int16 left, int16 right){
	if (left == BOT_SPEED_IGNORE)	
		left=BOT_SPEED_STOP;
		
	if (abs(left) > BOT_SPEED_MAX)	// Nicht schneller fahren als moeglich
		speed_l = BOT_SPEED_MAX;
	else if (left == 0)				// Stop wird nicht veraendert
		speed_l = BOT_SPEED_STOP;
	else if (abs(left) < BOT_SPEED_SLOW)	// Nicht langsamer als die 
		speed_l = BOT_SPEED_SLOW;	// Motoren koennen
	else				// Sonst den Wunsch uebernehmen
		speed_l = abs(left);


	if (right == BOT_SPEED_IGNORE)	
		right=BOT_SPEED_STOP;

	if (abs(right) > BOT_SPEED_MAX)// Nicht schneller fahren als moeglich
		speed_r = BOT_SPEED_MAX;
	else if (abs(right) == 0)	// Stop wird nicht veraendert
		speed_r = BOT_SPEED_STOP;
	else if (abs(right) < BOT_SPEED_SLOW)	// Nicht langsamer als die 
		speed_r = BOT_SPEED_SLOW;	// Motoren koennen
	else				// Sonst den Wunsch uebernehmen
		speed_r = abs(right);
	
	if (left < 0 ){
		speed_l=-speed_l;
		direction.left= DIRECTION_BACKWARD;
	} else  if (left > 0 )
		direction.left= DIRECTION_FORWARD;
	
	if (right < 0 )	{
		speed_r=-speed_r;
		direction.right= DIRECTION_BACKWARD;
	} else if (right > 0 )
		direction.right= DIRECTION_FORWARD;
			
	bot_motor(speed_l,speed_r);
}

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 * @param servo Nummer des Servos
 * @param servo Zielwert
 */
void servo_set(char servo, char pos){
	if (pos< SERVO_LEFT)
		pos=SERVO_LEFT;
	if (pos> SERVO_RIGHT)
		pos=SERVO_RIGHT;
		
	bot_servo(servo,pos);
}

/*!
 * Initialisiere den Motorkrams
 */
void motor_init(void){
	speed_l=0;
	speed_r=0;
	motor_low_init();
}
