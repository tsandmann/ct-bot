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

	#define BAUDRATE	57600	/*!< Baudrate fuer UART-Kommunikation */
	//#define BAUDRATE	115200	/*!< Baudrate fuer UART-Kommunikation */
	#if BAUDRATE == 115200
		#define UART_DOUBLESPEED	// 2X-Mode, sonst Takt zu ungenau
	#endif
	
	#ifdef UART_DOUBLESPEED
		#define UART_CALC_BAUDRATE(baudRate) ((uint32_t)(F_CPU) / ((uint32_t)(baudRate) *8) -1)
	#else
		#define UART_CALC_BAUDRATE(baudRate) ((uint32_t)(F_CPU) / ((uint32_t)(baudRate)*16) -1)
	#endif

	#ifdef __AVR_ATmega644__
		/* Auf dem ATMega644 benutzen wir UART 0 */
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
	#endif	// __AVR_ATmega644__	
		
	/*!
	 * @brief		Sende Kommando per UART im Little Endian
	 * @param cmd	Zeiger auf das Kommando
	 * @return 	Anzahl der gesendete Bytes
	 */
	//int uart_send_cmd(command_t *cmd);
	#define uart_send_cmd(cmd)  uart_write((uint8*)cmd,sizeof(command_t));
	
	/*!
	 * @brief			Sendet Daten per UART im Little Endian
	 * @param data		Datenpuffer
	 * @param length	Groesse des Datenpuffers in Bytes
	 */
	void uart_write(uint8_t* data, uint8_t length);
	
	/*!
	 * @brief			Liest Zeichen von der UART
	 * @param data		Der Zeiger an den die gelesenen Zeichen kommen
	 * @param length	Anzahl der zu lesenden Bytes
	 * @return			Anzahl der tatsaechlich gelesenen Zeichen
	 */
	//int16 uart_read(uint8* data, int16 length);
	#define uart_read(data, length)	fifo_get_data(&infifo, data, length);
	
	/*!
	 * @brief	Initialisiert den UART und aktiviert Receiver und Transmitter sowie den Receive-Interrupt. 
	 * Die Ein- und Ausgebe-FIFO werden initialisiert. Das globale Interrupt-Enable-Flag (I-Bit in SREG) wird nicht veraendert.
	 */
	extern void uart_init(void);
	
	/*!
	 * @brief	Wartet, bis die Uebertragung fertig ist.
	 */
	static inline void uart_flush(void){
		while (UCSRB & (1 << UDRIE));
	}
	
	extern fifo_t infifo;	/*!< FIFO fuer Empfangspuffer */
	
	/*! 
 	 * @brief	Prueft, ob Daten verfuegbar 
 	 * @return	Anzahl der verfuegbaren Bytes
 	 */
	//uint8 uart_data_available(void);
	#define uart_data_available()	infifo.count

#endif	// MCU
#endif	// UART_H_
