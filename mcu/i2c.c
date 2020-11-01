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
 * \file 	i2c.c
 * \brief 	I2C-Treiber, derzeit nur Master, interruptbasiert
 * \author 	Timo Sandmann
 * \date 	05.09.2007
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef I2C_AVAILABLE
#include <util/twi.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "i2c.h"
#include "timer.h"
#include "log.h"


//#define I2C_DEBUG
#define I2C_PRESCALER 0					/**< Prescaler fuer I2C-CLK */

static uint8_t sl_addr;					/**< Adresse des Slaves */
static const uint8_t* pTxData;			/**< Zeiger auf Puffer fuer Datenversand */
static uint8_t* pRxData;				/**< Zeiger auf Puffer fuer Datenempfang */
static uint8_t* pRxEnd;
static uint8_t txSize;					/**< Anzahl der zu sendenden Datenbytes */
static uint8_t rxSize;					/**< Anzahl der zu lesenden Datenbytes */
static uint8_t i2c_error;				/**< letzter Bus-Fehler */
static volatile uint8_t i2c_complete;	/**< Spin-Lock; 0: Transfer aktiv, 128: idle */
#ifdef I2C_DEBUG
static uint8_t rx_overflow;
#endif


/**
 * ISR fuer I2C-Master
 */
ISR(TWI_vect) {
	const uint8_t state = TWSR & 0xf8;
	switch (state) {
		/* Start gesendet */
		case TW_START:
			CASE_NO_BREAK;
		case TW_REP_START: {
			if (txSize > 0) {
				/* Adresse+W senden */
				TWDR = (uint8_t) (sl_addr & 0xfe); // W
			} else {
				/* Adresse+R senden */
				TWDR = sl_addr | 0x1; // R
			}
			TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
			break;
		}
		/* Datum versandt, ACK empfangen */
		case TW_MT_DATA_ACK: {
			if (txSize == 0) {
				if (rxSize == 0) {
					/* Stopp Senden und beenden */
					TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO); // Stopp senden
					i2c_complete = 128;	// Lock freigeben
					pRxData = NULL;
					break;
				}
 				/* ReStart senden */
				TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA); // ReStart senden
				break;
			}
			CASE_NO_BREAK;
		}
		/* SLA+W, ACK empfangen */
		case TW_MT_SLA_ACK: {
			/* Daten senden */
			TWDR = *pTxData;
			pTxData++;
			txSize--;
			TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
			break;
		}
		/* SLA+R, ACK empfangen */
		case TW_MR_SLA_ACK: {
			/* Auf Daten warten */
			if (rxSize > 1) {
				TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA); // ACK senden
			} else {
				TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE); // NACK senden
			}
			break;
		}
		/* Datum empfangen, ACK gesendet */
		case TW_MR_DATA_ACK: {
			const uint8_t tmp = TWDR;
			/* Daten speichern */
			if (pRxData && pRxData < pRxEnd) {
				*pRxData = tmp;
				pRxData++;
#ifdef I2C_DEBUG
			} else {
				rx_overflow = 1;
#endif
			}
			rxSize--;
			if (rxSize > 1) {
				/* es folgen noch weitere Daten, ACK senden */
				TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA); // ACK senden
			} else {
				/* das letzte Byte folgt */
				TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE); // NACK senden
			}
			break;
		}
		/* Letztes Datum empfangen, NACK gesendet */
		case TW_MR_DATA_NACK: {
			const uint8_t tmp = TWDR;
			if (pRxData && pRxData < pRxEnd) {
				/* Letztes Datum speichern */
				*pRxData = tmp;
#ifdef I2C_DEBUG
			} else {
				rx_overflow = 1;
#endif
			}
			TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO); // Stopp-Code senden
			i2c_complete = 128; // Lock freigeben
			pRxData = NULL;
			break;
		}
		/* Fehler */
		default: {
			/* Abbruch */
			i2c_error = state;
			i2c_complete = 128; // Lock freigeben
			pRxData = NULL;
			TWCR = _BV(TWINT); // Int zuruecksetzen und I2C aus
		}
	}
}

/**
 * Initialisiert das I2C-Modul
 * \param bitrate	Init-Wert fuer Bit Rate Register (TWBR)
 */
void i2c_init(uint8_t bitrate) {
	TWBR = bitrate;
	TWSR = I2C_PRESCALER; // Prescaler
	TWCR = 0; // I2C aus
	DDRC |= _BV(0) | _BV(1);
#ifndef SHIFT_AVAILABLE
	PORTC |= _BV(0) | _BV(1);
#endif
	i2c_error = TW_NO_INFO;
	i2c_complete = 128;
}

/**
 * Sendet nTx Bytes an einen I2C-Slave und liest anschliessend nRx Bytes
 * \param sla	Slave-Adresse
 * \param *pTx	Zeiger auf Puffer fuer zu sendende Daten
 * \param nTx	Anzahl der zu sendenden Bytes, [1; 255]
 * \param *pRx	Zeiger auf Puffer fuer zu lesende Daten
 * \param nRx	Anzahl der zu lesenden Bytes, [0; 255]
 */
void i2c_write_read(uint8_t sla, const void* pTx, uint8_t nTx, void* pRx, uint8_t nRx) {
	// LOG_DEBUG("i2c_write_read(0x%x, 0x%x, %u, 0x%x, %u)", sla, (uint16_t) pTx, nTx, (uint16_t) pRx, nRx);

	if (i2c_complete != 128) {
		return;
	}
	/* Inits */
	i2c_complete = 0;
	i2c_error = TW_NO_INFO;
	sl_addr = sla;
	pTxData = pTx;
	txSize = nTx;
	pRxData = pRx;
	rxSize = nRx;
	pRxEnd = pRxData + rxSize;
#ifdef I2C_DEBUG
	rx_overflow = 0;
#endif
	/* Start-Code senden */
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
}

/**
 * Sendet ein Byte an einen I2C-Slave und liest anschliessend nRx Bytes
 * \param sla		Slave-Adresse
 * \param txData	Byte, das zunaechst an den Slave gesendet wird
 * \param *pRx		Zeiger auf Puffer fuer zu lesende Daten
 * \param nRx		Anzahl der zu lesenden Bytes
 */
void i2c_read(uint8_t sla, uint8_t txData, void* pRx, uint8_t nRx) {
	static uint8_t data;
	// LOG_DEBUG("i2c_read(0x%x, %u, 0x%x, %u)", sla, txData, (uint16_t) pRx, nRx);

	data = txData;
	i2c_write_read(sla, &data, 1, pRx, nRx);
}

/**
 * Wartet, bis der aktuelle I2C-Transfer beendet ist
 * \return TW_NO_INFO (0xf8) falls alles ok, sonst Fehlercode
 */
uint8_t i2c_wait(void) {
	uint8_t ticks = TIMER_GET_TICKCOUNT_8;
	/* wait while i2c busy */
	while (i2c_complete == 0) {
		if (timer_ms_passed_8(&ticks, 3)) {
			/* Timeout */
			TWCR = _BV(TWINT); // I2C aus
			pRxData = NULL;
			i2c_complete = 128;
			return TW_BUS_ERROR;
		}
	}

 	while (TWCR & _BV(TWSTO)) {}
#ifdef SHIFT_AVAILABLE
	_delay_us(10);
	i2c_off();
#endif
	i2c_complete = 128;
#ifdef I2C_DEBUG
	if (rx_overflow) {
		return 0xfe;
	}
#endif
	return i2c_error;
}
#endif // I2C_AVAILABLE
#endif // MCU
