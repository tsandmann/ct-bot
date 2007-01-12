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

/*! @file 	behaviour_drive_square.c
 * @brief 	Bot faehrt im Quadrat
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/


#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
/*!
 * Laesst den Roboter ein Quadrat abfahren.
 * Einfaches Beispiel fuer ein Verhalten, das einen Zustand besitzt.
 * Es greift auf andere Behaviours zurueck und setzt daher 
 * selbst keine speedWishes.
 * @param *data der Verhaltensdatensatz
 */
void bot_drive_square_behaviour(Behaviour_t *data){
	#define STATE_TURN 1
	#define STATE_FORWARD 0
	#define STATE_INTERRUPTED 2
	
	static uint8 state = STATE_FORWARD;

   if (data->subResult == SUBFAIL) // letzter Auftrag schlug fehl?
   		state= STATE_INTERRUPTED;
	
	switch (state) {
		case STATE_FORWARD: // Vorwaerts
		   bot_goto(data,100,100);
		   state = STATE_TURN;
		   break;
		case STATE_TURN: // Drehen
		   bot_goto(data,22,-22);
		   state=STATE_FORWARD;
		   break;		
		case STATE_INTERRUPTED:
			return_from_behaviour(data);	// Beleidigt sein und sich selbst deaktiviern			
			break;   
		   
		default:		/* Sind wir fertig, dann Kontrolle zurueck an Aufrufer */
			return_from_behaviour(data);
			break;
	}
}
#endif
