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
 * @file 	behaviour_goto_obstacle.h
 * @brief 	Anfahren eines Hindernisses
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	15.10.2007
 */

#ifndef BEHAVIOUR_GOTO_OBSTACLE_H_
#define BEHAVIOUR_GOTO_OBSTACLE_H_

#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE

/*!
 * Hilfsverhalten von bot_goto_pos(), das den Bot auf eine gewuenschte Entfernung
 * an ein Hindernis heranfaehrt.
 * @param *data	Der Verhaltensdatensatz
 */
void bot_goto_obstacle_behaviour(Behaviour_t * data);

/*!
 * Botenfunktion des goto_obstacle-Verhaltens.
 * Bewegt den Bot auf distance mm in aktueller Blickrichtung an ein Hindernis heran
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param distance	Distanz in mm, in der der Bot vor dem Hindernis stehen bleiben soll
 * @param parallel	richtet die Front des Bots parallel zum Hindernis aus, falls 1
 */
void bot_goto_obstacle(Behaviour_t * caller, int16_t distance, uint8_t parallel);

#endif	// BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
#endif	/* BEHAVIOUR_GOTO_OBSTACLE_H_ */
