/*
 * c't-Bot
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

/*!
 * @file 	ir-rc5_pc.c
 * @brief 	Routinen fuer die Dekodierung von RC5-Fernbedienungs-Codes
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
 */

#ifdef PC
#include "ct-Bot.h"

#ifdef IR_AVAILABLE
#include "ir-rc5.h"
#include "command.h"
#include "bot-2-sim.h"

/*!
 * IR-Daten lesen
 * @param *data Zeiger auf Arbeitsdaten
 * @return Wert von ir_data, loescht anschliessend ir_data
 */
uint16_t ir_read(ir_data_t * data) {
	uint16_t retvalue = data->ir_data;
	data->ir_data = 0;
	return retvalue;
}

#endif // IR_AVAILABLE
#endif // PC
