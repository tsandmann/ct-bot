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

/*! @file 	motor-low.h 
 * @brief 	Low-Level Routinen fuer die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#ifndef motor_low_H_
#define motor_low_H_

#include "global.h"


#ifdef VARIABLE_PWM_F
	extern uint8 pwm_frequency; 
	/*!
 	*  Initialisiert alles fuer die Motosteuerung 
 	*/
	void motor_low_init(uint8 pwm_f);
#else
	/*!
 	*  Initialisiert alles fuer die Motosteuerung 
 	*/
	void motor_low_init(void);
#endif	//VARIABLE_PWM_F

/*!
 * unmittelbarer Zugriff auf die Motoren
 * @param dev 	Motor (0: links; 1: rechts)
 * @param speed	Soll-Geschwindigkeit
 */
void bot_motor(int16 left, int16 right);

/*!
 * Stellt einen PWM-Wert fuer einen Motor ein
 * low-level
 * @param dev Motor (0: links; 1: rechts)
 */
void motor_update(uint8 dev);

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 */
void servo_low(uint8 servo, uint8 pos);

#endif
