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
 * @file 	behaviour_solve_maze.c
 * @brief 	Wandfolger durchs Labyrinth
 * @author 	Torsten Evers (tevers@onlinehome.de)
 * @date 	03.11.06
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
#include <math.h>
#include <stdlib.h>
#include "log.h"
#include "timer.h"
#include "math_utils.h"

/* Parameter fuer das check_wall_behaviour() */
static int8_t wall_detected = 0;	/*!< enthaelt True oder False, je nach Ergebnis des Verhaltens */
static int8_t check_direction = 0;	/*!< enthaelt CHECK_WALL_LEFT oder CHECK_WALL_RIGHT */
static int16_t wall_distance = 0;	/*!< enthaelt gemessene Entfernung */
static int8_t checkState = 0;		/*!< wenn die Wand noch da ist aber aus dem Blickfeld rueckt, Entfernung und Winkel korrigieren */


/* Konstanten fuer check_wall_behaviour-Verhalten */
#define CHECK_WALL_RIGHT			0
#define CHECK_WALL_LEFT				1

/* Parameter fuer das measure_angle_behaviour() */
static int8_t measure_direction;		/*!< enthaelt MEASURE_RIGHT oder MEASURE_LEFT */
static int16_t measure_distance;		/*!< enthaelt maximale Messentfernung, enthaelt nach der Messung die Entfernung */
static int16_t measured_angle;			/*!< enthaelt gedrehten Winkel oder 0, falls nichts entdeckt */
#ifdef MEASURE_MOUSE_AVAILABLE
static int16_t start_heading;		/*!< Blickwinkel des Bots zu Beginn der Messung */
#else
static int16_t startEncL;			/*!< enthaelt Encoderstand zu Beginn der Messung */
static int16_t startEncR;			/*!< enthaelt Encoderstand zu Beginn der Messung */
#endif
/* Konstanten fuer measure_angle_behaviour-Verhalten */
#define MEASURE_LEFT				1
#define MEASURE_RIGHT				-1


/*!
 * Das Verhalten dreht sich um 45 Grad in die angegebene Richtung (0=rechts, 1=links)
 * und prueft, ob auf dem Sensor auf der Seite der angegebenen Richtung eine Wand
 * im Abstand von 12cm zu sehen ist. Wenn dem so ist, wird die Variable wall_present
 * auf True gesetzt, sonst False
 * @param *data	Verhaltensdatensatz
 */
