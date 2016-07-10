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
 * \file 	behaviour_turn.c
 * \brief 	Drehe den Bot
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author 	Torsten Evers (tevers@onlinehome.de)
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	03.11.2006
 */


#include "bot-logic/bot-logic.h"
#include "eeprom.h"

//#define DEBUG_TURN // Schalter fuer Debug-Ausgaben

#ifdef MCU
#define TURN_ERR		{30, 110, 220, 380, 550, 800} 	/**< Fehler bei Drehungen im EEPROM */
#else
#define TURN_ERR		{0, 0, 0, 0, 0, 0} 				/**< Fehler bei Drehungen im EEPROM */
#endif // MCU

uint16_t EEPROM turn_err[6] = TURN_ERR;	/**< Fehler bei Drehungen (im EEPROM gespeichert) */

#ifdef BEHAVIOUR_TURN_AVAILABLE
#include <stdlib.h>
#include <math.h>
#include "log.h"

#ifndef DEBUG_TURN
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif

/* Parameter fuer das bot_turn_behaviour() */
static int16_t target = 0;				/**< Zielwinkel der Drehung * 10 + 360 Grad */
static int16_t old_heading = 0;			/**< letztes heading*10 */
static int8_t turn_direction = 0;		/**< Richtung der Drehung, 0: pos. math. Drehsinn, -1: Uhrzeigersinn */
static int16_t d_max_speed = 0;			/**< Anfangsgeschwindigkeit - Endgeschwindigkeit*/
static int16_t min_speed = 0;			/**< Endgeschwindigkeit */
static uint8_t state = 0;				/**< Status des Verhaltens */
static uint8_t wait_cnt = 0;			/**< Zaehler fuer Wartezyklen */
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
static float target_fl = 0.0f;			/**< Zielwinkel * 10 als float */
float turn_last_err = 0.0f;				/**< letzter Drehfehler in Grad */
#endif // BEHAVIOUR_TURN_TEST_AVAILABLE

#define ERR_SPEED_1		60
#define ERR_SPEED_2		120
#define ERR_SPEED_3		180
#define ERR_SPEED_4		250
#define ERR_SPEED_5		280

#define STATE_RUNNNING		0	/**< Drehen aktiv */
#define STATE_WAIT_FOR_STOP	1	/**< Warten bis Bot stillsteht */
#define STATE_END			99	/**< Verhalten fertig */

/**
 * Das Verhalten laesst den Bot eine Punktdrehung durchfuehren.
 * \param *data 	Der Verhaltensdatensatz
 * \see bot_turn()
 * Das Drehen findet in mehreren Schritten statt. Die Drehung wird dabei
 * zunaechst mit hoeherer Geschwindigkeit ausgefuehrt. Bei kleineren
 * Winkeln dann nur noch mit geringer Geschwindigkeit.
 */
