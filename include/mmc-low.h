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
 * @file 	mmc-low.h
 * @brief 	Low-Level-Routinen zum Lesen/Schreiben einer MMC / SD-Card
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	14.11.06
 */

#ifndef MMC_LOW_H_
#define MMC_LOW_H_

/* Portkonfiguration */
#define MMC_PORT_OUT	PORTB	// Port an der die MMC/SD-Karte angeschlossen ist also des SPI 
#define MMC_PORT_IN		PINB
#define MMC_DDR			DDRB	
#define SPI_DI			6		// Port Pin an dem Data Output der MMC/SD-Karte angeschlossen ist 
#define SPI_DO			5		// Port Pin an dem Data Input der MMC/SD-Karte angeschlossen ist

#define MMC_CLK_DDR		DDRB
#define MMC_CLK_PORT	PORTB
#define SPI_CLK			7		// Port Pin an dem die Clock der MMC/SD-Karte angeschlossen ist (clk)

#define MMC_TIMEOUT		500		// Wartezyklen auf Cardresponse

#endif	// MMC_LOW_H
