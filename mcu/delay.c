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
