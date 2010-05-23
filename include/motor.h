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
 * @file 	motor.h
 * @brief 	High-Level Routinen fuer die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
 */
#ifndef MOTOR_H_
#define MOTOR_H_


#include "global.h"
#include "ct-Bot.h"
#include "ui/available_screens.h"
#include "bot-logic/bot-logik.h"

#define BOT_SPEED_IGNORE	1000	/*!< Wert ausserhalb von -BOT_SPEED_MAX und BOT_SPEED_MAX wird verwendet um einen Eintrag zu ignorieren */
#define BOT_SPEED_STOP		0		/*!< Motor aus */

#define BOT_SPEED_MIN		 50 	/*!< langsamste Fahrt in mm/s */
#define BOT_SPEED_SLOW		 50 	/*!< langsame Fahrt in mm/s */
#define BOT_SPEED_FOLLOW	 70		/*!< vorsichtige Fahrt, fuer Folgeverhalten in mm/s */
#define BOT_SPEED_MEDIUM	100		/*!< mittlere Fahrt in mm/s */
#define BOT_SPEED_NORMAL	150		/*!< normale Fahrt in mm/s  */
#define BOT_SPEED_FAST		300		/*!< schnelle Fahrt in mm/s */
#define BOT_SPEED_MAX		450		/*!< maximale Fahrt in mm/s */


#define DIRECTION_FORWARD  0		/*!< Drehrichtung vorwaerts */
#define DIRECTION_BACKWARD 1		/*!< Drehrichtung rueckwaerts */

#define SERVO_OFF 0					/*!< Servo wird zum Stromsparen deaktiviert */

#define SERVO1 1					/*!< Servo 1 */
#define SERVO2 2					/*!< Servo 2 */

extern int16_t speed_l;				/*!< Sollgeschwindigkeit des linken Motors */
extern int16_t speed_r;				/*!< Sollgeschwindigkeit des rechten Motors */

extern volatile int16_t motor_left;		/*!< zuletzt gestellter Wert linker Motor */
extern volatile int16_t motor_right;	/*!< zuletzt gestellter Wert rechter Motor */

/*! In diesem Typ steht die Drehrichtung, auch wenn die Speed-Variablen bereits wieder auf Null sind */
typedef union {
	struct {
		unsigned left:1;	/*!< linksrum */
		unsigned right:1;	/*!< rechtsrum */
	} PACKED;
	uint8_t raw;
} direction_t;

extern direction_t direction;		/*!< Drehrichtung der Motoren, auch wenn die Speed-Variablen bereits wieder auf Null sind */

/*!
 * Initialisiere den Motorkrams
 */
void motor_init(void);

/*!
 * Direkter Zugriff auf den Motor
 * @param left	Geschwindigkeit fuer den linken Motor
 * @param right	Geschwindigkeit fuer den linken Motor
 * Geschwindigkeit liegt zwischen -450 und +450. 0 bedeutet Stillstand, 450 volle Kraft voraus, -450 volle Kraft zurueck.
 * Sinnvoll ist die Verwendung der Konstanten: BOT_SPEED_XXX, also z.B. motor_set(BOT_SPEED_SLOW,-BOT_SPEED_SLOW) fuer eine langsame Drehung
 */
void motor_set(int16_t left, int16_t right);

/*!
 * Stellt die Servos
 * @param servo	Nummer des Servos
 * @param pos	Zielwert
 * Sinnvolle Werte liegen zwischen 7 und 16, oder 0 fuer Servo aus
 */
void servo_set(uint8_t servo, uint8_t pos);

#ifdef SPEED_CONTROL_AVAILABLE
/*!
 * Drehzahlregelung fuer die Motoren des c't-Bots
 * @param dev		0: linker Motor, 1: rechter Motor
 * @param actVar	Zeiger auf Stellgroesse (nicht volatile, da Aufruf aus ISR heraus)
 * @param encTime	Zeiger auf Encodertimestamps, mit denen gerechnet werden soll
 * @param i_time	Index des aktuellen Timestamps in encTime
 * @param enc		Encoder-Pegel (binaer) von dev
 * Drehzahlregelung sorgt fuer konstante Drehzahl und somit annaehernd Geradeauslauf.
 * Feintuning von PID_Kp bis PID_SPEED_THRESHOLD (bot-local.h) verbessert die Genauigkeit und Schnelligkeit der Regelung.
 * Mit PWMMIN, PWMSTART_L und PWMSTART_R laesst sich der Minimal- bzw. Startwert fuer die Motoren anpassen.
 */
void speed_control(uint8_t dev, int16_t * actVar, uint16_t * encTime, uint8_t i_time, uint8_t enc);

#ifdef DISPLAY_REGELUNG_AVAILABLE
/*!
 * Zeigt Debug-Informationen der Motorregelung an.
 * Dargestellt werden pro Moto Ist- / Sollgeschwindigkeit, die Differenz davon, der PWM-Stellwert und die
 * Reglerparameter Kp, Ki und Kd.
 * Die Tasten 1 und 4 veraendern Kp, 2 und 5 veraendern Ki, 3 und 6 veraendern Kd, wenn ADJUST_PID_PARAMS an ist.
 */
void speedcontrol_display(void);
#endif	// DISPLAY_REGELUNG_AVAILABLE
#endif	// SPEED_CONTROL_AVAILABLE
#endif	// MOTOR_H_
