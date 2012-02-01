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
 * @file 	behaviour_simple.c
 * @brief 	ganz einfache Beispielverhalten
 * Diese Datei sollte der Einstiegspunkt fuer eigene Experimente sein,
 * den Roboter zu steuern.
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.2006
 */


#include "bot-logic/bot-logic.h"
#ifdef BEHAVIOUR_SIMPLE_AVAILABLE

static uint8_t simple_state = 0;	/*!< Status des simple-Verhaltens */
static uint8_t simple2_state = 0;	/*!< Status des simple2-Verhaltens */
static int16_t simple2_light = 0;	/*!< Uebergabevariable fuer SIMPLE2 */


/*!
 * Ein ganz einfaches Verhalten, es hat maximale Prioritaet
 * Hier kann man auf ganz einfache Weise die ersten Schritte wagen.
 * Wer die Moeglichkeiten des ganzen Verhaltensframeworks ausschoepfen will, kann diese Funktion getrost auskommentieren
 * und findet dann in bot_behave_init() und bot_behave() weitere Hinweise fuer elegante Bot-Programmierung....
 *
 * Das Verhalten wird per Default nicht mitkompiliert. Sobald sie aber in include/bot-logik/available_behaviours die Kommentarzeichen vor BEHAVIOUR_SIMPLE_AVAILABLE entfernen
 * startet es automatisch und laeuft endlos.
 * Achtung, da bot_simple_behaviour() maximale Prioritaet hat, kommt es vor den anderen Regeln, wie dem Schutz vor Abgruenden, etc. zum Zuge
 * Das sollte am Anfang nicht stoeren, spaeter sollte man jedoch die Prioritaet herabsetzen.
 *
 * @param *data der Verhaltensdatensatz
 */
void bot_simple_behaviour(Behaviour_t * data) {
	switch (simple_state) {
	case 0:
		bot_drive_distance(data, 0, BOT_SPEED_MAX, 14);
		simple_state = 1;
		break;
	case 1:
		bot_turn(data, 90);
		simple_state = 0;
		break;
	default:
		return_from_behaviour(data);
		break;
	}

}

/*!
 * Rufe das Simple-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_simple(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_simple_behaviour, BEHAVIOUR_OVERRIDE);
	simple_state = 0;
}

/*!
 * Ein ganz einfaches Beispiel fuer ein Hilfsverhalten,
 * das Sensoren auswertet und einen Uebergabeparameter nutzt
 *
 * Zu diesem Verhalten gehoert die Botenfunktion bot_simple2()
 *
 * Hier kann man auf ganz einfache Weise die ersten Schritte wagen.
 * Wer die Moeglichkeiten des ganzen Verhaltensframeworks ausschoepfen will, kann diese Funktion getrost auskommentieren
 * und findet dann in bot_behave_init() und bot_behave() weitere Hinweise fuer elegante Bot-Programmierung....
 *
 * Das Verhalten ist per default abgeschaltet.
 * Damit es laeuft, muss man in include/bot-logik/available_behaviours die Kommentarzeichen vor BEHAVIOUR_SIMPLE_AVAILABLE entfernen.
 * UND in bot-logic/bot-logic.c in der Funktion bot_behave_init(void) die Kommentare vor:
 *  	activateBehaviour(bot_simple_behaviour);
 * 	und
 * //	activateBehaviour(bot_simple2_behaviour);
 * bearbeiten, so dass simple2 drankommt und simple aus ist.
 * Achtung, da bot_simple2_behaviour() maximale Prioritaet hat, kommt es vor den anderen Regeln, wie dem Schutz vor Abgruenden, etc. zum Zuge
 * Das sollte am Anfang nicht stoeren, spaeter sollte man jedoch die Prioritaet herabsetzen.
 *
 * bot_simple2_behaviour faehrt je nach Uebergabeparamerter entweder zum Licht (Parameter=0) oder davon weg (Parameter != 0)
 *
 * @param *data der Verhaltensdatensatz
 */
void bot_simple2_behaviour(Behaviour_t * data) {
	#define STATE_SIMPLE2_INIT 0
	#define STATE_SIMPLE2_SEARCH 1
	#define STATE_SIMPLE2_DRIVE 2
	#define STATE_SIMPLE2_DONE 3

	static int16_t max_light;
	static uint16_t max_dir;
	static uint16_t dir;

	uint16_t light=(sensLDRL+sensLDRR)/2;
	uint16_t free;

	switch (simple2_state) {
		case STATE_SIMPLE2_INIT:	// Initialisieren
			max_light = INT16_MAX;
			max_dir=0;
			dir=0;
			simple2_state=STATE_SIMPLE2_SEARCH;
			break;
		case STATE_SIMPLE2_SEARCH:	// Einmal drehen und maximum suchen
			if (light<max_light){
				max_dir=dir;
				max_light=light;
			}

			if (dir < 360) {	// Noch nicht ganz rum?
				bot_turn(data, 10);	// drehen
				dir+=10;		// neue Position sichern
			} else {				// wir sind ganz rum
				if (simple2_light ==0)
					bot_turn(data, max_dir);	// zum Licht drehen
				else
					bot_turn(data, max_dir-180);	// vom Licht wegdrehen

				simple2_state=STATE_SIMPLE2_DRIVE;		// Naechster Zustand
			}

			break;
		case STATE_SIMPLE2_DRIVE:	// vorwaerts fahren, bis Hinderniss
			free = (sensDistL < sensDistR) ? sensDistL : sensDistR;
			free = (free > SENS_IR_MAX_DIST) ? SENS_IR_MAX_DIST : free;

			if (free > SENS_IR_SAFE_DIST) {
#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
				bot_goto_dist(data, free - SENS_IR_SAFE_DIST, 1);	// nach vorne
#else
				bot_drive_distance(data, 0, BOT_SPEED_NORMAL, (int16_t) (free - SENS_IR_SAFE_DIST) / 10);	// nach vorne
#endif // BEHAVIOUR_GOTO_POS_AVAILABLE
				simple2_state = STATE_SIMPLE2_INIT;
			}else
				simple2_state=STATE_SIMPLE2_DONE;	// beenden
			break;

		default:
			return_from_behaviour(data);
			break;
	}
}

/*!
 * Rufe das Simple2-Verhalten auf und uebergebe light
 * @param *caller Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param light Uebergabeparameter
 */
void bot_simple2(Behaviour_t * caller, int16_t light) {
	switch_to_behaviour(caller, bot_simple2_behaviour, BEHAVIOUR_OVERRIDE);
	simple2_light = light;
	simple2_state = 0;
}
#endif // BEHAVIOUR_SIMPLE_AVAILABLE
