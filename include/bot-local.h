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
#define WHEEL_PERIMETER		179 	/*!< Durchmesser eines Rades in mm */	
#define WHEEL_TO_WHEEL_DIAMETER 97 /*!< Abstand der beiden Raeder in mm */

/* Einstellunge fuer die Verhaltensregeln */
#define BORDER_DANGEROUS	0x340	/*!< Wert, ab dem wir sicher sind, dass es eine Kante ist */

#define COL_CLOSEST		100		/*!< Abstand in mm, den wir als zu nah betrachten */
#define COL_NEAR			300		/*!< Nahbereich */
#define COL_FAR			400		/*!< Fernbereich */

#define ZONE_CLOSEST	0			/*!< Zone fuer extremen Nahbereich */
#define ZONE_NEAR		1			/*!< Zone fuer Nahbereich */
#define ZONE_FAR		2			/*!< Zone fuer Fernbereich */
#define ZONE_CLEAR		3			/*!< Zone fuer Freien Bereich */

#define BRAKE_CLOSEST 	-1.0		/*!< Bremsfaktor fuer extremen Nahbereich ( <1 ==> bremsen <0 ==> rueckwaerts) */
#define BRAKE_NEAR		0.6 		/*!< Bremsfaktor fuer Nahbereich ( <1 ==> bremsen <0 ==> rueckwaerts) */
#define BRAKE_FAR		0.8			/*!< Bremsfaktor fuer Fernbereich ( <1 ==> bremsen <0 ==> rueckwaerts) */

#define GLANCE_FACTOR 		0.9		/*!< Schlangenlinienfaktor zur Erweiterung des Sensorfeldes */
#define GLANCE_STRAIGHT	20		/*!< Anzahl der Zyklen, die nicht geschielt wird Gesamtzahl der Zyklen ist GLANCE_STRAIGHT + GLANCE_SIDE*4 */
#define GLANCE_SIDE 		5		/*!< Anzahl der Zyklen, die geschielt wird (jeweils pro Seite) Gesamtzahl der Zyklen ist GLANCE_STRAIGHT + GLANCE_SIDE*4 */

#define MOT_GOTO_MAX  	 20 			/*!< Richtungsaenderungen, bis goto erreicht sein muss */
#define GOTO_REACHED	 2			/*!< Wenn Encoder-Distanz <= GOTO_REACHED dann stop */
#define GOTO_SLOW		 4			/*!< Wenn Encoder-Distanz < GOTO_SLOW dann langsame Fahrt */
#define GOTO_NORMAL	10			/*!< Wenn Encoder-Distanz < GOTO_NORMAL dann normale Fahrt */
#define GOTO_FAST		40			/*!< Wenn Encoder-Distanz < GOTO_FAST dann schnelle Fahrt, sonst maximale Fahrt */

#define EXPLORATION_STATE_GOTO_WALL 			1	/*!< Zustand: Bot sucht eine Wand o.ae. Hinderniss */
#define EXPLORATION_STATE_TURN_PARALLEL_LEFT 	2	/*!< Zustand: Bot dreht sich nach links, bis er parallel zur Wand blickt. */
#define EXPLORATION_STATE_TURN_PARALLEL_RIGHT 	3	/*!< Zustand: Bot dreht sich nach rechts, bis er parallel zur Wand blickt. */
#define EXPLORATION_STATE_DRIVE_PARALLEL_LEFT	4	/*!< Zustand: Bot faehrt parallel zur Wand links von sich. */
#define EXPLORATION_STATE_DRIVE_PARALLEL_RIGHT	5	/*!< Zustand: Bot faehrt parallel zur Wand rechts von sich. */
#define EXPLORATION_STATE_TURN_ORTHOGONAL_LEFT	6	/*!< Zustand: Bot dreht sich nach linke, bis er senkrecht zur Wand steht. */
#define EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT	7	/*!< Zustand: Bot dreht sich nach linke, bis er senkrecht zur Wand steht. */
#define EXPLORATION_STATE_DRIVE_ARC				8	/*!< Zustand: Bot faehrt einen Bogen. Der Winkel des Bogens sollte in einer 													 
													 *!< weiteren static Variablen (z.B. curve) gespeichert sein. */

#define BOT_BEHAVIOUR_RUNNING	1
#define BOT_BEHAVIOUR_DONE		0

#endif /*BOTLOCAL_H_*/