void bot_check_wall_behaviour(Behaviour_t * data) {
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

	static int8_t correctDistance = CORRECT_NONE;
	/* letzte, gueltige Distanz fuer Abweichungsberechnung */
	static int16_t lastDistance = 0;
	/* enthaelt anzahl der +/-5 identischen Messungen */
	static int8_t measureCount = 0;
	/* letzter Messwert */
	static int16_t lastSensor = 0;

	int16_t sensor;	/*!< fuer temporaer benutzte Senorwerte */

	switch(checkState) {
		case CHECK_WALL_TURN:
			/* haben wir links oder rechts eine Wand? */
			if (check_direction == CHECK_WALL_LEFT) {
				checkState = CHECK_WALL_SENS;
				bot_turn(data,45);
				break;
			} else {
				checkState = CHECK_WALL_SENS;
				bot_turn(data,-45);
				break;
			}

		case CHECK_WALL_SENS:
			if (check_direction == CHECK_WALL_LEFT) {
				sensor = sensDistL;
			} else {
				sensor = sensDistR;
			}
			/* dafuer sorgen, dass wir nur verlaessliche Werte haben
			 * dazu muss der wert dreimal nacheinander max. um +/- 5
			 * unterschiedlich sein */
			 if (measureCount == 0) {
			// 	old_ms=TIMER_GET_TICKCOUNT_16;
			 	lastSensor = sensor;
			 	measureCount++;
			 	bot_delay(data, 50);	// 50 ms nix tun
			 	break;
			 }
			 //if (!timer_ms_passed_16(&old_ms, 50)) break;	// 50 ms nix tun

			 if (sensor >= lastSensor - 5 && sensor <= lastSensor + 5 && measureCount < 4) {
			 	/* Messwert ist ok */
			 	measureCount++;
			 	bot_delay(data, 50);	// 50 ms nix tun
			 	break;
			 } else if (measureCount < 4) {
			 	/* Messwert weicht zu doll ab -> von Neuem messen */
			 	measureCount = 0;
			 	bot_delay(data, 50);	// 50 ms nix tun
			 	break;
			 }
			 //old_ms=TIMER_GET_TICKCOUNT_16;
			 measureCount = 0;
			 /* ok, wir hatten drei Messungen mit nahezu identischen Werten */
			/* keine wand in eingestellter Maximalentfernung? */
			if (sensor > IGNORE_DISTANCE) {
				correctDistance = CORRECT_NONE; /* dann auch keine Korrektur */
				lastDistance = 0;				  /* auch kein Vergleichswert */
				wall_detected = False;		  /* keine Wand da */
			} else if (sensor < OPTIMAL_DISTANCE - ADJUST_DISTANCE) {
				/* bot hat den falschen Abstand zur Wand, zu nah dran */
				wall_detected = True;
				correctDistance = CORRECT_NEAR;
			} else if (sensor > OPTIMAL_DISTANCE + ADJUST_DISTANCE) {
				/* bot hat den falschen Abstand zur Wand, zu weit weg */
				wall_detected = True;
				correctDistance = CORRECT_FAR;
			} else {
				/* perfekter Abstand */
				correctDistance = CORRECT_NONE;
				wall_detected = True;
			}
			wall_distance = sensor;
			/* Wenn Korrektur noetig, dann durchfuehren, sonst gleich zurueckdrehen */
			if (correctDistance == CORRECT_NONE) checkState = CHECK_WALL_TURN_BACK;
			else checkState = CHECK_WALL_CORRECT;
			break;


		case CHECK_WALL_TURN_BACK:
			checkState = CHECK_WALL_FINISHED;
			int16_t turnAngle = 45;

			/* welcher Sensorwert wird gebraucht? */
			if (check_direction == CHECK_WALL_LEFT) {
				sensor = sensDistL;
			} else {
				sensor = sensDistR;
			}
			/* wenn vorhanden, aus letztem Abstand und aktuellem Abstand
			 * Korrekturwinkel berechnen, falls vorheriger Abstand da.
			 * wird durch Abstand>IGNORE_DISTANCE zurueckgesetzt */
			if (lastDistance != 0) {
				turnAngle = turnAngle + (lastDistance - wall_distance) / 10;
			}
			if (sensor < IGNORE_DISTANCE) lastDistance=sensor;
			if (check_direction == CHECK_WALL_LEFT) {
				bot_turn(data, -turnAngle);
			} else {
				bot_turn(data, turnAngle);
			}
			break;

		case CHECK_WALL_FINISHED:
			/* ok, Check beendet -> Verhalten verlassen */
			checkState = CHECK_WALL_TURN;
			return_from_behaviour(data);
			break;

		case CHECK_WALL_CORRECT:
			/* Abhaengig von der Seite, auf der die Wand ist, Entfernung korrigieren */
			if (check_direction == CHECK_WALL_LEFT) {
				sensor = sensDistL;
			} else {
				sensor = sensDistR;
			}
			if (sensor - OPTIMAL_DISTANCE + ADJUST_DISTANCE < 0) {
				speedWishLeft = -BOT_SPEED_SLOW;
				speedWishRight = -BOT_SPEED_SLOW;
			} else if (sensor - OPTIMAL_DISTANCE - ADJUST_DISTANCE > 0) {
				speedWishLeft = BOT_SPEED_SLOW;
				speedWishRight = BOT_SPEED_SLOW;
			}
			else {
				checkState = CHECK_WALL_TURN_BACK;
				speedWishLeft = BOT_SPEED_STOP;
				speedWishRight = BOT_SPEED_STOP;
			}
			break;

		default:
			checkState = CHECK_WALL_TURN;
			return_from_behaviour(data);
			break;
	}
}


