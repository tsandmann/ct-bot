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
 * @file 	behaviour_get_utilization.h
 * @brief 	Misst die CPU-Auslastung eines anderen Verhaltens
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	14.12.2008
 */

#ifndef BEHAVIOUR_GET_UTILIZATION_H_
#define BEHAVIOUR_GET_UTILIZATION_H_

#ifdef BEHAVIOUR_GET_UTILIZATION_AVAILABLE
/*!
 * Verhalten, das die CPU-Auslastung eines anderen Verhaltens misst
 * @param *data	Der Verhaltensdatensatz
 */
void bot_get_utilization_behaviour(Behaviour_t * data);

/*!
 * Verhalten, das die CPU-Auslastung eines anderen Verhaltens misst
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param behaviour	Prioritaet des zu ueberwachenden Verhaltens
 */
void bot_get_utilization(Behaviour_t * caller, uint8_t behaviour);

#endif // BEHAVIOUR_GET_UTILIZATION_AVAILABLE
#endif // BEHAVIOUR_GET_UTILIZATION_H_
