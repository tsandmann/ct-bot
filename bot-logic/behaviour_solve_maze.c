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

/*! @file 	behaviour_solve_maze.c
 * @brief 	Wandfolger durchs Labyrinth
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#include "bot-logik.h"
#include <math.h>
#include <stdlib.h>

#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE

/* Parameter fuer das check_wall_behaviour() */
int8 wall_detected;				/*!< enthaelt True oder False, je nach Ergebnis des Verhaltens */
int8 check_direction;			/*!< enthaelt CHECK_WALL_LEFT oder CHECK_WALL_RIGHT */
int16 wall_distance;			/*!< enthaelt gemessene Entfernung */

/* Konstanten fuer check_wall_behaviour-Verhalten */
#define CHECK_WALL_RIGHT			0
#define CHECK_WALL_LEFT				1

/* Parameter fuer das measure_angle_behaviour() */
int8 measure_direction;			/*!< enthaelt MEASURE_RIGHT oder MEASURE_LEFT */
int16 measure_distance;			/*!< enthaelt maximale Messentfernung, enthaelt nach der Messung die Entfernung */
int16 measured_angle;			/*!< enthaelt gedrehten Winkel oder 0, falls nichts entdeckt */
#ifdef MEASURE_MOUSE_AVAILABLE
	int16 start_heading;		/*!< Blickwinkel des Bots zu Beginn der Messung */
#else
	int16 startEncL;				/*!< enthaelt Encoderstand zu Beginn der Messung */
	int16 startEncR;				/*!< enthaelt Encoderstand zu Beginn der Messung */
#endif
/* Konstanten fuer measure_angle_behaviour-Verhalten */
#define MEASURE_LEFT				1
#define MEASURE_RIGHT				-1


/*!
 * Das Verhalten dreht sich um 45 Grad in die angegebene Richtung (0=rechts, 1=links)
 * und prueft, ob auf dem Sensor auf der Seite der angegebenen Richtung eine Wand
 * im Abstand von 12cm zu sehen ist. Wenn dem so ist, wird die Variable wall_present
 * auf True gesetzt, sonst False */
