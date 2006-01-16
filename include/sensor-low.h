/*! @file 	bot-sens.h  
 * @brief 	Low-Level Routinen für die Sensor Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/
#ifndef sens_low_H_
#define sens_low_H_

/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void);

/*!
 * Alle Sensoren aktualisieren
 */
void bot_sens_isr(void);
#endif
