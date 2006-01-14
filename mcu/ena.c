/*! @file 	ena.c 
 * @brief 	Routinen zur Steuerung der Enable-Leitungen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU 

#include <avr/io.h>
#include "ena.h"
#include "ct-Bot.h"
#include "shift.h"

#ifdef ENA_AVAILABLE


volatile char ena =0;	///< Sichert den Zustand der Enable-Leitungen

/*!
 * Initialisiert die Enable-Leitungen
 */
void ENA_init(){
	shift_init();
	ENA_set(0x00);
}

/*! 
 * Schaltet einzelne Enable-Transistoren an
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_on schaltet einen Transistor durch
 * Daher zieht es die entsprechende ENA_XXX-Leitung auf Low und NICHT auf High
 * @param enable Bitmaske der anzuschaltenden LEDs
 */
void ENA_on(char enable){
	ena |= enable;
	ENA_set(ena);
}

/*! 
 * Schaltet einzelne Enable-Transistoren aus
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_off schaltet einen Transistor ab
 * Daher zieht es die entsprechende ENA_XXX-Leitung auf High und NICHT auf Low
 * @param enable Bitmaske der anzuschaltenden LEDs
 */
void ENA_off(char enable){
	ena &= ~enable;
	ENA_set(ena);
}

/*!
 * Schaltet die Enable-Transistoren
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_set bezieht sich auf die Transistor
 * Daher zieht es die entsprechende ENA_XXX-Leitung auf ~enable
 * @param LED Wert der gezeigt werden soll
 */
void ENA_set(char enable){
	ena=enable;
	shift_data(~enable,SHIFT_REGISTER_ENA); 
}

#endif
#endif
