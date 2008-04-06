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
 * @file 	behaviour_drive_stack.c
 * @brief 	Anfahren aller auf dem Stack befindlichen Punkte
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	13.12.2007
 */

#include "bot-logic/available_behaviours.h"
#include "ui/available_screens.h"
#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
#include "display.h"
#include "rc5.h"
#include "rc5-codes.h"
#include "pos_stack.h"
#include "math_utils.h"
#include <stdlib.h>

static uint8_t drivestack_state = 0;	/*!< Status des drive_stack-Verhaltens */
static uint8_t put_stack_active = 0;	/*!< merkt sich, ob put_stack_waypos aktiv war */

/* hier die vom Stack geholten oder zu sichernden xy-Koordinaten; werden im Display angezeigt */
static int16_t posx = 0;
static int16_t posy = 0;

/*!
 * Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte, wobei das Fahr-Unterverhalten bot_goto_pos benutzt wird
 * @param *data	Der Verhaltensdatensatz
 */
void bot_drive_stack_behaviour(Behaviour_t * data) {
	switch (drivestack_state) {
	case 0:
		// Koordinaten werden vom Stack geholt und angefahren; Ende nach nicht mehr erfolgreichem Pop
		if (!pos_stack_pop(&posx, &posy))
			drivestack_state = 1;
		else
			bot_goto_pos(data, posx, posy, 999);
		break;

	default:
		pos_stack_clear(); // sicherheitshalber bereinigen und auf Null
//		deactivateBehaviour(bot_goto_pos_behaviour); //komischerweise fuhr bot hier weiter, daher deaktivieren
		return_from_behaviour(data);
		if (put_stack_active) {
			/* put_stack_waypos wieder an */
			bot_put_stack_waypositions(NULL);
		}
		break;
	}
}

/*!
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_stack(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_drive_stack_behaviour, OVERRIDE);
	drivestack_state = 0;
	if (behaviour_is_activated(bot_put_stack_waypositions_behaviour)) {
		/* falls put_stack_waypos an ist, temporaer deaktivieren */
		deactivateBehaviour(bot_put_stack_waypositions_behaviour);
		put_stack_active = 1;
	} else {
		put_stack_active = 0;
	}
}

/*!
 * Speichern der uebergebenen Koordinaten auf dem Stack
 * @param pos_x	X-Koordinate
 * @param pos_y	Y-Koordinate
 */
void bot_push_pos(int16_t pos_x, int16_t pos_y) {
	// sichern der Koordinaten in den Stack
	posx = pos_x;
	posy = pos_y;
	pos_stack_push(posx, posy);
}

/*!
 * Sichern der aktuellen Botposition auf den Stack
 * @param *caller einfach nur Zeiger, damit remotecall verwendbar
 */
void bot_push_actpos(Behaviour_t * caller) {
	// sichern der aktuellen Botposition auf den Stack
	bot_push_pos(x_pos, y_pos);
	if (caller) {
		caller->subResult = SUBSUCCESS;
	}
}

static uint8_t waypos_state = 0; /*!< Status des drive_stack-Push-Verhaltens */

static int16 last_xpos=0; /*!< letzte gemerkte x-Position */
static int16 last_ypos=0; /*!< letzte gemerkte y-Position */
static int16 last_heading=0; /*!< letzte gemerkte Botausrichtung */

#define DIST_FOR_PUSH 14400         /*!< Quadrat des Abstandes [mm^2] zum letzten Punkt, ab dem gepush wird */
#define DIST_FOR_PUSH_TURN 3600     /*!< Quadrat des Abstandes [mm^2] nach erreichen eines Drehwinkels zum letzten Punkt */
#define ANGLE_FOR_PUSH 20           /*!< nach Erreichen dieses Drehwinkels [Grad] wird ebenfalls gepusht */

/*!
 * Hilfsroutine zum Speichern der aktuellen Botposition in die Zwischenvariablen
 */
static void set_pos_to_last(void) {
	last_xpos=x_pos;
	last_ypos=y_pos;
	last_heading=heading;
}