void bot_check_wall_behaviour(Behaviour_t *data) {
	/* Konstantenfuer check_wall_behaviour-Verhalten */
	#define CORRECT_NEAR				0
	#define CORRECT_NONE				1
	#define CORRECT_FAR					2
	/* Zustaende fuer check_wall_behaviour-Verhalten */
	#define CHECK_WALL_TURN				0
	#define CHECK_WALL_SENS				1
	#define CHECK_WALL_TURN_BACK		2
	#define CHECK_WALL_FINISHED			3
	#define CHECK_WALL_CORRECT			4
	
	static int8 checkState=CHECK_WALL_TURN;
	/* wenn die Wand noch da ist aber aus dem Blickfeld rueckt, Entfernung und Winkel korrigieren */
	static int8 correctDistance=CORRECT_NONE;	
	/* letzte, gueltige Distanz fuer Abweichungsberechnung */
	static int16 lastDistance=0;
	/* enthaelt anzahl der +/-5 identischen Messungen */
	static int8 measureCount=0;
	/* letzter Messwert */
	static int16 lastSensor=0;
	
	int16 sensor;	/* fuer temporaer benutzte Senorwerte */

	switch(checkState) {
		case CHECK_WALL_TURN:
			/* haben wir links oder rechts eine Wand? */
			if (check_direction==CHECK_WALL_LEFT) {
				checkState=CHECK_WALL_SENS;
				bot_turn(data,45);
				break;	
			} else {
				checkState=CHECK_WALL_SENS;
				bot_turn(data,-45);
				break;
			}
			
		case CHECK_WALL_SENS:
			if (check_direction==CHECK_WALL_LEFT) {
				sensor=sensDistL;
			} else {
				sensor=sensDistR;
			}
			/* dafuer sorgen, dass wir nur verlaessliche Werte haben
			 * dazu muss der wert dreimal nacheinander max. um +/- 5
			 * unterschiedlich sein */
			 if (measureCount==0) {
			 	lastSensor=sensor;
			 	measureCount++;
			 	break;
			 }
			 if (sensor>=lastSensor-5 && sensor<=lastSensor+5 && measureCount<4) {
			 	/* Messwert ist ok */
			 	measureCount++;
			 	break;
			 } else  if (measureCount<4) {
			 	/* Messwert weicht zu doll ab -> von Neuem messen */
			 	measureCount=0;
			 	break;
			 }
			 /* ok, wir hatten drei Messungen mit nahezu identischen Werten */
			
			/* keine wand in eingestellter Maximalentfernung? */
			if (sensor>IGNORE_DISTANCE) {
				correctDistance=CORRECT_NONE; /* dann auch keine Korrektur */
				lastDistance=0;				  /* auch kein Vergleichswert */
				wall_detected=False;		  /* keine Wand da */
			} else if (sensor<OPTIMAL_DISTANCE-ADJUST_DISTANCE) {
				/* bot hat den falschen Abstand zur Wand, zu nah dran */
				wall_detected=True;
				correctDistance=CORRECT_NEAR;
			} else if (sensor>OPTIMAL_DISTANCE+ADJUST_DISTANCE) {
				/* bot hat den falschen Abstand zur Wand, zu weit weg */
				wall_detected=True;
				correctDistance=CORRECT_FAR;
			} else {
				/* perfekter Abstand */
				correctDistance=CORRECT_NONE;
				wall_detected=True;
			}
			wall_distance=sensor;
			/* Wenn Korrektur noetig, dann durchfuehren, sonst gleich zurueckdrehen */
			if (correctDistance==CORRECT_NONE) checkState=CHECK_WALL_TURN_BACK;
			else checkState=CHECK_WALL_CORRECT;
			break;

			
		case CHECK_WALL_TURN_BACK:
			checkState=CHECK_WALL_FINISHED;
			int16 turnAngle=45;
			
			/* welcher Sensorwert wird gebraucht? */
			if (check_direction==CHECK_WALL_LEFT) {
				sensor=sensDistL;
			} else {
				sensor=sensDistR;
			}
			/* wenn vorhanden, aus letztem Abstand und aktuellem Abstand
			 * Korrekturwinkel berechnen, falls vorheriger Abstand da.
			 * wird durch Abstand>IGNORE_DISTANCE zurueckgesetzt */
			if (lastDistance!=0) {
				turnAngle=turnAngle+(lastDistance-wall_distance)/10;
			}
			if (sensor<IGNORE_DISTANCE) lastDistance=sensor;
			if (check_direction==CHECK_WALL_LEFT) {
				bot_turn(data,-turnAngle);
			} else {
				bot_turn(data,turnAngle);
			}
			break;
		
		case CHECK_WALL_FINISHED:
			/* ok, check beendet -> Verhalten verlassen */
			checkState=CHECK_WALL_TURN;
			return_from_behaviour(data);
			break; 
			
		case CHECK_WALL_CORRECT:
			/* Abhaengig von der Seite, auf der die Wand ist, Entfernung korrigieren */
			if (check_direction==CHECK_WALL_LEFT) {
				sensor=sensDistL;
			} else {
				sensor=sensDistR;
			}
			if (sensor-OPTIMAL_DISTANCE<0) {
				speedWishLeft=-BOT_SPEED_SLOW;
				speedWishRight=-BOT_SPEED_SLOW;
			} else if (sensor-OPTIMAL_DISTANCE>0) {
				speedWishLeft=BOT_SPEED_SLOW;
				speedWishRight=BOT_SPEED_SLOW;
			}
			else {
				checkState=CHECK_WALL_TURN_BACK;
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
			}
			break;
			
			
		default:
			checkState=CHECK_WALL_TURN;
			return_from_behaviour(data);
			break;
	}
}
 

/*!
 * Das Verhalten dreht sich um 45 Grad in die angegebene Richtung (0=rechts, 1=links)
 * und prueft, ob auf dem Sensor auf der Seite der angegebenen Richtung eine Wand
 * im Abstand von 12-22cm zu sehen ist. Wenn dem so ist, wird die Variable wall_present
 * auf True gesetzt, sonst False */
void bot_check_wall(Behaviour_t *caller,int8 direction) {
	check_direction=direction;
	wall_detected=False;
	switch_to_behaviour(caller, bot_check_wall_behaviour,NOOVERRIDE);
}

