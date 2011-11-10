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

//#define OLD_VERSION	/*!< Zu Testzwecken alte Version (1465) mit diesem define anschaltbar */

#ifdef OLD_VERSION
#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
#include <stdlib.h>
#include <math.h>
#include "math_utils.h"
#include "log.h"

//#define DEBUG_GOTO_POS		// Schalter um recht viel Debug-Code anzumachen

#ifndef DEBUG_GOTO_POS
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif

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
		bot_measure_distance(data, &distLeft, &distRight, 15);
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
	switch_to_behaviour(caller, bot_goto_obstacle_behaviour, BEHAVIOUR_OVERRIDE);
	obst_state = 0;
}

#endif // BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE

#else // neue Version

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
#include <stdlib.h>
#include <math.h>
#include "math_utils.h"
#include "log.h"

//#define DEBUG_GOTO_OBSTACLE		// Schalter um recht viel Debug-Code anzumachen

#ifndef DEBUG_GOTO_OBSTACLE
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif


#ifdef MCU
#ifndef SPEED_CONTROL_AVAILABLE
#error "Das goto_obstacle-Verhalten geht nur mit Motorregelung!"
#endif
#endif

#define TARGET_MARGIN	10					/*!< Entfernung zum Ziel [mm], ab der das Ziel als erreicht gilt */
static const int16_t max_par_diff = 40;		/*!< Differenz [mm] der Distanzsensorwerte, bis zu der eine parallele Ausrichtung moeglich ist */
//static const uint8_t max_measure_diff = 15;	/*!< Maximale Differenz [mm] der Distanzsensormessungen fuer bot_measure_distance() */

static int16_t obst_distance = 0;			/*!< gewuenschte Entfernung zum Hindernis */
static uint8_t obst_parallel = 0;			/*!< richtet die Front des Bots parallel zum Hindernis aus, falls 1 */
static uint8_t obst_state = 0;				/*!< Status von bot_goto_obstacle */

#define MEASURE_DIST_STATE	0
#define GOTO_DIST_STATE		1
#define ALIGN_STATE			2
#define END					99

///*!
// * Prueft, ob ein Hindernis in Sichtweite ist
// * @result	True, falls ein Hindernis naeher als 60 cm ist
// */
//static uint8_t check_distance(void) {
//	int16_t dist = sensDistL < sensDistR ? sensDistL : sensDistR;
//	if (dist <= 600) {
//		LOG_DEBUG("Hindernis in Sichtweite (%d mm)", dist);
//		return True;
//	}
//	return False;
//}

/*!
 * Hilfsverhalten von bot_goto_pos(), das den Bot auf eine gewuenschte Entfernung
 * an ein Hindernis heranfaehrt.
 * @param *data	Der Verhaltensdatensatz
 */
void bot_goto_obstacle_behaviour(Behaviour_t * data) {
	static int16_t distLeft, distRight;
	switch (obst_state) {
//	case MEASURE_DIST_STATE:
//		/* Entfernung zum Hindernis messen */
//		bot_measure_distance(data, &distLeft, &distRight, max_measure_diff);
//		LOG_DEBUG("Hindernis ist %d|%d mm entfernt", distLeft, distRight);
//		obst_state = GOTO_DIST_STATE;
//		break;
	case GOTO_DIST_STATE: {
		/* kleinere Distanz als Richtwert benutzen */
		int16_t dist = sensDistL < sensDistR ? sensDistL : sensDistR;
		LOG_DEBUG("Hindernis ist %d|%d mm entfernt", sensDistL, sensDistR);
		if (dist <= 600) {
			/* Entfernung - gewuenschte Entfernung fahren */
			int16_t to_drive = dist - obst_distance;
			LOG_DEBUG("to_drive=%d", to_drive);
			if (abs(to_drive) > TARGET_MARGIN) {
				bot_goto_dist(data, abs(to_drive), sign16(to_drive));
			} else {
				obst_state = ALIGN_STATE;
			}
		} else {
			/* kein Hindernis in Sichtweite, also erstmal vorfahren */
			LOG_DEBUG("Noch kein Hindernis in Sichtweite");
			bot_goto_dist(data, 400, 1);
/*! @todo	Eleganter waere es, nicht nur 40 cm zu fahren und bei Hinderniserkennung per
			cancel_behaviour abzubrechen. Geht allerdings nicht, wenn cancel_behaviour
			schon anderweitig verwendet wird. */
	//		bot_goto_dist(data, 4000, 1);
	//		bot_cancel_behaviour(data, bot_goto_pos_behaviour, check_distance);
//			obst_state = MEASURE_DIST_STATE;
		}
		break;
	}
	case ALIGN_STATE:
		if (obst_parallel == 1) {
			/* heading korrigieren, so dass Bot parallel zum Hindernis steht */
			int16_t diff = distRight - distLeft;
			if (abs(diff) < max_par_diff) {
				/* unsinnig bei zu grosser Differenz */
				LOG_DEBUG("diff=%d", diff);
				float alpha = (float) diff / (float)(DISTSENSOR_POS_SW * 2);
				alpha /= 2.0 * M_PI / 360.0;
				LOG_DEBUG("alpha=%f", alpha);
				bot_turn(data, (int16_t) alpha);
			}
		}
		obst_state = END;
		break;
	default:
		/* fertig :-) */
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Botenfunktion des goto_obstacle-Verhaltens.
 * Bewegt den Bot auf distance mm in aktueller Blickrichtung an ein Hindernis heran
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param distance	Distanz in mm, in der der Bot vor dem Hindernis stehen bleiben soll
 * @param parallel	richtet die Front des Bots parallel zum Hindernis aus, falls 1
 */
void bot_goto_obstacle(Behaviour_t * caller, int16_t distance, uint8_t parallel) {
	obst_distance = distance;
	obst_parallel = parallel;
	switch_to_behaviour(caller, bot_goto_obstacle_behaviour, BEHAVIOUR_OVERRIDE);
	obst_state = GOTO_DIST_STATE;
}

#endif // BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE


#endif // OLD_VERSION
