/*! @file 	led.h 
 * @brief 	Routinen zur LED-Steuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#define LED_GRUEN  0x40		///< LED Grüne
#define LED_GELB   0x20		///< LED Gelb
#define LED_ORANGE 0x10		///< LED Orange
#define LED_ROT    0x08		///< LED Rot
#define LED_ALL    0x78		///< LED Alle

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

