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

/*! @file 	behaviour_gotoxy.c
 * @brief 	Bot faehrt eine Position an
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/


#ifndef BEHAVIOUR_GOTOXY_H_
#define BEHAVIOUR_GOTOXY_H_

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
/*!
 * Das Verhalten faehrt von der aktuellen Position zur angegebenen Position (x/y)
 * @param *data der Verhaltensdatensatz
 * Verbessert von Thomas Noll, Jens Schoemann, Ben Horst (Philipps-Universitaet Marburg)
 */
void bot_gotoxy_behaviour(Behaviour_t *data);

/*!
 * Botenfunktion: Das Verhalten faehrt von der aktuellen Position zur angegebenen Position (x/y)
 * @param caller Aufrufendes Verhalten
 * @param x X-Ordinate an die der Bot fahren soll
 * @param y Y-Ordinate an die der Bot fahren soll
 */
void bot_gotoxy(Behaviour_t *caller, float x, float y);
#endif

#endif /*BEHAVIOUR_GOTOXY_H_*/
