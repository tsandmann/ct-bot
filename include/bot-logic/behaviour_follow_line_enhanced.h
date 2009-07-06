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
 * @file 	behaviour_follow_line_enhanced.h
 * @brief 	erweiterter Linienverfolger, der auch mit Unterbrechungen und Hindernissen klarkommt
 * @author 	Frank Menzel (Menzelfr@gmx.de)
 * @date 	25.02.2009
 */

#include "bot-logic/bot-logik.h"

#ifndef BEHAVIOUR_FOLLOW_LINE_ENHANCED_H_
#define BEHAVIOUR_FOLLOW_LINE_ENHANCED_H_

#ifdef BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE

/*!
 * erweiterter Linienfolger, der auch Linienunterbrechungen und Hindernisse handhabt, waehrend der Bot die Linie verfolgt;
 * die Linienunterbrechung darf nur relativ klein sein (~3cm), so dass sich beim Drehen am Ende der Linie der rechte Abgrundsensor
 * ueber dem Neubeginn der unterbrochenen Linie drehn muss
 * @param *data	Verhaltensdatensatz
 */
void bot_follow_line_enh_behaviour(Behaviour_t * data);

/*!
 * Botenverhalten fuer den erweiterten Linienfolger
 * @param *caller Verhaltensdatensatz des Aufrufers
 */
void bot_follow_line_enh(Behaviour_t * caller);

#endif	// BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE
#endif	/*BEHAVIOUR_FOLLOW_LINE_ENHANCED_H_*/

