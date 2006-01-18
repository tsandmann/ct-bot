/*! @file 	ir_pc.c
 * @brief 	Routinen für die Dekodierung von RC5-Fernbedienungs-Codes
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include "ir-rc5.h"
#include "command.h"
#include "bot-2-sim.h"

#ifdef IR_AVAILABLE

volatile uint16	ir_data	= 0;	///< letztes komplett gelesenes RC5-paket

/*!
 * IR-Daten lesen
 * @return wert von ir_data, löscht anschliessend ir_data
 */
uint16 ir_read(void) {
	uint16 retvalue = ir_data;
	ir_data = 0;
	return retvalue;
}

/*!
 * Init IR-System
 */
void ir_init(void) {
}
#endif
#endif
