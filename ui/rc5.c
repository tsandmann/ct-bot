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
 * @file 	rc5.c
 * @brief 	RC5-Fernbedienung / Basic-Tasten-Handler
 * Um RC5-Codes fuer eine eigene Fernbedienung anzupassen, reicht es diese
 * in eine Header-Datei auszulagern und anstatt der rc5code.h einzubinden.
 * Die Maskierung fuer die Auswertung der Codes nicht vergessen!
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007
 */
 
#include "bot-logic/bot-logik.h"
#include "map.h"
#include "ir-rc5.h"
#include "display.h"
#include "motor-low.h"
#include "rc5-codes.h"
#include "gui.h"
#include "ui/available_screens.h"

#include "mmc.h"
#include "mmc-vm.h"
#include <stdlib.h>

#ifdef RC5_AVAILABLE

uint16 RC5_Code = 0;	/*!< Letzter empfangener RC5-Code */

/*!
 * @brief			Setzt das Display auf eine andere Ausgabe.
 * @param screen	Parameter mit dem zu setzenden Screen.
 */	
static void rc5_screen_set(uint8 screen){
	#ifdef DISPLAY_AVAILABLE
		if (screen == DISPLAY_SCREEN_TOGGLE)
			display_screen++;			// zappen
		else
			display_screen = screen;	// Direktwahl

		if (display_screen >= max_screens)
			display_screen = 0;			// endliche Screenanzahl
		display_clear();				// alten Screen loeschen, das Zeichnen uerbernimmt GUI-Handler
	#endif	// DISPLAY_AVAILABLE
}

/*!
 * @brief	Stellt die Not-Aus-Funktion dar. 
 * Sie laesst den Bot anhalten und setzt alle Verhalten zurueck mit Sicherung der vorherigen Aktivitaeten.
 */	
 static void rc5_emergency_stop(void) {
	#ifdef BEHAVIOUR_AVAILABLE
		target_speed_l = 0;	// Geschwindigkeit nullsetzen
		target_speed_r = 0;
		deactivateAllBehaviours();  // alle Verhalten deaktivieren mit vorheriger Sicherung
	#endif
}
 
/*!
 * @brief		Aendert die Geschwindigkeit um den angegebenen Wert.
 * @param left	linke, relative Geschwindigkeitsaenderung
 * @param right	rechte, relative Geschwindigkeitsaenderung 
 */	
static void rc5_bot_change_speed(int16 left, int16 right) {
	#ifdef BEHAVIOUR_AVAILABLE
		int16 old;
		old = target_speed_l;
		target_speed_l += left;
		if ((target_speed_l < -BOT_SPEED_MAX) || (target_speed_l > BOT_SPEED_MAX))
			target_speed_l = old;
		if (target_speed_l < BOT_SPEED_SLOW && target_speed_l > 0)
			target_speed_l = BOT_SPEED_SLOW;
		else if (target_speed_l > -BOT_SPEED_SLOW && target_speed_l < 0)
			target_speed_l = -BOT_SPEED_SLOW;
			
		old = target_speed_r;		
		target_speed_r += right;
		if ((target_speed_r <-BOT_SPEED_MAX) ||(target_speed_r > BOT_SPEED_MAX))
			target_speed_r = old;
		if (target_speed_r < BOT_SPEED_SLOW && target_speed_r > 0)
			target_speed_r = BOT_SPEED_SLOW;
		else if (target_speed_r > -BOT_SPEED_SLOW && target_speed_r < 0)
			target_speed_r = -BOT_SPEED_SLOW;
	#endif	// BEHAVIOUR_AVAILABLE
}

/*!
 * @brief		Verarbeitet die Zifferntasten.
 * @param key	Parameter mit der betaetigten Zifferntaste
 */
static void rc5_number(uint8 key) {
	switch (key){	// richtige Aktion heraussuchen
		#ifdef BEHAVIOUR_AVAILABLE
			case 0:	target_speed_l = BOT_SPEED_STOP; target_speed_r = BOT_SPEED_STOP; break;
			case 1:	target_speed_l = BOT_SPEED_SLOW; target_speed_r = BOT_SPEED_SLOW; break;
			case 3: target_speed_l = BOT_SPEED_NORMAL; target_speed_r = BOT_SPEED_NORMAL; break;
		#endif	// BEHAVIOUR_AVAILABLE
		
		#ifdef BEHAVIOUR_TURN_AVAILABLE
			case 2: bot_turn(NULL, 90); break;
			case 7: bot_turn(NULL, 180); break;
			case 9: bot_turn(NULL, -180); break;
		#endif	// BEHAVIOUR_TURN_AVAILABLE							

		#if defined BEHAVIOUR_CATCH_PILLAR_AVAILABLE
			case 4: bot_catch_pillar(NULL); break;
		#elif defined BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE
			case 4: bot_follow_object(NULL); break;
		#endif	// BEHAVIOUR_CATCH_PILLAR_AVAILABLE

		#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
			case 5: bot_solve_maze(NULL); break;
		#endif	// BEHAVIOUR_SOLVE_MAZE_AVAILABLE
		
		#if defined BEHAVIOUR_CALIBRATE_PID_AVAILABLE
			case 6: bot_calibrate_pid(NULL, BOT_SPEED_SLOW); break;
		#elif defined BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
			case 6: bot_calibrate_sharps(NULL); break;
		#elif defined BEHAVIOUR_TURN_AVAILABLE
			case 6: bot_turn(NULL, -90); break;
		#endif	// BEHAVIOUR_CALIBRATE_PID_AVAILABLE
		
		#if defined PC && defined MAP_AVAILABLE
			case 8: map_print(); break;
		#else
			#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
				case 8: bot_drive_distance(NULL, 0, BOT_SPEED_NORMAL, 10); break;
			#endif
		#endif
	}
}

