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
 * @file 	behaviour_turn.c
 * @brief 	Drehe den Bot
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author 	Torsten Evers (tevers@onlinehome.de)
 * @author	Timo Sandmann (mail@timosandmann.de)
 * @date 	03.11.06
 */


#include "bot-logic/bot-logik.h"

#ifdef MCU
	#include <avr/eeprom.h>
#ifdef SPEED_CONTROL_AVAILABLE
	#define TURN_ERR		{45, 35, 20}	/*!< Fehler bei Drehungen im EEPROM */
	#define TUNR_ERR_BIG	90				/*!< Schwellwert in Grad, ab dem turn_err[1] benutzt wird */
	#define TUNR_ERR_SMALL	20				/*!< Schwellwert in Grad, ab dem turn_err[0] benutzt wird */
#else
	#define TURN_ERR		{0, 25, 45}		/*!< Fehler bei Drehungen im EEPROM */
	#define TUNR_ERR_BIG	90				/*!< Schwellwert in Grad, ab dem turn_err[1] benutzt wird */
	#define TUNR_ERR_SMALL	35				/*!< Schwellwert in Grad, ab dem turn_err[0] benutzt wird */
#endif	// SPEED_CONTROAL_AVAILABLE
#else
	#include "eeprom-emu.h"
	#define TURN_ERR		{0, 0, 0} 		/*!< Fehler bei Drehungen im EEPROM */
	#define TUNR_ERR_BIG	90				/*!< Schwellwert in Grad, ab dem turn_err[1] benutzt wird */
	#define TUNR_ERR_SMALL	35				/*!< Schwellwert in Grad, ab dem turn_err[0] benutzt wird */
#endif	// MCU


uint8_t EEPROM turn_err[3] = TURN_ERR;		/*!< Fehler bei Drehungen im EEPROM */

#ifdef BEHAVIOUR_TURN_AVAILABLE
#include <stdlib.h>
#include <math.h>
#include "timer.h"
#include "log.h"

/* Parameter fuer das bot_turn_behaviour() */
static int16_t target = 0;						/*!< Zielwinkel der Drehung*10 + 360 Grad */
static int16_t old_heading = 0;					/*!< letztes heading*10 */
static int8_t turn_direction = 0;				/*!< Richtung der Drehung, 0: pos. math. Drehsinn, -1: Uhrzeigersinn */
static int8_t *ee_err;							/*!< Zeiger auf Wert fuer Drehfehler */
static int8_t err_cache[3] = {-128,-128,-128};	/*!< Fehler bei Drehungen */
static uint8_t max_speed = 0;					/*!< Anfangsgeschwindigkeit */
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
 	static float target_fl = 0.0;				/*!< Zielwinkel*10 als float */
 	float turn_last_err = 0.0;					/*!< letzter Drehfehler in Grad */
#endif


/*!
 * @brief			Das Verhalten laesst den Bot eine Punktdrehung durchfuehren. 
 * @param *data 	Der Verhaltensdatensatz
 * @see bot_turn()
 * Das Drehen findet in mehreren Schritten statt. Die Drehung wird dabei
 * zunaechst mit hoeherer Geschwindigkeit ausgefuehrt. Bei kleineren
 * Winkeln dann nur noch mit geringer Geschwindigkeit.
 */
