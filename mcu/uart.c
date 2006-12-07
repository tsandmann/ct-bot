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

#define BAUDRATE	57600

#define UART_RX_BUFFER_SIZE 16	/*!< Größe des UART-Puffers */

#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1 )
#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
	#error RX buffer size is not a power of 2
#endif

//#define UART_TIMEOUT	20000	/*!< Timeout. Wartet UART_TIMEOUT CPU-Takte */

static uint8 UART_RxBuf[UART_RX_BUFFER_SIZE];	/*!< UART-Puffer */
static volatile uint8 UART_RxHead;				/*!< Zeiger für UART-Puffer */
static volatile uint8 UART_RxTail;				/*!< Zeiger für UART-Puffer */

//char uart_timeout;	/*!< 0, wenn uart_read/uart_send erfolgreich 1, wenn timeout erreicht */

/*!
 * Initialisiere UART
 */
void uart_init(void){	 
	/* Senden und Empfangen ermöglichen + RX Interrupt an */
	UCSRB= (1<<RXEN) | (1<<TXEN)|(1<<RXCIE); 

	/* 8 Bit, 1 Stop, Keine Parity */
	UCSRC=0x86;
	
	/* UART auf 9600 baud */
//	UBRRH=0;
//	UBRRL= 103;  /* Werte stehen im Datenblatt tabelarisch */

	UBRRL = (uint8) (( ((uint32)F_CPU) / 16 / ((uint32)BAUDRATE) - 1) & 0xFF);
	UBRRH = (uint8) (( ((uint32)F_CPU) / 16 / ((uint32)BAUDRATE) - 1) >> 8);
	
	/* Puffer leeren */
	UART_RxTail = 0;
	UART_RxHead = 0;
}

/*!
 *  Interrupt Handler fuer den Datenempfang per UART
 */
SIGNAL (SIG_UART_RECV){

	/* Pufferindex berechnen */
	UART_RxHead++;						/* erhoehen */ 
	UART_RxHead %= UART_RX_BUFFER_MASK; /* Und bei Bedarf umklappen, da Ringpuffer */
	
	if (UART_RxHead == UART_RxTail){
		/* TODO Fehler behandeln !!
		 * ERROR! Receive buffer overflow */
	}
	UART_RxBuf[UART_RxHead] = UDR; /* Daten lesen und sichern*/	
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
 * Überträgt ein Zeichen per UART
 * Achtung ist noch blockierend!!!!
 * TODO: umstellen auf nicht blockierend und mehr als ein Zeichen
 * @param data Das Zeichen
 */
void uart_send_byte(uint8 data){ // Achtung ist noch blockierend!!!!
	while ((UCSRA & _BV(UDRE)) ==0){asm volatile("nop"); }	// warten bis UART sendebereit
	UDR= data;
}

/*!
 * Sende Kommando per UART im Little Endian
 * @param cmd Zeiger auf das Kommando
 * @return Anzahl der gesendete Bytes
 */
//#define uart_send_cmd(cmd)  uart_write(cmd,sizeof(command_t));

/* 
int uart_send_cmd(command_t *cmd){
	int i;
	char * ptr = (char*) cmd;
	for (i=0; i<sizeof(command_t); i++)
		uart_send_byte(*ptr++);
		
	return sizeof(command_t);
}
*/

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
