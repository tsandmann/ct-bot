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
 * @file 	behaviour_gotoxy.h
 * @brief 	Bot faehrt eine Position an
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */


/*!
 * bot_gotoxy() beruecksichtig keine waehrend der Fahrt aufgetretenen Fehler, daher ist die
 * Endposition nicht unbedingt auch die gewuenschte Position des Bots. Das komplexere Verhalten
 * bot_goto_pos() arbeitet hier deutlich genauer, darum werden jetzt alle bot_gotoxy()-Aufrufe
 * auf das goto_pos-Verhalten "umgeleitet", falls dieses vorhanden ist. 
 * Moechte man das jedoch nicht und lieber weiterhin das alte bot_gotxy-Verhalten, deaktiviert 
 * man den Schalter USE_GOTO_POS_XY ein paar Zeilen unter diesem Text, indem man ihm // voranstellt. 
 */

#ifndef BEHAVIOUR_GOTOXY_H_
#define BEHAVIOUR_GOTOXY_H_

#include "bot-logic/bot-logik.h"

#define USE_GOTO_POS_XY	/*!< Ersetzt alle goto_xy()-Aufrufe mit dem goto_pos-Verhalten, falls vorhanden */


#ifndef BEHAVIOUR_GOTO_POS_AVAILABLE
#undef USE_GOTO_POS_XY
#endif

#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
#ifndef USE_GOTO_POS_XY
/*!
 * Das Verhalten faehrt von der aktuellen Position zur angegebenen Position (x/y)
 * @param *data der Verhaltensdatensatz
 * Verbessert von Thomas Noll, Jens Schoemann, Ben Horst (Philipps-Universitaet Marburg)
 */
void bot_gotoxy_behaviour(Behaviour_t *data);

/*!
 * Botenfunktion: Das Verhalten faehrt von der aktuellen Position zur angegebenen Position (x/y)
 * @param caller	Aufrufendes Verhalten
 * @param x 		X-Ordinate an die der Bot fahren soll
 * @param y 		Y-Ordinate an die der Bot fahren soll
 */
void bot_gotoxy(Behaviour_t * caller, int16_t x, int16_t y);

#else	// USE_GOTO_POS_XY
/* wenn goto_pos() vorhanden und USE_GOTO_POS_XY an ist, leiten wir alle goto_xy()-Aufrufe dorthin um */
#undef BEHAVIOUR_GOTOXY_AVAILABLE
#define bot_gotoxy(caller, x, y)	bot_goto_pos(caller, x, y, 999)
#endif	// USE_GOTO_POS_XY

#endif	// BEHAVIOUR_GOTOXY_AVAILABLE
#endif	/*BEHAVIOUR_GOTOXY_H_*/
