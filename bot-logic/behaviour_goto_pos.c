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
 * @file 	behaviour_goto_pos.c
 * @brief 	Anfahren einer Position
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	15.10.2007
 */

#include "bot-logic/bot-logik.h"
#include "eeprom.h"

//#define DEBUG_GOTO_POS // Schalter um recht viel Debug-Code anzumachen

#define MIN_TARGET_MARGIN	(6 * 6)		/*!< (Entfernung zum Ziel)^2 [mm^2], ab der das Ziel auf jeden Fall als erreicht gilt */
#define MAX_TARGET_MARGIN	(30 * 30)	/*!< maximaler Wert von margin */
#ifdef MCU
#define TARGET_MARGIN	(20 * 20)	/*!< Init-Wert (Entfernung zum Ziel)^2 [mm^2], ab der das Ziel als erreicht gilt fuer MCU */
#else
#define TARGET_MARGIN	(10 * 10)	/*!< Init-Wert (Entfernung zum Ziel)^2 [mm^2], ab der das Ziel als erreicht gilt fuer PC */
#endif

uint16_t EEPROM goto_pos_err[2] = {TARGET_MARGIN, TARGET_MARGIN};	/*!< Fehlerwerte (Nachlauf) im EEPROM */

#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
#include <stdlib.h>
#include "math_utils.h"
#include "bot-local.h"
#include "log.h"

#ifndef DEBUG_GOTO_POS
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif


#ifdef MCU
#ifndef SPEED_CONTROL_AVAILABLE
#error "Das goto_pos-Verhalten geht nur mit Motorregelung!"
#endif
#endif

#define FIRST_TURN	0				/*!< Erste Drehung in ungefaehre Zielrichtung */
#define CALC_WAY	1				/*!< Berechnung des Kreisbogens */
#define RUNNING		2				/*!< Fahrt auf der berechneten Kreisbahn */
#define LAST_TURN	3				/*!< Abschliessende Drehung */
#define END			99				/*!< Verhalten beenden */

static int16_t dest_x = 0;			/*!< x-Komponente des Zielpunktes */
static int16_t dest_y = 0;			/*!< y-Komponente des Zielpunktes */
static int16_t dest_head = 0;		/*!< gewuenschte Blickrichtung am Zielpunkt */
static int8_t drive_dir = 1;		/*!< Fahrtrichtung: 1: vorwaerts, -1: rueckwaerts */
static uint8_t state = END;			/*!< Status des Verhaltens */
static int32_t last_diff_to_target;	/*!< letzte Entfernung zum Ziel */
static uint16_t * p_goto_pos_err;	/*!< Zeiger auf Fehlervariable im EEPROM */
static uint16_t margin;				/*!< aktueller Fehlerwert laut EEPROM */

static const int32_t straight_go	= 200L * 200L;	/*!< (Entfernung zum Ziel)^2 [mm^2], bis zu der geradeaus zum Ziel gefahren wird */
static const int16_t max_angle		= 30;	/*!< Maximaler Winkel [Grad] zwischen Start-Blickrichtung und Ziel */
static const int16_t v_m_min		= 100;	/*!< Minimale (mittlere) Geschwindigkeit [mm/s], mit der der Bot zum Ziel fahert */
static const int16_t v_m_max		= 200;	/*!< Maximale (mittlere) Geschwindigkeit [mm/s], mit der der Bot zum Ziel fahert */
static const int16_t v_min			= 50;	/*!< Minimale Geschwindigkeit [mm/s] */
static const int16_t v_max			= 350;	/*!< Maximale Geschwindigkeit [mm/s] */
static const int32_t recalc_dist	= 30 * 30;	/*!< Entfernung^2 [mm^2], nach der die Kreisbahn neu berechnet wird */

/*!
 * Das Positionierungsverhalten
 * @param *data	Der Verhaltensdatensatz
 */
