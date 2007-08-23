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



/*! @file 	behaviour_catch_pillar.h
 * @brief 	sucht nach einer Dose und fängt sie ein
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	08.12.06
*/

#ifndef BEHAVIOUR_CATCH_PILLAR_H_
#define BEHAVIOUR_CATCH_PILLAR_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
/*! 
 * Fange eine Dose ein 
 * @param *data der Verhaltensdatensatz
 */
void bot_catch_pillar_behaviour(Behaviour_t *data);

/*!
 * Botenfkt
 * Fange eine Dose ein 
 * @param caller Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_catch_pillar(Behaviour_t * caller);

/*!
 * Gibt die Dose wieder aus, Entladevorgang
 * @param *data der Verhaltensdatensatz
 */
void bot_unload_pillar_behaviour(Behaviour_t *data);

/*!
 * Botenfkt
 * Entlaedt das Objekt wieder
 * @param caller Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_unload_pillar(Behaviour_t * caller);

#endif

#endif /*BEHAVIOUR_CATCH_PILLAR_H_*/
