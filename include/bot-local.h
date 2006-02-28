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
 * @date 	28.02.06
*/

#ifndef BOTLOCAL_H_
#define BOTLOCAL_H_

#include "ct-Bot.h"

#define ENCODER_MARKS		60		/*!< Anzahl der Flanken, die ein Encoder bei einer Radumdrehung liefert, also Anzahl der weißen + Anzahl der schwarzen Felder */
#define WHEEL_DIAMETER		57		/*!< Durchmesser eines Rades in mm */
#define WHEEL_PERIMETER	179 	/*!< Durchmesser eines Rades in mm */	

/* Einstellunge für die Verhaltensregeln */
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


#endif /*BOTLOCAL_H_*/
