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
 
//TODO: Verhaltensanzeige wieder einbauen, aber NICHT HIER
 
#include "bot-logic/bot-logik.h"
#include "map.h"
#include "ir-rc5.h"
#include "display.h"
#include "motor-low.h"
#include "rc5-codes.h"
#include "ui/available_screens.h"

#include "mmc.h"
#include "mmc-vm.h"
#include <stdlib.h>

#ifdef RC5_AVAILABLE

uint16 RC5_Code = 0;	/*!< Letzter empfangener RC5-Code */


/* typedefs zunaechst noch mal aufbewahren, sind evtl. fuer spezielle key-handler sinnvoll */ 

///*! 
// * Dieser Typ definiert Parameter fuer RC5-Kommando-Code Funktionen.
// */
//typedef struct {
//	uint16 value1;	/*!< Wert 1 */
//	uint16 value2;	/*!< Wert 2 */
//} RemCtrlFuncPar;
//
///*! Dieser Typ definiert die RC5-Kommando-Code Funktion. */
//typedef void (*RemCtrlFunc)(RemCtrlFuncPar *par);
//
///*! Dieser Typ definiert den RC5-Kommando-Code und die auszufuehrende Funktion. */
//typedef struct {
//	uint16 command;		/*!< Kommando Code */
//	RemCtrlFunc func;	/*!< Auszufuehrende Funktion */
//	RemCtrlFuncPar par;	/*!< Parameter */
//} RemCtrlAction;


#ifdef DISPLAY_AVAILABLE
	/*!
	 * @brief			Setzt das Display auf eine andere Ausgabe.
	 * @param screen	Parameter mit dem zu setzenden Screen.
	 */	
	static void rc5_screen_set(uint8 screen){
		if (screen == DISPLAY_SCREEN_TOGGLE)
			display_screen++;			// zappen
		else
			display_screen = screen;	// Direktwahl

		if (display_screen >= DISPLAY_SCREENS)
			display_screen = 0;			// endliche Screenanzahl
		display_clear();				// alten Screen loeschen, das Zeichnen uerbernimmt GUI-Handler
	}
#endif	// DISPLAY_AVAILABLE

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
#ifdef BEHAVIOUR_AVAILABLE
	static void rc5_bot_change_speed(int16 left, int16 right) {
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
	}
#endif

/*!
 * @brief		Verarbeitet die Zifferntasten.
 * @param key	Parameter mit der betaetigten Zifferntaste
 */
static void rc5_number(uint8 key) {
	switch (key){	// richtige Aktion heraussuchen
		#ifdef BEHAVIOUR_AVAILABLE
			case 0:	target_speed_l=0;target_speed_r=0; break;
			case 1:	target_speed_l = BOT_SPEED_SLOW; target_speed_r = BOT_SPEED_SLOW; break;
			case 3: target_speed_l = BOT_SPEED_NORMAL; target_speed_r = BOT_SPEED_NORMAL; break;
		#endif	// BEHAVIOUR_AVAILABLE
		
		#ifdef BEHAVIOUR_TURN_AVAILABLE
			case 2: bot_turn(0, 90); break;
			case 7: bot_turn(0, 180); break;
			case 9: bot_turn(0, -180); break;
		#endif	// BEHAVIOUR_TURN_AVAILABLE							

		#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
			case 4: bot_catch_pillar(0); break;
		#endif
		
		#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
			case 5: bot_gotoxy(0, 20, 20);
		#endif
		#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
			case 5: bot_solve_maze(0); break;
		#endif

		#ifdef BEHAVIOUR_CALIBRATE_PWM_AVAILABLE
			case 6: bot_calibrate_pwm(0); break;
		#else 
			#ifdef BEHAVIOUR_TURN_AVAILABLE
				case 6: bot_turn(0, -90); break;
			#endif	// BEHAVIOUR_TURN_AVAILABLE
		#endif	// BEHAVIOUR_CALIBRATE_PWM_AVAILABLE
		
		#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
			case 8: bot_drive_distance(0, 0, BOT_SPEED_NORMAL, 10); break;
		#endif
	}
}

