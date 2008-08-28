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
 * @file 	behaviour_goto_obstacle.c
 * @brief 	Anfahren eines Hindernisses
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	15.10.2007
 */

#include "bot-logic/bot-logik.h"
#include <stdlib.h>
#include <math.h>
#include "math_utils.h"
#include "log.h"

//#define DEBUG_GOTO_POS		// Schalter um recht viel Debug-Code anzumachen

#ifndef DEBUG_GOTO_POS
	#undef LOG_DEBUG
	#define LOG_DEBUG(a, ...) {}
#endif

#ifdef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE

#ifdef MCU
#ifndef SPEED_CONTROL_AVAILABLE
#error "Das goto_pos-Verhalten geht nur mit Motorregelung!"
#endif
#endif

#define TARGET_MARGIN	20					/*!< Entfernung zum Ziel [mm], ab der das Ziel als erreicht gilt */
static const int16_t max_par_diff	= 30;	/*!< Differenz [mm] der Distanzsensorwerte, bis zu der eine parallele Ausrichtung moeglich ist */

static int16_t obst_distance = 0;			/*!< gewuenschte Entfernung zum Hindernis */
static uint8_t obst_parallel = 0;			/*!< richtet die Front des Bots parallel zum Hindernis aus, falls 1 */
static uint8_t obst_state = 0;				/*!< Status von bot_goto_obstacle */

/*!
 * Hilfsverhalten von bot_goto_pos(), das den Bot auf eine gewuenschte Entfernung
 * an ein Hindernis heranfaehrt.
 * @param *data	Der Verhaltensdatensatz
 */
void bot_goto_obstacle_behaviour(Behaviour_t * data) {
	static int16_t distLeft, distRight;
	switch (obst_state) {
	case 0:
		/* Entfernung zum Hindernis messen */
		bot_measure_distance(data, &distLeft, &distRight);
		LOG_DEBUG("Hindernis ist %d|%d mm entfernt", distLeft, distRight);
		obst_state = 1;
		break;
	case 1: {
		/* kleinere Distanz als Richtwert benutzen */
		int16_t dist = distLeft < distRight ? distLeft : distRight;
		if (dist <= 600) {
			/* Entfernung - gewuenschte Entfernung fahren */
			int16_t to_drive = dist - obst_distance;
			LOG_DEBUG("to_drive=%d", to_drive);
			if (abs(to_drive) > TARGET_MARGIN) {
				bot_goto_dist(data, abs(to_drive), sign16(to_drive));
				obst_state = 0;
			} else {
				obst_state = 2;
			}
		} else {
			/* kein Hindernis in Sichtweite, also erstmal vorfahren */
			LOG_DEBUG("Noch kein Hindernis in Sichtweite");
			bot_goto_dist(data, 400, 1);
			obst_state = 0;
		}
		break;
	}
	case 2:
		if (obst_parallel == 1) {
			/* heading korrigieren, so dass Bot parallel zum Hindernis steht */
			int16_t diff = distRight - distLeft;
			if (abs(diff) < max_par_diff) {
				/* unsinnig bei zu grosser Differenz */
				LOG_DEBUG("diff=%d", diff);
				float alpha = (float)diff / (float)(DISTSENSOR_POS_SW*2);
				alpha /= 2.0*M_PI/360.0;
				LOG_DEBUG("alpha=%f", alpha);
				bot_turn(data, (int16_t)alpha);
			}
		}
		obst_state = 3;
		break;
	case 3:
		/* fertig :-) */
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Botenfunktion des Positionierungsverhaltens.
 * Bewegt den Bot auf distance mm in aktueller Blickrichtung an ein Hindernis heran
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param distance	Distanz in mm, in der der Bot vor dem Hindernis stehen bleiben soll
 * @param parallel	richtet die Front des Bots parallel zum Hindernis aus, falls 1
 */
void bot_goto_obstacle(Behaviour_t * caller, int16_t distance, uint8_t parallel) {
	obst_distance = distance;
	obst_parallel = parallel;
	switch_to_behaviour(caller, bot_goto_obstacle_behaviour, OVERRIDE);
	obst_state = 0;
}

#endif	// BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