/*!
 * Das Verhalten dreht sich um 45 Grad in die angegebene Richtung (0=rechts, 1=links)
 * und prueft, ob auf dem Sensor auf der Seite der angegebenen Richtung eine Wand
 * im Abstand von 12-22cm zu sehen ist. Wenn dem so ist, wird die Variable wall_present
 * auf True gesetzt, sonst False
 * @param *caller	Verhaltensdatensatz des Aufrufers
 * @param direction	Richtung
 */
static void bot_check_wall(Behaviour_t * caller, int8_t direction) {
	check_direction = direction;
	wall_detected = False;
	checkState = CHECK_WALL_TURN;
	switch_to_behaviour(caller, bot_check_wall_behaviour, NOOVERRIDE);
}

static int8_t measureState = 0;	/*!< Zustand des measure_angle-Verhaltens*/

#ifdef MEASURE_MOUSE_AVAILABLE
static int8_t measureCount = 0;	/*!< enthaelt Anzahl der +/-5 identischen Messungen */

/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 * @param *data	Verhaltensdatensatz
 */
void bot_measure_angle_behaviour(Behaviour_t * data) {
	/* Zustaende measure_angle_behaviour-Verhalten */
	#define MEASURE_TURN				0
	#define FOUND_OBSTACLE				1
	#define MEASUREMENT_DONE			2

	/* letzter Messwert */
	static int16_t lastSensor=0;

	/* bereits gedrehten Winkel */
	int16_t turned_angle = 0;
	if (measure_direction > 0) {
		if ((int16_t) heading_mou < start_heading) {
			/* war ein Ueberlauf */
			turned_angle  =360 - start_heading + (int16_t) heading_mou;
		} else {
			/* sonst normale Differenz berechnen */
			turned_angle = (int16_t) heading_mou - start_heading;
		}
	} else {
		if ((int16_t) heading_mou > start_heading) {
			/* war ein Ueberlauf */
			turned_angle = 360 - (int16_t) heading_mou + start_heading;
		} else {
			turned_angle = start_heading - (int16_t) heading_mou;
		}
	}

	/* Sensorwert abhaengig von der Drehrichtung abnehmen */
	int16_t sensor = (measure_direction == MEASURE_LEFT) ? sensDistL : sensDistR;
	/* solange drehen, bis Hindernis innerhalb Messstrecke oder 360 Grad komplett */
	switch(measureState) {
		case MEASURE_TURN:
			/* nicht mehr als eine komplette Drehung machen! */
			if (turned_angle >= 360) {
				measure_direction = (int8_t) (-measure_direction);
				measureState = MEASUREMENT_DONE;
				bot_turn(data, measure_direction * turned_angle);
				measured_angle = 0; // kein Hindernis gefunden
				break;
			}
			/* Ist ein Objekt in Reichweite? */
			if (sensor <= measure_distance) {
				speedWishLeft = BOT_SPEED_STOP;
				speedWishRight = BOT_SPEED_STOP;
				measureState = FOUND_OBSTACLE;
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
			 if (measureCount == 0) {
				lastSensor = sensor;
				measureCount++;
				break;
			 }
			 if (sensor >= lastSensor - 5 && sensor <= lastSensor + 5 && measureCount < 4) {
				/* Messwert ist ok */
				measureCount++;
				break;
			 } else if (measureCount < 4) {
				/* Messwert weicht zu doll ab -> von Neuem messen */
				measureCount = 0;
				break;
			 }
			 /* ok, wir hatten drei Messungen mit nahezu identischen Werten */
			measure_distance = sensor;
			/* Hindernis gefunden, nun Bot wieder in Ausgangsstellung drehen */
			measure_direction = (int8_t) (-measure_direction);
			measured_angle = turned_angle;
			measureState = MEASUREMENT_DONE;
			bot_turn(data, measure_direction * turned_angle);
			break;

		case MEASUREMENT_DONE:
			return_from_behaviour(data);
			break;
	}
}

/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 * @param *caller	Verhaltensdatensatz des Aufrufers
 * @param direction	Richtung
 * @param distance	max. Distanz
 */
static void bot_measure_angle(Behaviour_t * caller, int8_t direction, int16_t distance) {
	/* maximale Messentfernung und Richtung setzen */
	measure_direction = direction;
	measure_distance = distance;
	/* Heading zu Anfang des Verhaltens merken */
	start_heading = (int16_t) heading_mou;
	measureState = MEASURE_TURN;
	measureCount = 0;
	switch_to_behaviour(caller, bot_measure_angle_behaviour, NOOVERRIDE);
}
#else
/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 * @param *data	Verhaltensdatensatz
 */
void bot_measure_angle_behaviour(Behaviour_t * data) {
	/* Zustaende measure_angle_behaviour-Verhalten */
	#define MEASURE_TURN				0
	#define FOUND_OBSTACLE				1
	#define TURN_COMPLETED				2
	#define TURN_BACK					3
	#define CORRECT_ANGLE				4
	#define MEASUREMENT_DONE			5

	/*! Hilfskonstante */
	#define ANGLE_CONSTANT		(WHEEL_TO_WHEEL_DIAMETER * ENCODER_MARKS / WHEEL_DIAMETER)

	/* bereits gedrehte Strecke errechnen */
	int16_t turnedLeft = (measure_direction > 0) ? -(sensEncL - startEncL) : (sensEncL - startEncL);
	int16_t turnedRight = (measure_direction > 0) ? (sensEncR - startEncR) : -(sensEncR - startEncR);
	int16_t turnedSteps = (abs(turnedLeft) + abs(turnedRight)) / 2;

	/* Sensorwert abhaengig von der Drehrichtung abnehmen */
	int16_t sensor = (measure_direction == MEASURE_LEFT) ? sensDistL : sensDistR;
	/* solange drehen, bis Hindernis innerhalb Messstrecke oder 360 Grad komplett */
	switch(measureState) {
		case MEASURE_TURN:
			/* nicht mehr als eine komplette Drehung machen! */
			if (turnedSteps >= ANGLE_CONSTANT) {
				speedWishLeft = BOT_SPEED_STOP;
				speedWishRight = BOT_SPEED_STOP;
				measureState = TURN_COMPLETED;
				measured_angle = 0; // kein Hindernis gefunden
				break;
			}
			/* Ist ein Objekt in Reichweite? */
			if (sensor <= measure_distance) {
				measure_distance=sensor;
				speedWishLeft = BOT_SPEED_STOP;
				speedWishRight = BOT_SPEED_STOP;
				measureState = FOUND_OBSTACLE;
				break;
			}
			/* Beginnen, zurueckzudrehen */
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			break;

		case FOUND_OBSTACLE:
			/* Hindernis gefunden, nun Bot wieder in Ausgangsstellung drehen */
			measure_direction = (int8_t) -measure_direction;
			measured_angle = (int16_t) ((float)(int32_t)(turnedSteps * 360) / ANGLE_CONSTANT);
			measureState = TURN_BACK;
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			break;

		case TURN_COMPLETED:
			/* Bot steht wieder in Ausgangsrichtung, Verhalten beenden */
			speedWishLeft = BOT_SPEED_STOP;
			speedWishRight = BOT_SPEED_STOP;
			measureState = CORRECT_ANGLE;
			break;

		case TURN_BACK:
			/* Bot in Ausgangsposition drehen */
			if ((turnedLeft + turnedRight) / 2 >= 0) {
				speedWishLeft = BOT_SPEED_STOP;
				speedWishRight = BOT_SPEED_STOP;
				measureState = TURN_COMPLETED;
				break;
			}
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			break;

		case CORRECT_ANGLE:
			/* Evtl. etwas zuruecksetzen, falls wir zu weit gefahren sind */
			if (turnedRight > 0) {
				/* rechts zu weit gefahren..langsam zurueck */
				speedWishRight = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			} else if (turnedRight < 0) {
				/* rechts noch nicht weit genug...langsam vor */
				speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, rechtes Rad anhalten */
				speedWishRight = BOT_SPEED_STOP;
			}
			if (turnedLeft < 0) {
				/* links zu weit gefahren..langsam zurueck */
				speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			} else if (turnedLeft > 0) {
				/* links noch nicht weit genug...langsam vor */
				speedWishLeft = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, linkes Rad anhalten */
				speedWishLeft = BOT_SPEED_STOP;
			}
			if (speedWishLeft == BOT_SPEED_STOP && speedWishRight == BOT_SPEED_STOP) {
				/* beide Raeder haben nun wirklich die Endposition erreicht, daher anhalten */
				measureState = MEASUREMENT_DONE;
			}
			break;

		case MEASUREMENT_DONE:
			return_from_behaviour(data);
			break;
	}
}

/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 * @param *caller	Verhaltensdatensatz des Aufrufers
 * @param direction	Richtung
 * @param distance	max. Distanz
 */
static void bot_measure_angle(Behaviour_t * caller, int8_t direction, int16_t distance) {
	/* maximale Messentfernung und Richtung setzen */
	measure_direction = direction;
	measure_distance = distance;
	/* Encoderwerte zu Anfang des Verhaltens merken */
	startEncL = sensEncL;
	startEncR = sensEncR;
	measureState = MEASURE_TURN;
	switch_to_behaviour(caller, bot_measure_angle_behaviour, NOOVERRIDE);
}
#endif	// MEASURE_MOUSE_AVAILABLE

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
static int8_t mazeState = 0;	/*!< Zustand des solve_maze-Verhaltens */

/*!
 * Das Verhalten findet seinen Weg durch ein Labyrinth, das nach gewissen Grundregeln gebaut ist
 * in nicht immer optimaler Weise aber in jedem Fall. Es arbeitet nach dem Hoehlenforscher-Algorithmus.
 * Einschraenkung: Objekte im Labyrinth, die Endlossschleifen verursachen koennen, z.b. ein einzeln
 * stehender Pfeiler im Labyrinth um den der Bot dann immer wieder herum fahren wuerde.
 * @param *data	Verhaltensdatensatz
 */
void bot_solve_maze_behaviour(Behaviour_t * data) {
	static int8_t followWall = -1;
	static int8_t checkedWalls = 0;

	int16_t distance;
	float x;
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
			checkedWalls = 0;
			if ((sensLineL >= STARTPAD1 - 10 && sensLineL <= STARTPAD1 + 10) ||
				(sensLineR >= STARTPAD1 - 10 && sensLineR <= STARTPAD1 + 10)) {
				mazeState=CHECK_FOR_WALL_LEFT;
				break;
			}
			mazeState = CHECK_FOR_WALL_RIGHT;
			break;

		case CHECK_FOR_WALL_RIGHT:
			mazeState = CHECK_WALL_PRESENT;
			followWall = CHECK_WALL_RIGHT;
			bot_check_wall(data, followWall);
			break;

		case CHECK_FOR_WALL_LEFT:
			followWall = CHECK_WALL_LEFT;
			bot_check_wall(data, followWall);
			mazeState = CHECK_WALL_PRESENT;
			break;

		case CHECK_WALL_PRESENT:
			/* wenn keine Wand gefunden aber links noch nicht nachgesehen, andere
			 * Richtung checken, sonst vorfahren */
			checkedWalls++;
			if (wall_detected == False){
				/* Wand noch nicht entdeckt...haben wir schon beide gecheckt? */
				if (checkedWalls < 2) {
					if (followWall == CHECK_WALL_RIGHT) {
						mazeState = CHECK_FOR_WALL_LEFT;
						break;
					} else {
						mazeState = CHECK_FOR_WALL_RIGHT;
						break;
					}
				} else {
					/* keine Wand? Dann vorfahren und selbes Prozedere nochmal */
					bot_drive_distance(data, 0, BOT_SPEED_NORMAL, BOT_DIAMETER / 10);
					mazeState = CHECK_FOR_WALL_RIGHT;
					checkedWalls = 0;
					break;
				}
			}
			/* ok, wir haben unsere Richtung im Labyrinth gefunden jetzt dieser
			 * nur noch folgen bis Ziel, Abzweig oder Abgrund */
			mazeState = SOLVE_MAZE_LOOP;
			break;

		case SOLVE_MAZE_LOOP:
			/* Einen Schritt (=halbe Bot-Groesse) vorwaerts */
			mazeState = SOLVE_TURN_WALL;
			bot_drive_distance(data, 0, BOT_SPEED_NORMAL, BOT_DIAMETER / 10);
			break;

		case SOLVE_TURN_WALL:
			/* Ziel erreicht? */
			if ((sensLineL > GROUND_GOAL - 20 && sensLineL < GROUND_GOAL + 20) ||
			    (sensLineR > GROUND_GOAL - 20 && sensLineR < GROUND_GOAL + 20)) {
				/* Bot hat Ziel erreicht...aus Freude einmal um die Achse drehen */
				bot_turn(data, 360);
				mazeState = REACHED_GOAL;
				break;
			}
			/* checken, ob wand vor uns (Abstand 2.5*Bot-Durchmesser in mm) */
			distance = (sensDistL + sensDistR) / 2;
			if (distance <= (int16_t) (2.5 * BOT_DIAMETER)) {
				/* berechnete Entfernung zur Wand abzueglich optimale Distanz fahren */
				mazeState = DETECTED_CROSS_BRANCH;
				bot_drive_distance(data, 0, BOT_SPEED_NORMAL, (distance - OPTIMAL_DISTANCE) / 10);
				break;
			}
			/* Zur Wand drehen....ist die Wand noch da? */
			mazeState = CHECK_CONDITION;
			bot_check_wall(data, followWall);
			break;

		case CHECK_CONDITION:
			/* Solange weiter, wie die Wand zu sehen ist */
			if (wall_detected == True) {
				mazeState = SOLVE_MAZE_LOOP;
				break;
			}
			/* messen, wo genau die Ecke ist */
			mazeState = APPROACH_CORNER;
			if (followWall == CHECK_WALL_LEFT){
				bot_measure_angle(data, MEASURE_LEFT, 300);
			} else {
				bot_measure_angle(data, MEASURE_RIGHT, 300);
			}
			break;

		case TURN_TO_BRANCH:
			/* nun in Richtung Abzweig drehen , dann mit Hauptschleife weiter */
			mazeState = SOLVE_MAZE_LOOP;
			if (followWall == CHECK_WALL_RIGHT) {
				bot_turn(data, -90);
			} else {
				bot_turn(data, 90);
			}
			break;

		case DETECTED_CROSS_BRANCH:
			/* Bot faehrt auf eine Ecke zu, in Richtung Gang drehen */
			if (followWall == CHECK_WALL_LEFT) {
				mazeState = SOLVE_MAZE_LOOP;
				bot_turn(data, -90);
			} else {
				mazeState = SOLVE_MAZE_LOOP;
				bot_turn(data, 90);
			}
			break;

		case APPROACH_CORNER:
			/* ok, nun Strecke bis zur Kante berechnen */
			x = measure_distance * cosf(rad(measured_angle)) / 10.0f + (BOT_DIAMETER * 0.15f);
			mazeState = TURN_TO_BRANCH;
			bot_drive_distance(data, 0, BOT_SPEED_NORMAL, (int16_t) x);
			break;

		case REACHED_GOAL:
			speedWishLeft = BOT_SPEED_STOP;
			speedWishRight = BOT_SPEED_STOP;
			return_from_behaviour(data);
			break;
	}
}

/*!
 * Das Verhalten findet seinen Weg durch ein Labyrinth, das nach gewissen Grundregeln gebaut ist
 * in nicht immer optimaler Weise aber in jedem Fall. Es arbeitet nach dem Hoehlenforscher-Algorithmus.
 * Einschraenkung: Objekte im Labyrinth, die Endlossschleifen verursachen koennen, z.b. ein einzeln
 * stehender Pfeiler im Labyrinth um den der Bot dann immer wieder herum fahren wuerde.
 * @param *caller	Verhaltensdatensatz des Aufrufers
 */
void bot_solve_maze(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_solve_maze_behaviour, NOOVERRIDE);
	mazeState = CHECK_FOR_STARTPAD;
}
#endif // BEHAVIOUR_SOLVE_MAZE_AVAILABLE
