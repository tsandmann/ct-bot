/*! @file 	delay.h
 * @brief 	Hilfsroutinen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#ifndef delay_H_
#define delay_H_


/*!
 * Warte 100 ms
 */
void delay_100ms(void);

/*!
 * Delays for ms milliseconds
 * Wenn RTC_AVAILABLE dann �ber rtc, sonst �ber delay_100ms
 * ==> aufl�sung ohne rtc: 100ms-schritte mir rtc: 5ms-Schritte
 * @param ms Anzahl der Millisekunden
 */
void delay(int ms);
#endif
