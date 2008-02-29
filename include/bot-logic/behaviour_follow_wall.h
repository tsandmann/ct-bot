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
 * @file 	behaviour_follow_wall.h
 * @brief 	Wandfolger, der sich an Wand gewisse Zeit dreht; solange bis kein Hindernis mehr sichtbar ist
 * @author 	Frank Menzel(Menzelfr@gmx.net)
 * @date 	30.08.2007
 */

#include "bot-logic/bot-logik.h"

#ifndef BEHAVIOUR_FOLLOW_WALL_H_
#define BEHAVIOUR_FOLLOW_WALL_H_

#ifdef BEHAVIOUR_FOLLOW_WALL_AVAILABLE


void border_follow_wall_handler(void);

/*!
 * Faehrt vorwaerts bis zu einer Wand, von die er sich wegdreht
 * @param data Verhaltensdatensatz
 */
void bot_follow_wall_behaviour(Behaviour_t *data);

/*! 
 * Botenfunktion
 * Faehrt vorwaerts bis zu einer Wand, von die er sich wegdreht
 * @param check 	Abbruchfunktion; wenn diese True liefert wird das Verhalten beendet sonst endlos
 * 			        einfach NULL uebergeben, wenn keine definiert ist 
 * @param caller 	Verhaltensdatensatz
 */
void bot_follow_wall(Behaviour_t *caller,uint8 (*check)(void));

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
/*! 
 * Botenverhalten zum Aufruf via Remotecall ohne weitere params, d.h. da kein Abbruchverhalten
 * uebergeben wird, ist dies dann ein Endlos-Explorerverhalten 
 * @param caller Verhaltensdatensatz
 */
void bot_do_wall_explore(Behaviour_t *caller);
#endif	// BEHAVIOUR_REMOTECALL_AVAILABLE

#endif	// BEHAVIOUR_FOLLOW_WALL_AVAILABLE
#endif	/*BEHAVIOUR_FOLLOW_WALL_H_*/
