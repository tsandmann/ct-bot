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


/**
 * \file 	behaviour_drive_stack.c
 * \brief 	Anfahren aller auf dem Stack befindlichen Punkte
 * \author 	Frank Menzel (Menzelfr@gmx.net)
 * \date 	13.12.2007
 */

#include "bot-logic.h"

#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
#include "ui/available_screens.h"
#include "display.h"
#include "rc5.h"
#include "rc5-codes.h"
#include "pos_store.h"
#include "math_utils.h"
#include "rc5-codes.h"
#include "map.h"
#include "command.h"
#include "log.h"
#include "bot-2-bot.h"
#include <stdlib.h>

#define DEBUG

/* bot_save_waypos()-Parameter */
#define STACK_SIZE		64 /**< Groesse des Positionsspeichers */
#define MAX_GRADIENT	10 /**< Maximale Steigungsdifferenz, die noch als gleich angesehen wird */
#define MAX_POS_DIFF	(200L * 200L) /**< Quadrat der maximalen Entfernung, bis zu der eine Schleife noch geschlossen wird */

#if STACK_SIZE > POS_STORE_SIZE
#undef STACK_SIZE
#define STACK_SIZE POS_STORE_SIZE
#endif

#ifndef LOG_AVAILABLE
#undef DEBUG
#endif
#ifndef DEBUG
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif // DEBUG

static uint8_t drivestack_state = 0; /**< Status des drive_stack-Verhaltens */
static uint8_t put_stack_active = 0; /**< merkt sich, ob put_stack_waypos aktiv war */

/** hier die vom Stack geholten oder zu sichernden xy-Koordinaten; werden im Display angezeigt */
static position_t pos = { 0, 0 };
static uint8_t go_fifo = 0; 			/**< falls True, wird nich mit Pop nach LIFO sondern via Queue FIFO gefahren */
static uint8_t optimized_push = 0;		/**< Optimierungslevel */
static pos_store_t * pos_store = NULL;	/**< Positionsspeicher, den das Verhalten benutzt */

/**
 * Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte, wobei das Fahr-Unterverhalten bot_goto_pos benutzt wird
 * \param *data	Der Verhaltensdatensatz
 */
void bot_drive_stack_behaviour(Behaviour_t * data) {
	uint8_t get_pos;

	switch (drivestack_state) {
	case 0:
		// Koordinaten werden vom Stack geholt und angefahren; Ende nach nicht mehr erfolgreichem Pop

		/* wenn Fifo-Queue definiert, kann sowohl nach LIFO (Stack) oder FIFO (Queue) gefahren werden */
		if (go_fifo) {
			get_pos = pos_store_dequeue(pos_store, &pos);
		} else {
			get_pos = pos_store_pop(pos_store, &pos);
		}

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
			bot_save_waypos(NULL, optimized_push);
		}
		break;
	}
}

/**
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack X befindlichen Punkte
 * \param *caller	Der Verhaltensdatensatz des Aufrufers
 * \param stack		Nummer des Stacks, der abgefahren werden soll
 * \param mode		0: Stack, 1: FIFO
 */
void bot_drive_stack_x(Behaviour_t * caller, uint8_t stack, uint8_t mode) {
	pos_store_t * store = pos_store_from_index(stack);
	if (store == NULL) {
		caller->subResult = BEHAVIOUR_SUBFAIL;
		LOG_DEBUG("Fehler, Positionsspeicher %u ungueltig", stack);
		return;
	}
	if (behaviour_is_activated(bot_save_waypos_behaviour)) {
		/* falls put_stack_waypos an ist, temporaer deaktivieren */
		deactivateBehaviour(bot_save_waypos_behaviour);
		put_stack_active = 1;
	} else {
		put_stack_active = 0;
	}

	pos_store = store;
	switch_to_behaviour(caller, bot_drive_stack_behaviour, BEHAVIOUR_OVERRIDE);
	drivestack_state = 0;
	go_fifo = mode;
}

/**
 * Ermittelt den Index des Positionsspeichers vom Wayposverhalten
 * \return	Indexwert oder 255, falls kein Positionsspeicher angelegt
 */
