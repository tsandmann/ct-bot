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
 * \file 	behaviour_goto_pos.h
 * \brief 	Anfahren einer Position
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	15.10.2007
 */

#ifndef BEHAVIOUR_GOTO_POS_H_
#define BEHAVIOUR_GOTO_POS_H_

#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
/**
 * Das Positionierungsverhalten
 * \param data	Der Verhaltensdatensatz
 */
void bot_goto_pos_behaviour(Behaviour_t * data);

/**
 * Botenfunktion des Positionierungsverhaltens.
 * Faehrt einen absoluten angegebenen Punkt an und dreht den Bot in die gewuenschte Blickrichtung.
 * \param *caller	Der Verhaltensdatensatz des Aufrufers
 * \param x			x-Komponente des Ziels
 * \param y			y-Komponente des Ziels
 * \param head		neue Blickrichtung am Zielpunkt
 * \return			Zeiger auf Verhaltensdatensatz
 */
Behaviour_t * bot_goto_pos(Behaviour_t * caller, int16_t x, int16_t y, int16_t head);

/**
 * Botenfunktion des relativen Positionierungsverhaltens.
 * Faehrt einen als Verschiebungsvektor angegebenen Punkt an und dreht den Bot in die gewuenschte Blickrichtung.
 * \param *caller	Der Verhaltensdatensatz des Aufrufers
 * \param x			x-Komponente des Vektors vom Standort zum Ziel
 * \param y			y-Komponente des Vektors vom Standort zum Ziel
 * \param head		neue Blickrichtung am Zielpunkt oder 999, falls egal
 * \return			Zeiger auf Verhaltensdatensatz
 */
Behaviour_t * bot_goto_pos_rel(Behaviour_t * caller, int16_t x, int16_t y, int16_t head);

/**
 * Botenfunktion des Distanz-Positionierungsverhaltens.
 * Bewegt den Bot um distance mm in aktueller Blickrichtung ("drive_distance(...)")
 * \param *caller	Der Verhaltensdatensatz des Aufrufers
 * \param distance	Distanz in mm, die der Bot fahren soll
 * \param dir		Fahrtrichtung: >=0: vorwaerts, <0 rueckwaerts
 * \return			Zeiger auf Verhaltensdatensatz
 */
Behaviour_t * bot_goto_dist(Behaviour_t * caller, int16_t distance, int8_t dir);

#else // BEHAVIOUR_GOTO_POS_AVAILABLE
#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
Behaviour_t * bot_drive_distance(Behaviour_t * caller, int8_t curve, int16_t speed, int16_t cm);

/**
 * Botenfunktion des Distanz-Positionierungsverhaltens als Wrapper, falls das goto_pos-Verhalten nicht aktiv ist.
 * Bewegt den Bot um distance mm in aktueller Blickrichtung ("drive_distance(...)")
 * \param *caller	Der Verhaltensdatensatz des Aufrufers
 * \param distance	Distanz in mm, die der Bot fahren soll
 * \param dir		Fahrtrichtung: >=0: vorwaerts, <0 rueckwaerts
 * \return			Zeiger auf Verhaltensdatensatz
 */
static inline Behaviour_t * bot_goto_dist(Behaviour_t * caller, int16_t distance, int8_t dir) {
	if (dir == 0) {
		dir = 1;
	}
	return bot_drive_distance(caller, 0, BOT_SPEED_NORMAL * dir, distance / 10);
}
#endif //BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#endif // BEHAVIOUR_GOTO_POS_AVAILABLE
#endif // BEHAVIOUR_GOTO_POS_H_