void bot_goto_pos_behaviour(Behaviour_t * data) {
	static int32_t done;
	static int16_t v_m;
	static int16_t v_l;
	static int16_t v_r;

	/* Abstand zum Ziel berechnen (als Metrik euklidischen Abstand benutzen) */
	int32_t diff_to_target = get_dist(dest_x, dest_y, x_pos, y_pos);
	LOG_DEBUG("diff_to_target=%ld", diff_to_target);

	/* gefahrene Strecke berechnen */
	int32_t driven = last_diff_to_target - diff_to_target;
	LOG_DEBUG("last_diff_to_t=%ld", last_diff_to_target);
	LOG_DEBUG("driven=%ld", driven);
	last_diff_to_target = diff_to_target;

	/* Pruefen, ob wir schon am Ziel sind */
	if (state != END && diff_to_target <= (int32_t) margin) {
		state = LAST_TURN;
	}

	switch (state) {
	case FIRST_TURN: {
		/* ungefaehr in die Zielrichtung drehen */
		LOG_DEBUG("first turn");
		int16_t alpha = (int16_t) calc_angle_diff(dest_x - x_pos, dest_y - y_pos);
		if (drive_dir < 0) {
			/* Winkelkorrektur, falls rueckwaerts */
			alpha += 180;
			if (alpha > 180) {
				alpha -= 360;
			}
		}
		LOG_DEBUG("alpha=%d", alpha);
		state = CALC_WAY;
		if (diff_to_target < straight_go) {
			LOG_DEBUG("bot_turn(%d)", alpha);
			bot_turn(data, alpha);
			return;
		}
		if (abs(alpha) > max_angle) {
			int16_t to_turn = abs(alpha) - (max_angle - 10);
			if (alpha < 0) {
				to_turn = -to_turn;
			}
			bot_turn(data, to_turn);
			LOG_DEBUG("bot_turn(%d)", to_turn);
			break;
		}

		/* no break */
	}
	case CALC_WAY: {
		/* Kreisbogenfahrt zum Ziel berechnen */
		LOG_DEBUG("calc way...");
		/* Winkel- und Streckenbezeichnungen wie in -> Documentation/images/bot_pos.png */
		int16_t diff_x = dest_x - x_pos;
		int16_t diff_y = dest_y - y_pos;
		float sin_alpha = heading_sin;
		float tan_alpha = heading_sin / heading_cos;
		if (drive_dir < 0) {
			sin_alpha = -sin_alpha;
			LOG_DEBUG("alpha=%f", heading + 180.0f);
		} else {
			LOG_DEBUG("alpha=%f", heading);
		}
		float beta = calc_angle_diff_rad(diff_x, diff_y); // [-M_PI; M_PI]
		if (drive_dir < 0) {
			LOG_DEBUG("beta=%f", deg(beta) + 180.0f);
		} else {
			LOG_DEBUG("beta=%f", deg(beta));
		}
		const float gamma = (float) M_PI_2 - beta - rad(heading);
		if (sin_alpha == 0.0f) {
			sin_alpha = 0.000001f;
		}
		/* gegenueber bot_pos.png enthalten alle Variablen mit dem Suffix "_2" den doppelten Wert ("/2" ausgeklammert) */
		const int16_t h1_2 = diff_y;
		const int16_t h2_2 = diff_x;
		const float h4_2 = h1_2 * tan_alpha;
		const float h5_2 = h4_2 / sin_alpha;
		const float h6_2 = (h2_2 + h4_2) * sinf(gamma);
		float sin_beta = sinf(beta);
		if (sin_beta == 0.0f) {
			sin_beta = 0.000001f;
		}
		const float h7_2 = h6_2 / sin_beta;
		float radius = (h5_2 + h7_2) / 2.0f;
		LOG_DEBUG("radius=%f", radius);
		if ((int16_t) radius == 0) {
			radius = 100000.0f;	// geradeaus
		}
		/* Geschwindigkeit an Entfernung zum Zielpunkt anpassen */
		float x = diff_to_target < 360L * 360L ? (float) diff_to_target / ((360.0f / M_PI * 2.0f) * (360.0f / M_PI
			* 2.0f)) : M_PI / 2; // (0; pi/2]
		v_m = (int16_t) (sinf(x) * (float) (v_m_max - v_m_min)); // [0; v_m_max - v_m_min]
		v_m += v_m_min; // [v_m_min; v_m_max]
		if (drive_dir < 0) {
			v_m = -v_m;
			radius = -radius;
		}
		/* Geschwindigkeiten auf die beiden Raeder verteilen, um den berechneten Radius der Kreisbahn zu erhalten */
		v_l = iroundf(radius / (radius + ((float) WHEEL_TO_WHEEL_DIAMETER / 2.0f)) * (float) v_m);
		v_r = iroundf(radius / (radius - ((float) WHEEL_TO_WHEEL_DIAMETER / 2.0f)) * (float) v_m);

		int16_t vl_abs = abs(v_l);
		int16_t vr_abs = abs(v_r);
		/* Wenn der Radius zu klein wird, bekommen wir fuer die Raeder Geschwindigkeiten,
		 * die kleiner bzw. groesser als moeglich sind, auesserdem faehrt der Bot dann
		 * eher Slalom, darum beginnen wir in diesem Fall neu mit dem Verhalten.
		 */
		if (vl_abs < v_min || vr_abs < v_min || vl_abs > v_max || vr_abs > v_max) {
			state = FIRST_TURN;
			LOG_DEBUG("Geschwindigkeiten ungueltig, beginne neu");
			LOG_DEBUG("v_l=%d\tv_r=%d\tv_m=%d", v_l, v_r, v_m);
			return;
		}
		/* Statusupdate */
		done = 0;
		state = RUNNING;
		/* no break */
	}
	case RUNNING: {
		/* Berechnete Geschwindigkeiten setzen */
		LOG_DEBUG("v_l=%d; v_r=%d", v_l, v_r);
		LOG_DEBUG("x_pos=%d; y_pos=%d", x_pos, y_pos);
		speedWishLeft = v_l;
		speedWishRight = v_r;
		/* Alle sqrt(recalc_dist) mm rechnen wir neu, um Fehler zu korrigieren */
		done += labs(driven);
		if (done > recalc_dist) {
			state = CALC_WAY;
		}
		break;
	}
	case LAST_TURN: {
		/* Nachlauf abwarten */
		LOG_DEBUG("Nachlauf abwarten...");
		speedWishLeft = BOT_SPEED_STOP;
		speedWishRight = BOT_SPEED_STOP;
#ifdef MCU
		// Sim hat derzeit keinen Nachlauf
		BLOCK_BEHAVIOUR(data, 1200);
#endif
		int16_t diff = (int16_t) diff_to_target;
		int16_t last_diff = (int16_t) last_diff_to_target;
		if (last_diff < (int16_t) driven || (int16_t) driven < 0) {
			/* zu weit gefahren */
			diff = -diff;
		}
		LOG_DEBUG("Fehler^2=%d mm^2", diff);
		/* Aus Fehler neuen Korrekturwert berechnen und im EEPROM speichern */
		const int16_t error = (int16_t) margin;
		LOG_DEBUG("error=%d", error);
		int16_t new_error = (error + diff) / 2;
		LOG_DEBUG("new_error=%d", new_error);
		if (new_error < MIN_TARGET_MARGIN) {
			new_error = MIN_TARGET_MARGIN;
		} else if (new_error > MAX_TARGET_MARGIN) {
			new_error = MAX_TARGET_MARGIN;
		}
		if (new_error != error) {
			ctbot_eeprom_write_word(p_goto_pos_err, (uint16_t) new_error);
			LOG_DEBUG("EEPROM: new_error=%d", new_error);
		}
		/* fast fertig, evtl. noch drehen */
		if (dest_head != 999) {
			/* Noch in die gewuenschte Blickrichtung drehen */
			int16_t to_turn = (int16_t) (dest_head - heading_int);
			if (to_turn > 180) {
				to_turn = -360 + to_turn;
			} else if (to_turn < -180) {
				to_turn += 360;
			}
			LOG_DEBUG("to_turn=%d", to_turn);
			bot_turn(data, to_turn);
		}
		state = END;
		break;
	}
	default:
		drive_dir = 1;
		return_from_behaviour(data);
		return;
	}
}

