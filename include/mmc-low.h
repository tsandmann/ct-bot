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
 * \file 	mmc-low.h
 * \brief 	Low-Level-Routinen zum Lesen/Schreiben einer MMC / SD-Card
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	14.11.2006
 */

#ifndef MMC_LOW_H_
#define MMC_LOW_H_

/* Portkonfiguration */
#define MMC_PORT_OUT	PORTB	/*!< Ausgangs-Port fuer die MMC/SD-Karte */
#define MMC_PORT_IN		PINB	/*!< Eingangs-Port fuer die MMC/SD-Karte */
#define MMC_DDR			DDRB	/*!< DDR-Register fuer MMC/SD-Karte */
#define SPI_DI			6		/*!< Port Pin an dem Data Output der MMC/SD-Karte angeschlossen ist */
#define SPI_DO			5		/*!< Port Pin an dem Data Input der MMC/SD-Karte angeschlossen ist */

#define MMC_CLK_DDR		DDRB	/*!< DDR-Register fuer MMC-Clock */
#define MMC_CLK_PORT	PORTB	/*!< Port fuer MMC-Clock */
#define SPI_CLK			7		/*!< Port Pin an dem die Clock der MMC/SD-Karte angeschlossen ist (clk) */

#define MMC_TIMEOUT		1000	/*!< Wartezyklen auf MMC-Antwort */

#endif // MMC_LOW_H
