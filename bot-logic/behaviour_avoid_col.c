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
 * @file 	behaviour_avoid_col.c
 * @brief 	Vermeide Kollisionen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.2006
 */


#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE

#define ZONE_CLOSEST	0			/*!< Zone fuer extremen Nahbereich */
#define ZONE_NEAR		1			/*!< Zone fuer Nahbereich */
#define ZONE_FAR		2			/*!< Zone fuer Fernbereich */
#define ZONE_CLEAR		3			/*!< Zone fuer Freien Bereich */

#define BRAKE_CLOSEST 	-1.0		/*!< Bremsfaktor fuer extremen Nahbereich ( <1 ==> bremsen <0 ==> rueckwaerts) */
#define BRAKE_NEAR		0.6 		/*!< Bremsfaktor fuer Nahbereich ( <1 ==> bremsen <0 ==> rueckwaerts) */
#define BRAKE_FAR		0.8			/*!< Bremsfaktor fuer Fernbereich ( <1 ==> bremsen <0 ==> rueckwaerts) */

static uint8_t col_zone_l = ZONE_CLEAR;	/*!< Kollisionszone, in der sich der linke Sensor befindet */
static uint8_t col_zone_r = ZONE_CLEAR;	/*!< Kollisionszone, in der sich der rechte Sensor befindet */


/*!
 * Passt auf, dass keine Kollision mit Hindernissen an der Front des Roboters
 * geschieht.
 * Diese Funktion ist nur ein Dummy-Beispiel, wie eine Kollisionsvermeidung aussehen
 * koennte. Hier ist ein guter Einstiegspunkt fuer eigene Experimente und Algorithmen!
 * @param *data der Verhaltensdatensatz
 */
void bot_avoid_col_behaviour(Behaviour_t * data) {
	if (sensDistR < COL_CLOSEST)	/* sehr nah */
		col_zone_r = ZONE_CLOSEST;	/* dann auf jeden Fall CLOSEST Zone */
	else
	/* sind wir naeher als NEAR und nicht in der inneren Zone gewesen */
	if ((sensDistR < COL_NEAR) && (col_zone_r > ZONE_CLOSEST))
		col_zone_r = ZONE_NEAR;	/* dann auf in die NEAR-Zone */
	else
	/* sind wir naeher als FAR und nicht in der NEAR-Zone gewesen */
	if ((sensDistR < COL_FAR) && (col_zone_r > ZONE_NEAR))
		col_zone_r = ZONE_FAR;	/* dann auf in die FAR-Zone */
	else
	/* wir waren in einer engeren Zone und verlassen sie in Richtung NEAR */
	if (sensDistR < (int16_t)(COL_NEAR * 0.50))
		col_zone_r = ZONE_NEAR;	/* dann auf in die NEAR-Zone */
	else
	if (sensDistR < (int16_t)(COL_FAR * 0.50))
		col_zone_r = ZONE_FAR;	/* dann auf in die NEAR-Zone */
	else
		col_zone_r = ZONE_CLEAR;	/* dann auf in die CLEAR-Zone */

	if (sensDistL < COL_CLOSEST)	/* sehr nah */
		col_zone_l = ZONE_CLOSEST;	/* dann auf jeden Fall CLOSEST-Zone */
	else
	/* sind wir naeher als NEAR und nicht in der inneren Zone gewesen */
	if ((sensDistL < COL_NEAR) && (col_zone_l > ZONE_CLOSEST))
		col_zone_l = ZONE_NEAR;	/* dann auf in die NEAR-Zone */
	else
	/* sind wir naeher als FAR und nicht in der NEAR-Zone gewesen */
	if ((sensDistL < COL_FAR) && (col_zone_l > ZONE_NEAR))
		col_zone_l = ZONE_FAR;	/* dann auf in die FAR-Zone */
	else
	/* wir waren in einer engeren Zone und verlassen sie in Richtung NEAR */
	if (sensDistL < (int16_t)(COL_NEAR * 0.50))
		col_zone_l = ZONE_NEAR;	/* dann auf in die NEAR-Zone */
	else
	if (sensDistL < (int16_t)(COL_FAR * 0.50))
		col_zone_l = ZONE_FAR;	/* dann auf in die NEAR-Zone */
	else
		col_zone_l = ZONE_CLEAR;	/* dann auf in die CLEAR-Zone */

	// Nur reagieren, wenn der Bot vorwaerts faehrt
	if ((speed_l > 0) && (speed_r > 0)) {
		// wenn auf beiden Seiten in der Kollisionszone
		if ((col_zone_r == ZONE_CLOSEST) && (col_zone_l == ZONE_CLOSEST)){
			// Drehe Dich zur Seite, wo mehr Platz ist
			if (sensDistR < sensDistL)
				bot_turn(data, 20);
			else
				bot_turn(data, -20);
			return;
		}

		switch (col_zone_l) {
			case ZONE_CLOSEST:
				factorWishRight = BRAKE_CLOSEST;
				break;
			case ZONE_NEAR:
				factorWishRight =  BRAKE_NEAR;
				break;
			case ZONE_FAR:
				factorWishRight =  BRAKE_FAR;
				break;
			case ZONE_CLEAR:
				factorWishRight = 1;
				break;
			default:
				col_zone_l = ZONE_CLEAR;
				break;
		}

		switch (col_zone_r) {
			case ZONE_CLOSEST:
				factorWishLeft = BRAKE_CLOSEST;
				break;
			case ZONE_NEAR:
				factorWishLeft = BRAKE_NEAR;
				break;
			case ZONE_FAR:
				factorWishLeft = BRAKE_FAR;
				break;
			case ZONE_CLEAR:
				factorWishLeft = 1;
				break;
			default:
				col_zone_r = ZONE_CLEAR;
				break;
		}
	}
}
#endif	// BEHAVIOUR_AVOID_COL_AVAILABLE
