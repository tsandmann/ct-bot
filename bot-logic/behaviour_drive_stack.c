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
#include "display.h"
#include "rc5.h"
#include "rc5-codes.h"
#include "pos_store.h"
#include "math_utils.h"
#include "rc5-codes.h"
#include <stdlib.h>

#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE

static uint8_t drivestack_state = 0; /*!< Status des drive_stack-Verhaltens */
static uint8_t put_stack_active = 0; /*!< merkt sich, ob put_stack_waypos aktiv war */

/*! hier die vom Stack geholten oder zu sichernden xy-Koordinaten; werden im Display angezeigt */
static position_t pos = { 0, 0 };
static uint8_t go_fifo = 0; 			/*!< falls True wird nich mit Pop nach LIFO sondern via Queue FIFO gefahren */
static pos_store_t * pos_store = NULL;	/*!< Positionsspeicher, den das Verhalten benutzt */

/*!
 * Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte, wobei das Fahr-Unterverhalten bot_goto_pos benutzt wird
 * @param *data	Der Verhaltensdatensatz
 */
void bot_drive_stack_behaviour(Behaviour_t * data) {
	uint8_t get_pos;

	switch (drivestack_state) {
	case 0:
		// Koordinaten werden vom Stack geholt und angefahren; Ende nach nicht mehr erfolgreichem Pop

		// wenn Fifo-Queue definiert, kann sowohl nach LIFO (Stack) oder FIFO (Queue) gefahren werden
		get_pos = (go_fifo) ? pos_store_dequeue(pos_store, &pos) : pos_store_pop(pos_store, &pos);

		if (!get_pos) {
			drivestack_state = 1;
		} else {
			bot_goto_pos(data, pos.x, pos.y, 999);
		}
		break;

	default:
//		pos_store_release(pos_store); // Speicher freigeben
		pos_store = NULL;
		return_from_behaviour(data);
		if (put_stack_active == 1) {
			/* put_stack_waypos wieder an */
			bot_save_waypositions(NULL);
		}
		break;
	}
}

/*!
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack X befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param stack		Nummer des Stacks, der abgefahren werden soll
 * @param mode		0: Stack, 1: FIFO
 */
void bot_drive_stack_x(Behaviour_t * caller, uint8_t stack, uint8_t mode) {
	pos_store_t * store = pos_store_from_index(stack);
	if (store == NULL) {
		caller->subResult = SUBFAIL;
		return;
	}
	if (behaviour_is_activated(bot_save_waypositions_behaviour)) {
		/* falls put_stack_waypos an ist, temporaer deaktivieren */
		deactivateBehaviour(bot_save_waypositions_behaviour);
		put_stack_active = 1;
	} else {
		put_stack_active = 0;
	}

	pos_store = store;
	switch_to_behaviour(caller, bot_drive_stack_behaviour, OVERRIDE);
	drivestack_state = 0;
	go_fifo = mode;
}

/*!
 * Ermittelt den Index des Positionsspeichers vom Wayposverhalten
 * @return	Indexwert oder 255, falls kein Positionsspeicher angelegt
 */
static uint8_t get_waypos_index(void) {
	pos_store_t * store = pos_store_from_beh(get_behaviour(bot_save_waypositions_behaviour));
	if (pos_store != NULL) {
		return pos_store_get_index(store);
	} else {
		return 255;
	}
}

/*!
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_stack(Behaviour_t * caller) {
	bot_drive_stack_x(caller, get_waypos_index(), 0);
}

/*!
 * Botenfunktion: Verhalten zum Anfahren aller in der FIFO-Queue befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_fifo(Behaviour_t * caller) {
	bot_drive_stack_x(caller, get_waypos_index(), 1);
}

/*!
 * Sichern der aktuellen Botposition auf den Stack
 * @param *caller	einfach nur Zeiger, damit remotecall verwendbar
 * @param stack		Nummer des Stacks, auf den gepusht werden soll
 */
void bot_push_actpos(Behaviour_t * caller, uint8_t stack) {
	position_t pos;
	pos.x = x_pos;
	pos.y = y_pos;
	// sichern der aktuellen Botposition auf den Stack
	pos_store_push(pos_store_from_index(stack), pos);
	if (caller) {
		caller->subResult = SUBSUCCESS;
	}
}

static uint8_t waypos_state = 0; 		/*!< Status des drive_stack-Push-Verhaltens */
static position_t last_pos = { 0, 0 };	/*!< letzte gemerkte Position */
static int16_t last_heading = 0;		/*!< letzte gemerkte Botausrichtung */

#define DIST_FOR_PUSH 14400         /*!< Quadrat des Abstandes [mm^2] zum letzten Punkt, ab dem gepush wird */
#define DIST_FOR_PUSH_TURN 3600     /*!< Quadrat des Abstandes [mm^2] nach erreichen eines Drehwinkels zum letzten Punkt */
#define ANGLE_FOR_PUSH 20           /*!< nach Erreichen dieses Drehwinkels [Grad] wird ebenfalls gepusht */

/*!
 * Hilfsroutine zum Speichern der aktuellen Botposition in die Zwischenvariablen
 */
