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

/*! @file 	behaviour_avoid_border.h
 * @brief 	Vermeide Abgruende
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#ifndef BEHAVIOUR_AVOID_BORDER_H_
#define BEHAVIOUR_AVOID_BORDER_H_

#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
/*!
 * Verhindert, dass der Bot in Graeben faellt
 * @param *data der Verhaltensdatensatz
 */
void bot_avoid_border_behaviour(Behaviour_t *data);
#endif

#endif /*BEHAVIOUR_AVOID_BORDER_H_*/