/*!
 * @brief	Ordnet den Tasten eine Aktion zu und fuehrt diese aus.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007
 * Hier gehoeren nur die absoluten Basics an Tastenzuordnungen rein, nicht Spezielles! Sonderwuensche
 * evtl. nach rc5_number(), ab besten aber eigenen Screenhandler anlegen und mit GUI/register_screen()
 * einhaengen.	  
 * Prinzip hier: Uebersichtlichkeit! :)
 */
void default_key_handler(void){
	switch (RC5_Code){	
		/* Not-Aus */
		case RC5_CODE_PWR:		rc5_emergency_stop(); break;
		
		/* Screenwechsel */
		case RC5_CODE_GREEN:	rc5_screen_set(0); break;
		case RC5_CODE_RED:		rc5_screen_set(1); break;
		case RC5_CODE_YELLOW:	rc5_screen_set(2); break;
		case RC5_CODE_BLUE:		rc5_screen_set(3); break;
		case RC5_CODE_TV_VCR:	rc5_screen_set(DISPLAY_SCREEN_TOGGLE); break;
		
		/* Geschwindigkeitsaenderung */
		case RC5_CODE_UP:		rc5_bot_change_speed( 10,  10); break;
		case RC5_CODE_DOWN:		rc5_bot_change_speed(-10, -10); break;
		case RC5_CODE_LEFT:		rc5_bot_change_speed(-10,  10); break;
		case RC5_CODE_RIGHT:	rc5_bot_change_speed( 10, -10); break;
		
		/* Servoaktivitaet */
		#ifdef BEHAVIOUR_SERVO_AVAILABLE
		case RC5_CH_PLUS:		bot_servo(0, SERVO1, DOOR_CLOSE); break;
		case RC5_CH_MINUS:		bot_servo(0, SERVO1, DOOR_OPEN);  break;
		#endif	// BEHAVIOUR_SERVO_AVAILABLE
		
		/* numerische Tasten */
		case RC5_CODE_0:		rc5_number(0); break;
		case RC5_CODE_1:		rc5_number(1); break;
		case RC5_CODE_2:		rc5_number(2); break;
		case RC5_CODE_3:		rc5_number(3); break;
		case RC5_CODE_4:		rc5_number(4); break;
		case RC5_CODE_5:		rc5_number(5); break;
		case RC5_CODE_6:		rc5_number(6); break;
		case RC5_CODE_7:		rc5_number(7); break;
		case RC5_CODE_8:		rc5_number(8); break;
		case RC5_CODE_9:		rc5_number(9); break;
	}
}

/*!
 * @brief	Liest ein RC5-Codeword und wertet es aus
 */
void rc5_control(void){
	static uint16 RC5_Last_Toggle = 1;	/*!< Toggle-Wert des zuletzt empfangenen RC5-Codes*/
	uint16 rc5 = ir_read();				// empfangenes RC5-Kommando
	
	if (rc5 != 0){
		/* Toggle kommt nicht im Simulator, immer gewechseltes Toggle-Bit sicherstellen */ 
		#ifdef PC
		  RC5_Last_Toggle = !(rc5 & RC5_TOGGLE);
		#endif
		/* Bei Aenderung des Toggle-Bits, entspricht neuem Tastendruck, gehts nur weiter */
		if ((rc5 & RC5_TOGGLE) != RC5_Last_Toggle){	// Nur Toggle-Bit abfragen, bei Ungleichheit weiter
		  RC5_Last_Toggle = rc5 & RC5_TOGGLE;           // Toggle-Bit neu belegen
		  RC5_Code = rc5 & RC5_MASK;	// alle uninteressanten Bits ausblenden
		}
	}
	#ifndef DISPLAY_AVAILABLE
		default_key_handler();	// Falls Display aus ist, ist auch GUI aus => Tastenbehandlung hier abarbeiten
	#endif
}

#endif	// RC5_AVAILABLE
