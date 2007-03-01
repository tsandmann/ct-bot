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

/*! @file 	uart.c 
 * @brief 	Routinen zur seriellen Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
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

#ifdef UART_AVAILABLE

//#define UART_TX_BUFFER	// Ja, wir wollen es mal mit einem Sendepuffer probieren.
						// Achtung das kostet ordentlich RAM


#define UART_RX_BUFFER_SIZE  64	/*!< Größe des UART-Puffers */

#ifdef UART_TX_BUFFER
	#define UART_TX_BUFFER_SIZE 128	/*!< Größe des UART-Puffers */
#endif

#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1 )
#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
	#error RX buffer size is not a power of 2
#endif

#ifdef UART_TX_BUFFER
	#define UART_TX_BUFFER_MASK ( UART_TX_BUFFER_SIZE - 1 )
	#if ( UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK )
		#error TX buffer size is not a power of 2
	#endif
#endif

//#define UART_TIMEOUT	20000	/*!< Timeout. Wartet UART_TIMEOUT CPU-Takte */

static uint8 UART_RxBuf[UART_RX_BUFFER_SIZE];	/*!< UART-Puffer */
static volatile uint8 UART_RxHead;				/*!< Zeiger für UART-Puffer */
static volatile uint8 UART_RxTail;				/*!< Zeiger für UART-Puffer */

#ifdef UART_TX_BUFFER
	static uint8 UART_TxBuf[UART_TX_BUFFER_SIZE];	/*!< UART-Puffer */
	static volatile uint8 UART_TxHead;				/*!< Zeiger für UART-Puffer */
	static volatile uint8 UART_TxTail;				/*!< Zeiger für UART-Puffer */
#endif


//char uart_timeout;	/*!< 0, wenn uart_read/uart_send erfolgreich 1, wenn timeout erreicht */

/*!
 * Initialisiere UART
 */
void uart_init(void){	 
	#ifdef __AVR_ATmega644__
		/* Senden und Empfangen ermöglichen + RX Interrupt an */
		UCSR0B= (1<<RXEN0) | (1<<TXEN0)|(1<<RXCIE0); 			
		/* 8 Bit, 1 Stop, Keine Parity */
		UCSR0C=0x86;
	#else
		/* Senden und Empfangen ermöglichen + RX Interrupt an */
		UCSRB= (1<<RXEN) | (1<<TXEN)|(1<<RXCIE); 	
		/* 8 Bit, 1 Stop, Keine Parity */
		UCSRC=0x86;
	#endif
	
	/* UART auf 9600 baud */
//	UBRRH=0;
//	UBRRL= 103;  /* Werte stehen im Datenblatt tabelarisch */
	#ifdef __AVR_ATmega644__
		UBRR0L = (uint8) (( ((uint32)F_CPU) / 16 / ((uint32)BAUDRATE) - 1) & 0xFF);
		UBRR0H = (uint8) (( ((uint32)F_CPU) / 16 / ((uint32)BAUDRATE) - 1) >> 8);
	#else
		UBRRL = (uint8) (( ((uint32)F_CPU) / 16 / ((uint32)BAUDRATE) - 1) & 0xFF);
		UBRRH = (uint8) (( ((uint32)F_CPU) / 16 / ((uint32)BAUDRATE) - 1) >> 8);
	#endif
	
	/* Puffer leeren */
	UART_RxTail = 0;
	UART_RxHead = 0;
	
	#ifdef UART_TX_BUFFER
		/* Puffer leeren */
		UART_TxTail = 0;
		UART_TxHead = 0;
	#endif
}

/*!
 *  Interrupt Handler fuer den Datenempfang per UART
 */