static uint8_t get_waypos_index(void) {
	pos_store_t * store = pos_store_from_beh(get_behaviour(bot_save_waypos_behaviour));
	if (store != NULL) {
		return pos_store_get_index(store);
	} else {
		return 255;
	}
}

/**
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte
 * \param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_stack(Behaviour_t * caller) {
	bot_drive_stack_x(caller, get_waypos_index(), 0);
}

/**
 * Botenfunktion: Verhalten zum Anfahren aller in der FIFO-Queue befindlichen Punkte
 * \param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_fifo(Behaviour_t * caller) {
	bot_drive_stack_x(caller, get_waypos_index(), 1);
}

#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
/**
 * Schickt den Positionsspeicher per Bot-2-Bot-Kommunikation an einen anderen Bot
 * \param *caller Der Verhaltensdatensatz des Aufrufers
 * \param bot Adresse des Zielbots
 */
void bot_send_stack_b2b(Behaviour_t * caller, uint8_t bot) {
	struct {
		unsigned subresult:3;
	} result = {BEHAVIOUR_SUBFAIL};
	LOG_DEBUG("pos_store_send_to_bot(0x%" PRIxPTR ", %" PRIu8 ")", (uintptr_t) pos_store_from_beh(get_behaviour(bot_save_waypos_behaviour)), bot);
	if (pos_store_send_to_bot(pos_store_from_beh(get_behaviour(bot_save_waypos_behaviour)), bot) == 0) {
		if (bot_2_bot_start_remotecall(bot, "bot_drive_fifo", (remote_call_data_t) 0, (remote_call_data_t) 0, (remote_call_data_t) 0) == 0) {
			result.subresult = BEHAVIOUR_SUBSUCCESS;
		} else {
			LOG_DEBUG("Fehler, konnte bot_drive_fifo() nicht starten");
		}
	} else {
		LOG_DEBUG("Fehler, konnte Positionsspeicher nicht uebertragen");
	}
	if (caller) {
		caller->subResult = result.subresult;
		caller->active = BEHAVIOUR_ACTIVE;
	}
}
#endif // BOT_2_BOT_PAYLOAD_AVAILABLE

/**
 * Sichern der aktuellen Botposition auf den Stack
 * \param *caller	einfach nur Zeiger, damit remotecall verwendbar
 * \param stack		Nummer des Stacks, auf den gepusht werden soll
 */
void bot_push_actpos(Behaviour_t * caller, uint8_t stack) {
	position_t pos_;
	pos_.x = x_pos;
	pos_.y = y_pos;
	// sichern der aktuellen Botposition auf den Stack
	pos_store_push(pos_store_from_index(stack), pos_);
	if (caller) {
		caller->subResult = BEHAVIOUR_SUBSUCCESS;
	}
}

static uint8_t waypos_state = 0; 		/**< Status des drive_stack-Push-Verhaltens */
static position_t last_pos = { 0, 0 };	/**< letzte gemerkte Position */
static int16_t last_heading = 0;		/**< letzte gemerkte Botausrichtung */

#define DIST_FOR_PUSH 14400         /**< Quadrat des Abstandes [mm^2] zum letzten Punkt, ab dem gepush wird */
#define DIST_FOR_PUSH_TURN 3600     /**< Quadrat des Abstandes [mm^2] nach erreichen eines Drehwinkels zum letzten Punkt */
#define ANGLE_FOR_PUSH 20           /**< nach Erreichen dieses Drehwinkels [Grad] wird ebenfalls gepusht */

/**
 * Hilfsroutine zum Speichern der aktuellen Botposition in die Zwischenvariablen
 */
static void set_pos_to_last(void) {
	last_pos.x = x_pos;
	last_pos.y = y_pos;
	last_heading = (int16_t) heading;
}

/**
 * Speichern der uebergebenen Koordinaten auf dem Stack
 * \param pos_x	X-Koordinate
 * \param pos_y	Y-Koordinate
 */
static void bot_push_pos(int16_t pos_x, int16_t pos_y) {
	// sichern der Koordinaten in den Stack
	position_t pos_;
	pos_.x = pos_x;
	pos_.y = pos_y;
	pos_store_push(pos_store, pos_);
}

