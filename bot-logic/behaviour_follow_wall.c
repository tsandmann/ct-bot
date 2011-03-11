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
 * \file 	behaviour_follow_wall.c
 * \brief 	Wandfolger Explorer
 *
 * Faehrt solange vorwaerts, bis er an eine Wand kommt, an die er sich gewisse Zeit wegdreht;
 * nach links dreht er sich, wenn rechts eine Wand als naeher erkannt wird sonst nach rechts; es erfolgt
 * hier auch eine Abgrundauswertung; wird erkannt dass entweder
 * beide Abgrundsensoren den Abgrund detektieren oder der bot senkrecht zur Wand steht, so wird via Zeitzaehler
 * ein Pseudo-Zufallswert bis 255 ausgewertet und danach die neue Drehrichtung links/ rechts ermittelt;
 * zur Mindestdrehzeit wird ebenfalls immer dieser Zufallswert zuaddiert
 * sehr sinnvoll mit diesem Explorer-Verhalten ist das hang_on-Verhalten, welches durch Vergleich mit Mausdaten
 * ein Haengenbleiben erkennt, rueckwaerts faehrt und das hier registrierte Notverhalten startet. Dieses wiederum
 * sorgt dafuer, dass der bot sich wegdreht und weiterfaehrt wie an einer Wand. Gleiches gilt fuer das Abgrundverhalten.
 *
 * \author 	Frank Menzel (Menzelfr@gmx.net)
 * \date 	30.08.2007
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_FOLLOW_WALL_AVAILABLE
#include "timer.h"
#include <stdlib.h>

/* Zustaende fuer check_wall_behaviour-Verhalten */
#define CHECK_FOR_BACK		        1
#define CHECK_WALL_GO				2
#define CHECK_WALL_TURN				3
#define CHECK_FOR_READY             4

/* Verhaltens- und letzter gemerkter Status  */
static uint8_t state = CHECK_FOR_BACK;
static uint8_t laststate = 0;

/* Zeitzaehlerwert vor Neusetzen */
static uint16_t lastmsTime = 0;

/* Abgrund wurde erkannt */
static int8_t border_follow_wall_fired = False;

/* Mindest-Wartezeiten fuer Drehung; auf diese Werte wird noch der Pseudo-Zufallswert aus
 * TIMER_GET_TICKCOUNT_8 raufaddiert, kann also bis 255 mehr sein */
#define DELAY_NORMAL     100       // normale Wartezeit
#ifdef PC
#define DELAY_AFTER_HOLE 300       // Wartezeit nach Abgrund
#define DELAY_AFTER_HOLE_VERT 400
#else
#define DELAY_AFTER_HOLE 150       // Wartezeit nach Abgrund
#define DELAY_AFTER_HOLE_VERT 200
#endif	// PC
/* Delay-Time, solange wird mindestens gedreht, gesetzt im Prog */
static uint16_t delay_time_turn = 0;

static int16_t wall_side = 0; // <0 Hindernis links, sonst rechts

/*!
 * Abbruchfunktion des Verhaltens()
 * Die Funktion, mit der das bot_explore_behaviour() feststellt, ob es etwas gefunden hat.
 * Die Funktion muss True (1) zurueckgeben, wenn dem so ist, sonst False (0).
 * Beispiele fuer eine solche Funktion sind check_for_light, is_good_pillar_ahead etc.
 */
 static uint8_t (*check_function)(void) = 0;

/*!
 * ermittelt Zufallswert je nach Zeitzaehler
 * \param compval	Zeit-Vergleichswert
 * \return  		liefert 1 oder -1 in Abhaengigkeit der abgelaufenen Zeit als Zufallskomponente
 */
static int8_t get_randomside(uint8_t compval) {
	return (int8_t) (TIMER_GET_TICKCOUNT_8 > compval ? -1 : 1);
}

/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung oder Haengenbleiben; muss registriert werden und laesst
 * den Bot etwas rueckwaerts fahren und dreht ihn zur Seite
 */
