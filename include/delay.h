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
 * @file 	delay.h
 * @brief 	Hilfsroutinen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
 */
#ifndef delay_H_
#define delay_H_


/*!
 * Warte 100 ms
 */
void delay_100ms(void);

/*!
 * Verzoegert um ms Millisekunden
 * Wenn RTC_AVAILABLE, dann ueber rtc, sonst ueber delay_100ms
 * ==> Aufloesung ohne rtc: 100-ms-schritte; mit rtc: 5-ms-Schritte
 * @param ms Anzahl der Millisekunden
 */
void delay(uint16_t ms);
#endif	// delay_H_
