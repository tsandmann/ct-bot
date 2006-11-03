#ifndef AVAILABLE_BEHAVIOURS_H_
#define AVAILABLE_BEHAVIOURS_H_

#ifdef BEHAVIOUR_AVAILABLE

//#define BEHAVIOUR_SIMPLE_AVAILABLE	/*! sind die Beispielverhalten vorhanden ?*/
//#define BEHAVIOUR_DRIVE_SQUARE_AVAILABLE	/*! Demoverhalten im quadrat fahren vorhanden ?*/

#define BEHAVIOUR_AVOID_BORDER_AVAILABLE	/*! Abgruenden ausweichen vorhanden ?*/
#define BEHAVIOUR_AVOID_COL_AVAILABLE	/*! Hindernis ausweichen vorhanden ?*/
//#define BEHAVIOUR_GOTO_AVAILABLE	/*! goto vorhanden ?*/
#define BEHAVIOUR_GOTOXY_AVAILABLE	/*! gotoxy vorhanden ?*/
#define BEHAVIOUR_TURN_AVAILABLE	/*! turn vorhanden ?*/

#define BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE	/*! strecke fahren vorhanden ?*/

#define BEHAVIOUR_SCAN_AVAILABLE	/*! gegend scannen vorhanden ?*/
#define BEHAVIOUR_SOLVE_MAZE_AVAILABLE	/*! Wandfolger vorhanden ?*/
#define BEHAVIOUR_FOLLOW_LINE_AVAILABLE	/*! Linienfolger vorhanden ?*/

//#define BEHAVIOUR_OLYMPIC_AVAILABLE	/*! Olympiadenverhalten vorhanden ?*/

/* Aufgrund einer ganzen reihe von Abhaengigkeiten sollte man beim Versuch Speicher 
 * zu sparen, zuerst mal bei den Hauptverhalten ausmisten, sonst kommen die 
 * Unterverhalten durch die Hintertuer wieder rein
 */
#ifndef MAP_AVAILABLE
	#undef BEHAVIOUR_SCAN_AVAILABLE
#endif

#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
	#define BEHAVIOUR_TURN_AVAILABLE
#endif	

#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
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
#endif	

#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
	#define BEHAVIOUR_GOTO_AVAILABLE
#endif	


#include "bot-logic/behaviour_simple.h"
#include "bot-logic/behaviour_drive_square.h"

#include "bot-logic/behaviour_avoid_border.h"
#include "bot-logic/behaviour_avoid_col.h"

#include "bot-logic/behaviour_goto.h"
#include "bot-logic/behaviour_gotoxy.h"

#include "bot-logic/behaviour_turn.h"
#include "bot-logic/behaviour_drive_distance.h"

#include "bot-logic/behaviour_scan.h"


#include "bot-logic/behaviour_solve_maze.h"
#include "bot-logic/behaviour_follow_line.h"

#include "bot-logic/behaviour_olympic.h"


#endif
#endif /*AVAILABLE_BEHAVIOURS_H_*/
