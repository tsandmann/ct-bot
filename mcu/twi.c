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
 * @file 	twi.c
 * @brief 	TWI-Treiber (I2C)
 * @author 	Chris efstathiou hendrix@otenet.gr & Carsten Giesen (info@cnau.de)
 * @date 	08.04.2006
 */

#ifdef MCU
#include "ct-Bot.h"
#ifdef TWI_AVAILABLE
#include "twi.h"
#include "global.h"

//#define OLD_VERSION

#ifdef OLD_VERSION
#warning "veraltete Version, besser i2c-Treiber verwenden"

#include <avr/io.h>

static uint8_t Send_adr(uint8_t adr);
static uint8_t Get_byte(uint8_t * rx_ptr, uint8_t last_byte);
static uint8_t Send_byte(uint8_t data);

/*!
 * TWI Buss schliesen
 */
static void Close_TWI(void) {
	TWCR = 0;	/*!< TWI-Interface ausschalten */
}

/*!
 * Warte auf TWI interrupt
 */
static void Wait_TWI_int(void) {
	while (!(TWCR & (1<<TWINT)))
	    ;
}

/*!
 * Sende Start Sequence
 * @return Resultat der Aktion
 */
static uint8_t Send_start(void) {
	TWCR = ((1<<TWINT)+(1<<TWSTA)+(1<<TWEN)); 		/*!< Sende START */

	Wait_TWI_int();									/*!< Warte auf TWI interrupt */

    if((TWSR != START)&&(TWSR != REP_START))		/*!< Ist der Status ein Anderer als Start (0x08) oder wiederholter Start (0x10) */
		return TWSR;								/*!< -> error  und Rueckgabe TWSR. */
	return SUCCESS;									/*!< wenn OK Rueckgabe SUCCESS */
}

/*!
 * Sende Stop Sequence
 */
static void Send_stop(void) {
	TWCR = ((1<<TWEN)+(1<<TWINT)+(1<<TWSTO));
}

/*!
 * Hier wird der eigentliche TWI-Treiber angesprochen
 * @param *data_pack Container mit den Daten fuer den Treiber
 * @return Resultat der Aktion
 */
uint8_t Send_to_TWI(tx_type_t * data_pack) {
	uint8_t state,i,j;

	state = SUCCESS;

	for(i=0;(data_pack[i].slave_adr != OWN_ADR)&&(state == SUCCESS);i++) {
		state = Send_start();
		if (state == SUCCESS)
			state = Send_adr(data_pack[i].slave_adr);

		/*!
		 * Abhaengig von W/R senden oder empfangen
		 */
		if(!(data_pack[i].slave_adr & R)) {
			if (state == SUCCESS){
				/*!
				 * Wenn W bis alle Daten gesendet sind
				 */
				for(j=0;((j<data_pack[i].size)&&(state == SUCCESS));j++)
					state = Send_byte(data_pack[i].data_ptr[j]);
			}
		}
		else{
			if (state == MRX_ADR_NACK)	{
				state = Send_start();
			}

			if (state == SUCCESS){
				/*!
				 * Wenn R bis alle Daten empfangen sind
				 */
				for(j=0;((j<data_pack[i].size)&&(state == SUCCESS));j++){
					/*!
					 * Wenn wir keine Daten mehr erwarten NACK senden
					 */
					if(j == data_pack[i].size-1)
						state = Get_byte(data_pack[i].data_ptr++,0);
					else
						state = Get_byte(data_pack[i].data_ptr++,1);
				}
			}
		}
	  	Send_stop();
	}
 	Close_TWI();

	return state;
}

/*!
 * Sende ein Byte
 * @param data das zu uebertragende Byte
 */
static uint8_t Send_byte(uint8_t data) {
	Wait_TWI_int();
	TWDR = data;
 	TWCR = ((1<<TWINT)+(1<<TWEN));
	Wait_TWI_int();
	if(TWSR != MTX_DATA_ACK)
		return TWSR;
	return SUCCESS;
}

/*!
 * Sende Slave Adresse
 * @param adr die gewuenschte Adresse
 * @return Resultat der Aktion
 */
static uint8_t Send_adr(uint8_t adr) {
	Wait_TWI_int();
	TWDR = adr;
	TWCR = ((1<<TWINT)+(1<<TWEN));
	Wait_TWI_int();
	if((TWSR != MTX_ADR_ACK)&&(TWSR != MRX_ADR_ACK))
		return TWSR;
	return SUCCESS;
}

/*!
 * Empfange ein Byte
 * @param *rx_ptr Container fuer die Daten
 * @param last_byte Flag ob noch Daten erwartet werden
 * @return Resultat der Aktion
 */
static uint8_t Get_byte(uint8_t * rx_ptr, uint8_t last_byte) {
	Wait_TWI_int();
	if(last_byte)
		TWCR = ((1<<TWINT)+(1<<TWEA)+(1<<TWEN));
	else
		TWCR = ((1<<TWINT)+(1<<TWEN));
	Wait_TWI_int();
	*rx_ptr = TWDR;
 	if(((TWSR == MRX_DATA_NACK)&&(last_byte == 0))||(TWSR == MRX_DATA_ACK))
		return SUCCESS;
	return TWSR;
}

#else

/*!
 * Datentransfer per I2C-Bus
 * @param *pData	Container mit den Daten fuer den Treiber
 * @return 			Resultat der Aktion
 */
uint8_t Send_to_TWI(tx_type_t * pData) {
	uint8_t toRead = 0;
	uint8_t toWrite = 0;
	uint8_t * pWrite = NULL;
	uint8_t * pRead = NULL;
	tx_type_t * ptr = pData;
	if (ptr == NULL) return TW_BUS_ERROR;

	/* Datenformat einlesen */
	for (; ptr->slave_adr != OWN_ADR; ptr++) {
		if ((ptr->slave_adr & 0x1) == 1) {
			/* zu lesende Daten */
			toRead = ptr->size;
			pRead = ptr->data_ptr;
		} else {
			/* zu sendende Daten */
			toWrite = ptr->size;
			pWrite = ptr->data_ptr;
		}
	}

	/* Daten senden und empfangen per i2c-Treiber (blockierend) */
	i2c_write_read(pData->slave_adr, pWrite, toWrite, pRead, toRead);
	uint8_t result = i2c_wait();
	return (uint8_t) (result == TW_NO_INFO ? SUCCESS : result);
}
#endif	// OLD_VERSION

#endif	// TWI_AVAILABLE
#endif	// MCU
