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
 * \file 	i2c.h
 * \brief 	I2C-Treiber, derzeit nur Master, interruptbasiert
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	05.09.2007
 */


#ifndef I2C_H_
#define I2C_H_

#ifdef I2C_AVAILABLE
#include <stddef.h>

/**
 * Initialisiert das I2C-Modul
 * \param bitrate	Init-Wert fuer Bit Rate Register (TWBR)
 */
void i2c_init(uint8_t bitrate);

/**
 * Sendet nTx Bytes an einen I2C-Slave und liest anschliessend nRx Bytes
 * \param sla	Slave-Adresse
 * \param *pTx	Zeiger auf Puffer fuer zu sendende Daten
 * \param nTx	Anzahl der zu sendenden Bytes
 * \param *pRx	Zeiger auf Puffer fuer zu lesende Daten
 * \param nRx	Anzahl der zu lesenden Bytes
 */
void i2c_write_read(uint8_t sla, void * pTx, uint8_t nTx, void * pRx, uint8_t nRx);

/**
 * Sendet ein Byte an einen I2C-Slave und liest anschliessend nRx Bytes
 * \param sla		Slave-Adresse
 * \param txData	Byte, das zunaechst an den Slave gesendet wird
 * \param *pRx		Zeiger auf Puffer fuer zu lesende Daten
 * \param nRx		Anzahl der zu lesenden Bytes
 */
void i2c_read(uint8_t sla, uint8_t txData, void * pRx, uint8_t nRx);

/**
 * Sendet nTx Bytes an einen I2C-Slave
 * \param sla	Slave-Adresse
 * \param *pTx	Zeiger auf Puffer fuer zu sendende Daten
 * \param nTx	Anzahl der zu sendenden Bytes
 */
static inline void i2c_write(uint8_t sla, void * pTx, uint8_t nTx) {
	i2c_write_read(sla, pTx, nTx, NULL, 0);
}

/**
 * Wartet, bis der aktuelle I2C-Transfer beendet ist
 * \return TW_NO_INFO (0xf8) falls alles ok, sonst Fehlercode
 */
uint8_t i2c_wait(void);

#endif // I2C_AVAILABLE
#endif // I2C_H_
