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
 * @file 	behaviour_adventcal.c
 * @brief 	Adventskalender-Verhalten
 * @author 	anonybot (anonybot@riseup.net), based on template "behaviour_prototype.h" by Benjamin Benz (bbe@heise.de), created with a lot of help by Timo Sandmann (mail@timosandmann.de)
 * @date 	2019-11-30
 */


#include "bot-logic/bot-logic.h"
#ifdef BEHAVIOUR_ADVENTCAL_AVAILABLE

//Log-Ausgaben, wenn in ct-bot.h aktiviert
#include "log.h"

// Status des Adventskalender-Verhaltens beim Start des Verhaltens
static uint8_t adventcal_state = 0;

#define STATE_ADVENTCAL_START 0
#define STATE_ADVENTCAL_FIND 1
#define STATE_ADVENTCAL_FLAPCLOSE 2
#define STATE_ADVENTCAL_TURNTODELIVER 3
#define STATE_ADVENTCAL_DELIVER 4
#define STATE_ADVENTCAL_FLAPOPEN 5
#define STATE_ADVENTCAL_MOVEBACK 6
#define STATE_ADVENTCAL_TURNTOSTARTPOS_TURN1 7
#define STATE_ADVENTCAL_TURNTOSTARTPOS_PASSLINE 8
#define STATE_ADVENTCAL_TURNTOSTARTPOS_TURN2 9

//Abbruch-Funktion, die spaeter im Programm benoetigt wird, um zu pruefen, ob das Verhalten, einer Linie zu folgen, beendet werden kann...
static uint8_t cancel_follow_line_object_catched(void) {
	//...und zwar in Abhaengigkeit davon, ob der Sensor im Transportfach etwas sieht oder nicht
	if (sensTrans == 1) {
		//"1" bedeutet, dass ein Objekt im Transportfach ist und die Abbruch-Funktion mittels "return" meldet, dass die Abbruch-Bedingung erfuellt ist und das Verhalten, einer Linie zu folgen, abgebrochen werden soll
		return 1;

	} else {
		//ansonsten, also wenn sich nichts im Transportfach befindet, meldet die Abbruch-Funktion, dass die Abbruch-Bedingung nicht erfuellt ist und das Verhalten, einer Linie zu folgen, weiterlaufen muss
		return 0;
	}
}

//Abbruch-Funktion, die spaeter im Programm benoetigt wird, um zu pruefen, ob das Verhalten, einer Linie zu folgen, beendet werden kann...
static uint8_t cancel_follow_line_on_border(void) {
	//...und zwar in Abhaengigkeit davon, ob die Abgrund- bzw. Kanten-Sensoren eine schwarze Linie sehen oder nicht
	//fuer Sim-Tests nur die folgende Zeile nutzen
	if (sensBorderL>BORDER_DANGEROUS && sensBorderR>BORDER_DANGEROUS) {
	//fuer Real-Tests nur die folgende Zeile nutzen und ggf. die Werte (0x2A0) an Schwarz-Werte des verwendeten Klebebandes anpassen, die der Bot uber die Linien-Sensoren meldet
	//if (sensBorderL>0x2A0 && sensBorderR>0x2A0) {
		//wenn die Abbruch-Bedingung erfuellt ist, die beiden Kanten-Sensoren also eine schwarze Linie sehen, meldet die Abbruch-Funktion mittels "return", dass die Abbruch-Bedingung erfuellt ist und das Verhalten, einer Linie zu folgen, abgebrochen werden soll
		return 1;

	} else {
		//ansonsten, sofern also die Kanten-Sensoren keine schwarze Linie sehen, meldet die Abbruch-Funktion, dass die Bedingung nicht erfuellt ist und das Verhalten, einer Linie zu folgen, weiterlaufen muss
		return 0;
	}
}

