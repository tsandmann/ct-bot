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
 * @file 	behaviour_solve_maze.h
 * @brief 	Wandfolger durchs Labyrinth
 * @author 	Torsten Evers (tevers@onlinehome.de)
 * @date 	03.11.06
 */

#ifndef BEHAVIOUR_SOLVE_MAZE_H_
#define BEHAVIOUR_SOLVE_MAZE_H_

#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE

/*!
 * Das Verhalten findet seinen Weg durch ein Labyrinth, das nach gewissen Grundregeln gebaut ist
 * in nicht immer optimaler Weise aber in jedem Fall. Es arbeitet nach dem Hoehlenforscher-Algorithmus.
 * Einschraenkung: Objekte im Labyrinth, die Endlossschleifen verursachen koennen, z.b. ein einzeln
 * stehender Pfeiler im Labyrinth um den der Bot dann immer wieder herum fahren wuerde.
 * @param *caller	Verhaltensdatensatz des Aufrufers
 */
void bot_solve_maze(Behaviour_t *caller);

/*!
 * Das Verhalten findet seinen Weg durch ein Labyrinth, das nach gewissen Grundregeln gebaut ist
 * in nicht immer optimaler Weise aber in jedem Fall. Es arbeitet nach dem Hoehlenforscher-Algorithmus.
 * Einschraenkung: Objekte im Labyrinth, die Endlossschleifen verursachen koennen, z.b. ein einzeln
 * stehender Pfeiler im Labyrinth um den der Bot dann immer wieder herum fahren wuerde.
 * @param *data	Verhaltensdatensatz
 */
void bot_solve_maze_behaviour(Behaviour_t *data);

/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 * @param *data	Verhaltensdatensatz
 */
void bot_measure_angle_behaviour(Behaviour_t * data);

/*!
 * Das Verhalten dreht sich um 45 Grad in die angegebene Richtung (0=rechts, 1=links)
 * und prueft, ob auf dem Sensor auf der Seite der angegebenen Richtung eine Wand
 * im Abstand von 12cm zu sehen ist. Wenn dem so ist, wird die Variable wall_present
 * auf True gesetzt, sonst False
 * @param *data	Verhaltensdatensatz
 */
void bot_check_wall_behaviour(Behaviour_t * data);

#endif	// BEHAVIOUR_SOLVE_MAZE_AVAILABLE
#endif	/*BEHAVIOUR_SOLVE_MAZE_H_*/
