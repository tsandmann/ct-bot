/*! @file 	bot-mot.h 
 * @brief 	Low-Level Routinen für die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#ifndef motor_low_H_
#define motor_low_H_

#include "global.h"

#define SERVO_LEFT 	8
#define SERVO_RIGHT	16
#define SERVO_MIDDLE   ((SERVO_RIGHT- SERVO-LEFT)/2)

#define SERVO1 1
#define SERVO2 2


/*!
 *  Initialisiert alles für die Motosteuerung 
 */
void motor_low_init(void);

/*!
 * unmittelbarere Zugriff auf die beiden Motoren
 * normalerweise NICHT verwenden!!!!!
 * @param left speed links
 * @param right speed rechts
*/
void bot_motor(int16 left, int16 right);

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 */
void servo_set(char servo, char pos);
#endif
