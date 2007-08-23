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
 * @file 	bot-local.h
 * @brief 	Konstanten, die den Bot an reale Umgebungen anpassen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Christoph Grimmer (c.grimmer@futurio.de)
 * @date 	28.02.06
 */

#ifndef BOTLOCAL_H_
#define BOTLOCAL_H_

#include "ct-Bot.h"

#define ENCODER_MARKS			60		/*!< Anzahl der Flanken, die ein Encoder bei einer Radumdrehung liefert, also Anzahl der weissen + Anzahl der schwarzen Felder */
#define WHEEL_DIAMETER			57		/*!< Durchmesser eines Rades in mm */
#define WHEEL_PERIMETER			179 	/*!< Umfang eines Rades in mm */	
#define WHEEL_TO_WHEEL_DIAMETER 97 		/*!< Abstand der beiden Raeder in mm */

#define DISTSENSOR_POS_FW	35			/*!< Abstand der Distanzsensoren von der Radachse (in fahrtrichtung)*/
#define DISTSENSOR_POS_SW	32			/*!< Abstand der Distanzsensoren von der Mittelachse (in querrichtung)*/
#define DISTSENSOR_POS_B_SW   (DISTSENSOR_POS_SW + 5) /*!< Abgrundsensoren 5 mm weiter aussen als Distsensoren*/

/* Parameter der Motorregelung */
#define PID_Kp				70				/*!< PID-Parameter proportional */
#define PID_Ki				10				/*!< PID-Parameter intergral */ 
#define PID_Kd				20				/*!< PID-Parameter differential */
#define PID_Ta				1				/*!< Abtastzeit */
#define PID_SHIFT			4				/*!< Rechtsshift der Stellgroessenkorrektur */
#define PID_TIME			215				/*!< max. Aufrufinterval [ms] */
#define PID_SPEED_THRESHOLD	BOT_SPEED_FOLLOW/*!< Grenzgeschwindigkeit, ab der die Regelgroesse interpoliert wird */
#define PWMMAX				511				/*!< Maximaler PWM-Wert */
#define PWMMIN				0				/*!< Minimaler PWM-Wert */
#define PWMSTART_L			100				/*!< Basis-PWM-Wert linker Motor (falls keine dauerhaft gespeicherte PWM-LT vorhanden ist) */
#define PWMSTART_R			100				/*!< Basis-PWM-Wert rechter Motor (falls keine dauerhaft gespeicherte PWM-LT vorhanden ist) */
#define PID_START_DELAY		20				/*!< Dauer der Anfahrverzoegerung */
#define ENC_CORRECT_L		5				/*!< Korrekturoffset fuer linken Radencoder */
#define ENC_CORRECT_R		5				/*!< Korrekturoffset fuer linken Radencoder */

/* Servo-Parameter */
#define DOOR_CLOSE 	7			/*!< Rechter Anschlag des Servos */
#define DOOR_OPEN	14			/*!< Linker Anschlag des Servos */

/* Odometrie-Konstanten */
#ifdef PC
	#define MOUSE_CPI		400		/*!< CPI-Wert aus Kalibrierung (Wert fuer den Sim) */
	#define MOUSE_FULL_TURN	1484	/*!< Mausaenderung in X-Richtung fuer einen vollen Kreis (Wert fuer den Sim) */
#else
	#define MOUSE_CPI		415		/*!< CPI-Wert aus Kalibrierung */
	#define MOUSE_FULL_TURN	1600	/*!< Mausaenderung in X-Richtung fuer einen vollen Kreis */
#endif

#define WHEEL_DISTANCE		48 		/*!< Abstand eines Rades zur Mitte des Bots */
#define STUCK_DIFF			100		/*!< ab welcher Differenz haben wir durchdrehende Raeder? */
#define G_SPEED				0.5		/*!< Kopplung Encoder- und Maussensor fuer Geschwindigkeiten (0.0=nur Radencoder, 1.0=nur Maussensor) */
#define G_POS				0.5		/*!< Kopplung Encoder- und Maussensor fuer Positionen und Winkel (0.0=nur Radencoder, 1.0=nur Maussensor) */

/*! Hilfskonstante */
#define ANGLE_CONSTANT		(WHEEL_TO_WHEEL_DIAMETER * ENCODER_MARKS / WHEEL_DIAMETER)

/* Einstellunge fuer die Verhaltensregeln */
#define BORDER_DANGEROUS	0x3A0	/*!< Wert, ab dem wir sicher sind, dass es eine Kante ist */


#define GLANCE_FACTOR 		0.9		/*!< Schlangenlinienfaktor zur Erweiterung des Sensorfeldes */
#define GLANCE_STRAIGHT		20		/*!< Anzahl der Zyklen, die nicht geschielt wird Gesamtzahl der Zyklen ist GLANCE_STRAIGHT + GLANCE_SIDE*4 */
#define GLANCE_SIDE 		5		/*!< Anzahl der Zyklen, die geschielt wird (jeweils pro Seite) Gesamtzahl der Zyklen ist GLANCE_STRAIGHT + GLANCE_SIDE*4 */



#define COL_CLOSEST		200		/*!< Abstand in mm, den wir als zu nah betrachten -- je nach echtem Sensor ist das schon zu nah! */
#define COL_NEAR		300		/*!< Nahbereich */
#define COL_FAR			400		/*!< Fernbereich */



#define SWEEP_STATE_TURN			0	/*!< Zustand: Drehung im Sweep. */
#define SWEEP_STATE_CHECK			1	/*!< Zustamd: Ueberpruefe Objekt vor dem Bot. */

/* Zustaende und Konstanten fuer das bot_solve_maze_behaviour-Verhalten */
#define BOT_DIAMETER				12				/*!< Bot-Durchmesser [cm] */
#define OPTIMAL_DISTANCE			BOT_DIAMETER*12	/*!< Optimale Distanz zur Wand [mm]. Etwas mehr als Bot-Durchmesser ist ideal (vergroessert aufgrund der kennlinien der sharps) */
#define ADJUST_DISTANCE				10				/*!< Toleranzbereich [mm] */
#define IGNORE_DISTANCE				240				/*!< Entfernung, ab der eine Wand ignoriert wird [mm] */
#define GROUND_GOAL					0x221			/*!< Farbe des Ziels */
#define STARTPAD1					0x2B2			/*!< Farbe des Startpads1 */
#define STARTPAD2					0x332			/*!< Fareb des Starpads2 */


/*! Konstante fuer das bot_follow_line_behaviour-Verhalten */
#define LINE_SENSE					0x350	// Ab wann ist es Linie? (Fuer den Sim auf 350 setzen, helle Tischflaeche 50)


/* Konstanten fuer Verhaltensanzeige, Verhalten mit prio von bis sichtbar */
#define PRIO_VISIBLE_MIN 3			/*!< Prioritaet, die ein Verhalten mindestens haben muss, um angezeigt zu werden */
#define PRIO_VISIBLE_MAX 154		/*!< Prioritaet, die ein Verhalten hoechstens haben darf, um angezeigt zu werden */


/* Konstanten fuer die Entfernung. innerhalb derer nur ein Objekt eingefangen wird */
#ifdef PC
  #define MAX_PILLAR_DISTANCE	350
#else
  #define MAX_PILLAR_DISTANCE	200
#endif


#endif /*BOTLOCAL_H_*/
