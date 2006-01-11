/*! @file 	delay_pc.c
 * @brief 	Hilfsroutinen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#include "ct-Bot.h"

#ifdef PC


#include <stdio.h>      /* for printf() and fprintf() */

/*!
 * Warte 100 ms
 */
void delay_100ms(void){
	printf("delay_100ms() NOT Implemented Yet");
}


/*!
 * Delays for ms milliseconds
 * Wenn RTC_AVAILABLE dann �ber rtc, sonst �ber delay_100ms
 * ==> aufl�sung ohne rtc: 100ms-schritte mir rtc: 5ms-Schritte
 * @param ms Anzahl der Millisekunden
 */
void delay(int ms){
	printf("delay() NOT Implemented Yet");
}
#endif
