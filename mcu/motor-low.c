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
 * \file 	motor-low.c
 * \brief 	Low-Level Routinen fuer die Motor- und Servosteuerung des c't-Bots
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	01.12.2005
 * Benutzt Bit 0 bis 2 von GPIOR0 fuer die Servos (auf ATmega1284P)
 */

#ifdef MCU
#include "ct-Bot.h"
#include "motor-low.h"
#include "motor.h"
#include "timer.h"
#include "sensor.h"
#include "display.h"

#include <avr/io.h>
#include <stdlib.h>

/* Drehrichtung der Motoren */
#define BOT_DIR_L_PIN  _BV(PC6)
#define BOT_DIR_L_PORT PORTC
#define BOT_DIR_L_DDR  DDRC

#define BOT_DIR_R_PIN  _BV(PC7)
#define BOT_DIR_R_PORT PORTC
#define BOT_DIR_R_DDR  DDRC

/* PWM der Motoren */
#define PWM_L OCR1A
#define PWM_R OCR1B

#ifdef SPEED_CONTROL_AVAILABLE
#define PWM_CLK_1 (_BV(CS10)) /**< Prescaler fuer PWM1 = 1 */
#else
#define PWM_CLK_1 (_BV(CS10) | _BV(CS11)) /**< Prescaler fuer PWM1 = 64 */
#endif

#define PWM_FREQUENCY 16129 /**< PWM Frequenz / Hz, ohne SPEED_CONTROL: / 64 Hz */
#define PWM_TOP (uint16_t)((float) F_CPU / 2.f / PWM_FREQUENCY)


/* PWM fuer Servos (ATmega32 / ATmega644(p)) */
#define PWM_CLK_0 (_BV(CS02) | _BV(CS00)) /**< Prescaler fuer PWM0 = 1024 -> ATmega32/644(p) 16 MHz: 30.64 Hz; ATmega644(p) 20 MHz: 38.30 Hz */


int16_t motor_left;  /**< zuletzt gestellter Wert linker Motor */
int16_t motor_right; /**< zuletzt gestellter Wert rechter Motor */

/**
 * Timer 1: Kontrolliert die Motoren per PWM
 * Initialisiert Timer 1 und startet ihn
 */
static void pwm_1_init(void) {
	DDRD |= _BV(PD4) | _BV(PD5); // PWM-Pins output
	TCNT1 = 0; // TIMER1 init

	TCCR1A = _BV(COM1A1) |_BV(COM1A0) |	// Clear on top, set on compare
			 _BV(COM1B1) |_BV(COM1B0);	// Clear on top, set on compare

	TCCR1B = _BV(WGM13) | PWM_CLK_1; // Phase and Frequency Correct PWM, TOP at ICR1

	ICR1 = PWM_TOP;

	OCR1A = PWM_TOP;
	OCR1B = PWM_TOP;
}

/**
 * Stellt einen PWM-Wert fuer einen Motor ein
 * low-level
 * \param dev Motor (0: links; 1: rechts)
 */