void border_follow_wall_handler(void) {
	// Routine muss zuerst checken, ob follow_wall auch gerade aktiv ist, da nur in diesem
	// Fall etwas gemacht werden muss
	if (!behaviour_is_activated(bot_follow_wall_behaviour))
		return;

	border_follow_wall_fired=True; // Setzen der Syncvar des Verhaltens, die immer abgefragt wird

	state = CHECK_FOR_BACK;

	if (sensBorderL > BORDER_DANGEROUS && sensBorderR > BORDER_DANGEROUS) {
		wall_side = get_randomside(127); // links fuer < 127 sonst rechts
		delay_time_turn = (uint16_t) (DELAY_AFTER_HOLE_VERT + TIMER_GET_TICKCOUNT_8); // Mindest- und Zufallszeit
	} else {
		wall_side = (sensBorderL > BORDER_DANGEROUS) ? -1 : 1; // Drehseite festlegen
		delay_time_turn = (uint16_t) (DELAY_AFTER_HOLE + TIMER_GET_TICKCOUNT_8); // Mindest- und Zufallszeit
	}

	// rueckwaertsfahren; bei schon aktivem Verhalten durch Notfallverhalten selbst wird dieses
	// hier nicht ausgefuehrt
	bot_drive_distance(0, 0, -BOT_SPEED_FOLLOW, 10);
}

/*!
 * Das eigentliche Wandfolgerverhalten; Bot faehrt gerade voraus bis zu einer Wand und dreht sich gewisse Zeit solange,
 * bis keine Wand mehr in geringem Abstand gefunden wird; trifft er senkrecht auf eine Wand, wird per Zufall die Seite
 * ermittelt, wohin sich der Bot wegdreht (gilt auch fuer Abgrund); das Speiel geht wieder von vorn los
 * \param *data  der Verhaltensdatensatz
 */
