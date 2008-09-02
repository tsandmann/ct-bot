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
 * @file 	behaviour_measure_distance.c
 * @brief 	Ermittelt die aktuelle Entfernung eines Hindernisses mit den Distanzsensoren
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	27.04.2007
 */

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE
#include <stdlib.h>

#include "sensor.h"
#include "log.h"

static uint8_t last_toggle = 0;			/*!< letztes Toggle-Bit der Distsensoren */
static int16_t *pSensL;					/*!< Zeiger auf Variable fuer linken Sensorwert */
static int16_t *pSensR;					/*!< Zeiger auf Variable fuer rechten Sensorwert */
static uint8_t measure_count = 0;		/*!< Counter fuer Sharp-Messungen */
static uint8_t max_diff = 0;			/*!< Maximal zulasessige Abweichung zwischen den Messungen [mm] */

static const uint8_t max_measure = 4;	/*!< Anzahl der Messungen, die aehnliche Werte liefern muessen */


/*!
 * @brief		Das eigentliche Verhalten
 * @param *data	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @see			bot_measure_distance()
 * Das Verhalten beendet sich erst, wenn mehrere Messungen sinnvolle Werte ergaben
 */
void bot_measure_distance_behaviour(Behaviour_t * data) {
	/* Anzahl durchgefuehrter Messungen pruefen */
	if (measure_count == max_measure-1) {
//		LOG_INFO("*pSensL=%d\t*pSensR=%d", *pSensL, *pSensR);
		return_from_behaviour(data);
		return;
	}

	/* auf Sensorupdate pruefen */
	if (last_toggle != sensDistLToggle) {
		last_toggle = sensDistLToggle;
		/* Abweichung pruefen */
		if (abs(*pSensL - sensDistL) <= max_diff && abs(*pSensR - sensDistR) <= max_diff)
			measure_count++;
		else
			measure_count = 0;
		/* neue Sensorwerte speichern */
		*pSensL = sensDistL;
		*pSensR = sensDistR;
	}
}

/*!
 * @brief			Ermittelt die aktuelle Entfernung eines Hindernisses mit den Distanzsensoren
 * @param *caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @param *p_sensL	Zeiger auf eine Variable fuer den linken Sensorwert (Messwert-Ausgabe)
 * @param *p_sensR	Zeiger auf eine Variable fuer den rechten Sensorwert (Messwert-Ausgabe)
 * @param diff		Maximal zulaessige Differenz zwischen den Messungen
 */
void bot_measure_distance(Behaviour_t * caller, int16_t * p_sensL, int16_t * p_sensR, uint8_t diff) {
	/* Inits */
	last_toggle = sensDistLToggle;
	pSensL = p_sensL ? p_sensL : &sensDistL;
	pSensR = p_sensR ? p_sensR : &sensDistR;
	*pSensL = sensDistL;
	*pSensR = sensDistR;
	max_diff = diff;
	measure_count = 0;

	/* Verhalten an */
	switch_to_behaviour(caller, bot_measure_distance_behaviour, OVERRIDE);
}

static int16_t range = 0;	/*!< Entfernung [mm] fuer bot_check_distance() */

/*!
 * @brief		Implementierung des check-distance-Verhaltens
 * @param *data	Zeiger auf den Verhaltensdatensatz
 */
void bot_check_distance_behaviour(Behaviour_t * data) {
	static int16_t left = 0;
	static int16_t right = 0;
	if (left == 0) {
		bot_measure_distance(data, &left, &right, max_diff);
	} else {
		exit_behaviour(data, (left <= range) || (right <= range));
		left = 0;
	}
}

/*!
 * @brief			Prueft, ob in Entfernung max_dist [mm] ein Hindernis zu sehen ist.
 * 					Der Aufrufer bekommt SUBSUCCESS in seinen Verhaltensdatensatz, falls ja,
 * 					sonst SUBFAIL.
 * @param *caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @param max_dist	Entfernung in mm, bis zu der geprueft werden soll
 * @param diff		Maximal zulaessige Differenz zwischen den Messungen
 */
void bot_check_distance(Behaviour_t * caller, int16_t max_dist, uint8_t diff) {
	switch_to_behaviour(caller, bot_check_distance_behaviour, OVERRIDE);
	range = max_dist;
	max_diff = diff;
}

#endif	// BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE
