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
 * @file 	motor-low.c 
 * @brief 	Low-Level Routinen fuer die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
 */
#ifdef MCU

#include "ct-Bot.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#ifndef NEW_AVR_LIB
	#include <avr/signal.h>
#endif
#include <stdlib.h>

#include "global.h"
#include "motor.h"
#include "timer.h"
#include "sensor.h"
#include "display.h"
#include "motor-low.h"

//Drehrichtung der Motoren
#define BOT_DIR_L_PIN 		(1<<6)	// PC7
#define BOT_DIR_L_PORT 	PORTC
#define BOT_DIR_L_DDR 		DDRC

#define BOT_DIR_R_PIN 		(1<<7)	// PC6 
#define BOT_DIR_R_PORT 	PORTC
#define BOT_DIR_R_DDR 		DDRC

#define PWM_L 	OCR1A
#define PWM_R 	OCR1B

#define PWM_CLK_0	 (_BV(CS02)  |  _BV(CS00))		/*!< Prescaler fuer PWM 0 = 1024*/
//#define PWM_CLK_2	 (_BV(CS22) | _BV(CS21) |_BV(CS20)) /*!< Prescaler fuer PWM 2 =1024		*/

volatile int16 motor_left;	/*!< zuletzt gestellter Wert linker Motor */
volatile int16 motor_right;	/*!< zuletzt gestellter Wert rechter Motor */

void pwm_0_init(void);

void pwm_1_init(void);
//void pwm_2_init(void);		// Kollidiert mit Timer2 fuer IR-Fernbedienung


/*!
 *  Initialisiert alles fuer die Motosteuerung 
 */
void motor_low_init(){
	BOT_DIR_L_DDR|=BOT_DIR_L_PIN;
	BOT_DIR_R_DDR|=BOT_DIR_R_PIN;
	
	pwm_0_init();
	pwm_1_init();
//	pwm_2_init();				// Kollidiert mit Timer2 fuer IR-Fernbedienung
	motor_left = 0;
	motor_right = 0;
	PWM_L = 511;
	PWM_R = 511;
	direction.left = DIRECTION_FORWARD;
	direction.right = DIRECTION_FORWARD;
}

/*!
 * Stellt einen PWM-Wert fuer einen Motor ein
 * low-level
 * @param dev Motor (0: links; 1: rechts)
 */
void motor_update(uint8 dev){
	if (dev == 0) {
		/* linker Motor */
		if (direction.left == DIRECTION_FORWARD) BOT_DIR_L_PORT |= BOT_DIR_L_PIN;	//vorwaerts
		else BOT_DIR_L_PORT &= ~BOT_DIR_L_PIN;	// rueckwaerts
		PWM_L = 511 - motor_left;
	} else {
		/* rechter Motor */
		/* Einer der Motoren ist invertiert, da er ja in die andere Richtung schaut */
		if (direction.right == DIRECTION_BACKWARD) BOT_DIR_R_PORT |= BOT_DIR_R_PIN;	// rueckwaerts
		else BOT_DIR_R_PORT &= ~BOT_DIR_R_PIN;	// vorwaerts
		PWM_R = 511 - motor_right;
	}				
}

// deprecated
///*!
// * unmittelbarer Zugriff auf die Motoren
// * @param dev 	Motor (0: links; 1: rechts)
// * @param speed	Soll-Geschwindigkeit
// */
//void bot_motor(uint8 dev, int16 speed) {
//	if (dev == 0) {
//		/* linker Motor */
//		if (speed >= 0){
//			direction.left = DIRECTION_FORWARD;
////			motor_left = speed;
//		} else {
//			direction.left = DIRECTION_BACKWARD;
////			motor_left = -speed;
//		}
//		motor_left = abs(speed);	
//	} else {
//		/* rechter Motor */
//		if (speed >= 0){
//			direction.right = DIRECTION_FORWARD;
////			motor_right = speed;
//		} else {
//			direction.right = DIRECTION_BACKWARD;
////			motor_right = -speed;
//		}			
//		motor_right = abs(speed);
//	}
//	motor_update(dev);			
//}

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 */
void servo_low(uint8 servo, uint8 pos){
	if (servo== SERVO1) {
		if (pos == SERVO_OFF) {
			#ifdef __AVR_ATmega644__			
				TCCR0B &= ~PWM_CLK_0 ; // PWM aus
			#else
				TCCR0 &= ~PWM_CLK_0 ; // PWM aus
			#endif	
		} else {
			#ifdef __AVR_ATmega644__
				TCCR0B |= PWM_CLK_0; // PWM an
				OCR0A=pos;
			#else
				TCCR0 |= PWM_CLK_0; // PWM an
				OCR0=pos;			
			#endif
		}

	}

//	if (servo== SERVO2) {
//		if (pos == 0) {
//			TCCR2 &= ~ (_BV(CS22)  |  _BV(CS21) | _BV(CS20)); // PWM an		
//		} else {
//			TCCR2 |= PWM_CLK_2; // PWM an
//			OCR2=pos;
//		}
//	}
	
}

