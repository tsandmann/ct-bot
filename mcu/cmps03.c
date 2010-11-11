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
 * @file 	cmps03.c
 * @brief 	CMPS03-Treiber
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	05.09.2007
 */

#ifdef MCU

#include "ct-Bot.h"
#ifdef CMPS03_AVAILABLE
#include "i2c.h"
#include "cmps03.h"
#include <stdlib.h>


/*!
 * Startet das Auslesen der aktuellen Lage per I2C
 * @param *pValue	Zeiger auf Datenablage
 */
void cmps03_get_bearing(cmps03_t * pValue) {
	/* 2 Bytes aus Register 2 lesen */
	i2c_read(CMPS03_ADDR, 2, (uint8_t *)pValue, 2);
}

/*!
 * Wartet auf die Beendigung des I2C-Transfers und formatiert die Daten korrekt
 * @param *pValue	Zeiger auf Datenablage
 */
void cmps03_finish(cmps03_t * pValue) {
	/* Auf I2C-Transfer warten */
	if (i2c_wait() == TW_NO_INFO) {
		/* Transfer OK, Bytes tauschen (big-endian => little-endian) */
		uint8_t tmp;
		tmp = pValue->hi8;
		pValue->hi8 = pValue->lo8;
		pValue->lo8 = tmp;
	} else {
		/* I2C-Fehler aufgetreten */
		pValue->bearing = 0;
	}
}

#endif // CMPS03_AVAILABLE
#endif // MCU
