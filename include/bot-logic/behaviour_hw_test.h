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
 * \file 	behaviour_hw_test.h
 * \brief 	Testcode fuer die Bot-Hardware (ehemals TEST_AVAILABLE_ANALOG, _DIGITAL, _MOTOR)
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	22.05.2011
 */

#ifndef BEHAVIOUR_HW_TEST_H_
#define BEHAVIOUR_HW_TEST_H_

#ifdef BEHAVIOUR_HW_TEST_AVAILABLE
/**
 * Das Testverhalten
 * \param data	Der Verhaltensdatensatz
 */
void bot_hw_test_behaviour(Behaviour_t * data);

/**
 * Veraendert den Testmode
 * \param *caller Zeiger auf Verhaltensdatensatz des Aufrufers
 * \param mode neuer Mode (0: analog, 1: digital, 2: motor)
 */
void bot_hw_test(Behaviour_t * caller, uint8_t mode);

#endif // BEHAVIOUR_HW_TEST_AVAILABLE
#endif // BEHAVIOUR_HW_TEST_H_
