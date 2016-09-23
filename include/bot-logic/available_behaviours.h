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
 * \file 	available_behaviours.h
 * \brief 	globale Schalter fuer die einzelnen Verhalten
 */

#ifndef AVAILABLE_BEHAVIOURS_H_
#define AVAILABLE_BEHAVIOURS_H_

#ifdef BEHAVIOUR_AVAILABLE

#define BEHAVIOUR_PROTOTYPE_AVAILABLE 			/**< Prototyp fuer neue Verhalten */

/* Demo-Verhalten */
#define BEHAVIOUR_SIMPLE_AVAILABLE				/**< Beispielverhalten */
#define BEHAVIOUR_DRIVE_SQUARE_AVAILABLE 			/**< Demoverhalten im Quadrat fahren */

/* Notfall-Verhalten */
//#define BEHAVIOUR_AVOID_BORDER_AVAILABLE 			/**< Abgruenden ausweichen */
//#define BEHAVIOUR_AVOID_COL_AVAILABLE 			/**< Hindernis ausweichen */
//#define BEHAVIOUR_HANG_ON_AVAILABLE 				/**< Erkennen des Haengenbleibens als Notfallverhalten */

/* Positionierungs-Verhalten */
//#define BEHAVIOUR_GOTO_AVAILABLE 					/**< Goto Verhalten*/
//#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE 		/**< Strecke fahren */
//#define BEHAVIOUR_GOTOXY_AVAILABLE 				/**< Punkt XY anfahren */
#define BEHAVIOUR_TURN_AVAILABLE 					/**< Dreh Verhalten */
//#define BEHAVIOUR_TURN_TEST_AVAILABLE 			/**< Test des Dreh Verhaltens */
#define BEHAVIOUR_GOTO_POS_AVAILABLE 				/**< Position anfahren */
//#define BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE 		/**< Abstand zu Hindernis einhalten */
//#define BEHAVIOUR_DRIVE_STACK_AVAILABLE 			/**< Abfahren der auf dem Stack gesicherten Koordinaten */
//#define BEHAVIOUR_TEST_ENCODER_AVAILABLE 			/**< Encoder-Test Verhalten */

/* Anwendungs-Verhalten */
#define BEHAVIOUR_SOLVE_MAZE_AVAILABLE 				/**< Wandfolger */
//#define BEHAVIOUR_FOLLOW_LINE_AVAILABLE			/**< Linienfolger */
//#define BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE 	/**< erweiterter Linienfolger, der auch mit Unterbrechungen und Hindernissen klarkommt */
//#define BEHAVIOUR_PATHPLANING_AVAILABLE 			/**< Pfadplanungsverhalten */
//#define BEHAVIOUR_OLYMPIC_AVAILABLE				/**< Olympiadenverhalten */
//#define BEHAVIOUR_CATCH_PILLAR_AVAILABLE 			/**< Suche eine Dose und fange sie ein */
//#define BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE 		/**< Trennt zwei Arten von Dosen (hell / dunkel) */
//#define BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE 		/**< Transport-Pillar Verhalten */
//#define BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE 		/**< verfolge ein (bewegliches) Objekt */
//#define BEHAVIOUR_FOLLOW_WALL_AVAILABLE 			/**< Follow Wall Explorer Verhalten */
//#define BEHAVIOUR_DRIVE_AREA_AVAILABLE 			/**< flaechendeckendes Fahren mit Map */
//#define BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE 	/**< Linienfolger ueber Kreuzungen zum Ziel */
//#define BEHAVIOUR_DRIVE_CHESS_AVAILABLE 			/**< Schach fuer den Bot */
//#define BEHAVIOUR_SCAN_BEACONS_AVAILABLE 			/**< Suchen von Landmarken zur Lokalisierung */
//#define BEHAVIOUR_UBASIC_AVAILABLE 				/**< uBasic Verhalten */
//#define BEHAVIOUR_ABL_AVAILABLE 					/**< ABL-Interpreter */
//#define BEHAVIOUR_NEURALNET_AVAILABLE 			/**< neuronales Netzwerk */
//#define BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE 		/**< Fahrverhalten fuer das neuronale Netzwerk */

