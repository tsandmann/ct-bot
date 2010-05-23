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
 * @file 	behaviour_scan_beacons.c
 * @brief 	Verhalten, das Landmarken im Umkreis des Bots sucht und die Bot-Position
 * 			aktualisiert, falls drei oder mehr Landmarken gefunden wurden.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	01.05.2009
 */

#include "bot-logic/bot-logik.h"
#include <stdlib.h>
#include <math.h>
#include "log.h"
#include "math_utils.h"

#ifdef BEHAVIOUR_SCAN_BEACONS_AVAILABLE

static uint8_t state;		/*!< Status des Verhaltens */
static uint8_t pos_update;	/*!< Update der Positionsdaten gewuenscht? */
static uint8_t turn_mode;	/*!< 0: Auf der Stelle drehen, 1: Kreis um das linke Rad fahren */
static uint8_t index;		/*!< Index fuer naechste Baken-Daten */
static struct {
	uint16_t id;					/*!< Landmarken-ID */
	float heading;					/*!< Bot-Ausrichtung, unter der die Landmarke gesehen wurde */
} recognized_beacons[3];			/*!< Array aller erkannten Landmarken */
static float appearance_heading;	/*!< Bot-Ausrichtung, bei der eine Landmarke zuerst gesehen wurde */
static float last_beacon_heading;	/*!< Bot-Ausrichtung bei letzter Landmarke */
static float last_heading;			/*!< Letzte Bot-Ausrichtung */
static float turned;				/*!< Winkel, um den sich der Bot bisher gedreht hat */

#define MAX_BEACONS (sizeof(recognized_beacons) / sizeof(recognized_beacons[0]))
#define BEACON_GRID_SIZE		240	/*!< Raster-Breite fuer die Baken [mm] */

#define SEARCH_APPEARANCE		0	/*!< Punkt suchen, ab dem das Baken-Signal zuerst erkannt wird */
#define SEARCH_DISAPPEARANCE	1	/*!< Punkt suchen, ab dem das Baken-Signal nicht mehr erkannt wird */
#define CALC_POSITION			2	/*!< Es wurden genuegend Landmarken erkannt, um die Position berechnen zu koennen */
#define END						99	/*!< Ende des Verhaltens */

/*!
 * Ermittelt aus einer Landmarken-ID die Position dieser Landmarke
 * @param id	ID der Landmarke. Darin ist ihre Position codiert
 * @return		Position der Landmarke in Weltkoordinaten
 */
static position_t get_position_from_id(uint16_t id) {
	position_t pos = {0, 0};
#ifdef PC
	pos.x = (id >> 8) * BEACON_GRID_SIZE + (BEACON_GRID_SIZE / 2);
	pos.y = (id & 0xff) * BEACON_GRID_SIZE + (BEACON_GRID_SIZE / 2);
#else // MCU
/*! @todo Nur Test-Daten */
	switch (id) {
	case 0xd:
		pos.x = 0;
		pos.y = 0;
		break;
	case 0xe:
		pos.x = 1250;
		pos.y = 0;
		break;
	case 0xc:
		pos.x = 1250;
		pos.y = 610;
		break;
	}
#endif // PC

	LOG_DEBUG("  Pos v. Landmarke %d: (%d|%d)", id, pos.x, pos.y);
	return pos;
}

/*!
 * Verhalten, das Landmarken im Umkreis des Bots sucht und die Bot-Position
 * aktualisiert, falls drei oder mehr Landmarken gefunden wurden.
 * @param *data	Der Verhaltensdatensatz
 */
