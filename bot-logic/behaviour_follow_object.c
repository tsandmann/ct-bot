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
 * @file 	behaviour_follow_object.c
 * @brief 	Verfolgung beweglicher Objekte
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	03.08.2007
 */

/* Das bot_follow_object-Verhalten laesst den Bot ein (bewegliches) Objekt verfolgen. Kommt umgekehrt das
 * Objekt dichter als 20 cm an den Bot heran, "flieht" er vor ihm und faehrt rueckwaerts.
 * Die Ausrichtung der Bot-Front ist immer (moeglichst) parallel zur Front des anderen
 * Objekts. Ist dieses z.B. ein Fahrzeug, dann faehrt der Bot durch diese Winkelanpassung
 * dem anderen Fahrzeug hinterher (oder rueckwaerts vor ihm weg).
 * Das Objekt muss breiter sein, als der Abstand der beiden Distanzsensoren, damit es
 * korrekt erkannt wird.
 * Sieht der Bot kein Objekt im Erfassungsbereich der Distanzsensoren, bleibt er stehen.
 *
 * Das Verhalten bietet noch sehr viel Spielraum fuer Anpassungen und eigene Experimente!
 * Insbesondere ist es bisher nicht sehr robust gegenueber grossen Entfernungsaenderungen
 * in kurzer Zeit. Die besten Ergebnisse werden erzielt, wenn sich das andere Objekt
 * langsam und kontinuierlich bewegt.
 * Obwohl der Verfolgungsalgorithmus ziemlich simpel gestrickt ist, bekommt der Kleine mit
 * ihm eine Art "Eigenleben" und es ist recht unterhaltsam, damit zu spielen ;-)
 * Sinnvolle Erweiterungen waeren ein Suchalgorithmus, falls kein Objekt im Erfassungsbereich
 * ist, oder ein Filter fuer die Werte der Distanzsensoren.
 */


#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE
#include <stdlib.h>
#include "sensor_correction.h"
#include "timer.h"
#include "log.h"

/* globale Variablen */
static int16_t speedLeft	= 0;	/*!< Geschwindigkeitsvorgabe links */
static int16_t speedRight	= 0;	/*!< Geschwindigkeitsvorgabe rechts */
static uint8_t state 		= 0;	/*!< aktueller Status {warten auf Messwerte,Geschw. anpassen,Stopp} */
#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
static uint32_t lastMove	= 0;	/*!< letzter Fahrt-Zeitpunkt */
#endif

/* Konfig-Parameter */
static const uint8_t FOLLOW_DISTANCE	= 200;	/*!< optimale Distanz zum anderen Objekt [mm] */
static const uint8_t MAX_SPEED			= 200;	/*!< maximale Verfolgungs- / Fluchtgeschwindigkeit [mm/s] */

/*!
 * @brief				Passt die aktuelle Geschwindigkeit eines Rades an die
 * 						Entfernung auf Seite des Rades zum Zielobjekt an
 * @param *pSensDist	Zeiger auf Entfernungswert (links oder rechts)
 * @param *pSpeed		Zeiger auf Geschwindigkeitsvorgabe (links oder rechts)
 */
static void update_speed(int16_t * pSensDist, int16_t * pSpeed) {
	/* Entfernung zu gross -> hinterher! */
	if (*pSensDist > FOLLOW_DISTANCE) *pSpeed += (*pSensDist - FOLLOW_DISTANCE) / (FOLLOW_DISTANCE / 10);

	/* Entfernung zu klein -> bitte Abstand halten! */
	if (*pSensDist < (FOLLOW_DISTANCE - 20)) *pSpeed -= ((int16_t) (FOLLOW_DISTANCE * 1.5f) - *pSensDist)
		/ ((int16_t) (FOLLOW_DISTANCE * 1.5f) / 10);

	/* Objekt wid nur noch halb erfasst -> abbiegen! */
	if (sensDistL - *pSensDist > FOLLOW_DISTANCE || sensDistR - *pSensDist > FOLLOW_DISTANCE) {
		// Drehungen teilweise zu stark => besser neu suchen in diesem Fall
//		*pSpeed -= 1;
	}

	/* anti wind-up */
	if (*pSpeed > MAX_SPEED) *pSpeed = MAX_SPEED;
	else if (*pSpeed < -MAX_SPEED) *pSpeed = -MAX_SPEED;

	/* Abstand ok -> Pause einlegen */
	if (abs(*pSensDist - FOLLOW_DISTANCE) <= 10) *pSpeed = BOT_SPEED_STOP;
}

/*!
 * @brief		Das Objektverfolgungsverhalten
 * @param *data	Der Verhaltensdatensatz
 */
void bot_follow_object_behaviour(Behaviour_t * data) {
	static uint8_t distToggle = 0;
#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
	static uint8_t movedState = 0;
#endif
	switch (state) {
	case 0:
		/* auf neue Messwerte der Distanzsensoren warten */
		if (distToggle != sensDistLToggle) {
			distToggle = sensDistLToggle;
			state = 1;
		}
		break;

	case 1:
		/* go! */
		update_speed(&sensDistL, &speedLeft);	// links
		update_speed(&sensDistR, &speedRight);	// rechts

		/* ganz allein :( -> anhalten */
		if (sensDistL == SENS_IR_INFINITE && sensDistR == SENS_IR_INFINITE) {
#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
			/* zurueck zum Startpunkt mit bot_drive_stack(), falls
			 * mindestens 8 Sekunden kein Objekt in Sichtweite */
			if (movedState == 0) {
				lastMove = TIMER_GET_TICKCOUNT_32;
				movedState = 1;
			} else {
				if (timer_ms_passed_32(&lastMove, 10000)) {
					bot_drive_stack(data);
					movedState = 0;
				}
			}
#endif	// BEHAVIOUR_DRIVE_STACK_AVAILABLE

			speedLeft	= BOT_SPEED_STOP;
			speedRight	= BOT_SPEED_STOP;
			break;
		}
		state = 0;
		break;

	case 2:
		/* hier koennte man abbrechen, wenn man denn wollte...
		 * Eigentlich gibt es aber keinen Zeitpunkt, an dem
		 * sich das Verhalten selbst beenden sollte.
		 */
//		return_from_behaviour(data);
		break;
	}

	/* Geschwindigkeiten setzen */
	speedWishLeft	= speedLeft;
	speedWishRight	= speedRight;
}

/*!
 * @brief			Botenfunktion des Objektverfolgungsverhaltens
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_follow_object(Behaviour_t * caller) {
	/* Verhalten starten */
	switch_to_behaviour(caller, bot_follow_object_behaviour, OVERRIDE);

	/* Inits */
#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
	bot_save_waypos(get_behaviour(bot_follow_object_behaviour), 1);
#endif
	state		= 0;
	speedLeft	= BOT_SPEED_STOP;
	speedRight	= BOT_SPEED_STOP;
}

#endif	// BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE
