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
	#define ANGLE_CORRECT	1						/*!< Drehfehler-Init */
#else
	/* derzeit kein EEPROM fuer PC vorhanden, Daten liegen einfach im RAM */
	#define eeprom_read_byte(x)			*x			/*!< Pseudo-EEPROM-Funktion fuer PC */
	#define eeprom_write_byte(ptr, x)	*ptr = x	/*!< Pseudo-EEPROM-Funktion fuer PC */
	#define ANGLE_CORRECT	0						/*!< Drehfehler-Init */
#endif	// MCU

/* EEPROM-Variablen immer deklarieren, damit die Adressen sich nicht veraendern je nach #define */
uint8 EE_SECTION err15=ANGLE_CORRECT	*1;			/*!< Fehler bei Drehungen unter 15 Grad */
uint8 EE_SECTION err45=ANGLE_CORRECT	*2;			/*!< Fehler bei Drehungen zwischen 15 und 45 Grad */
uint8 EE_SECTION err_big=ANGLE_CORRECT	*4;			/*!< Fehler bei groesseren Drehungen */


#ifdef BEHAVIOUR_TURN_AVAILABLE
#include <stdlib.h>
#include <math.h>
#include "timer.h"
#include "log.h"

/* Parameter fuer das bot_turn_behaviour() */
static float to_turn=0;			/*!< Wieviel Grad sind noch zu drehen? */
static float old_heading=0;		/*!< letztes heading */
static int8 turn_direction=0;	/*!< Richtung der Drehung */
static uint8_t *ee_err;			/*!< Zeiger auf Wert fuer Drehfehler im EEPROM */


/*! 
 * @brief			Hilfsfunktion zur Berechnung einer Winkeldifferenz 
 * @param direction	Drehrichtung
 * @param angle1	Winkel 1
 * @param angle2	Winkel 2
 * @return			Winkeldifferenz
 */
static inline float calc_turned_angle(int8 direction, float angle1, float angle2) {
	float diff_angle=0;

	if (direction>0){
		/* Drehung in mathematisch positivem Sinn */
		if (angle1>angle2) {
			/* Es gab einen Ueberlauf */
			diff_angle=360-angle1+angle2;
		} else {
			diff_angle=angle2-angle1;
		}
	} else {
		/* Drehung in mathematisch negativem Sinn */
		if (angle1<angle2) {
			/* Es gab einen Ueberlauf */
			diff_angle=angle1+360-angle2;
		} else {
			diff_angle=angle1-angle2;
		}
	}
	return diff_angle;
}

/*!
 * @brief			Das Verhalten laesst den Bot eine Punktdrehung durchfuehren. 
 * @param *data 	Der Verhaltensdatensatz
 * @see bot_turn()
 * Das Drehen findet in mehreren Schritten statt. Die Drehung wird dabei
 * zunaechst mit hoeherer Geschwindigkeit ausgefuehrt. Bei kleineren
 * Winkeln dann nur noch mit geringer Geschwindigkeit.
 */
void bot_turn_behaviour(Behaviour_t *data) {
	static uint16 lag_wait=0;				/*!< Timestamp fuer Abwarten des Nachlaufs */
	float turned=0;							/*!< seit dem letzten Mal gedrehte Grad */

	/* berechnen, wieviel Grad seit dem letzten Aufruf gedreht wurden */
	turned=calc_turned_angle(turn_direction,old_heading,heading);

	to_turn-=turned;			// aktuelle Drehung von zu drehendem Winkel abziehen

	if (to_turn > 1) {
		old_heading = heading;	// aktueller Winkel wird alter Winkel
		/* Solange drehen, bis Drehwinkel erreicht ist oder gar zu weit gedreht wurde */
		uint8_t abs_speed;	// schneller als 255 mm/s drehen ist zu ungenau, also reichen hier 8 Bit
		float x = to_turn < 180 ? to_turn/(360.0/M_PI) : M_PI / 2;	// (0; pi/2]
		abs_speed = sin(x) * 150.0;		// [0; 150]
		abs_speed++;					// [1; 151]
 		
 		speedWishRight = turn_direction > 0 ? abs_speed : -abs_speed;
 		speedWishLeft = -speedWishRight;
 		
 		lag_wait=TIMER_GET_TICKCOUNT_16;

	} else {
		/* Drehwinkel erreicht, nun Abwarten, bis Nachlauf beendet */
		speedWishLeft=BOT_SPEED_STOP;
		speedWishRight=BOT_SPEED_STOP;
		
		if (fabs(heading-old_heading) > 0.1) {
			old_heading=heading;
			lag_wait=TIMER_GET_TICKCOUNT_16;
			return;
		}
		/* 50 ms auf neue Messwerte (heading) warten */
		if (!timer_ms_passed(&lag_wait, 50)) {
			return;
		}

		/* Nachlauf beendet, jetzt Neue mit alter Abweichung vergleichen und ggfs neu bestimmen */
		uint8_t err = eeprom_read_byte(ee_err);
		uint8_t new_err = (uint8_t)(-(int16_t)to_turn + err) / 2;
		if (new_err != err) {
			eeprom_write_byte(ee_err, new_err);
		}
		
		/* fertig, Verhalten beenden */
		return_from_behaviour(data);
 	}
}


/*!
 * @brief			Dreht den Bot im mathematischen Drehsinn.
 * @param caller	Der Aufrufer
 * @param degrees 	Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 */
void bot_turn(Behaviour_t *caller, int16 degrees) {
//	 LOG_DEBUG("bot_turn(%d)", degrees);
 	/* Richtungsgerechte Umrechnung in den Zielwinkel */
 	if(degrees < 0) turn_direction = -1;
 	else turn_direction = 1;
	to_turn=(float)abs(degrees);
	old_heading=heading;
	
	/* Drehfehler aus EEPROM lesen und beruecksichtigen */
	ee_err = &err_big;
	if (to_turn <= 15) {
		ee_err = &err15;
	} else if (to_turn <= 45) {
		ee_err = &err45;
	}
	uint8_t correct = eeprom_read_byte(ee_err);
	if (correct > 20) {
		correct = 0;
		eeprom_write_byte(ee_err, 0);	
	}
	to_turn -= correct;
	
 	switch_to_behaviour(caller, bot_turn_behaviour,OVERRIDE); 
}
#endif	// BEHAVIOUR_TURN_AVAILABLE
