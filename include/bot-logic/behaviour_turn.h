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

/*! @file 	behaviour_turn.h
 * @brief 	Drehe den Bot
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#ifndef BEHAVIOUR_TURN_H_
#define BEHAVIOUR_TURN_H_

#ifdef BEHAVIOUR_TURN_AVAILABLE
#include "bot-logik.h"

 /*!
  * Das Verhalten laesst den Bot eine Punktdrehung durchfuehren. 
+ * Drehen findet in drei Schritten statt. Die Drehung wird dabei
+ * bei Winkeln > 15 Grad zunaechst mit hoeherer Geschwindigkeit ausgefuehrt. Bei kleineren
+ * Winkeln oder wenn nur noch 15 Grad zu drehen sind, nur noch mit geringer Geschwindigkeit
  * @param *data der Verhaltensdatensatz
  * @see bot_turn()
  */
void bot_turn_behaviour(Behaviour_t *data);

/*! 
 * Dreht den Bot im mathematisch positiven Sinn. 
 * @param degrees Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * Die Aufloesung betraegt rund 3 Grad
 */
void bot_turn(Behaviour_t* caller,int16 degrees);

#endif
#endif
