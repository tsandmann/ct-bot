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
 * @file 	rc5.h
 * @brief 	RC5-Fernbedienung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#ifndef rc5_H_
#define rc5_H_

/*!
 * @brief	Liest ein RC5-Codeword und wertet es aus
 */
void rc5_control(void);

/*!
 * @brief	Ordnet den Tasten eine Aktion zu und fuehrt diese aus.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007	  
 */
void default_key_handler(void);
#endif	// rc5_H_