#ifdef MEASURE_MOUSE_AVAILABLE
	/*!
	 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
	 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
	 * Distanz zum Bot hat.
	 */
	 
	void bot_measure_angle_behaviour(Behaviour_t *caller) {
		/* Zustaende measure_angle_behaviour-Verhalten */
		#define MEASURE_TURN				0
		#define FOUND_OBSTACLE				1
		#define MEASUREMENT_DONE			2
		
		static int8 measureState=MEASURE_TURN;
	
		/* enthaelt anzahl der +/-5 identischen Messungen */
		static int8 measureCount=0;
		/* letzter Messwert */
		static int16 lastSensor=0;
	
		/* bereits gedrehten Winkel */
		int16 turned_angle=0;
		if (measure_direction>0) {
			if ((int16)heading_mou<start_heading) {
				/* war ein ueberlauf */
				turned_angle=360-start_heading+(int16)heading_mou;
			} else {
				/* sonst normale differenz berechnen */
				turned_angle=(int16)heading_mou-start_heading;
			}
		} else {
			if ((int16)heading_mou>start_heading) {
				/* war ein ueberlauf */
				turned_angle=360-(int16)heading_mou+start_heading;
			} else {
				turned_angle=start_heading-(int16)heading_mou;
			}
			
		}
		
		/* sensorwert abhaengig von der Drehrichtung abnehmen */
		int16 sensor=(measure_direction==MEASURE_LEFT)?sensDistL:sensDistR;
		/* solange drehen, bis Hindernis innerhalb Messstrecke oder 360 Grad komplett */
		switch(measureState){
			case MEASURE_TURN:
				/* nicht mehr als eine komplette Drehung machen! */
				if (turned_angle>=360) {
					measure_direction=-measure_direction;
					measureState=MEASUREMENT_DONE;
					bot_turn(caller,measure_direction*turned_angle);
					measured_angle=0;		/* kein Hindernis gefunden */
					break;
				}
				/* Ist ein Objekt in Reichweite? */
				if (sensor<=measure_distance) {
					speedWishLeft=BOT_SPEED_STOP;
					speedWishRight=BOT_SPEED_STOP;
					measureState=FOUND_OBSTACLE;
					break;
				}
				/* Beginnen, zurueckzudrehen */
				speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_FOLLOW : BOT_SPEED_FOLLOW;
				speedWishRight = (measure_direction > 0) ? BOT_SPEED_FOLLOW : -BOT_SPEED_FOLLOW;
				break;
				
			case FOUND_OBSTACLE:
				 /* dafuer sorgen, dass wir nur verlaessliche Werte haben
				 * dazu muss der wert dreimal nacheinander max. um +/- 5
				 * unterschiedlich sein */
				 if (measureCount==0) {
				 	lastSensor=sensor;
				 	measureCount++;
				 	break;
				 }
				 if (sensor>=lastSensor-5 && sensor<=lastSensor+5 && measureCount<4) {
				 	/* Messwert ist ok */
				 	measureCount++;
				 	break;
				 } else  if (measureCount<4) {
				 	/* Messwert weicht zu doll ab -> von Neuem messen */
				 	measureCount=0;
				 	break;
				 }
				 /* ok, wir hatten drei Messungen mit nahezu identischen Werten */
				measure_distance=sensor;
				/* Hindernis gefunden, nun Bot wieder in Ausgangsstellung drehen */
				measure_direction=-measure_direction;
				measured_angle=turned_angle;
				measureState=MEASUREMENT_DONE;
				bot_turn(caller,measure_direction*turned_angle);
				break;
				
			case MEASUREMENT_DONE:
				measureState=MEASURE_TURN;
				measureCount=0;
				return_from_behaviour(caller);
				break;
		}	
	}
	
	/*!
	 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
	 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
	 * Distanz zum Bot hat.
	 */
	
	void bot_measure_angle(Behaviour_t *caller, int8 direction, int16 distance) {
		/* maximale Messentfernung und Richtung setzen */
		measure_direction=direction;
		measure_distance=distance;
		/* Heading zu Anfang des Verhaltens merken */
		start_heading=(int16)heading_mou;
		switch_to_behaviour(caller, bot_measure_angle_behaviour,NOOVERRIDE);	
	}
#else
/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 */
 
