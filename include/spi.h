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
 * @file 	include/spi.h
 * @brief 	(Hardware-) SPI-Treiber
 * @author	Timo Sandmann (mail@timosandmann.de)
 * @date 	01.08.2007
 */

#ifndef SPI_H_
#define SPI_H_

#include "ct-Bot.h"

#ifdef MCU
#ifdef SPI_AVAILABLE

typedef union {
	struct {
		uint8_t spr0:1;		/*!< SPR0 Bit in SPCR */
		uint8_t spr1:1;		/*!< SPR1 Bit in SPCR */
		uint8_t spi2x:1;	/*!< SPI 2x-Mode in SPSR */
	} __attribute__((packed));
	uint8_t raw;	/*!< Alle Bits als Rawdaten */
} spi_speed_t;	/*!< Speed-Einstellung fuer SPI */

#define SPI_SPEED_8MHZ		{{0, 0, 1}}	// !SPR0, !SPR1, SPI2X
#define SPI_SPEED_4MHZ		{{0, 0, 0}}	// !SPR0, !SPR1, !SPI2X
#define SPI_SPEED_250KHZ	{{0, 1, 0}}	// !SPR0, SPR1, !SPI2X

/*!
 * Initialisiert und aktiviert das SPI-Modul
 * MCU = Master, Taktgeschwindigkeit F_CPU/2 (Maximum)
 * @param speed	Konstante, die die Uebertragungsgeschwindigkeit bestimmt
 */
void SPI_MasterInit(spi_speed_t speed);

/*!
 * Uebertraegt ein Byte per SPI vom Master zum Slave
 */
void SPI_MasterTransmit(uint8_t data);

/*!
 * Uebertraegt ein Byte per SPI vom Slave zum Master
 */
uint8_t SPI_MasterReceive(void);

#endif	// SPI_AVAILABLE
#endif	// MCU
#endif	// SPI_H_
