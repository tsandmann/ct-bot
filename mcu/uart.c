/*! @file 	uart.c 
 * @brief 	Routinen zur seriellen Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include "ct-Bot.h"
#include "uart.h"

#ifdef UART_AVAILABLE

#define UART_RX_BUFFER_SIZE 16	/*!< Größe des UART-Puffers */

#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1 )
#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
	#error RX buffer size is not a power of 2
#endif

#define UART_TIMEOUT	20000	/*!< Timeout. Wartet UART_TIMEOUT CPU-Takte */

static unsigned char UART_RxBuf[UART_RX_BUFFER_SIZE];	/*!< UART-Puffer */
static volatile unsigned char UART_RxHead;				/*!< Zeiger für UART-Puffer */
static volatile unsigned char UART_RxTail;				/*!< Zeiger für UART-Puffer */

char uart_timeout;	/*!< 0, wenn uart_read/uart_send erfolgreich 1, wenn timeout erreicht */

/*!
 * Initialisiere UART
 */
void uart_init(void){
	DDRB|= 0x07;  // Multiplexleitungen seriell PB0-2 als Ausgang	
	PORTB|= 0x03; // UART verbinden mit COM
	
	
	UBRRH= 0x0;  // UART auf 9600 baud
	UBRRL= 0x5F; //  UBRR= (fquarz/ (16* BAUD) ) -1
	
	UCSRC=0x86; // 8 bit 1 Stop No Parity

	//Transmit&Receive Enable + RX Int
	UCSRB= ((1<<RXEN) | (1<<TXEN)|(1<<RXCIE)); 

	/* Flush receive buffer */

	UART_RxTail = 0;
	UART_RxHead = 0;
}

/*!
 *  Interrupt Handler for UART RECV 
 */
SIGNAL (SIG_UART_RECV){
	unsigned char data;
	unsigned char tmphead;
	
	data = UDR;                 /* Read the received data */

	/* Calculate buffer index */
	tmphead = ( UART_RxHead + 1 ) & UART_RX_BUFFER_MASK;
	UART_RxHead = tmphead;      /* Store new index */

	if ( tmphead == UART_RxTail ){
		/* ERROR! Receive buffer overflow */
	}
	
	UART_RxBuf[tmphead] = data; /* Store received data in buffer */
}

/*! 
 * Prüft, ob daten verfügbar 
 * @return 1, wenn daten verfügbar, sonst 0
 */
char uart_data_available(void){
	if (UART_RxHead == UART_RxTail) 
		return 0;
	else return 1;
}


/*!
 * Überträgt ein Zeichen per UART
 * Achtung ist noch blockierend!!!!
 * @param data Das Zeichen
 */
void uart_send(char data){ // Achtung ist noch blockierend!!!!
	while (((UCSRA >> UDRE) & 1) ==0){}	// warten bis UART sendebereit
	UDR= data;
}

/*!
 * Liest Zeichen von der UART
 */
char uart_read(char* data, int length){
        return -1;
}

#endif
#endif
