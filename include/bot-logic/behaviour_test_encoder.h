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
 * @file 	behaviour_test_encoder.h
 * @brief 	Verhalten, das die Genauigkeit der Encoder-Auswertung testet. Nur zu Debugging-Zwecken.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	08.02.2010
 */

#ifndef BEHAVIOUR_TEST_ENCODER_H_
#define BEHAVIOUR_TEST_ENCODER_H_

#ifdef BEHAVIOUR_TEST_ENCODER_AVAILABLE
/*!
 * Encoder-Test-Verhalten
 * @param *data der Verhaltensdatensatz
 */
void bot_test_encoder_behaviour(Behaviour_t * data);

/*!
 * Botenfunktion des Encoder-Test-Verhaltens
 * @param *caller Der Verhaltensdatensatz des Aufrufers
 */
void bot_test_encoder(Behaviour_t * caller);

#endif // BEHAVIOUR_TEST_ENCODER_AVAILABLE
#endif // BEHAVIOUR_TEST_ENCODER_H_