/*!
 * Verhalten um sich entlang des Fahrweges relevante Koordinaten auf dem Stack zu merken; Verhalten ist nach Aktivierung via Botenroutine
 * ein Endlosverhalten und sammelt bis zur Deaktivierung die Wegepunkte; deaktiviert wird es via Notaus oder direkt mit Befehl zum Zurueckfahren und
 * damit Start des Stack-Fahrverhaltens 
 * @param *data	Der Verhaltensdatensatz
 */
void bot_put_stack_waypositions_behaviour(Behaviour_t * data) {
	switch (waypos_state) {
	case 0:
		// Botpos wird in den Stack geschrieben zum Startzeitpunkt des Verhaltens (Stack noch leer) oder
		// wenn die Entfernung zur zuletzt gemerkten Koordinate gewissen Abstand ueberschreitet oder
		// wenn Entfernung noch nicht erreicht ist aber ein gewisser Drehwinkel erreicht wurde

		// Abstand zur letzten Position ueberschritten
		if (get_dist(last_xpos, last_ypos, x_pos, y_pos) > DIST_FOR_PUSH) {
			// kein Push notwendig bei gerader Fahrt voraus zum Sparen des Stack-Speicherplatzes  	    
			if ((int16) heading != last_heading) {
				bot_push_pos(x_pos, y_pos);
			}

			set_pos_to_last(); // Position jedenfalls merken
			break;
		}

		// bei Drehwinkelaenderung und Uberschreitung einer gewissen Groesse mit geringer Abstandsentfernung zum letzten Punkt kommt er in den Stack
		if (turned_angle(last_heading) > ANGLE_FOR_PUSH
				&& get_dist(last_xpos, last_ypos, x_pos, y_pos) > DIST_FOR_PUSH_TURN) {
			set_pos_to_last();
			bot_push_pos(x_pos, y_pos);
		}

		break;

	default:
		// kommt eigentlich nie hierher, da es solange aktiv ist bis Deaktivierung eintritt; das kann Notaus sein oder das Stackfahrverhalten
		// zum Zurueck-Abfahren der Stackpunkte
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Botenfunktion: Verhalten um sich entlang des Fahrweges relevante Koordinaten auf dem Stack zu merken
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_put_stack_waypositions(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_put_stack_waypositions_behaviour, OVERRIDE);
	pos_stack_clear();
	set_pos_to_last(); // aktuelle Botposition wird zum ersten Stackeintrag und merken der Position
	bot_push_pos(last_xpos, last_ypos);
	drivestack_state = 0;
	last_xpos=0;
	last_ypos=0;
	last_heading=0;
}

/*!
 * Keyhandler zur Verwendung via Fernbedienung auf dem Display zum Stackanfahren
 */
#ifdef DISPLAY_DRIVE_STACK_AVAILABLE
static void drivestack_disp_key_handler(void) {
	switch (RC5_Code) {
	case RC5_CODE_3:
		/* Speichern der aktuellen Botposition */
		RC5_Code = 0;
		bot_push_actpos(NULL);
		break;

	case RC5_CODE_4:
		/* Verhalten starten zum Anfahren der Stackpunkte */
		RC5_Code = 0;
		bot_drive_stack(NULL);
		break;

	case RC5_CODE_5:
		/* Verhalten zum Speichern relevanter Wegepopsitionen zum Spaeteren Zurueckfahren */
		RC5_Code = 0;
		bot_put_stack_waypositions(NULL);
		break;

	case RC5_CODE_8:
		/* Loeschen des Positionsstacks */
		RC5_Code = 0;
		pos_stack_clear();
		break;

	} // switch
} // Ende Keyhandler


/*!
 * Display zum Setzen und Anfahren der Stackpunkte 
 */
void drive_stack_display(void) {
	display_cursor(1, 1);
	display_printf("Bot-Pos %5d %5d", (int16_t)x_pos, (int16_t)y_pos);
	display_cursor(2, 1);
	display_printf("Stack   %5d %5d", posx, posy);
	display_cursor(3, 1);
	display_printf("Save/Goto/Del: 3/4/8");
	display_cursor(4, 1);
	display_printf("Start WayPushPos: 5");

	drivestack_disp_key_handler();	// aufrufen des Key-Handlers
}
#endif	// DISPLAY_DRIVE_STACK_AVAILABLE
#endif	// BEHAVIOUR_DRIVE_STACK_AVAILABLE