void bot_scan_beacons_behaviour(Behaviour_t * data) {
	static uint8_t disappeared_counter = 0;
	static uint16_t lastb_id = BPS_NO_DATA;
	float diff = heading - last_heading;

	switch (state) {
	case SEARCH_APPEARANCE:
		last_heading = heading;
		if (diff < -2.0f) {
			diff += 360.0f;
		}
		turned += diff;
		if (turned > 360.0f) {
			/* Bot hat sich bereits um mehr als 360 Grad gedreht -> Ende */
			state = END;
		}

		if (sensBPS != BPS_NO_DATA) {
			/* Landmarke erkannt */
			LOG_DEBUG(" BPS-Sensor meldet Landmarke:");
			LOG_DEBUG("  ID=0x%x", sensBPS);
			uint8_t i;
			for (i=0; i<index; i++) {
				if (recognized_beacons[i].id == sensBPS) {
					/* Diese Landmarke wurde bereits gesehen */
					break;
				}
			}
			if (i == index) {
				/* Neue Landmarke gefunden */
				recognized_beacons[index].id = sensBPS;
				lastb_id = sensBPS;
				appearance_heading = heading;
				disappeared_counter = 0;
				state = SEARCH_DISAPPEARANCE;
				LOG_DEBUG(" %u.Landmarke %d sichtbar ab %dG.", index + 1, sensBPS, heading_int);
			}
		}
		break;

	case SEARCH_DISAPPEARANCE:
		if (sensBPS == BPS_NO_DATA) {
			disappeared_counter++;
#ifdef PC
			disappeared_counter = 3; // im Sim kommt das Signal nicht gepulst, sondern dauerhaft
#endif
			if (disappeared_counter > 1 /*2*/) {
				/* Landmarke nicht mehr sichtbar */
				float diff = heading - appearance_heading;
				if (diff < 0.0f) {
					diff += 360.0f;
				}
				last_beacon_heading = heading - diff / 2.0f;
				if (last_beacon_heading < 0.0f) {
					last_beacon_heading += 360.0f;
				}
				LOG_DEBUG(" %u.Landmarke sichtbar bis %dG.", index + 1, heading_int);
				recognized_beacons[index].heading = last_beacon_heading;
				index++;
				LOG_DEBUG("  Nehme %dG. als Richtung d. Landmarke", (int16_t) last_beacon_heading);
				if (index == MAX_BEACONS) {
					/* maximale Anzahl an Landmarken erkannt */
					state = CALC_POSITION;
					LOG_DEBUG(" Berechne Position aus Landmarken");
					/* Bot stoppen */
					speedWishLeft = BOT_SPEED_STOP;
					speedWishRight = BOT_SPEED_STOP;
				} else {
					/* naechste Landmarke suchen */
					state = SEARCH_APPEARANCE;
					LOG_DEBUG("Suche %u. Landmarke...", index + 1);
				}
			}
		} else {
			disappeared_counter = 0;
		}
		break;

	case CALC_POSITION: {
		/* Drei oder mehr Landmarken gefunden, jetzt Position berechnen */
		position_t a, m, b, n;
		float angle_am, angle_mb;

		angle_am = recognized_beacons[2].heading - recognized_beacons[1].heading;
		if (angle_am < 0.0f) {
			angle_am += 360.0f;
		}
		angle_mb = recognized_beacons[1].heading - recognized_beacons[0].heading;
		if (angle_mb < 0.0f) {
			angle_mb += 360.0f;
		}

/*! @todo vielleicht laesst sich der Tausch-Spass hier noch optimieren */
		if (angle_am > 180.0f) {
			uint16_t tmp_id = recognized_beacons[2].id;
			float tmp_head = recognized_beacons[2].heading;
			recognized_beacons[2] = recognized_beacons[1];
			recognized_beacons[1] = recognized_beacons[0];
			recognized_beacons[0].id = tmp_id;
			recognized_beacons[0].heading = tmp_head;
			angle_am = recognized_beacons[2].heading - recognized_beacons[1].heading;
			if (angle_am < 0.0f) {
				angle_am += 360.0f;
			}
			angle_mb = recognized_beacons[1].heading - recognized_beacons[0].heading;
			if (angle_mb < 0.0f) {
				angle_mb += 360.0f;
			}
		} else if (angle_mb > 180.0f) {
			uint16_t tmp_id = recognized_beacons[2].id;
			float tmp_head = recognized_beacons[2].heading;
			recognized_beacons[2] = recognized_beacons[0];
			recognized_beacons[0] = recognized_beacons[1];
			recognized_beacons[1].id = tmp_id;
			recognized_beacons[1].heading = tmp_head;
			angle_am = recognized_beacons[2].heading - recognized_beacons[1].heading;
			if (angle_am < 0.0f) {
				angle_am += 360.0f;
			}
			angle_mb = recognized_beacons[1].heading - recognized_beacons[0].heading;
			if (angle_mb < 0.0f) {
				angle_mb += 360.0f;
			}
		}

		a = get_position_from_id(recognized_beacons[2].id);
		m = get_position_from_id(recognized_beacons[1].id);
		b = get_position_from_id(recognized_beacons[0].id);

		LOG_DEBUG(" A=(%d|%d) M=(%d|%d) B=(%d|%d)", a.x, a.y, m.x, m.y, b.x, b.y);
		LOG_DEBUG(" angle_am=%d; angle_mb=%d", (int16_t) angle_am, (int16_t) angle_mb);
		n = calc_resection(a, m, b, angle_am, angle_mb);
		position_t last_beacon = get_position_from_id(lastb_id);
		float head = atan2f(last_beacon.y - n.y, last_beacon.x - n.x);
		if (head < 0) {
			head += (2.0f * M_PI);
		}
		head = fmodf(head + M_PI_2, 2.0f * M_PI); // Sensor zeigt nach -90 Grad
		if (turn_mode == 1) {
			/* Botmittelpunkt berechnen aus Position des linken Rades */
			const float dX = sinf(head) * (WHEEL_TO_WHEEL_DIAMETER / 2.0f);
			const float dY = cosf(head) * (WHEEL_TO_WHEEL_DIAMETER / 2.0f);
			n.x += iroundf(dX);
			n.y += iroundf(dY);
		}
		head = deg(head);
		LOG_DEBUG(" > Berechnete Position: (%d|%d)", n.x, n.y);
		LOG_DEBUG(" >  Heading: %d", (int16_t) head);
		LOG_DEBUG(" > Bisherige Position: (%d|%d)", x_pos, y_pos);
		LOG_DEBUG(" >  Heading: %d", (int16_t) last_beacon_heading);

		if (pos_update != 1) {
			/* kein Positionsupdate gewuenscht */
			state = END;
			return;
		}

		/* Position des Bots auf die gerade Berechnete setzen */
		if (n.x != INT16_MAX && n.y != INT16_MAX) {
			LOG_DEBUG(" Aktualisiere Positionsdaten");
			x_pos = n.x;
			y_pos = n.y;

			float head_diff = head - last_beacon_heading;
			heading = fmodf(heading + head_diff, 360.0f);
			x_enc = n.x;
			y_enc = n.y;
			heading_enc = fmodf(heading_enc + head_diff, 360.0f);
#ifdef MEASURE_MOUSE_AVAILABLE
			x_mou = n.x;
			y_mou = n.y;
			heading_mou = fmodf(heading_mou + head_diff, 360.0f);
#endif	// MEASURE_MOUSE_AVAILABLE
		}

		state = END;
		break;
	}

	default:
		/* Ende */
		LOG_DEBUG("Suche nach Landmarken beendet");
		return_from_behaviour(data);
		return;
	}
	/* Bot drehen */
	speedWishRight = BOT_SPEED_MIN;
	if (turn_mode == 0) {
		speedWishLeft = -BOT_SPEED_MIN;
	} else if (turn_mode == 1) {
		speedWishLeft  = BOT_SPEED_STOP;
	}
}