#ifdef __AVR_ATmega644__
	SIGNAL (USART0_RX_vect){
#else
	SIGNAL (SIG_UART_RECV){
#endif

	/* Pufferindex berechnen */
	UART_RxHead++;						/* erhoehen */ 
	UART_RxHead %= UART_RX_BUFFER_MASK; /* Und bei Bedarf umklappen, da Ringpuffer */
	
	if (UART_RxHead == UART_RxTail){
		/* TODO Fehler behandeln !!
		 * ERROR! Receive buffer overflow */
	}
	#ifdef __AVR_ATmega644__
		UART_RxBuf[UART_RxHead] = UDR0; /* Daten lesen und sichern*/
	#else
		UART_RxBuf[UART_RxHead] = UDR; /* Daten lesen und sichern*/	
	#endif	
}

#ifdef UART_TX_BUFFER
	/*!
	 *  Interrupt Handler fuer den Datenversand per UART
	 */
	#ifdef __AVR_ATmega644__
		SIGNAL (USART0_TX_vect){
	#else
		SIGNAL (SIG_UART_TRANS){
	#endif

		if 	(UART_TxHead != UART_TxTail){ 	/* Puffer enthaelt mindestens ein Zeichen */
			#ifdef __AVR_ATmega644__
				UDR0= UART_TxBuf[UART_TxTail];
			#else
				UDR= UART_TxBuf[UART_TxTail];	
			#endif
	
			UART_RxTail++;		// Puffer erhoehen 
			UART_RxTail %= UART_RX_BUFFER_MASK;	// und bei Bedarf umklappen
		} else {
			#ifdef __AVR_ATmega644__
			// TODO wie geht das beim atmega644?
//				UCSR0B &= ~_BV(UDRIE); 		// TX_Buffer-Empty-Interrupt aus, da keine Daten da
			#else
				UCSRB &= ~_BV(UDRIE); 		// TX_Buffer-Empty-Interrupt aus, da keine Daten da
			#endif
		}
	}
#endif



#ifdef UART_TX_BUFFER
	/*!
	 * Überträgt ein Zeichen per UART
	 * blockiert nur, wenn der Sendepuffer voll ist !!!
	 * @param data Das Zeichen
	 * TODO Funktion fuer mehr als 1 Byte ==> schneller
	 */
	void uart_send_byte(uint8 data){
		uint8 oldindex=UART_TxHead;
		
		/* Pufferindex berechnen */
		UART_TxHead++;		/* alten Index erhoehen */ 
		UART_TxHead %= UART_TX_BUFFER_MASK; /* Und bei Bedarf umklappen, da Ringpuffer */
		
		while (UART_TxHead == UART_TxTail){
			// Warten, bis Platz im Puffer ist
		}

		UART_TxBuf[UART_TxHead] = data; /* Daten in den Puffersichern*/
		
		if (oldindex == UART_TxTail)	// war der Puffer vor dem Schreiben leer?
			#ifdef __AVR_ATmega644__
			// TODO wie geht das beim atmega644?
//				UCSR0B |= ~_BV(UDRIE); 		// TX_Buffer-Empty-Interrupt aus, da keine Daten da
			#else
				UCSRB |= _BV(UDRIE); 		// TX_Buffer-Empty-Interrupt an, da jetzt Daten da
			#endif
	}
	
#else	// Alter Code, wenn kein Tx-Puffer verfuegbar
	/*!
	 * Überträgt ein Zeichen per UART
	 * Achtung ist noch blockierend!!!!
	 * TODO: umstellen auf nicht blockierend und mehr als ein Zeichen
	 * @param data Das Zeichen
	 */
	void uart_send_byte(uint8 data){ // Achtung ist noch blockierend!!!!
		#ifdef __AVR_ATmega644__
			while ((UCSR0A & _BV(UDRE0)) ==0){asm volatile("nop"); }	// warten bis UART sendebereit
			UDR0= data;
		#else
			while ((UCSRA & _BV(UDRE)) ==0){asm volatile("nop"); }	// warten bis UART sendebereit
			UDR= data;	
		#endif
	}
#endif	// Ende von #else von #ifdef UART_TX_BUFFER

/*! 
 * Prüft, wieviel Platz im TX-Puffer ist
 * @return Anzahl der verfuegbaren Bytes
 */
uint8 uart_tx_space(void){
	#ifdef UART_TX_BUFFER
		if (UART_TxHead == UART_TxTail) 	/* Puffer leer */
			return 0;		
		else if (UART_TxHead > UART_TxTail)		/* Schreibzeiger vor Lesezeiger */ 
			return UART_TxHead - UART_TxTail; 
		else			/* Schreibzeiger ist schon umgelaufen */
			return UART_TxHead - UART_TxTail + UART_TX_BUFFER_SIZE;
	#else
		#ifdef __AVR_ATmega644__
			if ((UCSR0A & _BV(UDRE0)) ==0)
		#else
			if ((UCSRA & _BV(UDRE)) ==0)
		#endif
				return 0;
		return 1;	// Im TX-Register ist genau Platz fuer ein Byte
	#endif
}


/*!
 * Sende Daten per UART im Little Endian
 * @param data Datenpuffer
 * @param length Groesse des Datenpuffers in bytes
 * @return Anzahl der gesendete Bytes
 */
int uart_write(uint8 * data, int length){
	int i;
	char * ptr = (char*) data;
	for (i=0; i<length; i++)
		uart_send_byte(*ptr++);
		
	return length;	
}

/*! 
 * Prüft, ob daten verfügbar 
 * @return Anzahl der verfuegbaren Bytes
 */
uint8 uart_data_available(void){
	if (UART_RxHead == UART_RxTail) 	/* Puffer leer */
		return 0;		
	else if (UART_RxHead > UART_RxTail)		/* Schreibzeiger vor Lesezeiger */ 
		return UART_RxHead - UART_RxTail; 
	else			/* Schreibzeiger ist schon umgelaufen */
		return UART_RxHead - UART_RxTail + UART_RX_BUFFER_SIZE;
}

/*!
 * Liest Zeichen von der UART
 * @param data Der Zeiger an die die gelesenen Zeichen kommen
 * @param length Anzahl der zu lesenden Bytes
 * @return Anzahl der tatsaechlich gelesenen Zeichen
 */
int uart_read(void* data, int length){
	uint8 i;
	char* ptr = data;
	
	uint8 count= uart_data_available();

//	LOG_DEBUG(("%d/%d av/sel",count,length));
	
	if (count > length)
		count=length;
		
	for (i=0; i<count; i++){
		UART_RxTail++;
		UART_RxTail %= UART_RX_BUFFER_MASK;
		*ptr++ = UART_RxBuf[UART_RxTail];
		
	}
	
	return count;
}

#endif
#endif
