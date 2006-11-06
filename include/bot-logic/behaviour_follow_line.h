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

/*! @file 	behaviour_follow_line.h
 * @brief 	Linienverfolger
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#include "bot-logic/bot-logik.h"

#ifndef BEHAVIOUR_FOLLOW_LINE_H_
#define BEHAVIOUR_FOLLOW_LINE_H_

#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE

/*! Folgt einer Linie, sobald beide Liniensensoren ausloesen
 * Die Linie sollte in etwa die Breite beider CNY70 haben
 */
void bot_follow_line_behaviour(Behaviour_t *data);

/*! Folgt einer Linie, sobald beide Liniensensoren ausloesen
 * Die Linie sollte in etwa die Breite beider CNY70 haben
 */
void bot_follow_line(Behaviour_t *caller);
#endif
#endif /*BEHAVIOUR_FOLLOW_LINE_H_*/
