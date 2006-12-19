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

/*! @file 	mmc.h
 * @brief 	Routinen zum Auslesen/Schreiben einer MMC-Karte
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @date 	07.11.06
*/

#ifndef MMC_H_
#define MMC_H_

#include "ct-Bot.h"  

#ifdef MMC_AVAILABLE

#define MMC_INFO_AVAILABLE			/*!< Die Karte kann uns einiges ueber sich verrraten, wenn wir sie danach fragen. Aber es kostet halt Platz im Flash */
#define MMC_WRITE_TEST_AVAILABLE	/*!< Achtung dieser Test zerstoert die Daten auf der Karte!!! */

/*!
 * Checkt Initialisierung der Karte
 * @return	0, wenn initialisiert
 */
inline uint8 mmc_get_init_state(void);

uint8 mmc_enable(void);

/*!
 * Liest einen Block von der Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param Buffer 	Puffer von mindestens 512 Byte
 * @return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich oder Timeout vor / nach Kommando 17
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			17.11.2006
 * @see				mmc-low.s
 */	
uint8 mmc_read_sector(uint32 addr, uint8 *buffer);

/*! 
 * Schreibt einen 512-Byte Sektor auf die Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param Buffer 	Zeiger auf den Puffer
 * @param async		0: synchroner, 1: asynchroner Aufruf, siehe MMC_ASYNC_WRITE in mmc-low.h
 * @return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich oder Timeout vor / nach Kommando 24, 2 wenn Timeout bei busy
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			16.11.2006
 * @see				mmc-low.s
 */
uint8 mmc_write_sector(uint32 addr, uint8 *buffer, uint8 async);

/*! 
 * Initialisiere die SD/MMC-Karte
 * @return 0 wenn allles ok, sonst Nummer des Kommandos bei dem abgebrochen wurde
 */
uint8 mmc_init (void);

#ifdef MMC_INFO_AVAILABLE
	/*!
	 * Liest das CSD-Register (16 Byte) von der Karte
	 * @param Buffer Puffer von mindestens 16 Byte
	 */
	void mmc_read_csd (uint8 *buffer);
	
	/*!
	 * Liest das CID-Register (16 Byte) von der Karte
	 * @param Buffer Puffer von mindestens 16 Byte
	 */
	void mmc_read_cid (uint8 *buffer);
	
	/*!
	 * Liefert die Groesse der Karte zurueck
	 * @return Groesse der Karte in Byte. Bei einer 4 GByte-Karte kommt 0xFFFFFFFF zurueck
	 */
	uint32 mmc_get_size(void);
#endif

#ifdef MMC_WRITE_TEST_AVAILABLE
	/*! Testet die MMC-Karte. Schreibt nacheinander 2 Sektoren a 512 Byte mit testdaten voll und liest sie wieder aus
	 * !!! Achtung loescht die Karte
	 * @return 0, wenn alles ok
	 */
	uint8 mmc_test(void);
#endif

#endif

#endif /*MMC_H_*/
