/*! @file 	bot-mot.c 
 * @brief 	Low-Level Routinen für die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/
#ifdef MCU

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

#include <stdlib.h>
#include "bot-mot.h"
#include "bot-sens.h"

//Drehrichtung der Motoren
#define BOT_DIR_L_PIN 0x80	// PC7
#define BOT_DIR_L_PORT PORTC
#define BOT_DIR_L_DDR DDRC

#define BOT_DIR_R_PIN 0x40	// PC6 
#define BOT_DIR_R_PORT PORTC
#define BOT_DIR_R_DDR DDRC

#define PWM_R 	OCR1A
#define PWM_L 	OCR1B




// Steuerung der Motor-Pins

volatile int speed_l=0;		///< Geschwindigkeit linker Motor
volatile int speed_r=0;		///< Geschwindigkeit linker Motor

void pwm_0_init(void);
void pwm_1_init(void);
// void pwm_2_init(void);		// Kollidiert mit Timer2 für IR-Fernbedienung

/*!
 *  Initialisiert alles für die Motosteuerung 
 */
void bot_mot_init(){
	BOT_DIR_L_DDR|=BOT_DIR_L_PIN;
	BOT_DIR_R_DDR|=BOT_DIR_R_PIN;
	
	pwm_0_init();
	pwm_1_init();
//	pwm_2_init();
	bot_motor(0,0);
}

/*!
 * unmittelbarere Zugriff auf die beiden Motoren
 * normalerweise NICHT verwenden!!!!!
 * @param left Speed links
 * @param right Speed rechts
*/
void bot_motor(int left, int right){
	PWM_L = 255-abs(left);
	PWM_R = 255-abs(right);

	if (left < 0 )
		BOT_DIR_L_PORT |= BOT_DIR_L_PIN;
	else 
		BOT_DIR_L_PORT &= ~BOT_DIR_L_PIN;
	
	if (right > 0 )		// Einer der Motoren ist invertiert, da er ja in die andere Richtung schaut
		BOT_DIR_R_PORT |= BOT_DIR_R_PIN;
	else 
		BOT_DIR_R_PORT &= ~BOT_DIR_R_PIN;
}


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
void motor_set(int left, int right){
	if (abs(left) > BOT_SPEED_MAX)	// Nicht schneller fahren als moeglich
		speed_l = BOT_SPEED_MAX;
	else if (left == 0)				// Stop wird nicht veraendert
		speed_l = BOT_SPEED_STOP;
	else if (abs(left) < BOT_SPEED_SLOW)	// Nicht langsamer als die 
		speed_l = BOT_SPEED_SLOW;	// Motoren koennen
	else				// Sonst den Wunsch uebernehmen
		speed_l = (char) abs(left);
	
	if (abs(right) > BOT_SPEED_MAX)// Nicht schneller fahren als moeglich
		speed_r = BOT_SPEED_MAX;
	else if (abs(right) == 0)	// Stop wird nicht veraendert
		speed_r = BOT_SPEED_STOP;
	else if (abs(right) < BOT_SPEED_SLOW)	// Nicht langsamer als die 
		speed_r = BOT_SPEED_SLOW;	// Motoren koennen
	else				// Sonst den Wunsch uebernehmen
		speed_r = (char) abs(right);
	
	if (left < 0 )
		speed_l=-speed_l;
	
	if (right < 0 )	
		speed_r=-speed_r;
		
	bot_motor(speed_l,speed_r);
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
		
	// TODO Bereichsüberpüfung!!!

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
