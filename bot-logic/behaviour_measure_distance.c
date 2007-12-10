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

static const uint8_t max_measure = 4;	/*!< Anzahl der Messungen, die aehnliche Werte liefern muessen */
static const uint8_t max_diff = 10;		/*!< Maximal zulasessige Abweichung zwischen den Messungen [mm] */

/*!
 * @brief		Das eigentliche Verhalten
 * @param data	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @see			bot_measure_distance()
 * Das Verhalten beendet sich erst, wenn mehrere Messungen sinnvolle Werte ergaben
 */
void bot_measure_distance_behaviour(Behaviour_t *data) {
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
 * @param caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @param p_sensL	Zeiger auf eine Variable fuer den linken Sensorwert (Messwert-Ausgabe)
 * @param p_sensR	Zeiger auf eine Variable fuer den rechten Sensorwert (Messwert-Ausgabe)
 */
void bot_measure_distance(Behaviour_t *caller, int16_t *p_sensL, int16_t *p_sensR) {
	/* Inits */
	last_toggle = sensDistLToggle;
	pSensL = p_sensL ? p_sensL : &sensDistL;
	pSensR = p_sensR ? p_sensR : &sensDistR;
	*pSensL = sensDistL;
	*pSensR = sensDistR;
	measure_count = 0;
	
	/* Verhalten an */
	switch_to_behaviour(caller, bot_measure_distance_behaviour, OVERRIDE);
}

#endif	// BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE
