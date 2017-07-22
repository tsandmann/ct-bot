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
 * \file 	behaviour_classify_objects.c
 * \brief 	Teilt Objekte nach ihrer Farbe in Klassen ein und transportiert sie ins Lager der Klasse.
 *
 * Damit die Objekte erkannt werden koennen, muessen eine farbige Grundflaeche haben, die bis unter die Liniensensoren
 * reicht (z.B. runde Pappscheibe unter einer Dose).
 * Den Schwellwert fuer die Klasseneinteilung muss man derzeit im Array targets fest einstellen, ebenso die Zielpositionen.
 * Objekte gleicher Klasse werden mit einem Abstand von 10 cm in positiver Y-Richtung nebeneinander gestellt.
 * Funktioniert derzeit nur mit Catch-Pillar-Version 3.
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	15.06.2008
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE
#include <stdlib.h>
#include "log.h"

#define CO_SEARCH	0
#define CO_CATCH	1
#define CO_IDENTIFY	2
#define CO_DRIVE	3
#define CO_UNLOAD	4
#define CO_HOME		5
#define CO_END		99

static uint8_t state = 0;	/*!< Status des Verhaltens */
/*! Klasseneinteilung nach Farben */
static struct {
	const int16_t treshold; /*!< maximale Helligkeit des Objekts fuer diese Klasse */
	int16_t x;				/*!< X-Koordinate [mm] fuer Objekte dieser Klasse */
	int16_t y;				/*!< Y-Kooridnate [mm] fuer Objekte dieser Klasse, wird inkrementiert */
} PACKED targets[] = {
		{0x200, 0, -400}, // blau
		{0x3FF, 200, -400} // schwarz
};

/*!
 * Teilt Objekte nach ihrer Farbe in Klassen ein und
 * transportiert sie ins Lager der Klasse.
 * \param *data	Der Verhaltensdatensatz
 */
void bot_classify_objects_behaviour(Behaviour_t * data) {
	static uint8_t measure_count;
	static int16_t object_brightness;

	switch (state) {
	case CO_SEARCH:
		/* Objekte suchen */
		state = CO_CATCH;
		/* Such-Algo / Explore-Verhalten */
		CASE_NO_BREAK;

	case CO_CATCH:
		/* Objekt einfangen */
		bot_catch_pillar_turn(data, 180);
		state = CO_IDENTIFY;
		measure_count = 0;
		object_brightness = 0;
		break;

	case CO_IDENTIFY:
		/* Objekterkennung */
		if (data->subResult == BEHAVIOUR_SUBFAIL) {
			state = CO_END;
			bot_turn(data, -180);
			break;
		}
		if (measure_count < 10) {
			/* Durchschnitt aus 10 Messungen */
			object_brightness += sensLineL + sensLineR;
			measure_count++;
		} else {
			/* Identifizierung */
			object_brightness /= 20;
			state = CO_UNLOAD;
			uint8_t i;
			LOG_DEBUG("object_brightness=0x%x", object_brightness);
			/* Klasse suchen */
			for (i=0; i<sizeof(targets)/sizeof(int16_t)/3; i++) {
				if (object_brightness < targets[i].treshold) {
					LOG_DEBUG("object_class=%u", i);
					targets[i].y += 100;
					bot_goto_pos(data, targets[i].x, targets[i].y - 100, 270);
					break;
				}
			}
		}
		break;

	case CO_UNLOAD:
		/* Objekt ausladen */
		bot_unload_pillar(data);
		state = CO_HOME;
		break;

	case CO_HOME:
		/* zurueck zum Startpunkt */
		bot_goto_pos(data, 0, 0, 0);
		state = CO_SEARCH;
		break;

	default:
		exit_behaviour(data, BEHAVIOUR_SUBFAIL);
		return;
	}
}

/*!
 * Teilt Objekte nach ihrer Farbe in Klassen ein und
 * transportiert sie ins Lager der Klasse.
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_classify_objects(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_classify_objects_behaviour, BEHAVIOUR_OVERRIDE);
	state = CO_SEARCH;
}

#endif // BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE
