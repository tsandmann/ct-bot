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

/*! @file 	uart.h 
 * @brief 	Routinen zur seriellen Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef UART_H_
#define UART_H_

#include "command.h"

//extern char uart_timeout;	/*!< 0, wenn uart_read/uart_send erfolgreich; 1, wenn timeout erreicht */

/*!
 * Initialisiere UART
 */
void uart_init(void);

/*! 
 * Prüft, ob daten verfügbar 
 * @return Anzahl der verfuegbaren Bytes
 */
char uart_data_available(void);


/*!
 * Überträgt ein Zeichen per UART
 * Achtung ist noch blockierend!!!!
 * TODO: umstellen auf nicht blockierend und mehr als ein Zeichen
 * @param data Das Zeichen
 */
void uart_send_byte(char data);

/*!
 * Sende Kommando per UART im Little Endian
 * @param cmd Zeiger auf das Kommando
 * @return Anzahl der gesendete Bytes
 */
//int uart_send_cmd(command_t *cmd);
#define uart_send_cmd(cmd)  uart_write(cmd,sizeof(command_t));

/*!
 * Sende Daten per UART im Little Endian
 * @param data Datenpuffer
 * @param length Groesse des Datenpuffers in bytes
 * @return Anzahl der gesendete Bytes
 */
int uart_write(char * data, int length);

/* Liest Zeichen von der UART
 * @param data Der Zeiger an die die gelesenen Zeichen kommen
 * @param length Anzahl der zu lesenden Bytes
 * @return Anzahl der tatsaechlich gelesenen Zeichen
 */
int uart_read(void* data, int length);
#endif
