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

#define MMC_INFO_AVAILABLE /*!< Die Karte kann uns einiges ueber sich verrraten, wenn wir sie danach fragen. Aber es kostet halt Platz im Flash */
//#define MMC_WRITE_TEST_AVAILABLE /*!< Achtung dieser Test zerstoert die Daten auf der Karte!!! */

/*!
 * Liest einen Block von der Karte
 * @param addr Adresse in Bytes ab der gelesen wird
 * @param Buffer Puffer von mindestens 512 Byte
 */
void mmc_read_sector (uint32 addr,uint8 *Buffer);

/*! Schreibt einen 512-Byte Sektor auf die Karte
 * @param addr Adresse des Blocks in Byte
 * @param Buffer zeiger auf den Puffer
 * @return 0 wenn alles ok
 */
uint8 mmc_write_sector (uint32 addr,uint8 *Buffer);

/*! 
 * Initialisiere die SD/MMC-Karte
 * @return 0 wenn allles ok, sonst nummer des Kommandos beid em abgebrochen wurde
 */
uint8 mmc_init (void);

#ifdef MMC_INFO_AVAILABLE
	

	/*!
	 * Liest das CSD-Register (16 Byte) von der Karte
	 * @param Buffer Puffer von mindestens 16 Byte
	 */
	void mmc_read_csd (uint8 *Buffer);
	
	/*!
	 * Liest das CID-Register (16 Byte) von der Karte
	 * @param Buffer Puffer von mindestens 16 Byte
	 */
	void mmc_read_cid (uint8 *Buffer);
	
	/*!
	 * Liefert die Groesse der Karte zurueck
	 * @return Groesse der Karte in Byte. Bei einer 4 GByte-Karte kommt 0xFFFFFFFF zurueck
	 */
	uint32 mmc_get_size(void);
	
	/*! Testet die MMC-Karte. Schreibt nacheinander 2 Sektoren a 512 Byte mit testdaten voll und liest sie wieder aus
	 * !!! Achtung loescht die Karte
	 * @return 0, wenn alles ok
	 */
	uint8 mmc_test(void);
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