/*!
 * @brief	Ordnet den Tasten eine Aktion zu und fuehrt diese aus.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007	  
 */
void default_key_handler(void){
	switch (RC5_Code){	
		/* Not-Aus */
		case RC5_CODE_PWR:		rc5_emergency_stop(); break;
		
		/* Screenwechsel */
		#ifdef DISPLAY_AVAILABLE
		case RC5_CODE_GREEN:	rc5_screen_set(0); break;
		case RC5_CODE_RED:		rc5_screen_set(1); break;
		case RC5_CODE_YELLOW:	rc5_screen_set(2); break;
		case RC5_CODE_BLUE:		rc5_screen_set(3); break;
		case RC5_CODE_TV_VCR:	rc5_screen_set(DISPLAY_SCREEN_TOGGLE); break;
		#endif	// DISPLAY_AVAILABLE
		
		/* Geschwindigkeitsaenderung */
		#ifdef BEHAVIOUR_AVAILABLE
		case RC5_CODE_UP:		rc5_bot_change_speed( 10,  10); break;
		case RC5_CODE_DOWN:		rc5_bot_change_speed(-10, -10); break;
		case RC5_CODE_LEFT:		rc5_bot_change_speed(-10,  10); break;
		case RC5_CODE_RIGHT:	rc5_bot_change_speed( 10, -10); break;
		#endif	// BEHAVIOUR_AVAILABLE
		
		/* Servoaktivitaet */
		#ifdef BEHAVIOUR_SERVO_AVAILABLE
		case RC5_CH_PLUS:		bot_servo(0, SERVO1, DOOR_CLOSE); break;
		case RC5_CH_MINUS:		bot_servo(0, SERVO1, DOOR_OPEN); break;
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
		RC5_Code = rc5 & RC5_MASK;	// alle uninteressanten Bits ausblenden
		/* Toggle kommt nicht im Simulator, immer gewechseltes Toggle-Bit sicherstellen */ 
		#ifdef PC
		  RC5_Last_Toggle = !(rc5 & RC5_TOGGLE);
		#endif
		/* Bei Aenderung des Toggle-Bits, entspricht neuem Tastendruck, gehts nur weiter */
		if ((rc5 & RC5_TOGGLE) != RC5_Last_Toggle){	// Nur Toggle-Bit abfragen, bei Ungleichheit weiter
		  RC5_Last_Toggle = rc5 & RC5_TOGGLE;           // Toggle-Bit neu belegen
		}
	}
	#ifndef DISPLAY_AVAILABLE
		default_key_handler();	// Falls Display aus ist, ist auch GUI aus => Tastenbehandlung hier abarbeiten
	#endif
}



/* ------------------------------------------ Monsters here  ------------------------------------------ */

//TODO:	very strange diese Verhaltensanzeige... :/
//		das muss alles irgendwie in den Verhaltensanzeige-GUI-Handler und hier komplett verschwinden! 

//#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
///*! 
// * toggled ein Verhalten der Verhaltensliste an Position pos,
// * die Aenderung erfolgt nur auf die Puffervariable  
// * @param i Verhaltens-Listenposition, entspricht der Taste 1-6 der gewaehlten Verhaltensseite
// */	
//  void rc5_toggle_behaviour_new(int8 i) {
//	
//		  toggleNewBehaviourPos(i);	
//  } 
//  
///*!
// * Diese Funktion setzt die Aktivitaeten der Verhalten nach der Auswahl.
// * Hierdurch erfolgt der Startschuss fuer Umschaltung der Verhalten
// * nicht verwendet bei sofortiger Anzeige und Auswahl der Aktivitaet
// */	  
//  #ifndef DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
//   static void rc5_set_all_behaviours(void) {
//	   
//		  set_behaviours_active_to_new();
//	    
//   }
//  #endif
//#endif  

