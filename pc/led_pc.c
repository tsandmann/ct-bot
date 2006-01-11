/*! @file 	led_pc.c 
 * @brief 	Routinen zur LED-Steuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include "led.h"

#ifdef LED_AVAILABLE

/*!
 * Initialisiert die LEDs
 */
void LED_init(){
}

/*! Schaltet eine LEd an
 * 
 * @param LED HEX-Code der LED
 */
void LED_on(char LED){
}

/*! Schaltet eine LEd aus
 * 
 * @param LED HEX-Code der LED
 */
void LED_off(char LED){
}

/*!
 * Zeigt eine 8-Bit Variable mit den LEDs an
 * @param LED Wert der gezeigt werden soll
 */
void LED_set(char LED){
}

#endif
#endif
