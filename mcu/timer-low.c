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

#include "ct-Bot.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#ifndef NEW_AVR_LIB
	#include <avr/signal.h>
#endif
	

#include "ct-Bot.h"

#include "timer.h"
#include "ir-rc5.h"
#include "sensor-low.h"
#include "bot-local.h"

// ---- Timer 2 ------

/*!
  Interrupt Handler fuer Timer/Counter 2(A)
 */
#ifdef __AVR_ATmega644__
	SIGNAL (TIMER2_COMPA_vect){
#else
	SIGNAL (SIG_OUTPUT_COMPARE2){
#endif
   
	/* - FERNBEDIENUNG - */
	#ifdef IR_AVAILABLE
		ir_isr();
	#endif	
	/* ----- TIMER ----- */
	(*(uint32*)tickCount)++;	// TickCounter [176 us] erhoehen
	/* --- RADENCODER --- */
	bot_encoder_isr();
}

/*!
 * initilaisiert Timer 0 und startet ihn 
 */
void timer_2_init(void){
	TCNT2  = 0x00;            // TIMER vorladen
	
	// aendert man den Prescaler muss man die Formel fuer OCR2 anpassen !!!
	// Compare Register nur 8-Bit breit --> evtl. teiler anpassen
	#ifdef __AVR_ATmega644__
		TCCR2A = _BV(WGM21);	// CTC Mode
		TCCR2B = _BV(CS22);		// Prescaler = CLK/64
		OCR2A = ((XTAL/64/TIMER_2_CLOCK) - 1);	// Timer2A
		TIMSK2  |= _BV(OCIE2A);	// TIMER2 Output Compare Match A Interrupt an
	#else
		// use CLK/64 prescale value, clear timer/counter on compare match   
		TCCR2 = _BV(WGM21) | _BV(CS22);
		OCR2 = ((XTAL/64/TIMER_2_CLOCK) - 1);
		TIMSK  |= _BV(OCIE2);	// enable Output Compare 0 overflow interrupt
	#endif
	
	sei();                       // enable interrupts
}
#endif