/* Kalibrierungs-Verhalten */
//#define BEHAVIOUR_CALIBRATE_PID_AVAILABLE			/**< Kalibrierungsverhalten fuer Motorregelung */
//#define BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE 		/**< Kalibrierungsverhalten fuer Distanzsensoren */

/* System-Verhalten */
#define BEHAVIOUR_SCAN_AVAILABLE 					/**< Gegend scannen */
#define BEHAVIOUR_SERVO_AVAILABLE 					/**< Kontrollverhalten fuer die Servos */
#define BEHAVIOUR_REMOTECALL_AVAILABLE 				/**< Remote-Kommandos */
#define BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE 		/**< Distanzesensorasuwertung */
#define BEHAVIOUR_DELAY_AVAILABLE 					/**< Delay-Routine als Verhalten */
#define BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE 		/**< Deaktivieren von Verhalten, wenn eine Abbruchbedingung erfuellt ist */
//#define BEHAVIOUR_GET_UTILIZATION_AVAILABLE		/**< CPU-Auslastung eines Verhaltens messen */
//#define BEHAVIOUR_HW_TEST_AVAILABLE 				/**< Testverhalten (ehemals TEST_AVAILABLE_ANALOG, _DIGITAL, _MOTOR) */


/* Aufgrund einer ganzen Reihe von Abhaengigkeiten sollte man beim Versuch Speicher
 * zu sparen, zuerst mal bei den Hauptverhalten ausmisten, sonst kommen die
 * Unterverhalten durch die Hintertuer wieder rein */
#ifndef MAP_AVAILABLE
#undef BEHAVIOUR_SCAN_AVAILABLE
#undef BEHAVIOUR_DRIVE_AREA_AVAILABLE
#undef BEHAVIOUR_PATHPLANING_AVAILABLE
#endif // MAP_AVAILABLE

#ifdef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
#ifndef BEHAVIOUR_NEURALNET_AVAILABLE
#warning "NN-Fahrverhalten benoetigt BEHAVIOUR_NEURALNET_AVAILABLE (siehe available_behaviours.h)"
#undef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
#endif // BEHAVIOUR_NEURALNET_AVAILABLE
#endif // BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE


#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#ifndef BOT_FS_AVAILABLE
#warning "uBasic-Verhalten benoetigt BOT_FS_AVAILABLE (siehe ct-Bot.h)"
#undef BEHAVIOUR_UBASIC_AVAILABLE
#endif // BOT_FS_AVAILABLE
#endif // BEHAVIOUR_UBASIC_AVAILABLE

#ifdef BEHAVIOUR_ABL_AVAILABLE
#define BEHAVIOUR_REMOTECALL_AVAILABLE
#endif

#ifndef POS_STORE_AVAILABLE
#ifdef BEHAVIOUR_DRIVE_AREA_AVAILABLE
#warning "drive_area-Verhalten benoetigt POS_STORE_AVAILABLE (siehe ct-Bot.h)"
#undef BEHAVIOUR_DRIVE_AREA_AVAILABLE
#endif // !POS_STORE_AVAILABLE

#ifdef BEHAVIOUR_PATHPLANING_AVAILABLE
#warning "Pfadplanungs-Verhalten benoetigt POS_STORE_AVAILABLE (siehe ct-Bot.h)"
#undef BEHAVIOUR_PATHPLANING_AVAILABLE
#endif // BEHAVIOUR_PATHPLANING_AVAILABLE

#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
#warning "bot_line_shortest_way-Verhalten benoetigt POS_STORE_AVAILABLE (siehe ct-Bot.h)"
#undef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
#endif
#endif // BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE

#ifdef BEHAVIOUR_DRIVE_CHESS_AVAILABLE
#define BEHAVIOUR_GOTO_POS_AVAILABLE
#endif

#ifdef BEHAVIOUR_DRIVE_AREA_AVAILABLE
#define BEHAVIOUR_GOTO_POS_AVAILABLE
#define BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
#define BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
#endif // BEHAVIOUR_DRIVE_AREA_AVAILABLE

#ifdef BEHAVIOUR_PATHPLANING_AVAILABLE
#define BEHAVIOUR_DRIVE_STACK_AVAILABLE
#endif

#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
#define BEHAVIOUR_TURN_AVAILABLE
#endif

#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
#define BEHAVIOUR_GOTO_POS_AVAILABLE
#endif

#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
#define BEHAVIOUR_GOTO_POS_AVAILABLE
#define BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
#define BEHAVIOUR_FOLLOW_LINE_AVAILABLE
#endif // BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE

#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
#define BEHAVIOUR_TURN_AVAILABLE
#define BEHAVIOUR_FACTOR_WISH_AVAILABLE
#endif // BEHAVIOUR_AVOID_COL_AVAILABLE

#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
#define BEHAVIOUR_TURN_AVAILABLE
#endif

#ifndef POS_STORE_AVAILABLE
#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
#warning "DriveStack-Verhalten benoetigt POS_STORE_AVAILABLE (siehe ct-Bot.h)"
#endif
#undef BEHAVIOUR_DRIVE_STACK_AVAILABLE
#endif // !POS_STORE_AVAILABLE

#ifdef BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE
#define BEHAVIOUR_FOLLOW_LINE_AVAILABLE
#define BEHAVIOUR_GOTO_POS_AVAILABLE
#define BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
#define BEHAVIOUR_SOLVE_MAZE_AVAILABLE
#endif // BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE

#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#define BEHAVIOUR_TURN_AVAILABLE
#endif // BEHAVIOUR_FOLLOW_LINE_AVAILABLE

#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#define BEHAVIOUR_TURN_AVAILABLE
#endif // BEHAVIOUR_OLYMPIC_AVAILABLE

#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#endif

#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#define BEHAVIOUR_DELAY_AVAILABLE
#endif // BEHAVIOUR_SOLVE_MAZE_AVAILABLE

#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
#define BEHAVIOUR_TURN_AVAILABLE
#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#endif // BEHAVIOUR_DRIVE_SQUARE_AVAILABLE

#ifdef BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE
#define BEHAVIOUR_CATCH_PILLAR_AVAILABLE
#endif

#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
#define BEHAVIOUR_SERVO_AVAILABLE
#define BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
#endif // BEHAVIOUR_CATCH_PILLAR_AVAILABLE

#ifdef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
#define BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
#define BEHAVIOUR_FOLLOW_WALL_AVAILABLE
#endif // BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE

#ifndef MCU
#undef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
#undef BEHAVIOUR_GET_UTILIZATION_AVAILABLE
#endif // MCU

#ifdef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
#define SPEED_CONTROL_AVAILABLE		// Wenn die Regelung kalibriert werden soll, muss sie auch an sein!
#define DISPLAY_REGELUNG_AVAILABLE	// speichert Ist-Speed global
#define ADJUST_PID_PARAMS			// ja also die Parameter muessen schon einstellbar sein...
#endif // BEHAVIOUR_CALIBRATE_PID_AVAILABLE

#ifdef BEHAVIOUR_SERVO_AVAILABLE
#define BEHAVIOUR_DELAY_AVAILABLE
#endif

#ifdef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
#define BEHAVIOUR_GOTO_POS_AVAILABLE
#endif
#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
#define BEHAVIOUR_TURN_AVAILABLE
#endif

#ifdef BEHAVIOUR_TURN_AVAILABLE
#define BEHAVIOUR_DELAY_AVAILABLE
#endif

#ifndef MEASURE_MOUSE_AVAILABLE
#undef BEHAVIOUR_HANG_ON_AVAILABLE
#endif