void bot_measure_angle_behaviour(Behaviour_t *caller) {
	/* Zustaende measure_angle_behaviour-Verhalten */
	#define MEASURE_TURN				0
	#define FOUND_OBSTACLE				1
	#define TURN_COMPLETED				2
	#define TURN_BACK					3
	#define CORRECT_ANGLE				4
	#define MEASUREMENT_DONE			5
	
	static int8 measureState=MEASURE_TURN;

	/* bereits gedrehte Strecke errechnen */
	int16 turnedLeft=(measure_direction>0)?-(sensEncL-startEncL):(sensEncL-startEncL);
	int16 turnedRight=(measure_direction>0)?(sensEncR-startEncR):-(sensEncR-startEncR);
	int16 turnedSteps=(abs(turnedLeft)+abs(turnedRight))/2;
	
	/* sensorwert abhaengig von der Drehrichtung abnehmen */
	int16 sensor=(measure_direction==MEASURE_LEFT)?sensDistL:sensDistR;
	/* solange drehen, bis Hindernis innerhalb Messstrecke oder 360� komplett */
	switch(measureState){
		case MEASURE_TURN:
			/* nicht mehr als eine komplette Drehung machen! */
			if (turnedSteps>=ANGLE_CONSTANT) {
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				measureState=TURN_COMPLETED;
				measured_angle=0;		/* kein Hindernis gefunden */
				break;
			}
			/* Ist ein Objekt in Reichweite? */
			if (sensor<=measure_distance) {
				measure_distance=sensor;
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				measureState=FOUND_OBSTACLE;
				break;
			}
			/* Beginnen, zurueckzudrehen */
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			break;
			
		case FOUND_OBSTACLE:
			/* Hindernis gefunden, nun Bot wieder in Ausgangsstellung drehen */
			measure_direction=-measure_direction;
			measured_angle=(int16)((long)(turnedSteps*360)/ANGLE_CONSTANT);
			measureState=TURN_BACK;
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			break;
			
		case TURN_COMPLETED:
			/* bot steht wieder in Ausgangsrichtung, Verhalten beenden */
			speedWishLeft=BOT_SPEED_STOP;
			speedWishRight=BOT_SPEED_STOP;
			measureState=CORRECT_ANGLE;
			break;
			
		case TURN_BACK:
			/* Bot in Ausgangsposition drehen */
			if ((turnedLeft+turnedRight)/2>=0) {
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				measureState=TURN_COMPLETED;
				break;
			}
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			break;
			
		case CORRECT_ANGLE:
			/* Evtl. etwas zuruecksetzen, falls wir zu weit gefahren sind */
			if (turnedRight>0) {
				/* rechts zu weit gefahren..langsam zurueck */
				speedWishRight = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			} else if (turnedRight<0) {
				/* rechts noch nicht weit genug...langsam vor */
				speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, rechtes Rad anhalten */
				speedWishRight = BOT_SPEED_STOP;
			}		
			if (turnedLeft<0) {
				/* links zu weit gefahren..langsam zurueck */
				speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;				
			} else if (turnedLeft>0) {
				/* links noch nicht weit genug...langsam vor */
				speedWishLeft = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, linkes Rad anhalten */
				speedWishLeft = BOT_SPEED_STOP;
			}
			if (speedWishLeft == BOT_SPEED_STOP && speedWishRight == BOT_SPEED_STOP) {
				/* beide Raeder haben nun wirklich die Endposition erreicht, daher anhalten */
				measureState=MEASUREMENT_DONE;
			}
			break;
			
		case MEASUREMENT_DONE:
			measureState=MEASURE_TURN;
			return_from_behaviour(caller);
			break;
	}	
}

/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 */

void bot_measure_angle(Behaviour_t *caller, int8 direction, int16 distance) {
	/* maximale Messentfernung und Richtung setzen */
	measure_direction=direction;
	measure_distance=distance;
	/* Encoderwerte zu Anfang des Verhaltens merken */
	startEncL=sensEncL;
	startEncR=sensEncR;
	switch_to_behaviour(caller, bot_measure_angle_behaviour,NOOVERRIDE);	
}
#endif
/*!
 * Das Verhalten findet seinen Weg durch ein Labyrinth, das nach gewissen Grundregeln gebaut ist
 * in nicht immer optimaler Weise aber in jedem Fall. Es arbeitet nach dem Hoehlenforscher-Algorithmus.
 * Einschraenkung: Objekte im Labyrinth, die Endlossschleifen verursachen koennen, z.b. ein einzeln
 * stehender Pfeiler im Labyrinth um den der Bot dann immer wieder herum fahren wuerde. 
 */
