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
 * @file 	behaviour_hang_on.c
 * @brief 	Erkennen des Haengenbleibens als Notfallverhalten 
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	31.07.2007
 */

#include "bot-logic/bot-logik.h"
#include <stdlib.h>
#include <math.h>

#ifdef BEHAVIOUR_HANG_ON_AVAILABLE

/*!
 * Notfallhandler, ausgefuehrt nach Haengenbleiben zum rueckwaertsfahren; muss registriert werden zum
 * rueckwaertsfahren
 */
void hang_on_handler(void) {
	// Routine muss zuerst checken, ob das hang_on Verhalten auch gerade aktiv ist, da nur in diesem
	// Fall etwas gemacht werden muss
	if (!behaviour_is_activated(bot_hang_on_behaviour)) 
		return;

    #ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
	  bot_drive_distance(0,0,-BOT_SPEED_FOLLOW,5);	// 5cm rueckwaerts bei Haengenbleiben	
    #endif
}


/*!
 * Verhalten zum Erkennen des Haengenbleibens des Bots; wenn MEASURE_MOUSE_AVAILABLE gesetzt werden Maus- und
 * Encoderdaten miteinander verglichen; wird der Bot-Stillstand erkannt, wird etwas rueckwaerts gefahren
 * @param *data der Verhaltensdatensatz
 */
void bot_hang_on_behaviour(Behaviour_t * data) {	 
	if  ( !((abs(v_enc_left-v_mou_left) < STUCK_DIFF) && (abs(v_enc_right-v_mou_right) < STUCK_DIFF)) ) {            
		// Es kann entweder durch die eigene Notfallroutine rueckwaertsgefahren werden oder die Notverhalten 
		// machen dies in ihren eigenen registrierten Notfallroutinen

		// Ausfuehren der registrierten Notfallverhalten; durch registrierung der verhaltenseigenen 
		// Notfallroutinen koennen diese entsprechend reagieren
		start_registered_emergency_procs();
	}
}
#endif	// BEHAVIOUR_HANG_ON_AVAILABLE