#ifndef OS_AVAILABLE
#undef BEHAVIOUR_GET_UTILIZATION_AVAILABLE
#endif

#ifndef BPS_AVAILABLE
#undef BEHAVIOUR_SCAN_BEACONS_AVAILABLE
#endif

#ifdef MCU
#ifndef SPEED_CONTROL_AVAILABLE
// goto_pos geht nur, wenn wir uns auf die eingestellte Geschwindigkeit verlassen koennen
#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
#warning "GotoPos-Verhalten benoetigt SPEED_CONTROL_AVAILABLE (siehe ct-Bot.h)"
#endif
#undef BEHAVIOUR_GOTO_POS_AVAILABLE
#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
#warning "DriveStack-Verhalten benoetigt SPEED_CONTROL_AVAILABLE (siehe ct-Bot.h)"
#endif
#undef BEHAVIOUR_DRIVE_STACK_AVAILABLE
#undef BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE
#endif // SPEED_CONTROL_AVAILABLE
#endif // MCU

#ifndef BEHAVIOUR_GOTO_POS_AVAILABLE
#undef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
#undef BEHAVIOUR_DRIVE_AREA_AVAILABLE
#undef BEHAVIOUR_PATHPLANING_AVAILABLE
#endif // BEHAVIOUR_GOTO_POS_AVAILABLE

#include "bot-logic/behaviour_prototype.h"

#include "bot-logic/behaviour_hw_test.h"
#include "bot-logic/behaviour_simple.h"
#include "bot-logic/behaviour_drive_square.h"

#include "bot-logic/behaviour_avoid_border.h"
#include "bot-logic/behaviour_avoid_col.h"
#include "bot-logic/behaviour_hang_on.h"

#include "bot-logic/behaviour_goto.h"
#include "bot-logic/behaviour_gotoxy.h"
#include "bot-logic/behaviour_goto_pos.h"
#include "bot-logic/behaviour_goto_obstacle.h"

#include "bot-logic/behaviour_turn.h"
#include "bot-logic/behaviour_turn_test.h"
#include "bot-logic/behaviour_drive_distance.h"

#include "bot-logic/behaviour_measure_distance.h"

#include "bot-logic/behaviour_scan.h"

#include "bot-logic/behaviour_solve_maze.h"
#include "bot-logic/behaviour_follow_line.h"
#include "bot-logic/behaviour_follow_line_enhanced.h"

#include "bot-logic/behaviour_olympic.h"

#include "bot-logic/behaviour_servo.h"
#include "bot-logic/behaviour_catch_pillar.h"
#include "bot-logic/behaviour_classify_objects.h"

#include "bot-logic/behaviour_follow_object.h"

#include "bot-logic/behaviour_remotecall.h"

#include "bot-logic/behaviour_follow_wall.h"

#include "bot-logic/behaviour_calibrate_pid.h"
#include "bot-logic/behaviour_calibrate_sharps.h"

#include "bot-logic/behaviour_delay.h"

#include "bot-logic/behaviour_cancel_behaviour.h"
#include "bot-logic/behaviour_get_utilization.h"

#include "bot-logic/behaviour_transport_pillar.h"

#include "bot-logic/behaviour_drive_stack.h"

#include "bot-logic/behaviour_drive_area.h"

#include "bot-logic/behaviour_drive_chess.h"

#include "bot-logic/behaviour_pathplaning.h"

#include "bot-logic/behaviour_line_shortest_way.h"

#include "bot-logic/behaviour_scan_beacons.h"

#include "bot-logic/behaviour_test_encoder.h"

#include "bot-logic/behaviour_ubasic.h"

#include "behaviour_abl.h"

#include "behaviour_neuralnet.h"

#include "behaviour_drive_neuralnet.h"

#endif // BEHAVIOUR_AVAILABLE
#endif // AVAILABLE_BEHAVIOURS_H_
