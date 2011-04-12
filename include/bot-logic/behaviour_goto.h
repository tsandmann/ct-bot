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

/*! @file 	behaviour_goto.h
 * @brief 	Bot faehrt einen Punkt an
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#ifndef BEHAVIOUR_GOTO_H_
#define BEHAVIOUR_GOTO_H_

#ifdef BEHAVIOUR_GOTO_AVAILABLE
/*!
 * Kuemmert sich intern um die Ausfuehrung der goto-Kommandos,
 * @param *data der Verhaltensdatensatz
 * @see bot_goto()
 */
void bot_goto_behaviour(Behaviour_t  *data);

/*!
 * Drehe die Raeder um die gegebene Zahl an Encoder-Schritten weiter
 * @param *caller Verhaltensdatensatz des Aufrufers
 * @param left Schritte links
 * @param right Schritte rechts
 */
void bot_goto(Behaviour_t * caller, int16_t left, int16_t right);
#endif // BEHAVIOUR_GOTO_AVAILABLE
#endif // BEHAVIOUR_GOTO_H_
