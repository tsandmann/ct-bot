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

/*!
 * @file 	led.h
 * @brief 	Routinen zur LED-Steuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.2005
 */

#ifndef LED_H_
#define LED_H_

#define LED_RECHTS	(1<<0)		/*!< LED (blau) in Fahrichtung rechts */
#define LED_LINKS	(1<<1)		/*!< LED (blau) in Fahrichtung links */
#define LED_ROT     (1<<2)		/*!< LED rot */
#define LED_ORANGE  (1<<3)		/*!< LED orange */
#define LED_GELB    (1<<4)		/*!< LED gelb */
#define LED_GRUEN   (1<<5)		/*!< LED gruen */
#define LED_TUERKIS (1<<6)		/*!< LED tuerkis (blau) */
#define LED_WEISS   (1<<7)		/*!< LED weiss */

#define LED_ALL    0xFF		/*!< LED Alle */

#ifndef __ASSEMBLER__
/*! Datenfeld fuer den Zugriff auf die LEDs */
typedef struct {
	unsigned rechts:1;	/*!< LED in Fahrichtung rechts */
	unsigned links:1;	/*!< LED in Fahrichtung links */
	unsigned rot:1;		/*!< LED Rot */
	unsigned orange:1;	/*!< LED Orange */
	unsigned gelb:1;	/*!< LED Gelb */
	unsigned gruen:1;	/*!< LED Gruen */
	unsigned tuerkis:1;	/*!< LED Tuerkis */
	unsigned weiss:1;	/*!< LED Weiss */
} PACKED_FORCE led_t;

extern uint8_t led;	/*!< Zustand der LEDs */

/*!
 * Initialisiert die LEDs
 */
void LED_init(void);

/*!
 * Zeigt eine 8-Bit Variable mit den LEDs an
 * @param LED Wert der gezeigt werden soll
 */
void LED_set(uint8_t LED);

/*!
 * Schaltet einzelne LEDs aus
 * andere werden nicht beeinflusst
 * @param LED Bitmaske der anzuschaltenden LEDs
 */
void LED_off(uint8_t LED);

/*!
 * Schaltet einzelne LEDs an
 * andere werden nicht beeinflusst
 * @param LED Bitmaske der anzuschaltenden LEDs
 */
void LED_on(uint8_t LED);

#endif // __ASSEMBLER__
#endif // LED_H_
