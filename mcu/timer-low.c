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

/*! @file 	timer-low.c
 * @brief 	Timer und counter für den Mikrocontroller
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU

#include <avr/io.h>
#include <avr/interrupt.h>

#include "ct-Bot.h"

#include "timer.h"
#include "ir-rc5.h"
#include "sensor-low.h"
#include "motor.h"
#include "bot-local.h"

// ---- Timer 2 ------

/*!
  Interrupt Handler for Timer/Counter 2 
 */
SIGNAL (SIG_OUTPUT_COMPARE2){
   
	#ifdef IR_AVAILABLE
		ir_isr();
	#endif	
	#ifdef TIME_AVAILABLE
		system_time_isr();
	#endif
	bot_encoder_isr();
	
}

/*!
 * initilaisiert Timer 0 und startet ihn 
 */
void timer_2_init(void){
	TCNT2  = 0x00;            // TIMER vorladen
	
	// aendert man den Prescaler muss man die Formel fuer OCR2 anpassen !!!
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