void bot_solve_maze_behaviour(Behaviour_t *data){
	/* Zustaende fuer das bot_solve_maze_behaviour-Verhalten */
	#define CHECK_FOR_STARTPAD			0
	#define CHECK_FOR_WALL_RIGHT		1
	#define CHECK_FOR_WALL_LEFT			2
	#define CHECK_WALL_PRESENT			3
	#define SOLVE_MAZE_LOOP				4
	#define SOLVE_TURN_WALL				5
	#define CHECK_CONDITION				6
	#define TURN_TO_BRANCH				7
	#define DETECTED_CROSS_BRANCH		8
	#define APPROACH_CORNER				9
	#define AVOID_ABYSS					10
	#define REACHED_GOAL				11
	static int8 mazeState=CHECK_FOR_STARTPAD;
	static int8 followWall=-1;
	static int8 checkedWalls=0;
	
	int16 distance;
	double x;
	switch(mazeState) {
		case CHECK_FOR_STARTPAD:
			/* Wo beginnen wir, nach einer Wand zu suchen? 
			 * Abgrund- und Kollisions-Verhalten ausschalten */
			#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
				deactivateBehaviour(bot_avoid_col_behaviour);
			#endif
			#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
				deactivateBehaviour(bot_avoid_border_behaviour);
			#endif
			/* sieht nach, ob der Bot auf einem definierten Startpad steht und
			 * beginnt dann mit der Suche gleich an der richtigen Wand */
			/* Zuserst bei nach Startpad1 gucken */
			checkedWalls=0;
			if ((sensLineL>=STARTPAD1-10 && sensLineL<=STARTPAD1+10) ||
				(sensLineR>=STARTPAD1-10 && sensLineR<=STARTPAD1+10)) {
				mazeState=CHECK_FOR_WALL_LEFT;
				break;
			}
			mazeState=CHECK_FOR_WALL_RIGHT;
			break;
			
		case CHECK_FOR_WALL_RIGHT:

			mazeState=CHECK_WALL_PRESENT;
			followWall=CHECK_WALL_RIGHT;
			bot_check_wall(data,followWall);
			break;
			
		case CHECK_FOR_WALL_LEFT:
		
			followWall=CHECK_WALL_LEFT;
			bot_check_wall(data,followWall);
			mazeState=CHECK_WALL_PRESENT;
			break;
			
		
		case CHECK_WALL_PRESENT:
			/* wenn keine Wand gefunden aber links noch nicht nachgesehen, andere 
			 * Richtung checken, sonst vorfahren */
			checkedWalls++;
			if (wall_detected==False){
				/* Wand noch nicht entdeckt...haben wir schon beide gecheckt? */
				if (checkedWalls<2) {
					if (followWall==CHECK_WALL_RIGHT) {
						mazeState=CHECK_FOR_WALL_LEFT;
						break;
					} else {
						mazeState=CHECK_FOR_WALL_RIGHT;
						break;
					}
				} else {
					/* keine wand? dann vorfahren und selbes prozedere nochmal */
					bot_drive_distance(data,0,BOT_SPEED_NORMAL,BOT_DIAMETER);
					mazeState=CHECK_FOR_WALL_RIGHT;
					checkedWalls=0;
					break;
				}
			}
			/* ok, wir haben unsere richtung im labyrinth gefunden jetzt dieser 
			 * nur noch folgen bis Ziel, Abzweig oder Abgrund */
			mazeState=SOLVE_MAZE_LOOP;
			break;
			
		case SOLVE_MAZE_LOOP:
			/* Einen Schritt (=halbe BOT-Groesse) vorwaerts */
			mazeState=SOLVE_TURN_WALL;
			bot_drive_distance(data,0,BOT_SPEED_NORMAL,BOT_DIAMETER);
			break;
			
		case SOLVE_TURN_WALL:
			/* Ziel erreicht? */
			if ((sensLineL>GROUND_GOAL-20 && sensLineL<GROUND_GOAL+20) || 
			    (sensLineR>GROUND_GOAL-20 && sensLineR<GROUND_GOAL+20)) {
				/* Bot hat Ziel erreicht...aus Freude einmal um die Achse drehen */
				bot_turn(data,360);
				mazeState=REACHED_GOAL;
				break;
			}
			/* checken, ob wand vor uns (Abstand 2.5*Bot-Durchmesser in mm) */
			distance=(sensDistL+sensDistR)/2;
			if (distance<=25*BOT_DIAMETER) { // umrechnen 10*BOT_DIAMETER, da letzteres in cm angegeben wird
				/* berechnete Entfernung zur Wand abzueglich optimale Distanz fahren */
				mazeState=DETECTED_CROSS_BRANCH;
				bot_drive_distance(data,0,BOT_SPEED_NORMAL,(distance-OPTIMAL_DISTANCE)/10);
				break;
			}
			/* Zur Wand drehen....ist die Wand noch da? */
			mazeState=CHECK_CONDITION;
			bot_check_wall(data,followWall);
			break;
			
		case CHECK_CONDITION:
			/* Solange weiter, wie die Wand zu sehen ist */
			if (wall_detected==True) {
				mazeState=SOLVE_MAZE_LOOP;
				break;
			}
			/* messen, wo genau die Ecke ist */
			mazeState=APPROACH_CORNER;
			if (followWall==CHECK_WALL_LEFT){
				bot_measure_angle(data,MEASURE_LEFT,300);
			} else {
				bot_measure_angle(data,MEASURE_RIGHT,300);
			} 
			break;

		case TURN_TO_BRANCH:
			/* nun in Richtung Abzweig drehen , dann mit Hauptschleife weiter*/
			mazeState=SOLVE_MAZE_LOOP;
			if (followWall==CHECK_WALL_RIGHT) {
				bot_turn(data,-90);
			} else {
				bot_turn(data,90);
			}
			break;
			
		case DETECTED_CROSS_BRANCH:
			/* Bot faehrt auf eine Ecke zu, in Richtung Gang drehen */
			if (followWall==CHECK_WALL_LEFT) {
				mazeState=SOLVE_MAZE_LOOP;
				bot_turn(data,-90);
			} else {
				mazeState=SOLVE_MAZE_LOOP;
				bot_turn(data,90);
			}
			break;
			
		case APPROACH_CORNER:
			/* ok, nun strecke bis zur Kante berechnen */
			x=measure_distance*cos(measured_angle*M_PI/180)/10+BOT_DIAMETER*1.5;
			mazeState=TURN_TO_BRANCH;
			bot_drive_distance(data,0,BOT_SPEED_NORMAL,(int16)x);
			break;
			
		case REACHED_GOAL:
			mazeState=CHECK_WALL_RIGHT;
			speedWishLeft=BOT_SPEED_STOP;
			speedWishRight=BOT_SPEED_STOP;
			return_from_behaviour(data);
			break;
	}
}


