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
#include <stdlib.h>

#include "global.h"
#include "motor.h"
#include "timer.h"
#include "sensor.h"
#include "display.h"
#include "motor-low.h"

/* Drehrichtung der Motoren */
#define BOT_DIR_L_PIN 	(1<<6)	// PC7
#define BOT_DIR_L_PORT 	PORTC
#define BOT_DIR_L_DDR 	DDRC

#define BOT_DIR_R_PIN 	(1<<7)	// PC6
#define BOT_DIR_R_PORT 	PORTC
#define BOT_DIR_R_DDR 	DDRC

#define PWM_L 	OCR1A
#define PWM_R 	OCR1B

#define PWM_CLK_0	 (_BV(CS02) | _BV(CS00))			/*!< Prescaler fuer PWM 0 = 1024 */
//#define PWM_CLK_2	 (_BV(CS22) | _BV(CS21) |_BV(CS20)) /*!< Prescaler fuer PWM 2 = 1024 */

volatile int16_t motor_left;	/*!< zuletzt gestellter Wert linker Motor */
volatile int16_t motor_right;	/*!< zuletzt gestellter Wert rechter Motor */

/*!
 * Timer 0: Kontrolliert den Servo per PWM
 * Initilaisiert Timer 0 und startet ihn
 */
static void pwm_0_init(void) {
	DDRB |= (1 << 3);	// PWM-Pin als Output
	TCNT0 = 0x00;		// TIMER0 vorladen

#ifdef MCU_ATMEGA644X
	TCCR0A = _BV(WGM00) |	// Phase Correct PWM Mode
			 _BV(COM0A1); 	// Clear on Compare Match when up-counting. Set on Compare Match when down-counting

#else
	TCCR0 = _BV(WGM00) |	// Phase Correct PWM Mode
			_BV(COM01);		// Clear on Compare Match when up-counting. Set on Compare Match when down-counting

#endif	// MCU_ATMEGA644X
}

/*!
 * Timer 1: Kontrolliert die Motoren per PWM
 * Initilaisiert Timer 0 und startet ihn
 */
static void pwm_1_init(void) {
	DDRD |= 0x30;	// PWM-Pins als Output
	TCNT1 = 0x0000;	// TIMER1 vorladen

	TCCR1A = _BV(WGM11)  |				// Fast PWM 9 Bit @ 16 MHz
#if F_CPU == 20000000 && defined SPEED_CONTROL_AVAILABLE
			 _BV(WGM10)  |				// Fast PWM 10 Bit @ 20 MHz
#endif
			 _BV(COM1A1) |_BV(COM1A0) |	// Clear on Top, Set on Compare
			 _BV(COM1B1) |_BV(COM1B0);	// Clear on Top, Set on Compare

	TCCR1B = _BV(WGM12) |
#ifdef SPEED_CONTROL_AVAILABLE
	_BV(CS10);		// Prescaler = 1	=>	31.2 kHz @ 16 MHz, 19.5 kHz @ 20 MHz
#else
	_BV(CS12);		// Prescaler = 256	=>	122 Hz @ 16 MHz, 152.5 Hz @ 20 MHz
#endif	// SPEED_CONTROL_AVAILABLE

#if F_CPU == 20000000
	OCR1A = 1023;	// PWM loescht bei erreichen. daher steht in OCR1A 1023-Speed!!!
	OCR1B = 1023;
#else
	OCR1A = 511;	// PWM loescht bei erreichen. daher steht in OCR1A 511-Speed!!!
	OCR1B = 511;
#endif
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

/*!
 *  Initialisiert alles fuer die Motosteuerung
 */
void motor_low_init() {
	BOT_DIR_L_DDR |= BOT_DIR_L_PIN;
	BOT_DIR_R_DDR |= BOT_DIR_R_PIN;

	pwm_0_init();
	pwm_1_init();
//	pwm_2_init();	// Kollidiert mit Timer2 fuer IR-Fernbedienung
	motor_left  = 0;
	motor_right = 0;
#if F_CPU == 20000000 && defined SPEED_CONTROL_AVAILABLE
	PWM_L = 1023;
	PWM_R = 1023;
#else
	PWM_L = 511;
	PWM_R = 511;
#endif	// F_CPU
	direction.left  = DIRECTION_FORWARD;
	direction.right = DIRECTION_FORWARD;
}

/*!
 * Stellt einen PWM-Wert fuer einen Motor ein
 * low-level
 * @param dev Motor (0: links; 1: rechts)
 */
void motor_update(uint8_t dev) {
	if (dev == 0) {
		/* linker Motor */
		if (direction.left == DIRECTION_FORWARD) BOT_DIR_L_PORT |= BOT_DIR_L_PIN; // vorwaerts
		else BOT_DIR_L_PORT &= ~BOT_DIR_L_PIN; // rueckwaerts
#if F_CPU == 20000000 && defined SPEED_CONTROL_AVAILABLE
		PWM_L = 1023 - (motor_left << 1);
#else
		PWM_L = 511 - motor_left;
#endif	// F_CPU
	} else {
		/* rechter Motor */
		/* Einer der Motoren ist invertiert, da er ja in die andere Richtung schaut */
		if (direction.right == DIRECTION_BACKWARD) BOT_DIR_R_PORT |= BOT_DIR_R_PIN; // rueckwaerts
		else BOT_DIR_R_PORT &= ~BOT_DIR_R_PIN; // vorwaerts
#if F_CPU == 20000000 && defined SPEED_CONTROL_AVAILABLE
		PWM_R = 1023 - (motor_right << 1);
#else
		PWM_R = 511 - motor_right;
#endif	// F_CPU
	}
}

/*!
 * Stellt die Servos
 * @param servo	Nummer des Servos
 * @param pos	Zielwert
 * Sinnvolle Werte liegen zwischen 7 und 16, oder 0 fuer Servo aus
 */
void servo_low(uint8_t servo, uint8_t pos) {
	if (servo == SERVO1) {
		if (pos == SERVO_OFF) {
#ifdef MCU_ATMEGA644X
			TCCR0B &= ~PWM_CLK_0; // PWM aus
#else
			TCCR0 &= ~PWM_CLK_0; // PWM aus
#endif	// MCU_ATMEGA644X
		} else {
#ifdef MCU_ATMEGA644X
			TCCR0B |= PWM_CLK_0; // PWM an
			OCR0A = pos;
#else
			TCCR0 |= PWM_CLK_0; // PWM an
			OCR0 = pos;
#endif	// MCU_ATMEGA644X
		}
	}
}

#endif	// MCU
