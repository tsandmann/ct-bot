/*! @file 	bot-sens.h  
 * @brief 	Low-Level Routinen für die Sensor Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#include "global.h"

extern int16 sensDistL;				///< Distanz linker IR-Sensor
extern int16 sensDistR;				///< Distanz rechter IR-Sensor

extern volatile int16 encoderL;		///< Encoder linker Motor
extern volatile int16 encoderR;		///< Encoder rechter Motor

extern volatile int16 sensBorderL;	///< Abgrundsensor links
extern volatile int16 sensBorderR;	///<  Abgrundsensor rechts

extern volatile int16 sensLineL;	///<  Lininensensor links
extern volatile int16 sensLlineR;	///<  Lininensensor rechts

extern volatile int16 sensLdrL;		///< Helligkeitssensor links
extern volatile int16 sensLdrR;		///< Helligkeitssensor links

extern volatile char sensTrans;		///< Sensor Überwachung Transportfach

extern volatile char sensDoor;		///< Sensor Überwachung Klappe

extern volatile char sensError;		///< Überwachung Motor oder Batteriefehler

/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void);

/*!
 * Alle Sensoren aktualisieren
 */
void bot_sens_isr(void);
