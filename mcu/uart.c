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
 * \file 	uart.c
 * \brief 	Routinen zur seriellen Kommunikation
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	26.12.2005
 */

#ifdef MCU

#include "ct-Bot.h"

#ifdef UART_AVAILABLE
#include "uart.h"
#include "os_thread.h"
#include <avr/io.h>

uint8_t inbuf[UART_BUFSIZE_IN]; /**< Eingangspuffer */
fifo_t uart_infifo; /**< Eingangs-FIFO */

uint8_t outbuf[UART_BUFSIZE_OUT]; /**< Ausgangspuffer */
fifo_t uart_outfifo; /**< Ausgangs-FIFO */

/**
 * Initialisiert den UART und aktiviert Receiver und Transmitter sowie die Interrupts.
 * Die Ein- und Ausgebe-FIFO werden initialisiert. Das globale Interrupt-Enable-Flag (I-Bit in SREG) wird nicht veraendert.
 */
void uart_init(void) {
    /* FIFOs fuer Ein- und Ausgabe initialisieren */
    fifo_init(&uart_infifo, inbuf, UART_BUFSIZE_IN);
    fifo_init(&uart_outfifo, outbuf, UART_BUFSIZE_OUT);

#ifdef ENABLE_RX0_PULLUP
    PORTD |= PIND0; // Pullup an
#endif

	/* Interrupts kurz deaktivieren */
    uint8_t sreg = SREG;
    __builtin_avr_cli();

    UBRRH = UBRRH_VALUE;
    UBRRL = UBRRL_VALUE;

	/* UART Receiver und Transmitter anschalten, Receive-Interrupt aktivieren */
	UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
	/* Data mode 8N1, asynchron */
	uint8_t ucsrc = (1 << UCSZ1) | (1 << UCSZ0);
#ifdef URSEL
	ucsrc |= (1 << URSEL); // fuer ATMega32
#endif
	UCSRC = ucsrc;

    /* Flush Receive-Buffer (entfernen evtl. vorhandener ungueltiger Werte) */
    do {
		(void) UDR; // UDR auslesen (Wert wird nicht verwendet)
    } while (UCSRA & (1 << RXC));

    /* Ruecksetzen von Receive und Transmit Complete-Flags */
    UCSRA = (1 << RXC) | (1 << TXC)
#if USE_2X
    		| (1 << U2X)
#endif
    		;

    /* Global Interrupt-Flag wiederherstellen */
    SREG = sreg;
}

/**
 * Interrupthandler fuer eingehende Daten
 * Empfangene Zeichen werden in die Eingabgs-FIFO gespeichert und warten dort.
 */
#if defined MCU_ATMEGA644X || defined __AVR_ATmega1284P__
	ISR(USART0_RX_vect) {
#else
	ISR(USART_RXC_vect) {
#endif // MCU_ATMEGA644X || ATmega1284P
	_inline_fifo_put(&uart_infifo, UDR, True);
}

/**
 * Interrupthandler fuer ausgehende Daten
 * Ein Zeichen aus der Ausgabe-FIFO lesen und ausgeben.
 * Ist das Zeichen fertig ausgegeben, wird ein neuer SIG_UART_DATA-IRQ getriggert.
 * Ist die FIFO leer, deaktiviert die ISR ihren eigenen IRQ.
 */
#if defined MCU_ATMEGA644X || defined __AVR_ATmega1284P__
	ISR(USART0_UDRE_vect) {
#else
	ISR(USART_UDRE_vect) {
#endif // MCU_ATMEGA644X || ATmega1284P
	if (uart_outfifo.count > 0) {
		UDR = _inline_fifo_get(&uart_outfifo, True);
	} else {
		UCSRB = (uint8_t) (UCSRB & ~(1 << UDRIE)); // diesen Interrupt aus
	}
}

/**
 * Sendet Daten per UART im Little Endian
 * \param *data		Zeiger auf Datenpuffer
 * \param length		Groesse des Datenpuffers in Bytes
 * \return			Anzahl der geschriebenen Bytes
 */
int16_t uart_write(const void * data, int16_t length) {
	if (length > UART_BUFSIZE_OUT) {
		/* das ist zu viel auf einmal => teile und herrsche */
		uart_write(data, length / 2);
		uart_write(data + length / 2, length - length / 2);
		return length;
	}

	/* falls Sendepuffer zu voll, warten bis genug Platz vorhanden ist */
	while (UART_BUFSIZE_OUT - uart_outfifo.count < length) {}

	/* Daten in Ausgangs-FIFO kopieren */
	if (fifo_put_data(&uart_outfifo, data, (uint8_t) length, False)) {
		return 0;
	}

	/* Interrupt an */
	UCSRB |= (1 << UDRIE);

	return length;
}

#endif // UART_AVAILABLE
#endif // MCU
