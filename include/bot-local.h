 /*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	bot-local.h
 * @brief 	Konstanten, die den Bot an reale Umgebungen anpassen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author Christoph Grimmer (c.grimmer@futurio.de)
 * @date 	28.02.06
*/

#ifndef BOTLOCAL_H_
#define BOTLOCAL_H_

#include "ct-Bot.h"

#define ENCODER_MARKS		60		/*!< Anzahl der Flanken, die ein Encoder bei einer Radumdrehung liefert, also Anzahl der weissen + Anzahl der schwarzen Felder */
#define WHEEL_DIAMETER		57		/*!< Durchmesser eines Rades in mm */
#define WHEEL_PERIMETER		179 	/*!< Umfang eines Rades in mm */	
#define WHEEL_TO_WHEEL_DIAMETER 97 /*!< Abstand der beiden Raeder in mm */

#define DISTSENSOR_POS_FW	35	/*!< Abstand der Distanzsensoren von der Radachse (in fahrtrichtung)*/
#define DISTSENSOR_POS_SW	32	/*!< Abstand der Distanzsensoren von der Mittelachse (in querrichtung)*/

#define SPEED_CONTROL_INTERVAL		333		/*!> Intervall fuer die Motorregelung [ms]*/

//#define SPEED_TO_ENCODER_RATE		(WHEEL_PERIMETER /ENCODER_MARKS*(1000/SPEED_CONTROL_INTERVAL)) /*!< Faktor durch den man eine Speed [mm/s] teilt um Ticks/intervall zu erhalten */
#define SPEED_TO_ENCODER_RATE		9 /*!< Faktor durch den man eine Speed [mm/s] teilt um Ticks/intervall zu erhalten */

/* Odometrie-Konstanten */
//#define MOUSE_CPI			401		/*!< CPI-Wert aus Kalibrierung */
//#define MOUSE_FULL_TURN	1430	/*!< Mausaenderung in X-Richtung fuer einen vollen Kreis */
#define MOUSE_CPI			400		/*!< CPI-Wert aus Kalibrierung (Wert fuer den Sim) */
#define MOUSE_FULL_TURN		1484	/*!< Mausaenderung in X-Richtung fuer einen vollen Kreis (Wert fuer den Sim) */

#define WHEEL_DISTANCE		49		/*!< Abstand eines Rades zur Mitte des Bots */
#define STUCK_DIFF			100		/*!< ab welcher Differenz haben wir durchdrehende Raeder? */
#define G_SPEED			0.5		/*!< Kopplung Encoder- und Maussensor fuer Geschwindigkeiten (0.0=nur Radencoder, 1.0=nur Maussensor) */
#define G_POS				0.5		/*!< Kopplung Encoder- und Maussensor fuer Positionen und Winkel (0.0=nur Radencoder, 1.0=nur Maussensor) */

/*! Hilfskonstante */
#define ANGLE_CONSTANT		(WHEEL_TO_WHEEL_DIAMETER * ENCODER_MARKS / WHEEL_DIAMETER)

/* Motorregelung */
#define PID_LOW_RATE	(BOT_SPEED_MEDIUM / SPEED_TO_ENCODER_RATE)	/*!< Encoder-rate/aufruf, fuer den dieser PID-Satz gilt */
#define PID_LOW_Kp	8	/*!< Regelung PID-Parameter [Zehntel Schritte] */ 
#define PID_LOW_Ki	6	/*!< Regelung PID-Parameter [Zehntel Schritte] */ 
#define PID_LOW_Kd	0	/*!< Regelung PID-Parameter */ 

#define PID_HIGH_RATE	(BOT_SPEED_NORMAL / SPEED_TO_ENCODER_RATE) /*!< Encoder-rate/aufruf, fuer den dieser PID-Satz gilt */
#define PID_HIGH_Kp	70	/*!< Regelung PID-Parameter [Zehntel Schritte] */ 
#define PID_HIGH_Ki	40	/*!< Regelung PID-Parameter [Zehntel Schritte] */ 
#define PID_HIGH_Kd	0	/*!< Regelung PID-Parameter */ 

/* Einstellunge fuer die Verhaltensregeln */
#define BORDER_DANGEROUS	0x3A0	/*!< Wert, ab dem wir sicher sind, dass es eine Kante ist */


#define GLANCE_FACTOR 		0.9		/*!< Schlangenlinienfaktor zur Erweiterung des Sensorfeldes */
#define GLANCE_STRAIGHT	20		/*!< Anzahl der Zyklen, die nicht geschielt wird Gesamtzahl der Zyklen ist GLANCE_STRAIGHT + GLANCE_SIDE*4 */
#define GLANCE_SIDE 		5		/*!< Anzahl der Zyklen, die geschielt wird (jeweils pro Seite) Gesamtzahl der Zyklen ist GLANCE_STRAIGHT + GLANCE_SIDE*4 */



#define COL_CLOSEST		200		/*!< Abstand in mm, den wir als zu nah betrachten -- je nach echtem Sensor ist das schon zu nah! */
#define COL_NEAR			300		/*!< Nahbereich */
#define COL_FAR			400		/*!< Fernbereich */



#define SWEEP_STATE_TURN			0	/*!< Zustand: Drehung im Sweep. */
#define SWEEP_STATE_CHECK			1	/*!< Zustamd: Ueberpruefe Objekt vor dem Bot. */

/* Zustaende und Konstanten fuer das bot_solve_maze_behaviour-Verhalten */
#define BOT_DIAMETER				12
#define OPTIMAL_DISTANCE			BOT_DIAMETER*12	/* etwas mehr als Bot-Durchmesser ist ideal (vergroessert aufgrund der kennlinien der sharps) */
#define ADJUST_DISTANCE				10
#define IGNORE_DISTANCE				240
#define GROUND_GOAL					0x221
#define STARTPAD1					0x2B2
#define STARTPAD2					0x332


/* Konstanten fuer das bot_follow_line_behaviour-Verhalten */
#define LINE_SENSE					0x350	// Ab wann ist es Linie? (Fuer den Sim auf 350 setzen, helle Tischflaeche 50)








#endif /*BOTLOCAL_H_*/
