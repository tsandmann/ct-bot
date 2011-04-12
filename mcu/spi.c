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
 * \file 	spi.c
 * \brief 	(Hardware-) SPI-Treiber
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	01.08.2007
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef SPI_AVAILABLE
#include "spi.h"

/**
 * Initialisiert und aktiviert das SPI-Modul
 * MCU = Master, Taktgeschwindigkeit F_CPU/2 (Maximum)
 * \param speed	Konstante, die die Uebertragungsgeschwindigkeit bestimmt
 */
void SPI_MasterInit(spi_speed_t speed) {
	/* Set MOSI and SCK output, MISO input */
	uint8_t ddrb = DDRB;
	ddrb |=  _BV(DDB5) | _BV(DDB7) | _BV(DDB4);
	ddrb = (uint8_t) (ddrb & ~_BV(DDB6));
	DDRB = ddrb;
	/* Enable SPI, Master, set clock rate */
	uint8_t spcr = _BV(SPE) | _BV(MSTR);
	uint8_t tmp = (uint8_t) (speed.raw & 0x3);
	spcr |= tmp;
	SPCR = spcr;
	SPSR = speed.data.spi2x; // SPI2X
}

/**
 * Uebertraegt ein Byte per SPI vom Master zum Slave
 * \param data Das zu uebertragene Byte
 */
void SPI_MasterTransmit(uint8_t data) {
	/* Start transmission */
	SPDR = data;
	/* Wait for transmission complete */
	while (! (SPSR & _BV(SPIF))) {}
}

/**
 * Uebertraegt ein Byte per SPI vom Slave zum Master
 * \return Das empfangene Byte
 */
uint8_t SPI_MasterReceive(void) {
	SPDR = 0xff;
	/* Wait for reception complete */
	while(! (SPSR & _BV(SPIF))) {}
	/* Return Data Register */
	return SPDR;
}

#endif // SPI_AVAILABLE
#endif // MCU
