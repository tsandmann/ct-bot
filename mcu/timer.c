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

#include "timer.h"
#include "ir-rc5.h"


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
//	TCCR2 = _BV(WGM21) | _BV(CS22)| _BV(COM20);
	
	//Compare Register !!!Achtung nur 8-Bit breit --> evtl. teiler anpassen
	OCR2 = ((XTAL/64/TIMER_2_CLOCK) - 1 );
	
	// enable Output Compare 0 overflow interrupt
	TIMSK  |= _BV(OCIE2);
	sei();                       // enable interrupts
}
#endif
