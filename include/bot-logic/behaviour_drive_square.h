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

/**
 * \file 	behaviour_drive_square.h
 * \brief 	Bot faehrt im Quadrat
 * \author 	Benjamin Benz (bbe\heise.de)
 * \date 	03.11.2006
 */


#ifndef BEHAVIOUR_DRIVE_SQUARE_H_
#define BEHAVIOUR_DRIVE_SQUARE_H_

#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
/**
 * Laesst den Roboter ein Quadrat abfahren.
 * Einfaches Beispiel fuer ein Verhalten, das einen Zustand besitzt.
 * Es greift auf andere Behaviours zurueck und setzt daher selbst keine speedWishes.
 * \param *data der Verhaltensdatensatz
 */
void bot_drive_square_behaviour(Behaviour_t* data);

/**
 * Laesst den Roboter ein Quadrat abfahren.
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * \param length	Seitenlaenge des Quadrats in mm
 */
Behaviour_t* bot_drive_square_len(Behaviour_t* caller, int16_t length);

/**
 * Laesst den Roboter ein Quadrat abfahren.
 * \param *caller Der obligatorische Verhaltensdatensatz des Aufrufers
 */
static inline Behaviour_t* bot_drive_square(Behaviour_t* caller) {
	return bot_drive_square_len(caller, 400);
}

#endif // BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
#endif // BEHAVIOUR_DRIVE_SQUARE_H_