void motor_update(uint8_t dev) {
	if (dev == 0) {
		/* linker Motor */
#ifndef MOT_SWAP_L
		if (direction.left == DIRECTION_FORWARD) {
#else
		if (direction.left == DIRECTION_BACKWARD) {
#endif
			BOT_DIR_L_PORT |= BOT_DIR_L_PIN;
		} else {
			BOT_DIR_L_PORT = (uint8_t) (BOT_DIR_L_PORT & ~BOT_DIR_L_PIN);
		}

		uint8_t sreg = SREG;
		__builtin_avr_cli();
		const int16_t motor_l = motor_left;
		SREG = sreg;
		PWM_L = (uint16_t) (PWM_TOP - ((float) motor_l / 511.f) * PWM_TOP);
	} else {
		/* rechter Motor */
		/* Einer der Motoren ist invertiert, da er ja in die andere Richtung schaut */
#ifndef MOT_SWAP_R
		if (direction.right == DIRECTION_BACKWARD) {
#else
		if (direction.right == DIRECTION_FORWARD) {
#endif
			BOT_DIR_R_PORT |= BOT_DIR_R_PIN;
		} else {
			BOT_DIR_R_PORT = (uint8_t) (BOT_DIR_R_PORT & ~BOT_DIR_R_PIN);
		}

		uint8_t sreg = SREG;
		__builtin_avr_cli();
		const int16_t motor_r = motor_right;
		SREG = sreg;
		PWM_R = (uint16_t) (PWM_TOP - ((float) motor_r / 511.f) * PWM_TOP);
	}
}

/**
 * Initialisiert Timer3 fuer Servoansteuerung (nur ATmega1284P)
 */
static void timer_3_init(void) {
#ifdef __AVR_ATmega1284P__
	GPIOR0 = 0; // beide Servos aus
	TCNT3 = 0; // TIMER3 init
	TIMSK3 = 0; // Timer Interrupts Disable
	TIFR1 = _BV(TOV3) | _BV(OCF3A) | _BV(OCF3B); // Clear Timer Flags
	TCCR3A = 0; // normal port operation, OC3A/OC3B disconnected
	TCCR3B = _BV(CS31); // normal timer mode, prescaler = 8 -> 20 MHz: 26.2144 ms period, 16 MHz: 32.768 ms period
	OCR3A = 0;
	OCR3B = 0;
#endif // __AVR_ATmega1284P__
}

/**
 * Initialisiert Ansteuerung fuer Servo 1
 * Auf ATmega1284P mit Timer3, ansonsten mit Timer0
 */
static void servo_1_init(void) {
	DDRB |= _BV(DDB3); // PWM-Pin Output
#ifndef __AVR_ATmega1284P__
	TCNT0 = 0; // TIMER0 vorladen

#if defined MCU_ATMEGA644X
	TCCR0A = _BV(WGM00) | // Phase Correct PWM Mode
			 _BV(COM0A1); // Clear on Compare Match

#else
	TCCR0 = _BV(WGM00) | // Phase Correct PWM Mode
			_BV(COM01);  // Clear on Compare Match

#endif // MCU_ATMEGA644X
#endif // ! __AVR_ATmega1284P__
	servo_low(SERVO1, SERVO_OFF);
}

/**
 * Initialisiert Ansteuerung fuer Servo 2 (nur ATmega1284P)
 */
static void servo_2_init(void) {
#ifdef __AVR_ATmega1284P__
	DDRD |= _BV(DDD7); // PWM-Pin Output
	servo_low(SERVO2, SERVO_OFF);
#endif // __AVR_ATmega1284P__
}

#ifdef __AVR_ATmega1284P__
/**
 * Timer3 Overflow Interrupt
 */
ISR(TIMER3_OVF_vect, ISR_NAKED) {
	GPIOR0 = (uint8_t) (GPIOR0 & ~4);
	if (GPIOR0 & 1) {
		PORTB |= _BV(PB3); // PWM0 high
		GPIOR0 |= 4;
	}
	if (GPIOR0 & 2) {
		PORTD |= _BV(PD7); // PWM2 high
		GPIOR0 |= 4;
	}
	if ((GPIOR0 & 4) == 0) {
		/* Timer Interrupts Disable */
		__asm__ __volatile__ (
			"push r24		\n\t"
			"ldi r24, 0		\n\t"
			"sts %0, r24	\n\t"
			"sts %1, r24	\n\t"
			"pop r24			"
			::	"n" (&TIMSK3), "n" (&TCNT3)
			:	"memory"
		);
	}
	reti();
}

/**
 * Timer3 Output Compare A Match Interrupt
 */
ISR(TIMER3_COMPA_vect, ISR_NAKED) {
	PORTB = (uint8_t) (PORTB & ~_BV(PB3)); // PWM0 low
	reti();
}

/**
 * Timer3 Output Compare B Match Interrupt
 */
ISR(TIMER3_COMPB_vect, ISR_NAKED) {
	PORTD = (uint8_t) (PORTD & ~_BV(PD7)); // PWM2 low
	reti();
}
#endif // __AVR_ATmega1284P__

/**
 * Hilfsfunktion zur Umrechnung von Servoposition in PWM Pulsbreite
 * @param pos Zielwert fuer Servoposition [1; 255]
 * @return PWM Pulbreite in us
 */
static inline float servo_calc_pulse(uint8_t pos) {
	return pos * 7 + 600;
}

/**
 * Hilfsfunktion zur Berechnung des Timer Output Compare Match Werts fuer die Servos
 * @param pos Zielwert fuer Servoposition [1; 255]
 * @return Timer OC Wert fuer OCR
 */
static inline uint16_t servo_calc_ocr(uint8_t pos) {
#ifdef __AVR_ATmega1284P__
	const float TIMER_PRESCALER = 8.f;
	const float TIMER_MAX = 65535.f;
	const float TIMER_FACTOR = 1.f;
#else
	const float TIMER_PRESCALER = 1024.f;
	const float TIMER_MAX = 255.f;
	const float TIMER_FACTOR = 2.f;
#endif

	const float T_us = 1000000.f / (F_CPU / TIMER_PRESCALER / TIMER_FACTOR / TIMER_MAX);
	const float ocr = TIMER_MAX / T_us * servo_calc_pulse(pos); // duty_cylce = servo_calc_pulse(pos) / T_us; ocr = TIMER_MAX * duty_cycle
	return (uint16_t) ocr;
}

/**
 * Stellt die Servos
 * \param servo Nummer des Servos (1 oder 2)
 * \param pos Zielwert [1; 255] oder 0 fuer Servo aus
 */
void servo_low(uint8_t servo, uint8_t pos) {
#ifdef __AVR_ATmega1284P__
	if (servo == SERVO1) {
		if (pos == SERVO_OFF) {
			GPIOR0 = (uint8_t) (GPIOR0 & ~_BV(0)); // PWM0 stays low on next overflow
		} else {
			if ((GPIOR0 & 2) == 0) {
				TCNT3 = 0;
			}
			OCR3A = servo_calc_ocr(pos);
			GPIOR0 |= _BV(0);
			TIMSK3 |= (uint8_t) (_BV(TOIE3) | _BV(OCIE3A)); // Overflow Interrupt Enable, Output Compare A Match Interrupt Enable
		}
	} else if (servo == SERVO2) {
		if (pos == SERVO_OFF) {
			GPIOR0 = (uint8_t) (GPIOR0 & ~_BV(1)); // PWM2 stays low on next overflow
		} else {
			if ((GPIOR0 & 1) == 0) {
				TCNT3 = 0;
			}
			OCR3B = servo_calc_ocr(pos);
			GPIOR0 |= _BV(1);
			TIMSK3 |= (uint8_t) (_BV(TOIE3) | _BV(OCIE3B)); // Overflow Interrupt Enable, Output Compare B Match Interrupt Enable
		}
	}
#else // ! __AVR_ATmega1284P__
	if (servo == SERVO1) {
		if (pos == SERVO_OFF) {
#ifdef MCU_ATMEGA644X
			TCCR0B = (uint8_t) (TCCR0B & ~PWM_CLK_0); // PWM0 aus
#else
			TCCR0 = (uint8_t) (TCCR0 & ~PWM_CLK_0); // PWM0 aus
#endif // MCU_ATMEGA644X
		} else {
#ifdef MCU_ATMEGA644X
			TCCR0B |= PWM_CLK_0; // PWM0 an; ATmega644(p) 16 MHz: 30.64 Hz -> T = 32.640 ms; 20 MHz: 38.15 Hz -> T = 26.112 ms
			OCR0A = (uint8_t) servo_calc_ocr(pos);
#else
			TCCR0 |= PWM_CLK_0; // PWM0 an; ATmega32 16 MHz: 30.64 Hz -> T = 32.640 ms
			OCR0 = (uint8_t) servo_calc_ocr(pos);
#endif // MCU_ATMEGA644X
		}
	}
#endif // __AVR_ATmega1284P__
}

/**
 * Initialisiert alles fuer die Motor- und Servorsteuerung
 */
void motor_low_init() {
	BOT_DIR_L_DDR |= BOT_DIR_L_PIN;
	BOT_DIR_R_DDR |= BOT_DIR_R_PIN;

	pwm_1_init();
	timer_3_init();
	servo_1_init();
	servo_2_init();
	motor_left  = 0;
	motor_right = 0;
	PWM_L = PWM_TOP;
	PWM_R = PWM_TOP;
	direction.left  = DIRECTION_FORWARD;
	direction.right = DIRECTION_FORWARD;
}

#endif // MCU
