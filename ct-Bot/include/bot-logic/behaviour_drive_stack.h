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
 * @file 	behaviour_drive_stack.h
 * @brief 	Anfahren aller auf dem Stack befindlichen Punkte
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	13.12.2007
 */

#ifndef BEHAVIOUR_DRIVESTACK_H_
#define BEHAVIOUR_DRIVESTACK_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"


#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE

/*!
 * Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte, wobei das Fahr-Unterverhalten bot_goto_pos benutzt wird
 * @param *data	Der Verhaltensdatensatz
 */
void bot_drive_stack_behaviour(Behaviour_t * data);

/*!
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_stack(Behaviour_t * caller);

/*!
 * Sichern der aktuellen Botposition auf den Stack
 * @param *caller	einfach nur Zeiger, damit remotecall verwendbar
 */
void bot_push_actpos(Behaviour_t * caller);

/*!
 * Speichern der uebergebenen Koordinaten auf dem Stack
 * @param pos_x X-Koordinate
 * @param pos_y Y-Koordinate
 */
void bot_push_pos(int16_t pos_x, int16_t pos_y);

/*!
 * Display zum Setzen und Anfahren der Stackpunkte 
 */
void drive_stack_display(void);

#endif	// BEHAVIOUR_DRIVE_STACK_AVAILABLE
#endif /*BEHAVIOUR_DRIVESTACK_H_*/
