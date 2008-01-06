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
#include <stdlib.h>

static uint8_t drivestack_state = 0;	/*!< Status des drive_stack-Verhaltens */

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
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_stack(Behaviour_t * caller) {
	switch_to_behaviour(caller,bot_drive_stack_behaviour,OVERRIDE);
	drivestack_state = 0;	
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
	caller->subResult = SUBSUCCESS;
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

	} // switch
} // Ende Keyhandler


/*!
 * Display zum Setzen und Anfahren der Stackpunkte 
 */
void drive_stack_display(void) {
	display_cursor(1,1);
	display_printf("Bot-Pos %5d %5d", (int16_t)x_pos, (int16_t)y_pos);
	display_cursor(2,1);
	display_printf("Stack   %5d %5d", posx, posy);
	display_cursor(4,1);
	display_printf("Pos Save/Goto: 3/4");	

	drivestack_disp_key_handler();		  // aufrufen des Key-Handlers
}
#endif	// DISPLAY_DRIVE_STACK_AVAILABLE
#endif	// BEHAVIOUR_DRIVE_STACK_AVAILABLE
