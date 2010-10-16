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
 * @file 	behaviour_follow_object.h
 * @brief 	Verfolgung beweglicher Objekte
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	03.08.2007
 */

#ifndef BEHAVIOUR_FOLLOW_OBJECT_H_
#define BEHAVIOUR_FOLLOW_OBJECT_H_

#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE

/*!
 * @brief		Das Objektverfolgungsverhalten
 * @param data	Der Verhaltensdatensatz
 */
void bot_follow_object_behaviour(Behaviour_t * data);

/*!
 * @brief			Botenfunktion des Objektverfolgungsverhaltens
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_follow_object(Behaviour_t * caller);

#endif	// BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE
#endif	// BEHAVIOUR_FOLLOW_OBJECT_H_
