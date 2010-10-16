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
 * @file 	behaviour_classify_objects.h
 * @brief 	Teilt Objekte nach ihrer Farbe in Klassen ein und
 * 			transportiert sie ins Lager der Klasse.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	15.06.2008
 */


#ifndef BEHAVIOUR_CLASSIFY_OBJECTS_H_
#define BEHAVIOUR_CLASSIFY_OBJECTS_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE

/*!
 * Teilt Objekte nach ihrer Farbe in Klassen ein und
 * transportiert sie ins Lager der Klasse.
 * @param *data	Der Verhaltensdatensatz
 */
void bot_classify_objects_behaviour(Behaviour_t * data);

/*!
 * Teilt Objekte nach ihrer Farbe in Klassen ein und
 * transportiert sie ins Lager der Klasse.
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_classify_objects(Behaviour_t * caller);

#endif	// BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE
#endif	/*BEHAVIOUR_CLASSIFY_OBJECTS_H_*/
