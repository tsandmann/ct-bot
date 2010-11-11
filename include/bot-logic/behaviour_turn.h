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
 * @file 	behaviour_turn.h
 * @brief 	Drehe den Bot
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */

#ifndef BEHAVIOUR_TURN_H_
#define BEHAVIOUR_TURN_H_

#ifdef BEHAVIOUR_TURN_AVAILABLE
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
extern float turn_last_err; /*!< letzter Drehfehler in Grad */
#endif

/*!
 * Das Verhalten laesst den Bot eine Punktdrehung durchfuehren.
 * @param *data 	Der Verhaltensdatensatz
 * @see bot_turn()
 * Das Drehen findet in mehreren Schritten statt. Die Drehung wird dabei
 * zunaechst mit hoeherer Geschwindigkeit ausgefuehrt. Bei kleineren
 * Winkeln dann nur noch mit geringer Geschwindigkeit.
 */
void bot_turn_behaviour(Behaviour_t * data);

/*!
 * Dreht den Bot im mathematischen Drehsinn.
 * @param *caller	Der Aufrufer
 * @param degrees 	Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * 					zwischen -360 und +360
 */
void bot_turn(Behaviour_t * caller, int16_t degrees);

/*!
 * Dreht den Bot im mathematischen Drehsinn.
 * @param *caller	Der Aufrufer
 * @param degrees 	Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * 					zwischen -360 und +360
 * @param speed		maximale Drehgeschwindigkeit [mm/s]
 */
void bot_turn_speed(Behaviour_t * caller, int16_t degrees, uint16_t speed);

#endif // BEHAVIOUR_TURN_AVAILABLE
#endif // BEHAVIOUR_TURN_H_
