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
 * @file 	behaviour_calibrate_sharps.h
 * @brief 	Kalibriert die Distanzsensoren des Bots
 * 
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	21.04.2007
 */

#ifndef BEHAVIOUR_CALIBRATE_SHARPS_H_
#define BEHAVIOUR_CALIBRATE_SHARPS_H_

#ifdef BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
#include "bot-logic/bot-logik.h"


/*!
 * @brief		Das eigentliche Verhalten
 * @param data	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @see			bot_calibrate_sharps()
 * Die Funktionalitaet des Verhaltens ist aufgeteilt in: 
 * @see goto_next_pos(), @see measure_distance(), @see update_data()
 */
void bot_calibrate_sharps_behaviour(Behaviour_t *data);

/*!
 * @brief			Kalibriert die Distanzsensoren des Bots
 * @param caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 */
void bot_calibrate_sharps(Behaviour_t *caller);

/*!
 * @brief	Displayhandler fuer bot_calibrate_sharps-Verhalten
 */
void bot_calibrate_sharps_display(void);

#endif	// BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
#endif	// BEHAVIOUR_CALIBRATE_SHARPS_H_
