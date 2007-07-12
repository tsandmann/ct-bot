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


/*! @file 	behaviour_delay.h
 * @brief 	Delay-Routinen als Verhalten
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	12.07.07
*/


#ifndef BEHAVIOUR_DELAY_H_
#define BEHAVIOUR_DELAY_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"


/*!
 * Rufe das Delay-Verhalten auf 
 * @param caller		Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param delay_time	Die Verzögerungszeit in ms
 * @return	-1 wenn was schief gelaufen ist, sonst 0
 */
int8 bot_delay(Behaviour_t * caller, uint16 delay_time);

/*!
 * Verhalten fuer Delays
 */
void bot_delay_behaviour(Behaviour_t *data);

#endif /*BEHAVIOUR_DELAY_H_*/
