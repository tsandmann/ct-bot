/*! @file 	led.h 
 * @brief 	Routinen zur LED-Steuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU 

#include <avr/io.h>
#include "led.h"
#include "ct-Bot.h"

#ifdef LED_AVAILABLE

/*!
 * Initialisiert die LEDs
 */
void LED_init(){
	DDRD|= LED_ALL;	// LED-Ports als Ausgang
	LED_off(LED_ALL);
}

/*! Schaltet eine LEd an
 * 
 * @param LED HEX-Code der LED
 */
void LED_on(char LED){
	PORTD &= ~(LED & LED_ALL);
}

/*! Schaltet eine LEd aus
 * 
 * @param LED HEX-Code der LED
 */
void LED_off(char LED){
	PORTD |= (LED & LED_ALL);	// LED ausschalten
}

/*!
 * Zeigt eine 8-Bit Variable mit den LEDs an
 * @param LED Wert der gezeigt werden soll
 */
void LED_set(char LED){
	PORTD= (PORTD & ~LED_ALL) | ((~LED << 3) & LED_ALL); 
}

#endif
#endif
