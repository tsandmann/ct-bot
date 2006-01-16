/*! @file 	motor.c
 * @brief 	High-Level Routinen für die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
*/
#ifndef motor_H_
#define motor_H_


#include "global.h"

#define BOT_SPEED_STOP		0		///< Motor aus
#define BOT_SPEED_SLOW		10		///< langsame Fahrt
#define BOT_SPEED_NORMAL	50		///< normale Fahrt
#define BOT_SPEED_FAST		150		///< schnelle Fahrt
#define BOT_SPEED_MAX		255		///< maximale Fahrt

extern int16 volatile speed_l;			///< Geschwindigkeit des linken Motors
extern int16 volatile speed_r;			///< Geschwindigkeit des rechten Motors


/*!
 * Initialisiere den Motorkrams
 */
void motor_init();

/*!
 * Direkter Zugriff auf den Motor
 * @param left	Geschwindigkeit für den linken Motor
 * @param right Geschwindigkeit für den linken Motor
 * zwischen -255 und +255
 * 0 bedeutet steht, 255 volle Kraft voraus -255 volle Kraft zur�ck
 * Sinnvoll ist die Verwendung der Konstanten: BOT_SPEED_XXX 
 * Also z.B. motor_set(BOT_SPEED_LOW,-BOT_SPEED_LOW);
 * für eine langsame Drehung
*/
void motor_set(int16 left, int16 right);
#endif
