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
 * @file 	behaviour_goto_pos.h
 * @brief 	Anfahren einer Position
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	15.10.2007
 */

#ifndef BEHAVIOUR_GOTO_POS_H_
#define BEHAVIOUR_GOTO_POS_H_

#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE

/*!
 * @brief		Das Positionierungsverhalten
 * @param data	Der Verhaltensdatensatz
 */
void bot_goto_pos_behaviour(Behaviour_t * data);

/*!
 * @brief			Botenfunktion des Positionierungsverhaltens.
 * 					Faehrt einen absoluten angegebenen Punkt an und dreht den Bot in die gewuenschte Blickrichtung.
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param x			x-Komponente des Ziels
 * @param y			y-Komponente des Ziels
 * @param head		neue Blickrichtung am Zielpunkt
 */
void bot_goto_pos(Behaviour_t * caller, int16_t x, int16_t y, int16_t head);

/*!
 * @brief			Botenfunktion des Positionierungsverhaltens.
 * 					Faehrt einen als Verschiebungsvektor angegebenen Punkt an und dreht den Bot in die gewuenschte Blickrichtung.
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param x			x-Komponente des Vektors vom Standort zum Ziel
 * @param y			y-Komponente des Vektors vom Standort zum Ziel
 * @param head		neue Blickrichtung am Zielpunkt oder 999, falls egal
 */
void bot_goto_pos_rel(Behaviour_t * caller, int16_t x, int16_t y, int16_t head);

/*!
 * @brief			Botenfunktion des Positionierungsverhaltens.
 * 					Bewegt den Bot um distance mm in aktueller Blickrichtung ("drive_distance(...)")
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param distance	Distanz in mm, die der Bot fahren soll
 * @param dir		Fahrtrichtung: >=0: vorwaerts, <0 rueckwaerts
 */
void bot_goto_dist(Behaviour_t * caller, int16_t distance, int16_t dir);

/*!
 * @brief		Hilfsverhalten von bot_goto_pos(), das den Bot auf eine gewuenschte Entfernung
 * 				an ein Hindernis heranfaehrt.
 * @param *data	Der Verhaltensdatensatz
 */
void bot_goto_obstacle_behaviour(Behaviour_t * data);

/*!
 * @brief			Botenfunktion des Positionierungsverhaltens.
 * 					Bewegt den Bot auf distance mm in aktueller Blickrichtung an ein Hindernis heran
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param distance	Distanz in mm, in der der Bot vor dem Hindernis stehen bleiben soll
 */
void bot_goto_obstacle(Behaviour_t * caller, int16_t distance);

#endif	// BEHAVIOUR_GOTO_POS_AVAILABLE
#endif	/*BEHAVIOUR_GOTO_POS_H_*/