/**
 * Verhalten um sich entlang des Fahrweges relevante Koordinaten auf dem Stack zu merken; Verhalten ist nach Aktivierung via Botenroutine
 * ein Endlosverhalten und sammelt bis zur Deaktivierung die Wegepunkte; deaktiviert wird es via Notaus oder direkt mit Befehl zum Zurueckfahren und
 * damit Start des Stack-Fahrverhaltens
 * \param *data	Der Verhaltensdatensatz
 */
void bot_save_waypos_behaviour(Behaviour_t * data) {
	static uint8_t skip_count = 0;
	switch (waypos_state) {
	case 0:
		pos_store = pos_store_new_size(data, STACK_SIZE);
		if (pos_store == NULL) {
			exit_behaviour(data, BEHAVIOUR_SUBFAIL);
			return;
		}
		set_pos_to_last(); // aktuelle Botposition wird zum ersten Stackeintrag und merken der Position
		bot_push_pos(last_pos.x, last_pos.y);
		waypos_state = 1;
		break;
	case 1: {
		// Botpos wird in den Stack geschrieben zum Startzeitpunkt des Verhaltens (Stack noch leer) oder
		// wenn die Entfernung zur zuletzt gemerkten Koordinate gewissen Abstand ueberschreitet oder
		// wenn Entfernung noch nicht erreicht ist aber ein gewisser Drehwinkel erreicht wurde

		uint8_t update = False;
		// Abstand zur letzten Position ueberschritten
		if (get_dist(last_pos.x, last_pos.y, x_pos, y_pos) > DIST_FOR_PUSH) {
			// kein Push notwendig bei gerader Fahrt voraus zum Sparen des Stack-Speicherplatzes
			if ((int16_t) heading != last_heading) {
				update = True;
			}

			set_pos_to_last(); // Position jedenfalls merken
		}

		// bei Drehwinkelaenderung und Uberschreitung einer gewissen Groesse mit geringer Abstandsentfernung zum letzten Punkt kommt er in den Stack
		if (turned_angle(last_heading) > ANGLE_FOR_PUSH && get_dist(last_pos.x,	last_pos.y, x_pos, y_pos) > DIST_FOR_PUSH_TURN) {
			set_pos_to_last();
			update = True;
		}

		if (update) {
			LOG_DEBUG("Position (%d|%d) kommt in den Stack", x_pos, y_pos);
			position_t pos_0;
			pos_0.x = x_pos / 16;
			pos_0.y = y_pos / 16;
			if (optimized_push >= 1) {
				/* Positionen entfernen, die auf einer Linie liegen */
				position_t pos_1, pos_2;
				if (pos_store_top(pos_store, &pos_1, 1) == True && pos_store_top(pos_store, &pos_2, 2) == True) {
					pos_1.x /= 16;
					pos_1.y /= 16;
					if (abs(pos_1.x - pos_0.x) <= 1) {
						pos_0.x = pos_1.x;
					}
					pos_2.x /= 16;
					pos_2.y /= 16;
					if (abs(pos_2.x - pos_1.x) <= 1) {
						pos_2.x = pos_1.x;
					}
					int8_t m_1 = (int8_t) (pos_1.x == pos_2.x ? 100 : abs((pos_1.y - pos_2.y) / (pos_1.x - pos_2.x)));
					int8_t m = (int8_t) (pos_0.x == pos_1.x ? 100 : abs((pos_0.y - pos_1.y) / (pos_0.x - pos_1.x)));
					LOG_DEBUG(" pos_2=(%d|%d)", pos_2.x, pos_2.y);
					LOG_DEBUG(" pos_1=(%d|%d)", pos_1.x, pos_1.y);
					LOG_DEBUG(" pos  =(%d|%d)", pos_0.x, pos_0.y);
					LOG_DEBUG("  m_1=%3d\tm=%3d", m_1, m);
					LOG_DEBUG("  skip_count=%u", skip_count);
					if (abs(m_1 - m) < MAX_GRADIENT) {
						LOG_DEBUG("Neue Position auf einer Linie mit beiden Letzten");
						if (skip_count < 3) {
							LOG_DEBUG(" Verwerfe letzten Eintrag (%d|%d)", pos_1.x, pos_1.y);
							pos_store_pop(pos_store, &pos_1);
							skip_count++;
						} else {
							LOG_DEBUG(" Verwerfe Eintrag NICHT, skip_count=%u", skip_count);
							skip_count = 0;
						}
					} else {
						skip_count = 0;
					}
				}
				if (optimized_push >= 2) {
					/* Schleifen entfernen */
					uint8_t i;
					for (i=2; pos_store_top(pos_store, &pos_1, i); ++i) {
						int32_t diff = get_dist(x_pos, y_pos, pos_1.x, pos_1.y);
						LOG_DEBUG(" diff=%" PRId32, diff);
						if (diff < MAX_POS_DIFF) {
							LOG_DEBUG(" Position (%d|%d)@%u liegt in der Naehe", pos_1.x, pos_1.y, i);
							uint8_t k;
							for (k=i; k>1; --k) {
								pos_store_pop(pos_store, &pos_1); // die Positionen bis zur i-ten zurueck loeschen
								LOG_DEBUG(" Loesche (%d|%d) vom Stack", pos_1.x, pos_1.y);
							}
							i = 2;
						}
					}
				}
			}

			bot_push_pos(x_pos, y_pos);
#if defined PC && defined DEBUG
			pos_store_dump(pos_store);
#endif

#ifdef DEBUG
#ifdef MAP_2_SIM_AVAILABLE
			command_write(CMD_MAP, SUB_MAP_CLEAR_LINES, 0, 0, 0);
			position_t pos_1, pos_2;
			pos_store_top(pos_store, &pos_0, 1);
			uint16_t i;
			for (i = 2; pos_store_top(pos_store, &pos_1, (uint8_t) i); ++i) {
				map_draw_line_world(pos_0, pos_1, 0);
				pos_0 = pos_1;
				pos_1.x = pos_0.x - 16;
				pos_1.y = pos_0.y;
				pos_2.x = pos_0.x + 16;
				pos_2.y = pos_0.y;
				map_draw_line_world(pos_1, pos_2, 1);
				pos_1.x = pos_0.x;
				pos_1.y = pos_0.y - 16;
				pos_2.x = pos_0.x;
				pos_2.y = pos_0.y + 16;
				map_draw_line_world(pos_1, pos_2, 1);
			}
#endif // MAP_2_SIM_AVAILABLE
#endif // DEBUG
		}
		break;
	} // case 1
	} // switch
}

