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

/**
 * \file 	motor-low.h
 * \brief 	Low-Level Routinen fuer die Motor- und Servosteuerung des c't-Bots
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	01.12.2005
 */

#ifndef MOTOR_LOW_H_
#define MOTOR_LOW_H_

/**
 * Initialisiert alles fuer die Motor- und Servorsteuerung
 */
void motor_low_init(void);

#ifdef PC
/**
 * Unmittelbarer Zugriff auf die beiden Motoren, normalerweise NICHT verwenden!
 * \param left PWM links
 * \param right PWM rechts
 */
void bot_motor(int16_t left, int16_t right);
#endif // PC

/**
 * Stellt einen PWM-Wert fuer einen Motor ein
 * low-level
 * \param dev Motor (0: links; 1: rechts)
 */
void motor_update(uint8_t dev);

/**
 * Stellt die Servos
 * \param servo Nummer des Servos (1 oder 2)
 * \param pos Zielwert [1; 255] oder 0 fuer Servo aus
 */
void servo_low(uint8_t servo, uint8_t pos);

#endif // MOTOR_LOW_H_