void bot_adventcal_behaviour(Behaviour_t * data) {
	switch (adventcal_state) {
	//Sicherstellen, dass die Transportfach-Klappe geoeffnet ist, bevor die Suche nach dem "Tuerchen" beginnt
	case STATE_ADVENTCAL_START:
			LOG_DEBUG("STATE_ADVENTCAL_START_BEGIN");
			bot_servo(data, SERVO1, DOOR_OPEN);
			adventcal_state = STATE_ADVENTCAL_FIND;
			LOG_DEBUG("STATE_ADVENTCAL_START_END");
			break;
	//Suchen des Transportfach-Objekts
	case STATE_ADVENTCAL_FIND:
		LOG_DEBUG("STATE_ADVENTCAL_FIND_BEGIN");
		//Solange der Transportfach-Sensor nichts sieht...
		if (sensTrans == 0) {
			//...folgt der Bot der Linie...
			bot_follow_line(data, 0);
			//...und bricht ab, sobald der Transportfach-Senor ein Objekt im Transportfach registriert
			bot_cancel_behaviour(data, bot_follow_line_behaviour, cancel_follow_line_object_catched);
		}
		adventcal_state = STATE_ADVENTCAL_FLAPCLOSE;
		LOG_DEBUG("STATE_ADVENTCAL_FIND_END");
		break;
	//Schlieszen der Transportfach-Klappe
	case STATE_ADVENTCAL_FLAPCLOSE:
		LOG_DEBUG("STATE_ADVENTCAL_FLAPCLOSE_BEGIN");
		bot_servo(data, SERVO1, DOOR_CLOSE);
		adventcal_state = STATE_ADVENTCAL_TURNTODELIVER;
		LOG_DEBUG("STATE_ADVENTCAL_FLAPCLOSE_END");
		break;
	//Wende als Vorbereitung, um das Transportfach-Objekt zur Startposition zurueckzubringen
	case STATE_ADVENTCAL_TURNTODELIVER:
		LOG_DEBUG("STATE_ADVENTCAL_TURNTODELIVER_BEGIN");
		//nicht ganz 180 Grad, damit die Linien-Sensoren nicht beide über der schwarzen Linie zum Stehen kommen
		bot_turn(data, 170);
		LOG_DEBUG("STATE_ADVENTCAL_TURNTODELIVER_TURN_STARTED");
		//bot_drive_distance(data, 0, BOT_SPEED_FOLLOW, 1);
		//LOG_DEBUG("STATE_ADVENTCAL_TURNTODELIVER_MOVE1CM_DONE");
		adventcal_state = STATE_ADVENTCAL_DELIVER;
		LOG_DEBUG("STATE_ADVENTCAL_TURNTODELIVER_END");
		break;
	//Transport des Transportfach-Objekts zur Startposition
	case STATE_ADVENTCAL_DELIVER:
		LOG_DEBUG("STATE_ADVENTCAL_DELIVER_BEGIN");
		//Soalange die Kanten-Sensoren keine Linie erkennen...
		if (sensBorderL<BORDER_DANGEROUS && sensBorderR<BORDER_DANGEROUS) {
			//...folgt der Bot der Linie...
			bot_follow_line(data, 0);
			//...und bricht ab, sobald beide Kanten-Sensoren eine Linie erkennen.
			bot_cancel_behaviour(data, bot_follow_line_behaviour, cancel_follow_line_on_border);
		}
		adventcal_state = STATE_ADVENTCAL_FLAPOPEN;
		LOG_DEBUG("STATE_ADVENTCAL_DELIVER_END");
		break;
	//Oeffnen der Transportfach-Klappe, bevor das Transportfach-Objekt freigegeben wird
	case STATE_ADVENTCAL_FLAPOPEN:
		LOG_DEBUG("STATE_ADVENTCAL_FLAPOPEN_BEGIN");
		bot_servo(data, SERVO1, DOOR_OPEN);
		adventcal_state = STATE_ADVENTCAL_MOVEBACK;
		LOG_DEBUG("STATE_ADVENTCAL_FLAPOPEN_END");
		break;
	//Freigabe des Transportfach-Objekts...
	case STATE_ADVENTCAL_MOVEBACK:
		LOG_DEBUG("STATE_ADVENTCAL_MOVEBACK_BEGIN");
		//...durch rueckwaertiges Fahren von min. 8cm, um vor der letzten Drehung die Dose nicht zu beruehren
		bot_drive_distance(data, 0, -BOT_SPEED_FOLLOW, 8);
		adventcal_state = STATE_ADVENTCAL_TURNTOSTARTPOS_TURN1;
		LOG_DEBUG("STATE_ADVENTCAL_MOVEBACK_END");
		break;
	//3 Drehungsmaneuver, um zurueck in Ausgangsposition zu kommen
	 //Maneuver 1/3
	case STATE_ADVENTCAL_TURNTOSTARTPOS_TURN1:
		LOG_DEBUG("STATE_ADVENTCAL_TURNTOSTARTPOS_TURN1_BEGIN");
		//Bot 90Grad drehen, und...
		bot_turn(data, 90);
		adventcal_state = STATE_ADVENTCAL_TURNTOSTARTPOS_PASSLINE;
		LOG_DEBUG("STATE_ADVENTCAL_TURNTOSTARTPOS_TURN1_DONE");
		break;
	 //Maneuver 2/3
	case STATE_ADVENTCAL_TURNTOSTARTPOS_PASSLINE:
		LOG_DEBUG("STATE_ADVENTCAL_TURNTOSTARTPOS_PASSLINE_BEGIN");
		//...anschliessend 4cm geradeaus knapp ueber Linie fahren lassen
		//(Distanz je nach Breite der schwarzen Linie anpassen), damit...
		bot_drive_distance(data, 0, BOT_SPEED_FOLLOW, 4);
		LOG_DEBUG("STATE_ADVENTCAL_TURNTOSTARTPOS_PASSLINE_END");
		adventcal_state = STATE_ADVENTCAL_TURNTOSTARTPOS_TURN2;
		break;
	 //Maneuver 3/3
	case STATE_ADVENTCAL_TURNTOSTARTPOS_TURN2:
		LOG_DEBUG("STATE_ADVENTCAL_TURNTOSTARTPOS_TURN2_BEGIN");
		//...sich der rechte Linien-Sensor nach einer zweiten Drehung rechts von der Linie befindet
		bot_turn(data, 90);
		adventcal_state = 99;
		LOG_DEBUG("STATE_ADVENTCAL_TURNTOSTARTPOS_TURN2_DONE");
		break;
	//Default-Verhalten, in diesem Fall: Gar nichts tun (Ende des Verhaltens)
	default:
		LOG_DEBUG("default_BEGIN");
		return_from_behaviour(data);
		LOG_DEBUG("default_END");
		break;
	}

}

/*!
 * Rufe das Adventskalender-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_adventcal(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_adventcal_behaviour, BEHAVIOUR_OVERRIDE);
	adventcal_state = STATE_ADVENTCAL_START;
}

#endif // BEHAVIOUR_ADVENTCAL_AVAILABLE