// TODO: Verhaltensanzeige
///*!
// * Diese Funktion wechselt zwiaschen verschiednen Verhalten
// * @param par Parameter mit den zu setzenden Geschwindigkeiten.
// */	
//void rc5_bot_next_behaviour(RemCtrlFuncPar *par) {
//
//	static uint8 state =0;
//
//	state++;
//	
//	switch (state) {
//		case 0: 
//			break;
//		#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
//			case 1: activateBehaviour(bot_drive_square_behaviour);
//				break;
//		#endif
//		case 2: 
//				 #ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
//						deactivateBehaviour(bot_drive_square_behaviour);
//				 #endif
//				 #ifdef BEHAVIOUR_GOTO_AVAILABLE
//					 deactivateBehaviour(bot_goto_behaviour); 
//				 #endif
//				 #ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
//					 activateBehaviour(bot_olympic_behaviour);
//				 #endif
//			break;
//		default:
//			#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
//				deactivateBehaviour( bot_drive_distance_behaviour);
//			#endif
//			#ifdef BEHAVIOUR_TURN_AVAILABLE
//				deactivateBehaviour( bot_turn_behaviour);
//			#endif
//			#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
//				deactivateBehaviour(bot_olympic_behaviour);
//				deactivateBehaviour( bot_explore_behaviour);
//				deactivateBehaviour( bot_do_slalom_behaviour);
//			#endif
//		
//			state=0;
//	}
//}

/* Funktionen, die es oben schon gibt, wo aber Verhaltensanzeige-Legacy-Murks drin war. -- Backup hier */

// static void rc5_screen_set(uint8 screen){
//	if (screen == DISPLAY_SCREEN_TOGGLE){
// TODO: what's that?!?	=> Sinnvollitaet pruefen...
//				
//		  #ifdef DISPLAY_BEHAVIOUR_AVAILABLE
//		     /* erst nachsehen, ob noch weitere Verhalten auf anderen Pages vorhanden sind */
//		     /* nur neuer Screen, wenn alle Verhalten angezeigt wurden */
//		     if ((display_screen == 2) && (another_behaviour_page())
//		     ) 
//		           behaviour_page++;
//		     else {
//		     	display_screen ++;
//		     	 
//			       if (display_screen == 1) {
//			         behaviour_page = 1;
//			          #ifndef DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
//			            set_behaviours_equal();
//			          #endif
//			       }
//			   
//		         }
//	    
//	    	#else
//		display_screen++;
//		    #endif     
//	      
//	    
//	} else{
// TODO: Verhaltensanzeige
//
//		  /* Screen direkt waehlen */	 
//		  #ifdef DISPLAY_BEHAVIOUR_AVAILABLE
//			
//			  /* Screen direkt waehlen und Verhaltens-Puffervariablen abgleichen*/
//			  display_screen = par->value1;
//			  
//			  // bei dyn. Anzeige und Auswahl keine Ubernahme in Puffervariable benoetigt
//			  #ifndef DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
//		        if ((display_screen == 2)&& (behaviour_page == 1)) {	      
//			       set_behaviours_equal();
//			     }  
//			  #endif
//			  behaviour_page = 1;
//			  
//		   #else
//		     /* Screen direkt waehlen */
//		display_screen = screen;
//		   #endif
//	}		
//	display_screen %= DISPLAY_SCREENS;
//	if (display_screen >= DISPLAY_SCREENS) display_screen = 0;
//	display_clear();
//}


//static void rc5_number(uint8 key) {
// TODO: wofuer ist das da?!? => in Behavior-Anzeige-Keyhandler verschieben
//	#ifdef DISPLAY_SCREENS_AVAILABLE 
//	switch (display_screen) {
//		#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
//			case 2:
//				switch (par->value1) {
//					case 0:	break;
//					case 1: rc5_toggle_behaviour_new(1); break;
//					case 2: rc5_toggle_behaviour_new(2); break;
//					case 3: rc5_toggle_behaviour_new(3); break;
//					case 4: rc5_toggle_behaviour_new(4); break;
//					case 5: rc5_toggle_behaviour_new(5); break;
//					case 6: rc5_toggle_behaviour_new(6); break;
//					case 7: break;
//					case 8: break;
//					case 9: 
//					       #ifndef DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
//					         rc5_set_all_behaviours(); 
//					       #endif
//					       break;
//				}
//				break;
//		#endif
//		default:
//	#endif
//}

#endif	// RC5_AVAILABLE
