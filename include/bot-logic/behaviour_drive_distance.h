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

/*! @file 	behaviour_drive_distance.h
 * @brief 	Bot faehrt ein Stueck
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#ifndef BEHAVIOUR_DRIVE_DISTANCE_H_
#define BEHAVIOUR_DRIVE_DISTANCE_H_

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
/*!
 * Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren.
 * @param *data der Verhaltensdatensatz
 * @see bot_drive_distance() 
 */
void bot_drive_distance_behaviour(Behaviour_t* data);

/*! 
 * Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren. Dabei legt die Geschwindigkeit fest, ob der Bot vorwaerts oder rueckwaerts fahren soll.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll. Negative Werte lassen den Bot rueckwaerts fahren.
 * @param cm Gibt an, wie weit der Bot fahren soll. In cm :-) Die Strecke muss positiv sein, die Fahrtrichtung wird ueber speed geregelt.
 */
void bot_drive_distance(Behaviour_t* caller,int8 curve, int16 speed, int16 cm);

/*!
 * laesst den Bot in eine Richtung fahren. 
 * Es handelt sich hierbei nicht im eigentlichen Sinn um ein Verhalten, sondern ist nur eine Abstraktion der Motorkontrollen.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll. */
void bot_drive(int8 curve, int16 speed);

#endif
#endif /*BEHAVIOUR_DRIVE_DISTANCE_H_*/

