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
 * @file 	twi.h
 * @brief 	TWI Treiber
 * @author 	Chris efstathiou hendrix@otenet.gr & Carsten Giesen (info@cnau.de)
 * @date 	11.04.2006
 */

#ifndef TWI_driver_H
#define TWI_driver_H

#include "ct-Bot.h"
#ifdef TWI_AVAILABLE 
#include "global.h"
#include "i2c.h"

/*!
 * Struktur Definition 
 * tx_type_t ist eine Datenstruktur um den TWI-Treiber anzusprechen 
 * und behinhaltet folgende Informationen:
 * Slave Adresse + Datenrichtung
 * Anzahl der zu uebertragendenden Bytes (Senden oder Empfangen)
 * Pointer auf den Sende- oder Empfangspuffer
 */
typedef struct {
	uint8_t slave_adr;	/*!< Slave Adresse and W/R byte */
	uint8_t size;		/*!< Anzahl der Bytes, die gesendet oder empfagen werden sollen */
	uint8_t * data_ptr;	/*!< Pointer zum Sende und Empfangs Puffer */
} tx_type_t;							
	
/*!
 * Datentransfer per I2C-Bus
 * @param *pData	Container mit den Daten fuer den Treiber
 * @return 			Resultat der Aktion
 */
uint8_t Send_to_TWI(tx_type_t * pData);

/*!
 * TWI Bus initialsieren
 */
#define Init_TWI()	i2c_init(12)

#define W 				0			/*!< Daten Transfer Richtung Schreiben */
#define R 				1			/*!< Daten Transfer Richtung Lesen */
#define OWN_ADR 		60  		/*!< Die eigene Slave Adresse */
#define SUCCESS 		0xFF		/*!< Status Code alles OK */


/* TWI Stautus Register Definitionen */

/*! Genereller Master Statuscode */
#define START		0x08					/*!< START wurde uebertragen */	
#define	REP_START	0x10					/*!< Wiederholter START wurde uebertragen */
/*! Master Sender Statuscode	 */										
#define	MTX_ADR_ACK		0x18				/*!< SLA+W wurde uebertragen und ACK empfangen */
#define	MTX_ADR_NACK	0x20				/*!< SLA+W wurde uebertragen und NACK empfangen */
#define	MTX_DATA_ACK	0x28				/*!< Datenbyte wurde uebertragen und ACK empfangen */			
#define	MTX_DATA_NACK	0x30				/*!< Datenbyte wurde uebertragen und NACK empfangen */		
#define	MTX_ARB_LOST	0x38				/*!< Schlichtung verloren in SLA+W oder Datenbytes */
/*! Master Empfaenger Statuscode  */
#define	MRX_ARB_LOST	0x38				/*!< Schlichtung verloren in SLA+R oder NACK bit */
#define	MRX_ADR_ACK		0x40				/*!< SLA+R wurde uebertragen und ACK empfangen */
#define	MRX_ADR_NACK	0x48				/*!< SLA+R wurde uebertragen und NACK empfangen */	
#define	MRX_DATA_ACK	0x50				/*!< Datenbyte wurde empfangen und ACK gesendet */
#define	MRX_DATA_NACK	0x58				/*!< Datenbyte wurde empfangen und NACK gesendet */

#endif	// TWI_AVAILABLE
#endif	// TWI_driver_H
