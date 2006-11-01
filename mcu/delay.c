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

/*! @file 	delay.c
 * @brief 	Hilfsroutinen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#ifdef MCU

#include "ct-Bot.h"

#ifdef NEW_AVR_LIB
	#include <util/delay.h>
#else
	#include <avr/delay.h>
#endif

#include "timer.h"

/*!
 * Warte 100 ms
 */
void delay_100ms(void){ 
	char counter;
	//wait (10 * 120000) cycles = wait 1200000 cycles
	counter = 0;
	while (counter != 5)
	{
	//wait (30000 x 4) cycles = wait 120000 cycles
	_delay_loop_2(30000);
	counter++;
	}
}


/*!
 * Delays for ms milliseconds
 * @param ms Anzahl der Millisekunden
 */
void delay(uint16 ms){	
	uint32 start = TIMER_GET_TICKCOUNT_32;
	uint32 ticksToWait = MS_TO_TICKS((uint32)ms);
	while (TIMER_GET_TICKCOUNT_32-start < ticksToWait)
		asm volatile("nop");
}
#endif
