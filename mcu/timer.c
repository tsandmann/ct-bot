/*! @file 	timer.h
 * @brief 	Timer und counter
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include "ct-Bot.h"
#include "adc.h"
#include "key.h"

#include "bot-mot.h"
#include "bot-sens.h"
#include "bot-logik.h"
#include "timer.h"
#include "ir.h"



/*!
 * Interrupt Handler for Timer/Counter 0 
 */
SIGNAL (SIG_OUTPUT_COMPARE0){
	#ifdef RTC_AVAILABLE
		rtc_isr();
	#endif
	#ifdef KEY_AVAILABLE
		key_isr();
	#endif
	#ifdef DREH_AVAILABLE		
		dreh_isr();
	#endif
}

/*!
 * initilaisiert Timer 0 und startet ihn 
 */
void timer_0_init(void){
	TCNT0  = 0x00;            // TIMER0 vorladen
	
	// �ndert man den Prescaler muss man die Formel f�r OCR0 anpassen !!!
	// use CLK/1024 prescale value, clear timer/counter on compare0 match   
	TCCR0 = _BV(WGM01) | _BV(CS00) | _BV(CS02);
	
	//Compare Register !!!Achtung nur 8-Bit breit --> evtl. teiler anpassen
	OCR0 = ((XTAL/1024/TIMER_0_CLOCK) - 1 );
	
	// enable Output Compare 0 overflow interrupt
	TIMSK  |= _BV(OCIE0);
	sei();                       // enable interrupts
}


// ---- Timer 1 ------

/*!
 * Interrupt Handler for Timer/Counter 1A 
 */
SIGNAL (SIG_OUTPUT_COMPARE1A){
	bot_sens_isr();		// Sensoren aktualisieren
//	bot_behave();
	motor_isr();		// Motoren aktualisieren
}

/*!
 * Interrupt Handler for Timer/Counter 1B 
 */
SIGNAL (SIG_OUTPUT_COMPARE1B){
}

/*!
 * initilaisiert Timer 1 und startet ihn 
 */
void timer_1_init(void){
	
	TCCR1A = 0;	// Normal operation Output pins disconnected
	TCNT1= 0x000;	//reset counter

	// �ndert man den Prescaler muss man die Formel f�r OCR1A anpassen !!!
	// use CLK/1024 prescale value, clear timer/counter on compareA match   
	TCCR1B = _BV(CS10) | _BV(CS11)  | _BV(WGM12);
	
	// preset timer1 high/low byte
	OCR1A = ((XTAL/64/TIMER_1_CLOCK) - 1 );	
	
	OCR1B = 0;
	
	// enable Output Compare 1 overflow interrupt
	TIMSK|= _BV(OCIE1A) | _BV(OCIE1B);
	//TIMSK|=  _BV(OCIE1B);
	
	sei();                       // enable interrupts
}

// ---- Timer 2 ------

/*!
 * Interrupt Handler for Timer/Counter 2 
 */
SIGNAL (SIG_OUTPUT_COMPARE2){
	#ifdef IR_AVAILABLE
		ir_isr();
	#endif
}

/*!
 * initilaisiert Timer 0 und startet ihn 
 */
void timer_2_init(void){
	TCNT2  = 0x00;            // TIMER vorladen
	
	// �ndert man den Prescaler muss man die Formel f�r OCR2 anpassen !!!
	// use CLK/64 prescale value, clear timer/counter on compare match   
	TCCR2 = _BV(WGM21) | _BV(CS22);
	
	//Compare Register !!!Achtung nur 8-Bit breit --> evtl. teiler anpassen
	OCR2 = ((XTAL/64/TIMER_2_CLOCK) - 1 );
	
	// enable Output Compare 0 overflow interrupt
	TIMSK  |= _BV(OCIE2);
	sei();                       // enable interrupts
}
#endif
