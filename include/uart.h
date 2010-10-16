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
 * @file 	uart.h
 * @brief 	Routinen zur seriellen Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.2005
 */

#ifndef UART_H_
#define UART_H_

#include "command.h"
#include "ct-Bot.h"
#ifdef MCU
#include <avr/io.h>
#include "fifo.h"

//#define BAUD	57600	/*!< Baudrate  57600 fuer UART-Kommunikation */
#define BAUD	115200	/*!< Baudrate 115200 fuer UART-Kommunikation */
//#define BAUD	500000	/*!< Baudrate 500000 fuer UART-Kommunikation */

#define BAUD_TOL 3
#include <util/setbaud.h>

#define BUFSIZE_IN	0x30
#define BUFSIZE_OUT	0x90

#if defined MCU_ATMEGA644X || defined __AVR_ATmega1284P__
/* Auf dem ATMega644(P) benutzen wir UART 0 */
#define UBRRH	UBRR0H
#define UBRRL	UBRR0L
#define UCSRA	UCSR0A
#define UCSRB	UCSR0B
#define UCSRC	UCSR0C
#define UDR		UDR0
#define RXEN	RXEN0
#define TXEN	TXEN0
#define RXCIE	RXCIE0
#define UDRIE	UDRIE0
#define UDRE	UDRE0
#define UCSZ0	UCSZ00
#define UCSZ1	UCSZ01
#define RXC		RXC0
#define TXC		TXC0
#define U2X		U2X0
#endif	// MCU_ATMEGA644X || ATmega1284P

extern fifo_t uart_infifo;	/*!< FIFO fuer Empfangspuffer */
extern fifo_t uart_outfifo;	/*!< Ausgangs-FIFO */

/*!
 * Sende Kommando per UART im Little Endian
 * @param cmd	Zeiger auf das Kommando
 * @return 	Anzahl der gesendete Bytes
 */
#define uart_send_cmd(cmd)  uart_write(cmd, sizeof(command_t));

/*!
 * Sendet Daten per UART im Little Endian
 * @param data		Datenpuffer
 * @param length	Groesse des Datenpuffers in Bytes
 */
void uart_write(void * data, uint8_t length);

/*!
 * Liest Zeichen von der UART
 * @param data		Der Zeiger an den die gelesenen Zeichen kommen
 * @param length	Anzahl der zu lesenden Bytes
 * @return			Anzahl der tatsaechlich gelesenen Zeichen
 */
#define uart_read(data, length)	fifo_get_data(&uart_infifo, data, length);

/*!
 * Initialisiert den UART und aktiviert Receiver und Transmitter sowie den Receive-Interrupt.
 * Die Ein- und Ausgebe-FIFO werden initialisiert. Das globale Interrupt-Enable-Flag (I-Bit in SREG) wird nicht veraendert.
 */
extern void uart_init(void);

/*!
 * Wartet, bis die Uebertragung fertig ist.
 */
static inline void uart_flush(void) {
	while (UCSRB & (1 << UDRIE));
}

/*!
 * Prueft, ob Daten verfuegbar
 * @return	Anzahl der verfuegbaren Bytes
 */
#define uart_data_available()	uart_infifo.count

#endif	// MCU
#endif	// UART_H_
