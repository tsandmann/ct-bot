/*! @file 	led.h 
 * @brief 	Routinen zur LED-Steuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU 

#include <avr/io.h>
#include "led.h"
#include "ct-Bot.h"
#include "shift.h"

#ifdef LED_AVAILABLE

volatile char led =0;
/*!
 * Initialisiert die LEDs
 */
void LED_init(){
	shift_init();
	LED_off(LED_ALL);
}

/*! 
 * Schaltet einzelne LEDs an
 * andere werden nicht beeinflusst
 * @param LED Bitmaske der anzuschaltenden LEDs
 */
void LED_on(char LED){
	led |= LED;
	LED_set(led);
}

/*! 
 * Schaltet einzelne LEDs aus
 * andere werden nicht beeinflusst
 * @param LED Bitmaske der anzuschaltenden LEDs
 */
void LED_off(char LED){
	led &= ~LED;
	LED_set(led);
}

/*!
 * Zeigt eine 8-Bit Variable mit den LEDs an
 * @param LED Wert der gezeigt werden soll
 */
void LED_set(char LED){
	led=LED;
	shift_data(led,SHIFT_REGISTER_LED); 
}

#endif
#endif
