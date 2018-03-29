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

/**
 * \file 	uart.h
 * \brief 	Routinen zur seriellen Kommunikation
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#ifndef UART_H_
#define UART_H_

#ifdef MCU
#include <avr/io.h>
#include "fifo.h"

#define BAUD UART_BAUD // wird in bot-local.h definiert

#define BAUD_TOL 3
#include <util/setbaud.h>

#if BAUD == 115200 && ! defined BOT_2_RPI_AVAILABLE
#define UART_BUFSIZE_IN 0x50
#elif BAUD > 115200 || defined BOT_2_RPI_AVAILABLE
#define UART_BUFSIZE_IN 0xfe
#else // < 115200
#define UART_BUFSIZE_IN 0x45
#endif // BAUD

#define UART_BUFSIZE_OUT 0xfe

#if defined MCU_ATMEGA644X || defined __AVR_ATmega1284P__
/* Auf dem ATmega644(P) / ATmega1284P benutzen wir UART 0 */
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
#endif // MCU_ATMEGA644X || ATmega1284P

extern fifo_t uart_infifo;  /**< FIFO fuer Empfangspuffer */
extern fifo_t uart_outfifo; /**< Ausgangs-FIFO */

/**
 * Liest Zeichen vom UART
 * \param data		Der Zeiger an den die gelesenen Zeichen kommen
 * \param length	Anzahl der zu lesenden Bytes
 * \return			Anzahl der tatsaechlich gelesenen Zeichen
 */
static inline int16_t uart_read(void * data, int16_t length) {
	return fifo_get_data(&uart_infifo, data, length);
}

/**
 * Initialisiert den UART und aktiviert Receiver und Transmitter sowie den Receive-Interrupt.
 * Die Ein- und Ausgebe-FIFO werden initialisiert. Das globale Interrupt-Enable-Flag (I-Bit in SREG) wird nicht veraendert.
 */
void uart_init(void);

/**
 * Wartet, bis die Uebertragung fertig ist.
 */
static inline void uart_flush(void) {
	while (UCSRB & (1 << UDRIE));
}

/**
 * Prueft, ob Daten verfuegbar
 * \return	Anzahl der verfuegbaren Bytes
 */
#define uart_data_available() uart_infifo.count

static inline void uart_put(uint8_t data) {
	/* falls Sendepuffer zu voll, warten bis genug Platz vorhanden ist */
	while (UART_BUFSIZE_OUT == uart_outfifo.count) {}
	_inline_fifo_put(&uart_outfifo, data, 0);
	/* Interrupt an */
	UCSRB |= (1 << UDRIE);
}

#else // PC

/**
 * Initialisiert das UART
 * \param port Port des UARTs
 * \return Fehlercode oder 0, wenn alles OK
 */
uint8_t uart_init(const char * port);

/**
 * Initialisiert das UART mit der angegebenen Baudrate
 * \param port Port des UARTs
 * \param baudr gewuenschte Baudrate des UARTs
 * \return Fehlercode oder 0, wenn alles OK
 */
uint8_t uart_init_baud(const char * port, int baudr);

/**
 * Schliesst den UART-Port
 * @return Fehlercode oder 0, wenn alles OK
 */
uint8_t uart_close(void);

/**
 * Flusht das UART
 */
void uart_flush(void);

/**
 * Liest Zeichen vom UART
 * \param data		Der Zeiger an den die gelesenen Zeichen kommen
 * \param length	Anzahl der zu lesenden Bytes
 * \return			Anzahl der tatsaechlich gelesenen Zeichen
 */
int16_t uart_read(void * data, int16_t length);

/**
 * Prueft, ob Daten vom UART verfuegbar
 * \return	Anzahl der verfuegbaren Bytes oder -1 bei Fehler
 */
int16_t uart_data_available(void);

#ifdef ARM_LINUX_BOARD

void uart_test(uint32_t runs);
#endif // ARM_LINUX_BOARD

#endif // MCU

/**
 * Sendet Daten per UART im Little Endian
 * \param *data		Zeiger auf Datenpuffer
 * \param length	Groesse des Datenpuffers in Bytes
 * \return			Anzahl der geschriebenen Bytes
 */
int16_t uart_write(const void * data, int16_t length);

#endif // UART_H_
