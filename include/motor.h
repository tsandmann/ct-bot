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

/*! @file 	motor.h
 * @brief 	High-Level Routinen fuer die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
*/
#ifndef motor_H_
#define motor_H_


#include "global.h"
#include "ct-Bot.h"

#define BOT_SPEED_IGNORE	1000	/*!< Wert ausserhalb von -BOT_SPEED_MAX und BOT_SPEED_MAX wird verwendet um einen Eintrag zu ignorieren */
#define BOT_SPEED_STOP		0		/*!< Motor aus */

#define BOT_SPEED_SLOW		 49		/*!< langsame Fahrt in mm/s */
#define BOT_SPEED_MEDIUM	 99		/*!< mittlere Fahrt in mm/s */
#define BOT_SPEED_NORMAL	144		/*!< normale Fahrt in mm/s  */
#define BOT_SPEED_FAST		297		/*!< schnelle Fahrt in mm/s */
#define BOT_SPEED_MAX		450		/*!< maximale Fahrt in mm/s */


#define DIRECTION_FORWARD  0		/*!< Drehrichtung vorwaerts */
#define DIRECTION_BACKWARD 1		/*!< Drehrichtung rueckwaerts */

extern int16 volatile speed_l;			/*!< Geschwindigkeit des linken Motors */
extern int16 volatile speed_r;			/*!< Geschwindigkeit des rechten Motors */

extern volatile int16 motor_left;	/*!< zuletzt gestellter Wert linker Motor */
extern volatile int16 motor_right;	/*!< zuletzt gestellter Wert rechter Motor */


/*! In diesem Typ steht die Drehrichtung, auch wenn die Speed-Variablen bereits wieder auf Null sind */
typedef struct {
	uint8 left:1;
	uint8 right:1;
#ifndef DOXYGEN
	} __attribute__ ((packed)) direction_t;
#else
	} direction_t;
#endif

extern direction_t direction;		/*!< Drehrichtung der Motoren, auch wenn die Speed-Variablen bereits wieder auf Null sind */ 

/*!
 * Initialisiere den Motorkrams
 */
void motor_init(void);

/*!
 * Direkter Zugriff auf den Motor
 * @param left	Geschwindigkeit fuer den linken Motor
 * @param right Geschwindigkeit fuer den linken Motor
 * zwischen -255 und +255;
 * 0 bedeutet Stillstand, 255 volle Kraft voraus, -255 volle Kraft zurueck
 * Sinnvoll ist die Verwendung der Konstanten: BOT_SPEED_XXX, 
 * also z.B. motor_set(BOT_SPEED_LOW,-BOT_SPEED_LOW);
 * fuer eine langsame Drehung
*/
void motor_set(int16 left, int16 right);

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 * @param servo Nummer des Servos
 * @param servo Zielwert
 */
void servo_set(uint8 servo, uint8 pos);

#endif