void bot_turn_behaviour(Behaviour_t * data) {
	static int16_t diff; // aktuelle Winkeldifferenz gegenueber Zielwinkel
	static uint16_t * p_err;

	/* Differenz zum Zielwinkel berechnen (in Drehrichtung gesehen) */
	if (turn_direction < 0) { // Uhrzeigersinn
		if (heading_10_int > old_heading && old_heading < 50) {
			target += 3600;	// es gab einen Ueberlauf von heading
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
			target_fl += 3600.0f;
#endif
//			LOG_DEBUG("Ueberlauf");
//			LOG_DEBUG("head=%d, old=%d", heading_10_int, old_heading);
		}
		diff = heading_10_int - target + 3600;
	} else {
		if (heading_10_int < old_heading && old_heading > 3550) {
			target -= 3600;	// es gab einen Ueberlauf von heading
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
			target_fl -= 3600.0f;
#endif
//			LOG_DEBUG("Ueberlauf");
//			LOG_DEBUG("head=%d, old=%d", heading_10_int, old_heading);
		}
		diff = target - (heading_10_int + 3600);
	}
	old_heading = heading_10_int;

	switch (state) {
		case STATE_RUNNNING: {
			int16_t last_speed = (abs(v_enc_left) + abs(v_enc_right)) / 2;
			if (last_speed < ERR_SPEED_1) {
				p_err = &turn_err[0];
			} else if (last_speed < ERR_SPEED_2) {
				p_err = &turn_err[1];
			} else if (last_speed < ERR_SPEED_3) {
				p_err = &turn_err[2];
			} else if (last_speed < ERR_SPEED_4) {
				p_err = &turn_err[3];
			} else if (last_speed < ERR_SPEED_5) {
				p_err = &turn_err[4];
			} else {
				p_err = &turn_err[5];
			}
			uint16_t err_ee = ctbot_eeprom_read_word(p_err);
			if (err_ee > 1800) {
				err_ee = 0;
				ctbot_eeprom_update_word(p_err, 0);
			}
			int16_t err = (int16_t) err_ee;

			if (diff > err) {
				/* Bot drehen, solange Zielwinkel noch nicht erreicht ist */
				int16_t new_speed = 0;
				int16_t diff_9 = diff - 900;
				if (diff_9 > 0) {
					float x = diff_9 < 900 ? diff_9 / (float) (360.0 / M_PI * 10.0) : (float) (M_PI / 2.0); // (0; pi/2]
					new_speed = (int16_t) (sinf(x) * (float) d_max_speed); // [0; maxspeed - minspeed]
				}
				new_speed = new_speed + min_speed; // [min_speed; maxspeed]

				speedWishRight = turn_direction < 0 ? -new_speed : new_speed;
				speedWishLeft  = -speedWishRight;
			} else {
				/* Drehwinkel erreicht, Stopp und Abwarten, bis Nachlauf beendet */
				speedWishLeft  = BOT_SPEED_STOP;
				speedWishRight = BOT_SPEED_STOP;
				state = STATE_WAIT_FOR_STOP;
				LOG_DEBUG("diff=%d, err=%d, last_speed=%u", diff, err, last_speed);
			}
			break;
		}

		case STATE_WAIT_FOR_STOP: {
			if (v_enc_left == 0 && v_enc_right == 0) {
				if (wait_cnt-- == 0) {
					state = STATE_END;
				}
			} else {
				wait_cnt = 2;
			}
			break;
		}

		case STATE_END: {
			LOG_DEBUG("done, heading=%d", heading_int);
			LOG_DEBUG("target=%d", target / 10 - 360);

			/* Nachlauf beendet, jetzt Drehfehler aktualisieren */
			LOG_DEBUG("Fehler: %d.%u Grad", diff / 10, abs(diff - diff / 10 * 10));
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
			turn_last_err = fabs(target_fl / 10.0f - 360.0f - heading);
#endif
			int16_t err = (int16_t) ctbot_eeprom_read_word(p_err);
			LOG_DEBUG("old err=%d", err);
			int16_t new_err = err - diff;
			if (new_err < 0) {
				new_err = 0;
			}
			LOG_DEBUG("new_err=%d", new_err);
			uint16_t err_update = (uint16_t) ((err + new_err) / 2);
			LOG_DEBUG("updating err to %u", err_update);
			ctbot_eeprom_update_word(p_err, err_update);

			/* fertig, Verhalten beenden */
			return_from_behaviour(data);
			break;
		}
	}
}

/**
 * Dreht den Bot im mathematischen Drehsinn im Rahmen der angegebenen Geschwindigkeiten.
 * \param *caller	Der Aufrufer
 * \param degrees 	Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * 					zwischen -360 und +360
 * \param minspeed	minimale Drehgeschwindigkeit [mm/s]
 * \param maxspeed	maximale Drehgeschwindigkeit [mm/s]
 * \return			Zeiger auf Verhaltensdatensatz
 */
Behaviour_t * bot_turn_speed(Behaviour_t * caller, int16_t degrees, int16_t minspeed, int16_t maxspeed) {
	/* Parameter begrenzen */
 	while (degrees >  360) degrees -= 360;
 	while (degrees < -360) degrees += 360;
 	int16_t tmp = maxspeed >= BOT_SPEED_FAST ? BOT_SPEED_FAST : maxspeed;
 	d_max_speed = tmp <= BOT_SPEED_MIN ? BOT_SPEED_MIN : tmp;
 	tmp = minspeed > d_max_speed ? d_max_speed : minspeed;
 	min_speed = tmp < BOT_SPEED_MIN ? BOT_SPEED_MIN : tmp;
 	d_max_speed -= min_speed;

 	LOG_DEBUG("degrees=%d, min_speed=%d, d_max_speed=%d", degrees, min_speed, d_max_speed);
 	LOG_DEBUG("heading@start=%d", heading_int);

	/* Zielwinkel berechnen */
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
 	target_fl = heading * 10.0f + degrees * 10.0f + 3600.0f;
#endif
	target = heading_10_int + degrees * 10 + 3600;
 	old_heading = heading_10_int;

	/* Drehrichtung ermitteln */
 	turn_direction = (int8_t) (degrees < 0 ? -1 : 0);

	/* Verhalten aktiv schalten */
 	if (degrees) {
 		state = STATE_RUNNNING;
 		wait_cnt = 2;
 		return switch_to_behaviour(caller, bot_turn_behaviour, BEHAVIOUR_OVERRIDE);
 	} else {
 		return NULL;
 	}
}

/**
 * Dreht den Bot im mathematischen Drehsinn.
 * \param *caller	Der Aufrufer
 * \param degrees 	Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * 					zwischen -360 und +360
 * \return			Zeiger auf Verhaltensdatensatz
 */
Behaviour_t * bot_turn(Behaviour_t * caller, int16_t degrees) {
	return bot_turn_speed(caller, degrees, BOT_SPEED_MIN, BOT_SPEED_NORMAL);
}

#endif // BEHAVIOUR_TURN_AVAILABLE
