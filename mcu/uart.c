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
 * @file 	uart.c 
 * @brief 	Routinen zur seriellen Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author	Timo Sandmann (mail@timosandmann.de)
 * @date 	26.12.2005
 */

#ifdef MCU

#include "ct-Bot.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#ifndef NEW_AVR_LIB
	#include <avr/signal.h>
#endif
#include "ct-Bot.h"
#include "uart.h"
#include "command.h"
#include "log.h"
#include "bot-2-pc.h"

#ifdef UART_AVAILABLE

#define BUFSIZE_IN 0x30
uint8_t inbuf[BUFSIZE_IN];	/*!< Eingangspuffer */
fifo_t infifo;				/*!< Eingangs-FIFO */

#define BUFSIZE_OUT 0x80
uint8 outbuf[BUFSIZE_OUT];	/*!< Ausgangspuffer */
fifo_t outfifo;				/*!< Ausgangs-FIFO */

/*!
 * @brief	Initialisiert den UART und aktiviert Receiver und Transmitter sowie den Receive-Interrupt. 
 * Die Ein- und Ausgebe-FIFO werden initialisiert. Das globale Interrupt-Enable-Flag (I-Bit in SREG) wird nicht veraendert.
 */
void uart_init(void){	 
    uint8 sreg = SREG;
    UBRRH = (UART_CALC_BAUDRATE(BAUDRATE)>>8) & 0xFF;
    UBRRL = (UART_CALC_BAUDRATE(BAUDRATE) & 0xFF);
    
	/* Interrupts kurz deaktivieren */ 
	cli();

	/* UART Receiver und Transmitter anschalten, Receive-Interrupt aktivieren */ 
	UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
	/* Data mode 8N1, asynchron */
	uint8 ucsrc = (1 << UCSZ1) | (1 << UCSZ0);
	#ifdef URSEL 
		ucsrc |= (1 << URSEL);	// fuer ATMega32
	#endif    
	UCSRC = ucsrc;

    /* Flush Receive-Buffer (entfernen evtl. vorhandener ungueltiger Werte) */ 
    do{
		UDR;	// UDR auslesen (Wert wird nicht verwendet)
    } while (UCSRA & (1 << RXC));

    /* Ruecksetzen von Receive und Transmit Complete-Flags */ 
    UCSRA = (1 << RXC) | (1 << TXC)
#ifdef UART_DOUBLESPEED
    		| (1<<U2X)
#endif
    		;
	
    /* Global Interrupt-Flag wiederherstellen */
    SREG = sreg;

    /* FIFOs für Ein- und Ausgabe initialisieren */ 
    fifo_init(&infifo, inbuf, BUFSIZE_IN);
    fifo_init(&outfifo, outbuf, BUFSIZE_OUT);
}

/*!
 * @brief	Interrupthandler fuer eingehende Daten
 * Empfangene Zeichen werden in die Eingabgs-FIFO gespeichert und warten dort.
 */ 
#ifdef __AVR_ATmega644__
	SIGNAL (USART0_RX_vect){		
#else
	SIGNAL (SIG_UART_RECV){
#endif
	UCSRB &= ~(1 << RXCIE);	// diesen Interrupt aus (denn ISR ist nicht reentrant)
	sei();					// andere Interrupts wieder an
	if (infifo.count == BUFSIZE_IN){   
		/* ERROR! Receive buffer full!
		 * => Pufferinhalt erst verarbeiten - das funktioniert besser als es aussieht. ;-)
		 * Ist allerdings nur dann clever, wenn das ausgewertete Command nicht mehr Daten per
		 * uart_read() lesen will, als bereits im Puffer sind, denn der Interrupt ist ja aus... */
		#ifdef BOT_2_PC_AVAILABLE
			bot_2_pc_listen();		// Daten des Puffers auswerten
		#endif
	}
	_inline_fifo_put(&infifo, UDR);
	UCSRB |= (1 << RXCIE);	// diesen Interrupt wieder an 	
}

/*!
 * @brief	Interrupthandler fuer ausgehende Daten
 * Ein Zeichen aus der Ausgabe-FIFO lesen und ausgeben.
 * Ist das Zeichen fertig ausgegeben, wird ein neuer SIG_UART_DATA-IRQ getriggert.
 * Ist die FIFO leer, deaktiviert die ISR ihren eigenen IRQ.
 */ 
#ifdef __AVR_ATmega644__
	SIGNAL (USART0_UDRE_vect){
#else
	SIGNAL (SIG_UART_DATA){
#endif
	UCSRB &= ~(1 << UDRIE);	// diesen Interrupt aus (denn ISR ist nicht reentrant)
	sei();					// andere Interrupts wieder an
	if (outfifo.count > 0){
		UDR = _inline_fifo_get(&outfifo);
		UCSRB |= (1 << UDRIE);	// diesen Interrupt wieder an 
	}	
}

/*!
 * @brief			Sendet Daten per UART im Little Endian
 * @param data		Datenpuffer
 * @param length	Groesse des Datenpuffers in Bytes
 */
void uart_write(uint8* data, uint8 length){
	if (length > BUFSIZE_OUT){
		/* das ist zu viel auf einmal => teile und herrsche */
		uart_write(data, length/2);
		uart_write(data + length/2, length - length/2);
		return;
	} 
	/* falls Sendepuffer voll, diesen erst flushen */ 
	uint8 space = BUFSIZE_OUT - outfifo.count;
	if (space < length) uart_flush();
	/* Daten in Ausgangs-FIFO kopieren */
	fifo_put_data(&outfifo, data, length);
	/* Interrupt an */
	UCSRB |= (1 << UDRIE);
}

#endif	// UART_AVAILABLE
#endif	// MCU
