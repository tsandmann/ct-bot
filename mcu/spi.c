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
 * @file 	mcu/spi.c
 * @brief 	(Hardware-) SPI-Treiber
 * @author	Timo Sandmann (mail@timosandmann.de)
 * @date 	01.08.2007
 */

#include "ct-Bot.h"
#include "spi.h"

#ifdef MCU
#ifdef SPI_AVAILABLE

/*!
 * Initialisiert und aktiviert das SPI-Modul
 * MCU = Master, Taktgeschwindigkeit F_CPU/2 (Maximum)
 * @param speed	Konstante, die die Uebertragungsgeschwindigkeit bestimmt
 */
void SPI_MasterInit(spi_speed_t speed) {
	/* Set MOSI and SCK output, MISO input */
	uint8_t ddrb = DDRB;
	ddrb |=  (1<<DDB5) | (1<<DDB7) | (1<<DDB4);
	ddrb = (uint8_t) (ddrb & ~(1<<DDB6));
	DDRB = ddrb;
	/* Enable SPI, Master, set clock rate */
	uint8_t spcr = (1<<SPE) | (1<<MSTR);
	uint8_t tmp = (uint8_t) (speed.raw & 0x3);
	spcr |= tmp;
	SPCR = spcr;
	SPSR = speed.data.spi2x; // SPI2X
}

/*!
 * Uebertraegt ein Byte per SPI vom Master zum Slave
 */
void SPI_MasterTransmit(uint8_t data) {
	/* Start transmission */
	SPDR = data;
	/* Wait for transmission complete */
	while (!(SPSR & (1<<SPIF))) {}
}

/*!
 * Uebertraegt ein Byte per SPI vom Slave zum Master
 */
uint8_t SPI_MasterReceive(void) {
	SPDR = 0xff;
	/* Wait for reception complete */
	while(!(SPSR & (1<<SPIF))) {}
	/* Return Data Register */
	return SPDR;
}

#endif	// SPI_AVAILABLE
#endif	// MCU
