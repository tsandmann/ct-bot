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
 * @file 	behaviour_drive_distance.h
 * @brief 	Bot faehrt ein Stueck
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */


/*!
 * bot_drive_distance() beruecksichtig keine waehrend der Fahrt aufgetretenen Fehler, daher ist die
 * Endposition nicht unbedingt auch die gewuenschte Position des Bots. Das komplexere Verhalten
 * bot_goto_pos() arbeitet hier deutlich genauer, darum werden jetzt alle bot_drive_distance()-Aufrufe
 * auf das goto_pos-Verhalten "umgeleitet", falls dieses vorhanden ist.
 * Moechte man das jedoch nicht und lieber weiterhin das alte drive_distance-Verhalten, deaktiviert
 * man den Schalter USE_GOTO_POS_DIST ein paar Zeilen unter diesem Text, indem man ihm // voranstellt.
 */

#ifndef BEHAVIOUR_DRIVE_DISTANCE_H_
#define BEHAVIOUR_DRIVE_DISTANCE_H_

#include "bot-logic/bot-logik.h"

#define USE_GOTO_POS_DIST	/*!< Ersetzt alle drive_distance()-Aufrufe mit dem goto_pos-Verhalten, falls vorhanden */

#ifndef BEHAVIOUR_GOTO_POS_AVAILABLE
#undef USE_GOTO_POS_DIST
#endif

#if defined BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE || defined BEHAVIOUR_OLYMPIC_AVAILABLE
/*!
 * laesst den Bot in eine Richtung fahren.
 * Es handelt sich hierbei nicht im eigentlichen Sinn um ein Verhalten, sondern ist nur eine Abstraktion der Motorkontrollen.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll. */
void bot_drive(int8 curve, int16 speed);
#endif	// BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE || BEHAVIOUR_OLYMPIC_AVAILABLE

#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#ifndef USE_GOTO_POS_DIST
/*!
 * Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren.
 * @param *data der Verhaltensdatensatz
 * @see bot_drive_distance()
 */
void bot_drive_distance_behaviour(Behaviour_t * data);

/*!
 * Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren. Dabei legt die Geschwindigkeit fest, ob der Bot vorwaerts oder rueckwaerts fahren soll.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll. Negative Werte lassen den Bot rueckwaerts fahren.
 * @param cm Gibt an, wie weit der Bot fahren soll. In cm :-) Die Strecke muss positiv sein, die Fahrtrichtung wird ueber speed geregelt.
 */
void bot_drive_distance(Behaviour_t * caller, int8_t curve, int16_t speed, int16_t cm);

#else	// USE_GOTO_POS_DIST
/* wenn goto_pos() vorhanden ist und USE_GOTO_POS_DIST an, leiten wir alle drive_distance()-Aufurfe dorthin um */
#undef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#define bot_drive_distance(caller, curve, speed, distance)	bot_goto_dist(caller, (distance*10), speed)
#endif	// USE_GOTO_POS_DIST
#endif	// BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#endif	/*BEHAVIOUR_DRIVE_DISTANCE_H_*/