/*!
 * Botenfunktion des Positionierungsverhaltens.
 * Faehrt einen absoluten angegebenen Punkt an und dreht den Bot in die gewuenschte Blickrichtung.
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param x			x-Komponente des Ziels
 * @param y			y-Komponente des Ziels
 * @param head		neue Blickrichtung am Zielpunkt oder 999, falls egal
 */
void bot_goto_pos(Behaviour_t * caller, int16_t x, int16_t y, int16_t head) {
	dest_x = x;
	dest_y = y;
	dest_head = head;

	/* Verhalten starten */
	switch_to_behaviour(caller, bot_goto_pos_behaviour, OVERRIDE);

	/* Inits */
	if (state != END) {
		drive_dir = 1;	// unsanfter Abbruch beim letzten Mal
		LOG_DEBUG("Richtung unbekannt, nehme vorwaerts an");
		LOG_DEBUG("state=%u", state);
	}
	state = FIRST_TURN;

	int32_t diff_to_target = get_dist(dest_x, dest_y, x_pos, y_pos);
	last_diff_to_target = diff_to_target;
	LOG_DEBUG("diff_to_target=%ld", diff_to_target);
	if (diff_to_target > straight_go) {
		/* fuer grosse Strecken zweiten Fehlerwert verwenden */
		p_goto_pos_err = &goto_pos_err[1];
	} else {
		p_goto_pos_err = &goto_pos_err[0];
	}
	margin = ctbot_eeprom_read_word(p_goto_pos_err);
	LOG_DEBUG("margin=%u", margin);

	LOG_DEBUG("(%d mm|%d mm|%d Grad)", x, y, head);
	if (drive_dir >= 0) {
		LOG_DEBUG("vorwaerts");
	} else {
		LOG_DEBUG("rueckwaerts");
	}
}

