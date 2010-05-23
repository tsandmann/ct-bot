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
 * @file 	cmps03.h
 * @brief 	CMPS03-Treiber
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	05.09.2007
 */

#ifndef CMPS03_H_
#define CMPS03_H_

#include "ct-Bot.h"

#ifdef CMPS03_AVAILABLE

#define CMPS03_ADDR	0xc0	/*!< I2C-Adresse des Kompass */

/*! Kompass-Datentyp fuer Lage */
typedef union {
	int16_t bearing;
	struct {
		uint8_t hi8;
		uint8_t lo8;
	};
} cmps03_t;

/*!
 * Startet das Auslesen der aktuellen Lage per I2C
 * @param *pValue	Zeiger auf Datenablage
 */
void cmps03_get_bearing(cmps03_t * pValue);

/*!
 * Wartet auf die Beendigung des I2C-Transfers und formatiert die Daten korrekt
 * @param *pValue	Zeiger auf Datenablage
 */
void cmps03_finish(cmps03_t * pValue);
#endif	// CMPS03_AVAILABLE
#endif	// CMPS03_H_
