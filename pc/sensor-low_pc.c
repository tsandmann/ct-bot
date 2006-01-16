/*! @file 	bot-sens_pc.c  
 * @brief 	Low-Level Routinen für die Sensor Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include "sensor-low.h"

/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void){
}

/*!
 * Alle Sensoren aktualisieren
 * Das geschieht auf der PC Seite anders, in einem eigenen Thread
 * Diese Funktion ist nur ein Dummy zur Kompatibilität
 */
void bot_sens_isr(void){
}
#endif
