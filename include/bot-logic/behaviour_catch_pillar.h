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
 * \file 	behaviour_catch_pillar.h
 * \brief 	Sucht nach einer Dose und faengt sie ein
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	08.12.2006
 */

#ifndef BEHAVIOUR_CATCH_PILLAR_H_
#define BEHAVIOUR_CATCH_PILLAR_H_

#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
/**
 * Fange eine Dose ein
 * \param *data	Der Verhaltensdatensatz
 */
void bot_catch_pillar_behaviour(Behaviour_t * data);

/**
 * Fange ein Objekt ein
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * \param degrees	Wie weit [Grad] soll maximal gedreht werden?
 */
void bot_catch_pillar_turn(Behaviour_t * caller, int16_t degrees);

/**
 * Fange ein Objekt ein
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_catch_pillar(Behaviour_t * caller);

/**
 * Gibt die Dose wieder aus, Entladevorgang
 *\param *data	Der Verhaltensdatensatz
 */
void bot_unload_pillar_behaviour(Behaviour_t * data);

/**
 * Entlaedt das Objekt wieder
 * \param caller Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_unload_pillar(Behaviour_t * caller);

#endif // BEHAVIOUR_CATCH_PILLAR_AVAILABLE
#endif // BEHAVIOUR_CATCH_PILLAR_H_