/*!
 * Interrupt Handler fuer Timer/Counter 0 
 */
#ifdef __AVR_ATmega644__
	SIGNAL (TIMER0_COMPA_vect){
#else
	SIGNAL (SIG_OUTPUT_COMPARE0){
#endif
}

/*!
 * Timer 0: Kontrolliert den Servo per PWM
 * PWM loescht bei erreichen. daher steht in OCR0 255-Speed!!!
 * initilaisiert Timer 0 und startet ihn 
 */
void pwm_0_init(void){

	DDRB |= (1<<3);			   // PWM-Pin als Output
	TCNT0  = 0x00;            // TIMER0 vorladen

	#ifdef __AVR_ATmega644__
		TCCR0A = _BV(WGM00)  | 	// Normal PWM
			 	 _BV(COM0A1);	// Clear on Compare , Set on Top

		OCR0A = 8;
	#else
		TCCR0 = _BV(WGM00) | 	// Normal PWM
				_BV(COM01);		// Clear on Compare , Set on Top
	
		OCR0 = 8;	// PWM loescht bei erreichen. daher steht in OCR0 255-Speed!!!
	#endif		
	// TIMSK  |= _BV(OCIE0);	 // enable Output Compare 0 overflow interrupt
	//sei();                       // enable interrupts
}

// ---- Timer 1 ------

/*!
 * Interrupt Handler fuer Timer/Counter 1A 
 */
SIGNAL (SIG_OUTPUT_COMPARE1A){
}

/*!
 * Interrupt Handler fuer Timer/Counter 1B 
 */
SIGNAL (SIG_OUTPUT_COMPARE1B){
}

/*!
 * Timer 1: Kontrolliert die Motoren per PWM
 * PWM loescht bei erreichen. daher steht in OCR1A/OCR1B 255-Speed!!!
 * initilaisiert Timer 0 und startet ihn 
 */
void pwm_1_init(void){
	DDRD |= 0x30 ;			  // PWM-Pins als Output
	TCNT1 = 0x0000;           // TIMER1 vorladen

	TCCR1A = _BV(WGM11)  |				// Fast PWM 9 Bit
			 _BV(COM1A1) |_BV(COM1A0) |	// Clear on Top, Set on Compare
			 _BV(COM1B1) |_BV(COM1B0);	// Clear on Top, Set on Compare
	
	TCCR1B = _BV(WGM12) |
	#ifdef SPEED_CONTROL_AVAILABLE	
			 _BV(CS10);					// Prescaler = 1	=>	31.2 kHz	
	#else
			 _BV(CS12);					// Prescaler = 256	=>	122 Hz
	#endif	// SPEED_CONTROL_AVAILABLE	
	
	OCR1A = 255;	// PWM loescht bei erreichen. daher steht in OCR1A 255-Speed!!!
	OCR1B = 255;	// PWM loescht bei erreichen. daher steht in OCR1B 255-Speed!!!
	
	// TIMSK|= _BV(OCIE1A) | _BV(OCIE1B); // enable Output Compare 1 overflow interrupt
	// sei();                       // enable interrupts
}

// Kollidiert derzeit mit Timer2 fuer IR
///*!
// * Timer 0: Kontrolliert den Servo per PWM
// * PWM loescht bei erreichen. daher steht in OCR0 255-Speed!!!
// * initilaisiert Timer 0 und startet ihn 
// */
//void pwm_2_init(void){
//	DDRD |= 0x80;			   // PWM-Pin als Output
//	TCNT2  = 0x00;            // TIMER0 vorladen
//
//	TCCR2 = _BV(WGM20) | 	// Normal PWM
//			_BV(COM21) |    // Clear on Top, Set on Compare
//			_BV(CS22) | _BV(CS21) |_BV(CS20); 		// Prescaler = 1024		
//	
//	OCR2 = 8;	// PWM loescht bei erreichen. daher steht in OCR0 255-Speed!!!
//	// TIMSK  |= _BV(OCIE0);	 // enable Output Compare 0 overflow interrupt
//	//sei();                       // enable interrupts
//}

#endif
