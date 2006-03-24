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

/*! @file 	led.c 
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

volatile uint8 led=0;
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
void LED_on(uint8 LED){
	led |= LED;
	LED_set(led);
}

/*! 
 * Schaltet einzelne LEDs aus
 * andere werden nicht beeinflusst
 * @param LED Bitmaske der anzuschaltenden LEDs
 */
void LED_off(uint8 LED){
	led &= ~LED;
	LED_set(led);
}

/*!
 * Zeigt eine 8-Bit Variable mit den LEDs an
 * @param LED Wert der gezeigt werden soll
 */
void LED_set(uint8 LED){
	led=LED;
	shift_data(led,SHIFT_REGISTER_LED); 
}

#endif
#endif
