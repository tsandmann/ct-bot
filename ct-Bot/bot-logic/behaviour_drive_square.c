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
 * @file 	behaviour_drive_square.c
 * @brief 	Bot faehrt im Quadrat
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */


#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE

#define STATE_FORWARD		0
#define STATE_TURN 			1
#define STATE_INTERRUPTED	2
	
static uint8 state = STATE_FORWARD;	/*!< Status des Verhaltens */

/*!
 * Laesst den Roboter ein Quadrat abfahren.
 * Einfaches Beispiel fuer ein Verhalten, das einen Zustand besitzt.
 * Es greift auf andere Behaviours zurueck und setzt daher 
 * selbst keine speedWishes.
 * @param *data der Verhaltensdatensatz
 */
void bot_drive_square_behaviour(Behaviour_t *data) {
	switch (state) {
	case STATE_FORWARD:
		/* Vorwaerts */
		bot_drive_distance(data, 0, BOT_SPEED_FOLLOW, 20);
		state = STATE_TURN;
		break;

	case STATE_TURN:
		/* Drehen */
		bot_turn(data, 90);
		state = STATE_FORWARD;
		break;

	default:
		/* Sind wir fertig, dann Kontrolle zurueck an Aufrufer */
		return_from_behaviour(data);
		break;
	}
}

/*! 
 * Laesst den Roboter ein Quadrat abfahren.
 * @param caller Der obligatorische Verhaltensdatensatz des aufrufers
 */
void bot_drive_square(Behaviour_t* caller) {
	switch_to_behaviour(caller, bot_drive_square_behaviour, OVERRIDE);
	state = STATE_FORWARD;
}

#endif	// BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
