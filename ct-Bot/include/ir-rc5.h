/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	ir-rc5.h
 * @brief 	Routinen fuer die Dekodierung von RC5-Fernbedienungs-Codes
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#ifndef ir_rc5_H_
#define ir_rc5_H_


#include "ct-Bot.h"
#include "global.h"


extern volatile uint16	ir_data;	/*!< letztes komplett gelesenes RC5-Paket */

/*!
 * Init IR-System
 */
void 	ir_init		(void);

/*!
 * IR-Daten lesen
 * @return Wert von ir_data, loescht anschliessend ir_data
 */
uint16 	ir_read		(void);

/*!
 * Interrupt Serviceroutine,
 * wird ca. alle 177.8 �s aufgerufen
 */
void 	ir_isr		(void);
#endif
