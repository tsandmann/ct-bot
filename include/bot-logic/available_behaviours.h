#ifndef AVAILABLE_BEHAVIOURS_H_
#define AVAILABLE_BEHAVIOURS_H_

#include "ct-Bot.h"

#ifdef BEHAVIOUR_AVAILABLE

/*! 
 * @file 	available_behaviours.h
 * @brief 	globale Schalter fuer die einzelnen Verhalten
 */

//#define BEHAVIOUR_SIMPLE_AVAILABLE	/*!< sind die Beispielverhalten vorhanden? */
//#define BEHAVIOUR_DRIVE_SQUARE_AVAILABLE	/*!< Demoverhalten im Quadrat fahren vorhanden? */

#define BEHAVIOUR_AVOID_BORDER_AVAILABLE	/*!< Abgruenden ausweichen vorhanden? */
#define BEHAVIOUR_AVOID_COL_AVAILABLE	/*!< Hindernis ausweichen vorhanden? */
//#define BEHAVIOUR_HANG_ON_AVAILABLE	/*!< Erkennen des Haengenbleibens als Notfallverhalten? */
//#define BEHAVIOUR_GOTO_AVAILABLE	/*!< goto vorhanden? */
//#define BEHAVIOUR_GOTOXY_AVAILABLE	/*!< gotoxy vorhanden? */
//#define BEHAVIOUR_GOTO_POS_AVAILABLE	/*!< goto_pos vorhanden? */
#define BEHAVIOUR_TURN_AVAILABLE	/*!< turn vorhanden? */
//#define BEHAVIOUR_TURN_TEST_AVAILABLE	/*!< turn_test vorhanden? */

#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE	/*!< Strecke fahren vorhanden ?*/

#define BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE	/*!< Distanzesensorasuwertung vorhanden? */

#define BEHAVIOUR_SCAN_AVAILABLE	/*!< Gegend scannen vorhanden? */
#define BEHAVIOUR_SOLVE_MAZE_AVAILABLE	/*!< Wandfolger vorhanden? */
//#define BEHAVIOUR_FOLLOW_LINE_AVAILABLE	/*!< Linienfolger vorhanden? */
//#define BEHAVIOUR_MAP_GO_DESTINATION_AVAILABLE /*!< Fahren zu einem Ziel nach Pfadplanung */

#define BEHAVIOUR_SERVO_AVAILABLE 	/*!< Kontrollverhalten fuer die Servos */

//#define BEHAVIOUR_OLYMPIC_AVAILABLE	/*!< Olympiadenverhalten vorhanden? */

//#define BEHAVIOUR_CATCH_PILLAR_AVAILABLE /*!< Suche eine Dose und fange sie ein */

//#define BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE	/*!< verfolge ein (bewegliches) Objekt */

//#define BEHAVIOUR_FOLLOW_WALL_AVAILABLE /*!< Follow Wall Explorer Verhalten */

#define BEHAVIOUR_REMOTECALL_AVAILABLE /*!< Nehmen wir Remote-kommandos entgegen? */

//#define BEHAVIOUR_CALIBRATE_PID_AVAILABLE	/*!< Kalibrierungsverhalten fuer Motorregelung vorhanden? */
//#define BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE	/*!< Kalibrierungsverhalten fuer Distanzsensoren vorhanden? */

#define BEHAVIOUR_DELAY_AVAILABLE /*!< Delay-Routinen als Verhalten */

/* Aufgrund einer ganzen reihe von Abhaengigkeiten sollte man beim Versuch Speicher 
 * zu sparen, zuerst mal bei den Hauptverhalten ausmisten, sonst kommen die 
 * Unterverhalten durch die Hintertuer wieder rein
 */
#ifndef MAP_AVAILABLE
	#undef BEHAVIOUR_SCAN_AVAILABLE
	#undef BEHAVIOUR_MAP_GO_DESTINATION_AVAILABLE
#endif

#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
	#define BEHAVIOUR_TURN_AVAILABLE
#endif	

#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
	#define BEHAVIOUR_TURN_AVAILABLE
#endif	

#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
	#define BEHAVIOUR_TURN_AVAILABLE
#endif

#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
	#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
	#define BEHAVIOUR_TURN_AVAILABLE
#endif	

#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
	#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
	#define BEHAVIOUR_TURN_AVAILABLE
#endif	

#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
	#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#endif	

#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
	#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
	#define BEHAVIOUR_DELAY_AVAILABLE
#endif	

#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
	#define BEHAVIOUR_TURN_AVAILABLE
	#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
#endif	

#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
	#define BEHAVIOUR_SERVO_AVAILABLE
#endif

#ifndef MCU
	#undef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
#endif

#ifdef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
	#define SPEED_CONTROL_AVAILABLE			// Wenn die Regelung kalibriert werden soll, muss sie auch an sein!
	#define DISPLAY_REGELUNG_AVAILABLE		// speichert Ist-Speed global
	#define ADJUST_PID_PARAMS				// ja also die Parameter muessen schon einstellbar sein...
#endif

#ifndef BEHAVIOUR_MAP_GO_DESTINATION_AVAILABLE
	#undef DISPLAY_MAP_GO_DESTINATION
#endif

#ifdef BEHAVIOUR_SERVO_AVAILABLE
	#define BEHAVIOUR_DELAY_AVAILABLE
#endif

#ifdef BEHAVIOUR_TURN_AVAILABLE
	#define BEHAVIOUR_DELAY_AVAILABLE
#endif

#ifndef MEASURE_MOUSE_AVAILABLE
	#undef BEHAVIOUR_HANG_ON_AVAILABLE
#endif

#ifdef MCU
#ifndef SPEED_CONTROL_AVAILABLE
	// goto_pos geht nur, wenn wir uns auf die eingestellte Geschwindigkeit verlassen koennen
	#undef BEHAVIOUR_GOTO_POS_AVAILABLE
#endif
#endif

#include "bot-logic/behaviour_simple.h"
#include "bot-logic/behaviour_drive_square.h"

#include "bot-logic/behaviour_avoid_border.h"
#include "bot-logic/behaviour_avoid_col.h"
#include "bot-logic/behaviour_hang_on.h"

#include "bot-logic/behaviour_goto.h"
#include "bot-logic/behaviour_gotoxy.h"
#include "bot-logic/behaviour_goto_pos.h"

#include "bot-logic/behaviour_turn.h"
#include "bot-logic/behaviour_turn_test.h"
#include "bot-logic/behaviour_drive_distance.h"

#include "bot-logic/behaviour_measure_distance.h"

#include "bot-logic/behaviour_scan.h"

#include "bot-logic/behaviour_map_go_destination.h"

#include "bot-logic/behaviour_solve_maze.h"
#include "bot-logic/behaviour_follow_line.h"

#include "bot-logic/behaviour_olympic.h"

#include "bot-logic/behaviour_servo.h"
#include "bot-logic/behaviour_catch_pillar.h"

#include "bot-logic/behaviour_follow_object.h"

#include "bot-logic/remote_calls.h"

#include "bot-logic/behaviour_follow_wall.h"

#include "bot-logic/behaviour_calibrate_pid.h"
#include "bot-logic/behaviour_calibrate_sharps.h"

#include "bot-logic/behaviour_delay.h"

#endif	// BEHAVIOUR_AVAILABLE
#endif	/*AVAILABLE_BEHAVIOURS_H_*/
