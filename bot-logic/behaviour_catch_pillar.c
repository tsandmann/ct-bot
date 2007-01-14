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


/*! @file 	behaviour_catch_pillar.c
 * @brief 	sucht nach einer Dose und fängt sie ein
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	08.12.06
*/


#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
#include <math.h>

#define MAX_PILLAR_DISTANCE	350

#define START 0
#define LEFT_FOUND 1
#define TURN_MIDDLE 2
#define GO 3

static uint8 pillar_state=START;
/*! 
 * Fange eine Dose ein 
 * @param *data der Verhaltensdatensatz
 */
void bot_catch_pillar_behaviour(Behaviour_t *data){	
	static float angle;
	
	switch (pillar_state){
		case START:
			if (sensDistL < MAX_PILLAR_DISTANCE){	// sieht der linke Sensor schon was?
				angle=heading;
				pillar_state=LEFT_FOUND;
			} else
				speedWishLeft=-BOT_SPEED_SLOW;
				speedWishRight=+BOT_SPEED_SLOW;
				//bot_turn(data,5);	// Ein Stueck drehen
			break;
		case LEFT_FOUND:
			if (sensDistR < MAX_PILLAR_DISTANCE){	// sieht der rechte Sensor schon was?
				angle= heading- angle;
				if (angle < 0)
					angle+=360;
				angle= heading - angle/2;
				pillar_state=TURN_MIDDLE;
//				bot_turn(data,-angle/2);
			}else
				speedWishLeft=-BOT_SPEED_SLOW;
				speedWishRight=+BOT_SPEED_SLOW;
//				bot_turn(data,5);	// Eins Stueck drehen
			break;
		case TURN_MIDDLE:
				if (fabs(heading - angle) > 2){
					speedWishLeft=+BOT_SPEED_SLOW;
					speedWishRight=-BOT_SPEED_SLOW;
				} else {
					bot_servo(data,SERVO1,DOOR_OPEN); // Klappe auf
					pillar_state=GO;
				}
			break;
		case GO:
			if (sensTrans ==0){
				speedWishLeft=+BOT_SPEED_SLOW;
				speedWishRight=+BOT_SPEED_SLOW;
			} else {
				bot_servo(data,SERVO1,DOOR_CLOSE);
				pillar_state++;
			}
			break;
			
		default:
			pillar_state=START;
			return_from_behaviour(data);
			break;
	}
}

/*!
 * Fange ein Objekt ein
 * @param caller Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_catch_pillar(Behaviour_t * caller){
	pillar_state=0;
	// Zielwerte speichern
	switch_to_behaviour(caller,bot_catch_pillar_behaviour,OVERRIDE);	
}
#endif
