/*! @file 	led.h 
 * @brief 	Routinen zur LED-Steuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#define LED_RECHTS	(1<<0)
#define LED_LINKS	(1<<1)
#define LED_ROT     (1<<2)		///< LED Rot
#define LED_ORANGE  (1<<3)		///< LED Orange
#define LED_GELB    (1<<4)		///< LED Gelb
#define LED_GRUEN   (1<<5)		///< LED Grüne
#define LED_TUERKIS (1<<6)		///< LED Tuerkis
#define LED_WEISS   (1<<7)		///< LED WEISS

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

/*! Schaltet eine LEd aus
 * 
 * @param LED HEX-Code der LED
 */
void LED_off(char LED);

/*! Schaltet eine LEd an
 * 
 * @param LED HEX-Code der LED
 */
void LED_on(char LED);