/*!
 * Verhalten, das Landmarken im Umkreis des Bots sucht und die Bot-Position
 * aktualisiert, falls drei oder mehr Landmarken gefunden wurden.
 * @param *caller Der Verhaltensdatensatz des Aufrufers
 * @param position_update Sollen die Positionsdaten aktualisiert werden? 1: ja
 * @param mode 0: Auf der Stelle drehen, 1: Kreis um das linke Rad fahren
 */
void bot_scan_beacons(Behaviour_t * caller, uint8_t position_update, uint8_t mode) {
	pos_update = position_update;
	turn_mode = mode;
	if (mode > 1) {
		if (caller != NULL) {
			caller->subResult = SUBFAIL;
		}
		return;
	}
	switch_to_behaviour(caller, bot_scan_beacons_behaviour, OVERRIDE);
	state = SEARCH_APPEARANCE;
	index = 0;
	turned = 0.0f;
	last_heading = heading;
	int8_t i;
	/* Landmarken-Speicher leeren */
	for (i=MAX_BEACONS-1; i>=0; i--) {
		recognized_beacons[i].id = BPS_NO_DATA;
	}
	LOG_DEBUG("Suche 1. Landmarke...");
}

#endif	// BEHAVIOUR_SCAN_BEACONS_AVAILABLE
