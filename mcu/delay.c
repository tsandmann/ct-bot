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

#include <avr/delay.h>

#ifdef RTC_AVAILABLE
	#include "rtc.h"
#endif

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
 * Wenn RTC_AVAILABLE dann �ber rtc, sonst �ber delay_100ms
 * ==> aufl�sung ohne rtc: 100ms-schritte mir rtc: 5ms-Schritte
 * @param ms Anzahl der Millisekunden
 */
void delay(int ms){
	int ms_counter=0;
	
	#ifdef RTC_AVAILABLE
		int ms_temp=rtc_msecond;
	#endif
	
	while (ms_counter <ms){
		#ifdef RTC_AVAILABLE
			if (ms_temp!=rtc_msecond){
				ms_counter+=RTC_RESOLUTION;
				ms_temp=rtc_msecond;
			}
		#else
			delay_100ms();
			ms_counter+=100;
		#endif
	}
}
#endif