/*!
 * Das Verhalten findet seinen Weg durch ein Labyrinth, das nach gewissen Grundregeln gebaut ist
 * in nicht immer optimaler Weise aber in jedem Fall. Es arbeitet nach dem Hoehlenforscher-Algorithmus.
 * Einschraenkung: Objekte im Labyrinth, die Endlossschleifen verursachen koennen, z.b. ein einzeln
 * stehender Pfeiler im Labyrinth um den der Bot dann immer wieder herum fahren wuerde. 
 */
 
void bot_solve_maze(Behaviour_t *caller){
	switch_to_behaviour(caller, bot_solve_maze_behaviour,NOOVERRIDE);
}

/*!
 * Initialisiert den Hoelenforscher
 * @param prio_main Prioritaet des Hoehlenforschers (typ. 100)
 * @param prio_helper Prioritaet der Hilfsfunktionen (typ. 42)
 * @param active ACTIVE wenn der hoehlenforcher sofort starten soll, sonst INACTIVE
 */
void bot_solve_maze_init(int8 prio_main,int8 prio_helper, int8 active){
	// Verhalten, um ein Labyrinth nach der Hoehlenforscher-Methode loesen 
	insert_behaviour_to_list(&behaviour, new_behaviour(prio_main, bot_solve_maze_behaviour,active));
	
	insert_behaviour_to_list(&behaviour, new_behaviour(prio_helper--, bot_measure_angle_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(prio_helper, bot_check_wall_behaviour,INACTIVE));
	
}
#endif
