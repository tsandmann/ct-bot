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
 * \file 	behaviour_drive_square.c
 * \brief 	Bot faehrt im Quadrat
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	03.11.2006
 */


#include "bot-logic.h"
#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE

#include <math.h>
#include "math_utils.h"
#include "log.h"

//#define DEBUG_SQUARE

#ifndef DEBUG_SQUARE
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif

#define STATE_FORWARD	0
#define STATE_TURN 		1
#define STATE_WAIT_1	2
#define STATE_WAIT_2	3
	

static uint8_t state = STATE_FORWARD;	/**< Status des Verhaltens */
static int16_t len = 0;					/**< Seitenlaenge des gefahrenen Quadrats in mm */
static int16_t start_head = 0;			/**< Blickrichtung beim Start des Verhaltens */
static position_t edges[4];				/**< Zielkoordinaten (Ecken des Quadrats) */

#ifndef BEHAVIOUR_GOTO_POS_AVAILABLE
void bot_drive_square_behaviour(Behaviour_t* data) {
	switch (state) {
	case STATE_FORWARD:
		/* Vorwaerts */
		LOG_DEBUG("bot_drive_distance(%d, %d)", BOT_SPEED_FOLLOW, len / 10);
		bot_drive_distance(data, 0, BOT_SPEED_FOLLOW, len / 10);
		state = STATE_WAIT_1;
		break;

	case STATE_WAIT_1:
		BLOCK_BEHAVIOUR(data, 500);
		state = STATE_TURN;
		break;

	case STATE_TURN:
		/* Drehen */
		LOG_DEBUG("bot_turn(%d)", 90);
		bot_turn(data, 90);
		state = STATE_WAIT_2;
		break;

	case STATE_WAIT_2:
		BLOCK_BEHAVIOUR(data, 500);
		state = STATE_FORWARD;
		break;

	default:
		/* Sind wir fertig, dann Kontrolle zurueck an Aufrufer */
		return_from_behaviour(data);
		break;
	}
}

#else // BEHAVIOUR_GOTO_POS_AVAILABLE

void bot_drive_square_behaviour(Behaviour_t* data) {
	switch (state) {
	case 0:
		LOG_DEBUG("bot_goto_pos(%d, %d, %d)", edges[0].x, edges[0].y, (start_head + 90) % 360);
		bot_goto_pos(data, edges[0].x, edges[0].y, (start_head + 90) % 360);
		state = 1;
		break;

	case 1:
		LOG_DEBUG("bot_goto_pos(%d, %d, %d)", edges[1].x, edges[1].y, (start_head + 180) % 360);
		bot_goto_pos(data, edges[1].x, edges[1].y, (start_head + 180) % 360);
		state = 2;
		break;

	case 2:
		LOG_DEBUG("bot_goto_pos(%d, %d, %d)", edges[2].x, edges[2].y, (start_head + 270) % 360);
		bot_goto_pos(data, edges[2].x, edges[2].y, (start_head + 270) % 360);
		state = 3;
		break;

	case 3:
		LOG_DEBUG("bot_goto_pos(%d, %d, %d)", edges[3].x, edges[3].y, start_head);
		bot_goto_pos(data, edges[3].x, edges[3].y, start_head);
		state = 0;
		break;

	default:
		return_from_behaviour(data);
		break;
	}
}
#endif // ! BEHAVIOUR_GOTO_POS_AVAILABLE

Behaviour_t* bot_drive_square_len(Behaviour_t* caller, int16_t length) {
	state = 0;
	len = length;
	start_head = heading_int;

	/* Ecken des Quadrats berechnen */
	edges[3].x = x_pos;
	edges[3].y = y_pos;
	edges[0].x = x_pos + (int16_t) (len * heading_cos);
	edges[0].y = y_pos + (int16_t) (len * heading_sin);
	edges[1].x = edges[0].x + (int16_t) (len * cosf(fmodf(heading + 90.f, 360.f) * DEG2RAD));
	edges[1].y = edges[0].y + (int16_t) (len * sinf(fmodf(heading + 90.f, 360.f) * DEG2RAD));
	edges[2].x = edges[1].x + (int16_t) (len * cosf(fmodf(heading + 180.f, 360.f) * DEG2RAD));
	edges[2].y = edges[1].y + (int16_t) (len * sinf(fmodf(heading + 180.f, 360.f) * DEG2RAD));

	return switch_to_behaviour(caller, bot_drive_square_behaviour, BEHAVIOUR_OVERRIDE);
}

#endif // BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
