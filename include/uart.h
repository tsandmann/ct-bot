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

/*! @file 	uart.c 
 * @brief 	Routinen zur seriellen Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef UART_H_
#define UART_H_


extern char uart_timeout;	///< 0, wenn uart_read/uart_send erfolgreich; 1, wenn timeout erreicht

/*!
 * Uebertraegt ein Zeichen per UART.
 * Achtung: ist noch blockierend!
 * @param data Das Zeichen
 */
void uart_send(char data); // Achtung, ist noch blockierend!!!!

/*! 
 * Prueft, ob Daten verfuegbar 
 * @return 1, wenn Daten verfuegbar, sonst 0
 */
char uart_data_available(void);

/*!
 * Initialisiere UART
 */
void uart_init(void);

/*!
 * Liest Zeichen von der UART
 */
char uart_read(char* data, int length);
#endif