void bot_turn_behaviour(Behaviour_t * data) {
	/* Differenz zum Zielwinkel berechnen (in Drehrichtung gesehen) */
	int16_t heading_16 = (int16_t)(heading*10);
	int16_t diff;
	if (turn_direction < 0) {	// Uhrzeigersinn
		if (heading_16 > old_heading && old_heading < 50) {
			target += 3600;	// es gab einen Ueberlauf von heading
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
			target_fl += 3600.0;
#endif
//			LOG_DEBUG("Ueberlauf");
//			LOG_DEBUG("head16=%d, old=%d", heading_16, old_heading);
		}
		diff = heading_16 - target + 3600;
	} else {
		if (heading_16 < old_heading && old_heading > 3550) {
			target -= 3600;	// es gab einen Ueberlauf von heading
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
			target_fl -= 3600.0;
#endif
//			LOG_DEBUG("Ueberlauf");
//			LOG_DEBUG("head16=%d, old=%d", heading_16, old_heading);
		}
		diff = target - (heading_16 + 3600);
	}
	old_heading = heading_16;

	if (diff > 0) {
		/* Bot drehen, solange Zielwinkel noch nicht erreicht ist */
		uint8_t new_speed;	// schneller als mit 255 mm/s drehen ist zu ungenau, also reichen hier 8 Bit
		float x = diff < 1800 ? diff / (360.0/M_PI*10) : M_PI/2;	// (0; pi/2]
		new_speed = sin(x) * (float)max_speed;	// [ 0; 125]
		new_speed += 25;						// [25; 150]
 		
 		speedWishRight = turn_direction < 0 ? -new_speed : new_speed;
 		speedWishLeft  = -speedWishRight;
	} else {
		/* Drehwinkel erreicht, Stopp und Abwarten, bis Nachlauf beendet */
		speedWishLeft  = BOT_SPEED_STOP;
		speedWishRight = BOT_SPEED_STOP;
		
#ifdef MCU	// im Sim kein Nachlauf
		BLOCK_BEHAVIOUR(data, 1200);
#endif
		
//		LOG_DEBUG("done, heading=%d %u", heading_16/10, TICKS_TO_MS(TIMER_GET_TICKCOUNT_32));
//		LOG_DEBUG("target=%d", target/10-360); 

		/* Nachlauf beendet, jetzt Drehfehler aktualisieren */
		uint8_t diff_8 = -diff;
		uint8_t err = *ee_err;
//		LOG_DEBUG("Fehler: %d.%u Grad", (diff_8-err)/10, abs((err-diff_8)-(err-diff_8)/10*10));
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
		turn_last_err = fabs(target_fl/10.0-360.0-heading);
#endif
//		LOG_DEBUG("old err=%u\tdiff_8=%u", err, diff_8);
		*ee_err = (uint8_t)(diff_8 + err) / 2;
		if (abs(*ee_err - err) >= 10) {
			/* EEPROM-Update bei Aenderung um mehr als 1 Grad */
			eeprom_write_byte(&turn_err[ee_err-err_cache], *ee_err);
// 			LOG_DEBUG("err1=%d\terr2=%d\terr3=%d", err_cache[0], err_cache[1], err_cache[2]);
		}
//		LOG_DEBUG("new err=%u", *ee_err);
		
		/* fertig, Verhalten beenden */
		return_from_behaviour(data);
 	}
}

/*!
 * @brief			Dreht den Bot im mathematischen Drehsinn.
 * @param *caller	Der Aufrufer
 * @param degrees 	Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * 					zwischen -360 und +360
 * @param speed		maximale Drehgeschwindigkeit [mm/s]
 */
void bot_turn_speed(Behaviour_t * caller, int16_t degrees, uint16_t speed) {
	/* Parameter begrenzen */
 	while (degrees >  360) degrees -= 360;
 	while (degrees < -360) degrees += 360;
 	uint8_t tmp = speed > 150 ? 150 : speed;
 	max_speed = tmp < 50 ? 25 : tmp - 25;

	/* Zielwinkel berechnen */
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
 	target_fl = heading*10.0 + degrees*10.0 + 3600.0;
#endif
	target = (int16_t)(heading*10) + degrees*10 + 3600;
 	old_heading = (int16_t)(heading*10);
 	
	/* Drehfehler beruecksichtigen */
 	if (abs(degrees) <= TUNR_ERR_SMALL) {
		ee_err = &err_cache[0];
	} else if (abs(degrees) <= TUNR_ERR_BIG) {
		ee_err = &err_cache[1];
	} else 
		ee_err = &err_cache[2];
	
	/* (re-)Inits */
	if (*ee_err == -128) {
		*ee_err = eeprom_read_byte(&turn_err[ee_err-err_cache]);
	}	
	if ((uint8_t)*ee_err > 200) {
		*ee_err = 0;
	}
	int8_t err = *ee_err;
	
	if (abs(degrees*10) <= err) {
		/* nix zu tun */
		return;
	}

	/* Drehrichtung ermitteln */
 	if (degrees < 0) {
 		turn_direction = -1;
 	} else {
 		turn_direction = 0;
 		err = -err;
 	}
 	target += err;
 	
	/* Verhalten aktiv schalten */
	switch_to_behaviour(caller, bot_turn_behaviour, OVERRIDE);
}

/*!
 * @brief			Dreht den Bot im mathematischen Drehsinn.
 * @param *caller	Der Aufrufer
 * @param degrees 	Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * 					zwischen -360 und +360
 */
void bot_turn(Behaviour_t * caller, int16_t degrees) {
	bot_turn_speed(caller, degrees, 150);
}

#endif	// BEHAVIOUR_TURN_AVAILABLE
