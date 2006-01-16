/*! @file 	adc.h
 * @brief 	Routinen zum Einlesen der AnalogeingÄnge
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/
#ifndef ADC_H_
#define ADC_H_

/*!
 * Liest einen analogen Kanal aus
 * @param channel Kanal - hex-Wertigkeit des Pins (0x01 f�r PA0; 0x02 f�r PA1, ..)
 */
int adc_read(char channel);

/*!
 *  Wechselt einen ADU-kanal. Dafür muessen auch die Puffer zurückgesetzt werden 
 * @param channel Kanal - hex-Wertigkeit des Pins (0x01 f�r PA0; 0x02 f�r PA1, ..)
 */
void adc_select_channel(char channel);

/*!
 * Diese Routine wird vom Timer-Interrupt aufgerufen und speichert einen 
 * Messwert. (vorher wendet sie evtl. noch eine Filterfkt an
 */
void adc_isr(void);

/*!
 * Initialisert den AD-Umsetzer. 
 * @param channel Für jeden Kanal, den man nutzen möchte, 
 * muss das entsprechende Bit in channel gesetzt sein
 * Bit0 = Kanal 0 usw.
 */
void adc_init(char channel);
#endif
