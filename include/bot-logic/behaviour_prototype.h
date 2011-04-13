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
 * @file 	behaviour_prototype.h
 * @brief 	Rohling für eigene Verhalten
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	13.04.2011
 */

#ifndef BEHAVIOUR_PROTOTYPE_H_
#define BEHAVIOUR_PROTOTYPE_H_

#ifdef BEHAVIOUR_PROTOTYPE_AVAILABLE
/*! 
 * Prototyp für ein Verhalten
 *
 * Bitte umbenennen.
 * Siehe auch include/bot-logik/available_behaviours
 * und
 * bot-logic/bot-logic.c in der Funktion bot_behave_init(void)
 * 
 * @param *data der Verhaltensdatensatz
 */
void bot_prototype_behaviour(Behaviour_t * data);

/*!
 * Rufe das Prototyp-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_prototype(Behaviour_t * caller);


// Alternative Botenfunktion mit Uebergabeparameter
/*!
 * Rufe das Prototyp-Verhalten auf
 * und nutze dabei einen Uebergabeparameter
 * @param *caller Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param param Uebergabeparameter
 */

//void bot_prototype(Behaviour_t * caller, int16_t param);

#endif // BEHAVIOUR_PROTOTYPE_AVAILABLE
#endif // BEHAVIOUR_PROTOTYPE_H_
