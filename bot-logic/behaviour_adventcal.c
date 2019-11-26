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
 * @author 	Benjamin Benz (bbe@heise.de) (Template "behaviour_prototype.c")
 * @author 	anonybotATriseupDOTnet (behaviour_adventcal.c)
 * @date 	2019-11-07
 */


#include "bot-logic/bot-logic.h"
#ifdef BEHAVIOUR_ADVENTCAL_AVAILABLE

//Log-Ausgaben, wenn in ct-bot.h aktiviert
#include "log.h"

// Status des Adventskalender-Verhaltens beim Start des Verhaltens
static uint8_t adventcal_state = 0;

// Optionaler Puffer fuer den Uebergabeparameter
//static int16_t adventcal_parameter = 0;	/*!< Uebergabevariable fuer Prototype */

#define STATE_ADVENTCAL_START 0
#define STATE_ADVENTCAL_FIND 1
#define STATE_ADVENTCAL_FLAPCLOSE 2
#define STATE_ADVENTCAL_TURNTODELIVER 3
#define STATE_ADVENTCAL_DELIVER 4
#define STATE_ADVENTCAL_FLAPOPEN 5
#define STATE_ADVENTCAL_MOVEBACK 6
#define STATE_ADVENTCAL_TURNTOSTARTPOS 7

//Abbruch-Funktion, die spaeter im Programm benoetigt wird, um zu pruefen, ob das Verhalten, einer Linie zu folgen, beendet werden kann...
static uint8_t bot_follow_line_cancel_find(void) {
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
static uint8_t bot_follow_line_cancel_deliver(void) {
	//...und zwar in Abhaengigkeit davon, ob die Abgrund- bzw. Kanten-Sensoren eine schwarze Linie sehen oder nicht
	if (sensBorderL>BORDER_DANGEROUS && sensBorderR>BORDER_DANGEROUS) {
		//wenn die Abbruch-Bedingung erfuellt ist, die beiden Kanten-Sensoren also eine schwarze Linie sehen, meldet die Abbruch-Funktion mittels "return", dass die Abbruch-Bedingung erfuellt ist und das Verhalten, einer Linie zu folgen, abgebrochen werden soll
		return 1;

	} else {
		//ansonsten, sofern also die Kanten-Sensoren keine schwarze Linie sehen, meldet die Abbruch-Funktion, dass die Bedingung nicht erfuellt ist und das Verhalten, einer Linie zu folgen, weiterlaufen muss
		return 0;
	}
}

void bot_adventcal_behaviour(Behaviour_t * data) {
	switch (adventcal_state) {
	case STATE_ADVENTCAL_START:
			LOG_DEBUG("STATE_ADVENTCAL_START_BEGIN");
			bot_servo(data, SERVO1, DOOR_OPEN);
			adventcal_state = STATE_ADVENTCAL_FIND;
			LOG_DEBUG("STATE_ADVENTCAL_START_END");
			break;
	case STATE_ADVENTCAL_FIND:
		LOG_DEBUG("STATE_ADVENTCAL_FIND_BEGIN");
		if (sensTrans == 0) {
			bot_follow_line(data, 0);
			bot_cancel_behaviour(data, bot_follow_line_behaviour, bot_follow_line_cancel_find);
		}
		adventcal_state = STATE_ADVENTCAL_FLAPCLOSE;
		LOG_DEBUG("STATE_ADVENTCAL_FIND_END");
		break;
	case STATE_ADVENTCAL_FLAPCLOSE:
		LOG_DEBUG("STATE_ADVENTCAL_FLAPCLOSE_BEGIN");
		bot_servo(data, SERVO1, DOOR_CLOSE);
		adventcal_state = STATE_ADVENTCAL_TURNTODELIVER;
		LOG_DEBUG("STATE_ADVENTCAL_FLAPCLOSE_END");
		break;
	case STATE_ADVENTCAL_TURNTODELIVER:
		LOG_DEBUG("STATE_ADVENTCAL_TURNTODELIVER_BEGIN");
		bot_turn(data, 180);
		adventcal_state = STATE_ADVENTCAL_DELIVER;
		LOG_DEBUG("STATE_ADVENTCAL_TURNTODELIVER_END");
		break;
	case STATE_ADVENTCAL_DELIVER:
		LOG_DEBUG("STATE_ADVENTCAL_DELIVER_BEGIN");
		if (sensBorderL<BORDER_DANGEROUS && sensBorderR<BORDER_DANGEROUS) {
			bot_follow_line(data, 0);
			bot_cancel_behaviour(data, bot_follow_line_behaviour, bot_follow_line_cancel_deliver);
		}
		adventcal_state = STATE_ADVENTCAL_FLAPOPEN;
		LOG_DEBUG("STATE_ADVENTCAL_DELIVER_END");
		break;
	case STATE_ADVENTCAL_FLAPOPEN:
		LOG_DEBUG("STATE_ADVENTCAL_FLAPOPEN_BEGIN");
		bot_servo(data, SERVO1, DOOR_OPEN);
		adventcal_state = STATE_ADVENTCAL_MOVEBACK;
		LOG_DEBUG("STATE_ADVENTCAL_FLAPOPEN_END");
		break;
	case STATE_ADVENTCAL_MOVEBACK:
		LOG_DEBUG("STATE_ADVENTCAL_MOVEBACK_BEGIN");
		bot_drive_distance(data, 0, -BOT_SPEED_FOLLOW, 10);
		adventcal_state = STATE_ADVENTCAL_TURNTOSTARTPOS;
		LOG_DEBUG("STATE_ADVENTCAL_MOVEBACK_END");
		break;
	case STATE_ADVENTCAL_TURNTOSTARTPOS:
		LOG_DEBUG("STATE_ADVENTCAL_TURNTOSTARTPOS_BEGIN");
		bot_turn(data, 180);
		adventcal_state = 99;
		LOG_DEBUG("STATE_ADVENTCAL_TURNTOSTARTPOS_END");
		break;
	default:
		LOG_DEBUG("default_BEGIN");
		return_from_behaviour(data);
		LOG_DEBUG("default_END");
		break;
	}

}

/*!
 * Rufe das Prototyp-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_adventcal(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_adventcal_behaviour, BEHAVIOUR_OVERRIDE);
	adventcal_state = STATE_ADVENTCAL_FIND;
}

// Alternative Botenfunktion mit Uebergabeparameter
/*!
 * Rufe das Prototyp-Verhalten auf
 * und nutze dabei einen Uebergabeparameter
 * @param *caller Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param param Uebergabeparameter
 */
/*
void bot_adventcal(Behaviour_t * caller, int16_t param) {
	switch_to_behaviour(caller, bot_adventcal_behaviour, BEHAVIOUR_OVERRIDE);
	adventcal_parameter = param;
	prototyp_state = STATE_ADVENTCAL_INIT;
}
*/
#endif // BEHAVIOUR_ADVENTCAL_AVAILABLE
