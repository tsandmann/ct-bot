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
 * \file 	rc5.h
 * \brief 	RC5-Fernbedienung / Basic-Tasten-Handler
 *
 * Um RC5-Codes fuer eine eigene Fernbedienung anzupassen, reicht es diese
 * in eine Header-Datei auszulagern und anstatt der rc5code.h einzubinden.
 * Die Maskierung fuer die Auswertung der Codes nicht vergessen!
 *
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	12.02.2007
 */

#ifndef RC5_H_
#define RC5_H_

#include "ct-Bot.h"

#ifdef RC5_AVAILABLE
#include "ir-rc5.h"

extern ir_data_t rc5_ir_data; /*!< RC5-Konfiguration fuer Fernbedienung */

/*!
 * Liest ein RC5-Codeword und wertet es aus
 */
void rc5_control(void);

/*!
 * Ordnet den Tasten eine Aktion zu und fuehrt diese aus
 */
void default_key_handler(void);

#endif // RC5_AVAILABLE
#endif // RC5_H_
