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
 * @file 	behaviour_turn_test.h
 * @brief 	Fuehrt mehrere Drehungen mit bot_turn() aus und misst die Fehler
 * @author	Timo Sandmann (mail@timosandmann.de)
 * @date 	08.07.2007
 */

#ifndef BEHAVIOUR_TURN_TEST_H_
#define BEHAVIOUR_TURN_TEST_H_

#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
/*!
 * Das eigentliche Verhalten
 * @param data	Zeiger auf den Verhaltensdatensatz
 * @see			bot_turn_test()
 */
void bot_turn_test_behaviour(Behaviour_t * data);

/*!
 * Testet das bot_turn-Verhalten und gibt Informationen ueber die Drehgenauigkeit aus
 * @param caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 */
void bot_turn_test(Behaviour_t * caller);

#endif // BEHAVIOUR_TURN_TEST_AVAILABLE
#endif // BEHAVIOUR_TURN_TEST_H_
