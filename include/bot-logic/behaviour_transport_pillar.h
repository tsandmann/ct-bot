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
 * @file 	behaviour_transport_pillar.h
 * @brief 	Hin- und Herfahren zwischen zwei Positionen und evtl. Einfangen von Objekten
 * @author 	Frank Menzel (menzelfr@gmx.net)
 * @date 	23.10.2007
 */

#ifndef BEHAVIOUR_TRANSPORT_PILLAR_H_
#define BEHAVIOUR_TRANSPORT_PILLAR_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE

/*!
 * Transport-Pillarverhalten  
 * @param *data	Der Verhaltensdatensatz
 */
void bot_transport_pillar_behaviour(Behaviour_t * data);

/*!
 * Ruft das Pillarverhalten auf 
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_transport_pillar(Behaviour_t * caller);

/*!
 * Routine zum Setzen der Zielkoordinaten auf der Zielposition/ Zielpad 
 * @param x X-World-Zielkoordinate
 * @param y Y-World-Zielkoordinate
 */
void bot_set_destkoords(float x, float y);

/*! 
 * @brief	Display zum Start der Transport_Pillar-Routinen
 */
void transportpillar_display(void);

#endif	// BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
#endif	/*BEHAVIOUR_TRANSPORT_PILLAR_H_*/
