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
 * \file 	spi_select.h
 * \brief 	Base classes implementing SPI select methods (with ENA or diretly with PB4)
 * \author	Timo Sandmann
 * \date 	22.07.2017
 */

#ifndef SPI_SELECT_H_
#define SPI_SELECT_H_

#ifdef MCU
#include <avr/io.h>

extern "C" {
#include "ena.h"
}

/**
 * SPI slave select with ENA shift register
 */
class SelectEna {
protected:
	/**
	 * Sets CS the line
	 * \param status New status: true to set CS line high, false to set CS line low
	 */
	void set_cs(const bool status) const {
		status ? ENA_off(ENA_MMC) : ENA_on(ENA_MMC);
	}
};


/**
 * SPI slave select with PB4 pin
 */
class SelectPB4 {
protected:
	/**
	 * Sets CS the line
	 * \param status New status: true to set CS line high, false to set CS line low
	 */
	void set_cs(const bool status) const {
		status ? PORTB |= _BV(PB4) : PORTB &= ~_BV(PB4);
	}
};

#endif // MCU
#endif /* SPI_SELECT_H_ */