/*!
 * Botenfunktion des relativen Positionierungsverhaltens.
 * Faehrt einen als Verschiebungsvektor angegebenen Punkt an und dreht den Bot in die gewuenschte Blickrichtung.
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param x			x-Komponente des Vektors vom Standort zum Ziel
 * @param y			y-Komponente des Vektors vom Standort zum Ziel
 * @param head		neue Blickrichtung am Zielpunkt oder 999, falls egal
 */
void bot_goto_pos_rel(Behaviour_t * caller, int16_t x, int16_t y, int16_t head) {
	/* Zielposition aus Verschiebung berechnen und bot_goto_pos() aufrufen */
	bot_goto_pos(caller, x_pos + x, y_pos + y, head);
}

/*!
 * Botenfunktion des Distanz-Positionierungsverhaltens.
 * Bewegt den Bot um distance mm in aktueller Blickrichtung ("drive_distance(...)")
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param distance	Distanz in mm, die der Bot fahren soll
 * @param dir		Fahrtrichtung: >=0: vorwaerts, <0 rueckwaerts
 */
void bot_goto_dist(Behaviour_t * caller, int16_t distance, int8_t dir) {
	drive_dir = dir;

	/* Zielpunkt aus Blickrichtung und Distanz berechnen */
	int16_t target_x = (int16_t) (distance * heading_cos);
	int16_t target_y = (int16_t) (distance * heading_sin);
	if (drive_dir < 0) {
		// rueckwaerts
		target_x = -target_x;
		target_y = -target_y;
	}
	target_x += x_pos;
	target_y += y_pos;
	LOG_DEBUG("Zielpunkt=(%d|%d)", target_x, target_y);
	LOG_DEBUG("Richtung=%d", drive_dir);
	state = END;
	/* Verhalten starten */
	bot_goto_pos(caller, target_x, target_y, heading_int);
}

#endif	// BEHAVIOUR_GOTO_POS_AVAILABLE
