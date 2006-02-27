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

/*! @file 	motor-low.c 
 * @brief 	Low-Level Routinen für die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/
#ifdef MCU

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <stdlib.h>

#include "global.h"

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

void pwm_0_init(void);
void pwm_1_init(void);
// void pwm_2_init(void);		// Kollidiert mit Timer2 für IR-Fernbedienung

/*!
 *  Initialisiert alles für die Motosteuerung 
 */
void motor_low_init(){
	BOT_DIR_L_DDR|=BOT_DIR_L_PIN;
	BOT_DIR_R_DDR|=BOT_DIR_R_PIN;
	
	pwm_0_init();
	pwm_1_init();
//	pwm_2_init();				// Kollidiert mit Timer2 für IR-Fernbedienung
	bot_motor(0,0);
}

/*!
 * unmittelbarere Zugriff auf die beiden Motoren
 * normalerweise NICHT verwenden!!!!!
 * @param left Speed links
 * @param right Speed rechts
*/
void bot_motor(int16 left, int16 right){
	PWM_L = 255-abs(left);
	PWM_R = 255-abs(right);

	if (left > 0 )
		BOT_DIR_L_PORT |= BOT_DIR_L_PIN;
	else 
		BOT_DIR_L_PORT &= ~BOT_DIR_L_PIN;
	
	if (right < 0 )		// Einer der Motoren ist invertiert, da er ja in die andere Richtung schaut
		BOT_DIR_R_PORT |= BOT_DIR_R_PIN;
	else 
		BOT_DIR_R_PORT &= ~BOT_DIR_R_PIN;
}

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 */
void servo_set(char servo, char pos){
	if (pos< SERVO_LEFT)
		pos=SERVO_LEFT;
	if (pos> SERVO_RIGHT)
		pos=SERVO_RIGHT;
		
	/*! TODO Bereichsüberpüfung!!! */

	if (servo== SERVO1) {
		OCR0=pos;
	}
	if (servo== SERVO2) {
		OCR2=pos;
	}
	
}

/*!
 * Interrupt Handler for Timer/Counter 0 
 */
SIGNAL (SIG_OUTPUT_COMPARE0){
}

/*!
 * Timer 0: Kontrolliert den Servo per PWM
 * PWM löscht bei erreichen. daher steht in OCR0 255-Speed!!!
 * initilaisiert Timer 0 und startet ihn 
 */
void pwm_0_init(void){

	DDRB |= (1<<3);			   // PWM-Pin als Output
	TCNT0  = 0x00;            // TIMER0 vorladen

	TCCR0 = _BV(WGM00) | 	// Normal PWM
			_BV(COM01) |    // Clear on Compare , Set on Top
			_BV(CS02)  |  _BV(CS00); 		// Prescaler = 1024		
	
	OCR0 = 8;	// PWM löscht bei erreichen. daher steht in OCR0 255-Speed!!!
	// TIMSK  |= _BV(OCIE0);	 // enable Output Compare 0 overflow interrupt
	//sei();                       // enable interrupts
}

// ---- Timer 1 ------

/*!
 * Interrupt Handler for Timer/Counter 1A 
 */
SIGNAL (SIG_OUTPUT_COMPARE1A){
}

/*!
 * Interrupt Handler for Timer/Counter 1B 
 */
SIGNAL (SIG_OUTPUT_COMPARE1B){
}

/*!
 * Timer 1: Kontrolliert die Motoren per PWM
 * PWM löscht bei erreichen. daher steht in OCR1A/OCR1B 255-Speed!!!
 * initilaisiert Timer 0 und startet ihn 
 */
void pwm_1_init(void){
	DDRD |= 0x30 ;			  // PWM-Pins als Output
	TCNT1 = 0x0000;           // TIMER1 vorladen

	TCCR1A = _BV(WGM10) | 				  // Fast PWM 8 Bit
			_BV(COM1A1) |_BV(COM1A0) |    // Clear on Top, Set on Compare
			_BV(COM1B1) |_BV(COM1B0);     // Clear on Top, Set on Compare
	
	TCCR1B = _BV(WGM12) |
	 		 _BV(CS12) | _BV(CS10); 		// Prescaler = 1024		
	
	OCR1A = 255;	// PWM löscht bei erreichen. daher steht in OCR1A 255-Speed!!!
	OCR1B = 255;	// PWM löscht bei erreichen. daher steht in OCR1B 255-Speed!!!
	
	// TIMSK|= _BV(OCIE1A) | _BV(OCIE1B); // enable Output Compare 1 overflow interrupt
	// sei();                       // enable interrupts
}

/*!
 * Timer 0: Kontrolliert den Servo per PWM
 * PWM löscht bei erreichen. daher steht in OCR0 255-Speed!!!
 * initilaisiert Timer 0 und startet ihn 
 */
/* Kollidiert derzeit mit Timer2 für IR
void pwm_2_init(void){
	DDRD |= 0x80;			   // PWM-Pin als Output
	TCNT2  = 0x00;            // TIMER0 vorladen

	TCCR2 = _BV(WGM20) | 	// Normal PWM
			_BV(COM21) |    // Clear on Top, Set on Compare
			_BV(CS22) | _BV(CS21) |_BV(CS20); 		// Prescaler = 1024		
	
	OCR2 = 8;	// PWM löscht bei erreichen. daher steht in OCR0 255-Speed!!!
	// TIMSK  |= _BV(OCIE0);	 // enable Output Compare 0 overflow interrupt
	//sei();                       // enable interrupts
}
*/

#endif
