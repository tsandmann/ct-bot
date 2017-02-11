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

#ifndef INCLUDE_BOT_LOCAL_OVERRIDE_H_
#define INCLUDE_BOT_LOCAL_OVERRIDE_H_

/* Demo-Verhalten */
#define BEHAVIOUR_SIMPLE_AVAILABLE				/**< Beispielverhalten */
#define BEHAVIOUR_DRIVE_SQUARE_AVAILABLE 			/**< Demoverhalten im Quadrat fahren */

/* Notfall-Verhalten */
#define BEHAVIOUR_AVOID_BORDER_AVAILABLE 			/**< Abgruenden ausweichen */
#define BEHAVIOUR_AVOID_COL_AVAILABLE 			/**< Hindernis ausweichen */
#define BEHAVIOUR_HANG_ON_AVAILABLE 				/**< Erkennen des Haengenbleibens als Notfallverhalten */

/* Positionierungs-Verhalten */
#define BEHAVIOUR_TURN_AVAILABLE 					/**< Dreh Verhalten */
#define BEHAVIOUR_TURN_TEST_AVAILABLE 			/**< Test des Dreh Verhaltens */
#define BEHAVIOUR_GOTO_POS_AVAILABLE 				/**< Position anfahren */
#define BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE 		/**< Abstand zu Hindernis einhalten */
#define BEHAVIOUR_DRIVE_STACK_AVAILABLE 			/**< Abfahren der auf dem Stack gesicherten Koordinaten */
#define BEHAVIOUR_TEST_ENCODER_AVAILABLE 			/**< Encoder-Test Verhalten */

/* Anwendungs-Verhalten */
#define BEHAVIOUR_SOLVE_MAZE_AVAILABLE 				/**< Wandfolger */
#define BEHAVIOUR_FOLLOW_LINE_AVAILABLE			/**< Linienfolger */
#define BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE 	/**< erweiterter Linienfolger, der auch mit Unterbrechungen und Hindernissen klarkommt */
#define BEHAVIOUR_PATHPLANING_AVAILABLE 			/**< Pfadplanungsverhalten */
#define BEHAVIOUR_OLYMPIC_AVAILABLE				/**< Olympiadenverhalten */
#define BEHAVIOUR_CATCH_PILLAR_AVAILABLE 			/**< Suche eine Dose und fange sie ein */
#define BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE 		/**< Trennt zwei Arten von Dosen (hell / dunkel) */
#define BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE 		/**< Transport-Pillar Verhalten */
#define BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE 		/**< verfolge ein (bewegliches) Objekt */
#define BEHAVIOUR_FOLLOW_WALL_AVAILABLE 			/**< Follow Wall Explorer Verhalten */
#define BEHAVIOUR_DRIVE_AREA_AVAILABLE 			/**< flaechendeckendes Fahren mit Map */
#define BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE 	/**< Linienfolger ueber Kreuzungen zum Ziel */
#define BEHAVIOUR_DRIVE_CHESS_AVAILABLE 			/**< Schach fuer den Bot */
#define BEHAVIOUR_SCAN_BEACONS_AVAILABLE 			/**< Suchen von Landmarken zur Lokalisierung */
#define BEHAVIOUR_UBASIC_AVAILABLE 				/**< uBasic Verhalten */
#define BEHAVIOUR_ABL_AVAILABLE 					/**< ABL-Interpreter */
#define BEHAVIOUR_NEURALNET_AVAILABLE 			/**< neuronales Netzwerk */
#define BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE 		/**< Fahrverhalten fuer das neuronale Netzwerk */

/* Kalibrierungs-Verhalten */
#define BEHAVIOUR_CALIBRATE_PID_AVAILABLE			/**< Kalibrierungsverhalten fuer Motorregelung */
#define BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE 		/**< Kalibrierungsverhalten fuer Distanzsensoren */

/* System-Verhalten */
#define BEHAVIOUR_SCAN_AVAILABLE 					/**< Gegend scannen */
#define BEHAVIOUR_SERVO_AVAILABLE 					/**< Kontrollverhalten fuer die Servos */
#define BEHAVIOUR_REMOTECALL_AVAILABLE 				/**< Remote-Kommandos */
#define BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE 		/**< Distanzesensorasuwertung */
#define BEHAVIOUR_DELAY_AVAILABLE 					/**< Delay-Routine als Verhalten */
#define BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE 		/**< Deaktivieren von Verhalten, wenn eine Abbruchbedingung erfuellt ist */
#define BEHAVIOUR_GET_UTILIZATION_AVAILABLE		/**< CPU-Auslastung eines Verhaltens messen */
#define BEHAVIOUR_HW_TEST_AVAILABLE 				/**< Testverhalten (ehemals TEST_AVAILABLE_ANALOG, _DIGITAL, _MOTOR) */

#endif /* INCLUDE_BOT_LOCAL_OVERRIDE_H_ */
