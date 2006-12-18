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


/*! @file 	behaviour_servo.c
 * @brief 	kontrolliert die Servos
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	07.12.06
*/


#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_SERVO_AVAILABLE

#include "motor.h"
#include "timer.h"
/*! Uebergabevariable fuer Servo-Verhalten */
static uint16 servo_time; 
static uint8 servo_nr;
static uint8 servo_pos;

/*! 
 * Dieses Verhalten fuehrt ein Servo-Kommando aus und schaltet danach den Servo wieder ab
 * 
 * @param *data der Verhaltensdatensatz
 */
void bot_servo_behaviour(Behaviour_t *data){
	/* Servo ausschalten, falls Klappe zu oder Countdown abgelaufen */
	if ( (servo_pos == DOOR_CLOSE && sensDoor == 0) || (TIMER_GET_TICKCOUNT_16 - servo_time > MS_TO_TICKS(1000L)) ){
		servo_set(servo_nr, SERVO_OFF);	// Servo aus
		return_from_behaviour(data); 	// und Verhalten auch aus
	} 
}

/*!
 * Fahre den Servo an eine Position
 * @param servo Nummer des Servos
 * @param pos Zielposition des Servos
 */
void bot_servo(Behaviour_t * caller, uint8 servo, uint8 pos){
	servo_set(servo, pos);	// Servo-PWM einstellen
	servo_pos = pos;		// Zielposition merken
	servo_nr = servo;		// Servo-Nr speichern
	servo_time = TIMER_GET_TICKCOUNT_16;	// Der Count down verschafft dem Servo etwas Zeit

	switch_to_behaviour(caller,bot_servo_behaviour,OVERRIDE);	// Warte-Verahlten an
}
#endif
