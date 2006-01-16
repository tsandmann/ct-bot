/*! @file 	ir.h
 * @brief 	Routinen für die Dekodierung von RC5-Fernbedienungs-Codes
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#ifndef ir_rc5_H_
#define ir_rc5_H_


#include "ct-Bot.h"
#include "global.h"


extern volatile uint16	ir_data;	///< letztes komplett gelesenes RC5-paket

/*!
 * Init IR-System
 */
void 	ir_init		(void);

/*!
 * IR-Daten lesen
 * @return wert von ir_data, löscht anschliessend ir_data
 */
uint16 	ir_read		(void);

/*!
 * Interrupt Serviceroutine
 * wird ca alle 177.8us aufgerufen
 */
void 	ir_isr		(void);
#endif
