/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	led.h 
 * @brief 	Routinen zur LED-Steuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef led_H_
#define led_H_


#define LED_RECHTS	(1<<0)
#define LED_LINKS	(1<<1)
#define LED_ROT     (1<<2)		///< LED Rot
#define LED_ORANGE  (1<<3)		///< LED Orange
#define LED_GELB    (1<<4)		///< LED Gelb
#define LED_GRUEN   (1<<5)		///< LED Gruen
#define LED_TUERKIS (1<<6)		///< LED Tuerkis
#define LED_WEISS   (1<<7)		///< LED Weiss

#define LED_ALL    0xFF		///< LED Alle

/*!
 * Initialisiert die LEDs
 */
void LED_init(void);

/*!
 * Zeigt eine 8-Bit Variable mit den LEDs an
 * @param LED Wert der gezeigt werden soll
 */
void LED_set(char LED);

/*! Schaltet eine LED aus
 * 
 * @param LED HEX-Code der LED
 */
void LED_off(char LED);

/*! Schaltet eine LED an
 * 
 * @param LED HEX-Code der LED
 */
void LED_on(char LED);

#endif