static void set_pos_to_last(void) {
	last_pos.x = x_pos;
	last_pos.y = y_pos;
	last_heading = heading;
}

/*!
 * Speichern der uebergebenen Koordinaten auf dem Stack
 * @param pos_x	X-Koordinate
 * @param pos_y	Y-Koordinate
 */
static void bot_push_pos(int16_t pos_x, int16_t pos_y) {
	// sichern der Koordinaten in den Stack
	position_t pos;
	pos.x = pos_x;
	pos.y = pos_y;
	pos_store_push(pos_store, pos);
}

/*!
 * Verhalten um sich entlang des Fahrweges relevante Koordinaten auf dem Stack zu merken; Verhalten ist nach Aktivierung via Botenroutine
 * ein Endlosverhalten und sammelt bis zur Deaktivierung die Wegepunkte; deaktiviert wird es via Notaus oder direkt mit Befehl zum Zurueckfahren und
 * damit Start des Stack-Fahrverhaltens
 * @param *data	Der Verhaltensdatensatz
 */
void bot_save_waypositions_behaviour(Behaviour_t * data) {
	switch (waypos_state) {
	case 0:
		pos_store = pos_store_new(data);
		if (pos_store == NULL) {
			exit_behaviour(data, SUBFAIL);
			return;
		}
		set_pos_to_last(); // aktuelle Botposition wird zum ersten Stackeintrag und merken der Position
		bot_push_pos(last_pos.x, last_pos.y);
		waypos_state = 1;
		/* no break */
	case 1:
		// Botpos wird in den Stack geschrieben zum Startzeitpunkt des Verhaltens (Stack noch leer) oder
		// wenn die Entfernung zur zuletzt gemerkten Koordinate gewissen Abstand ueberschreitet oder
		// wenn Entfernung noch nicht erreicht ist aber ein gewisser Drehwinkel erreicht wurde

		// Abstand zur letzten Position ueberschritten
		if (get_dist(last_pos.x, last_pos.y, x_pos, y_pos) > DIST_FOR_PUSH) {
			// kein Push notwendig bei gerader Fahrt voraus zum Sparen des Stack-Speicherplatzes
			if ((int16_t) heading != last_heading) {
				bot_push_pos(x_pos, y_pos);
			}

			set_pos_to_last(); // Position jedenfalls merken
			break;
		}

		// bei Drehwinkelaenderung und Uberschreitung einer gewissen Groesse mit geringer Abstandsentfernung zum letzten Punkt kommt er in den Stack
		if (turned_angle(last_heading) > ANGLE_FOR_PUSH && get_dist(last_pos.x,
				last_pos.y, x_pos, y_pos) > DIST_FOR_PUSH_TURN) {
			set_pos_to_last();
			bot_push_pos(x_pos, y_pos);
		}

		break;

	default:
		// kommt eigentlich nie hierher, da es solange aktiv ist bis Deaktivierung eintritt; das kann Notaus sein oder das Stackfahrverhalten
		// zum Zurueck-Abfahren der Stackpunkte
		pos_store_release(pos_store);
		pos_store = NULL;
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Botenfunktion: Verhalten um sich entlang des Fahrweges relevante Koordinaten auf dem Stack zu merken
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_save_waypositions(Behaviour_t * caller) {
	activateBehaviour(caller, bot_save_waypositions_behaviour);
	drivestack_state = 0;
	waypos_state = 0;
	last_pos.x = 0;
	last_pos.y = 0;
	last_heading = 0;
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
		if (get_waypos_index() != 255) {
			bot_push_actpos(NULL, get_waypos_index());
		}
		break;

		case RC5_CODE_4:
		/* Verhalten starten zum Anfahren der Stackpunkte */
		RC5_Code = 0;
		bot_drive_stack(NULL);
		break;

		case RC5_CODE_5:
		/* Verhalten zum Speichern relevanter Wegepopsitionen zum Spaeteren Zurueckfahren */
		RC5_Code = 0;
		bot_save_waypositions(NULL);
		break;

		case RC5_CODE_7:
		/* Verhalten starten zum Anfahren der Stackpunkte vom Startpunkt an vorwaerts */
		RC5_Code = 0;
		bot_drive_fifo(NULL);
		break;

		case RC5_CODE_8:
		/* Loeschen des Positionsstacks */
		RC5_Code = 0;
		if (get_waypos_index() != 255) {
			pos_store_clear(pos_store);
		}
		break;

	} // switch
} // Ende Keyhandler


/*!
 * Display zum Setzen und Anfahren der Stackpunkte
 */
void drive_stack_display(void) {
	display_cursor(1, 1);
	display_printf("Stack   %5d %5d", pos.x, pos.y);
	display_cursor(2, 1);
	display_printf("Save/Del      : 3/8");
	display_cursor(3, 1);
	display_printf("GoBack/Forward: 4/7");
	display_cursor(4, 1);
	display_printf("Start WayPushPos: 5");

	drivestack_disp_key_handler(); // aufrufen des Key-Handlers
}
#endif	// DISPLAY_DRIVE_STACK_AVAILABLE
#endif	// BEHAVIOUR_DRIVE_STACK_AVAILABLE
