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
 * Verzoegert um ms Millisekunden
 * @param ms Anzahl der Millisekunden
 */
void delay(uint16 ms){	
	uint32 start = TIMER_GET_TICKCOUNT_32;
	if ((uint8)start != TIMER_GET_TICKCOUNT_8) start = TIMER_GET_TICKCOUNT_32;
	uint32 ticksToWait = MS_TO_TICKS((uint32)ms);
	uint32 now;
	do {
		now = TIMER_GET_TICKCOUNT_32;
		if ((uint8)now != TIMER_GET_TICKCOUNT_8) now = TIMER_GET_TICKCOUNT_32;
	} while (now-start < ticksToWait);
		
}
#endif	// MCU