void bot_follow_wall_behaviour(Behaviour_t * data) {
	int16_t sensor = 0; // Merker fuer Abstandssensor je nach Wand links oder rechts

	if (check_function && (*check_function)()) // wenn ueberhaupt definiert und Abbruchbedingung erfuellt
		state = CHECK_FOR_READY;

	switch (state) {

	case CHECK_FOR_BACK: // Check auf zu nah oder bei Abgrund mit Rueckwaertsfahren
		// Der Bot erkennt Hindernis
		if (border_follow_wall_fired
				|| is_obstacle_ahead(OPTIMAL_DISTANCE + ADJUST_DISTANCE)) { // aus bot_olympic
			/* Abhaengig von der Seite, auf der die Wand ist, Entfernung ermitteln fuer rueckwaerts */
			sensor = (is_obstacle_ahead(OPTIMAL_DISTANCE + ADJUST_DISTANCE) < 0) ? sensDistL
				: sensDistR ;

			if (border_follow_wall_fired) { // gesetzt durch registriertes Abgrundverhalten
				state = CHECK_WALL_TURN; // nach rueckwaerts immer zum drehen
				border_follow_wall_fired = False;
				lastmsTime = TIMER_GET_TICKCOUNT_16; // Zeitzaehler geht jetzt los, damit mindestens xxms gedreht wird
				break;
			} // Notfall erkannt

		} // Hindernis oder Abgrund vorhanden

		/* kam ich von Drehung, dann auch wieder dorthin sonst fahren */
		state = (uint8_t) (laststate == CHECK_WALL_TURN ? CHECK_WALL_TURN : CHECK_WALL_GO);

		break;

	case CHECK_WALL_GO:

		// Der Bot erkennt Hindernis oder Abgrund und muss gecheckt werden
		if (is_obstacle_ahead(OPTIMAL_DISTANCE + ADJUST_DISTANCE)) { // innerhalb xx cm Hindernis

			/* naechsten Status setzen und aktuellen merken */
			state = CHECK_FOR_BACK;
			laststate = CHECK_WALL_TURN;

			// Seite des Hindernisses ermitteln; <0 links, >= 0 rechts
			wall_side = is_obstacle_ahead(OPTIMAL_DISTANCE + ADJUST_DISTANCE); // Seite ermitteln

			/* bei Wand 90Grad voraus, dann per Zufall nach links oder rechts wegdrehen */
			if (sensDistL < COL_FAR && sensDistR < COL_FAR) {
				wall_side = get_randomside(127); // < 127 links sonst rechts
			}

			break;
		}

		// Geschwindigkeiten je nach gesehener Entfernung setzen
		if (sensDistL > COL_FAR && sensDistR > COL_FAR) {
			speedWishLeft = BOT_SPEED_FAST;
			speedWishRight = BOT_SPEED_FAST;
		} else {
			if (sensDistL > COL_CLOSEST && sensDistR > COL_CLOSEST) {
				speedWishLeft = BOT_SPEED_NORMAL;
				speedWishRight = BOT_SPEED_NORMAL;
			} else {
				speedWishLeft = BOT_SPEED_FOLLOW;
				speedWishRight = BOT_SPEED_FOLLOW;
			}
		}

		laststate = CHECK_WALL_GO; // merken dass ich aus GO komme

		break; // immer weiter fahren in GO

	case CHECK_WALL_TURN:
		// nun bestimmte Zeit von Wand wegdrehen, je nach bereits ermittelter Seite

		// Setzen der Drehgeschwindigkeiten
		if (wall_side < 0) { // rechtsdrehen wenn Wand links
			speedWishLeft = BOT_SPEED_FOLLOW;
			speedWishRight = -BOT_SPEED_FOLLOW;
		} else { // sonst linksdrehen, wenn Wand rechts
			speedWishLeft = -BOT_SPEED_FOLLOW;
			speedWishRight = BOT_SPEED_FOLLOW;
		}

		// solange drehen bis Zeit in ms ueberschritten wird; ist dann genug Abstand bei beiden Sensoren
		// zur Wand vorhanden ist Drehung beendet
		// Durch die Zeit kommt der bot den Wandecken weniger nah, da er sich dadurch nicht genau parallel ausrichtet
		// und mit Drehen aufhoert wenn er ueber die Ecke schielt sondern sich wieder etwas von wegdreht; bei Abgrund dreht er
		// sich damit auch sicher weg mit hoeherer Drehzeit
		// Wartezeit hier nicht init. da dies dann noch zufaelliger wird
		if (timer_ms_passed_16(&lastmsTime, delay_time_turn)) {
			delay_time_turn = (uint16_t) (DELAY_NORMAL + TIMER_GET_TICKCOUNT_8); // wieder mit normalen Pseudo-Zufallswert belegen

			if (sensDistL > COL_NEAR&& sensDistR > COL_NEAR) { // keine Wand mehr in Sichtweite
				state = CHECK_FOR_BACK; // Abgrund-/ Abstandscheck und dann los
				laststate = 0; // Initialwert
				break;
			}
		} // Check auf Ablauf der Wartezeit

		break;

	default:
		state = CHECK_FOR_BACK;
		laststate = 0;
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Faehrt vorwaerts bis zu einer Wand, von die er sich wegdreht
 * \param *check	Abbruchfunktion; wenn diese True liefert wird das Verhalten beendet sonst endlos
 * 					einfach NULL uebergeben, wenn keine definiert ist
 * \param *caller	Verhaltensdatensatz
 */
void bot_follow_wall(Behaviour_t * caller, uint8_t (*check)(void)) {
	// Umschalten zum eigentlichen Verhalten
	switch_to_behaviour(caller, bot_follow_wall_behaviour, BEHAVIOUR_NOOVERRIDE);

	// bei Aufruf von anderem Verhalten aus ist dies die Abbruchfunktion, d.h. es wird
	// wieder zum aufrufenden Verhalten zurueckgekehrt
	check_function = check;

#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
	// verwaltet selbst Abstand zur Wand
	deactivateBehaviour(bot_avoid_col_behaviour);
#endif

#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
	// falls durch anderes Verhalten vorher ausgeschaltet wurde
	activateBehaviour(NULL, bot_avoid_border_behaviour);
#endif

	// nach Notaus stehen diese Vars sonst undefiniert rum, also init.
	state = CHECK_FOR_BACK;
	laststate = 0;
}

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
/*!
 * Botenverhalten zum Aufruf via Remotecall ohne weitere params, d.h. da kein Abbruchverhalten
 * uebergeben wird, ist dies dann ein Endlos-Explorerverhalten
 * \param *caller Verhaltensdatensatz
 */
void bot_do_wall_explore(Behaviour_t * caller) {
	bot_follow_wall(caller, NULL);
}
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE
#endif // BEHAVIOUR_FOLLOW_WALL_AVAILABLE


