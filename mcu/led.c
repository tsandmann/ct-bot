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

/**
 * \file 	led.c
 * \brief 	Routinen zur LED-Steuerung
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#ifdef MCU 
#include "ct-Bot.h"

#ifdef LED_AVAILABLE
#include <avr/io.h>
#include "led.h"
#include "shift.h"

uint8_t led = 0; /**< Zustand der LEDs */

/**
 * Initialisiert die LEDs
 */
void LED_init(void) {
	shift_init();
	LED_set(0);
}

uint8_t LED_get(void) {
	return led;
}

/**
 * Schaltet einzelne LEDs an
 * andere werden nicht beeinflusst
 * \param LED Bitmaske der anzuschaltenden LEDs
 */
void LED_on(uint8_t LED) {
	LED_set(led | LED);
}

/**
 * Schaltet einzelne LEDs aus
 * andere werden nicht beeinflusst
 * \param LED Bitmaske der anzuschaltenden LEDs
 */
void LED_off(uint8_t LED) {
	LED_set(led & (uint8_t) (~LED));
}

/**
 * Zeigt eine 8-Bit Variable mit den LEDs an
 * \param LED Wert der gezeigt werden soll
 */
void LED_set(uint8_t LED) {
	led = LED;
	shift_data(led, SHIFT_REGISTER_LED);
}

#endif // LED_AVAILABLE
#endif // MCU