/**
 * Botenfunktion: Verhalten um sich entlang des Fahrweges relevante Koordinaten auf dem Stack zu merken
 * \param *caller	Der Verhaltensdatensatz des Aufrufers
 * \param optimize	Optimierungslevel (unnoetige Stackeintraege werden automatisch geloescht)
 */
void bot_save_waypos(Behaviour_t * caller, uint8_t optimize) {
	optimized_push = optimize;
	activateBehaviour(caller, bot_save_waypos_behaviour);
	drivestack_state = 0;
	waypos_state = 0;
	last_pos.x = 0;
	last_pos.y = 0;
	last_heading = 0;
}

/**
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
		/* Verhalten zum Speichern relevanter Wegepopsitionen zum spaeteren Zurueckfahren */
		RC5_Code = 0;
		bot_save_waypos(NULL, True);
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


/**
 * Display zum Setzen und Anfahren der Stackpunkte
 */
void drive_stack_display(void) {
	display_cursor(1, 1);
	display_printf("Stack   %5d %5d", pos.x, pos.y);
	display_cursor(2, 1);
	display_puts("Save/Del      : 3/8");
	display_cursor(3, 1);
	display_puts("GoBack/Forward: 4/7");
	display_cursor(4, 1);
	display_puts("Start WayPushPos: 5");

	drivestack_disp_key_handler(); // Aufruf des Key-Handlers
}
#endif // DISPLAY_DRIVE_STACK_AVAILABLE
#endif // BEHAVIOUR_DRIVE_STACK_AVAILABLE
