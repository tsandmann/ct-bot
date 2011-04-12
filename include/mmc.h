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
 * \file 	mmc.h
 * \brief 	Routinen zum Auslesen / Schreiben einer MMC-Karte
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * \date 	07.11.2006
 */

#ifndef MMC_H_
#define MMC_H_

#ifdef MMC_AVAILABLE
#include "ui/available_screens.h"

#define MMC_AGGRESSIVE_OPTIMIZATION	/*!< Aktiviert explizites Timing im SPI-Code (ca. 13 % schneller) */

#define MMC_INFO_AVAILABLE			/*!< Die Karte kann uns einiges ueber sich verrraten, wenn wir sie danach fragen. Aber es kostet halt Platz im Flash */
//#define MMC_WRITE_TEST_AVAILABLE	/*!< Achtung dieser Test zerstoert die Daten auf der Karte ab Sektor 0x20000 (^= 64 MB)!!! DISPLAY_MMC_INFO muss an sein */

#if defined MMC_WRITE_TEST_AVAILABLE && defined MAP_AVAILABLE
#error "MMC_WRITE_TEST_AVAILABLE und MAP_AVAILABLE duerfen nicht gleichzeitig aktiv sein!"
#endif

#if defined MMC_WRITE_TEST_AVAILABLE && ! defined DISPLAY_MMC_INFO
#warning "MMC_WRITE_TEST_AVAILABLE braucht DISPLAY_MMC_INFO (ui/available_screens.h)"
#endif

#ifdef SPI_AVAILABLE
#define mmc_read_sector(addr, buffer)		mmc_read_sector_spi(0x51, addr, buffer)
#define mmc_read_block(cmd, buffer, length)	mmc_read_sector_spi(cmd[0], 0, buffer)
#define mmc_write_sector(addr, buffer)		mmc_write_sector_spi(addr, buffer)
#endif // SPI_AVAILABLE

extern uint8_t mmc_init_state;	/*!< Initialierungsstatus der Karte, 0: ok, 1: Fehler  */

/**
 * Checkt Initialisierung der Karte
 * \return	0, wenn initialisiert
 */
static inline uint8_t mmc_get_init_state(void) {
	return mmc_init_state;
}

/**
 * Schaltet die Karte aktiv und checkt dabei die Initialisierung
 * \return 0 wenn alles ok ist, 1 wenn Init nicht moeglich
 */
uint8_t mmc_enable(void);

#ifndef SPI_AVAILABLE
/**
 * Liest einen Block von der Karte
 * \param addr 		Nummer des 512-Byte Blocks
 * \param *buffer 	Zeiger auf Puffer von mindestens 512 Byte
 * \return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich oder Timeout vor / nach Kommando 17
 */
uint8_t mmc_read_sector(uint32_t addr, void * buffer) __attribute__((naked));;

/**
 * Schreibt einen 512-Byte Sektor auf die Karte
 * \param addr 		Nummer des 512-Byte Blocks
 * \param *buffer 	Zeiger auf Puffer von mindestens 512 Byte
 * \return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich oder Timeout vor / nach Kommando 24, 2 wenn Timeout bei busy
 */
uint8_t mmc_write_sector(uint32_t addr, void * buffer) __attribute__((naked));

#else // SPI_AVAILABLE
/**
 * Liest einen Block von der Karte
 * \param cmd		Kommando zum Lesen (0x51 fuer 512-Byte Block)
 * \param addr 		Adresse des Blocks
 * \param *buffer 	Zeiger auf Puffer fuer die Daten
 * \return 			0 wenn alles ok ist, 1 wenn prepare_transfer_spi() Fehler meldet, 3 bei Timeout
 */
uint8_t mmc_read_sector_spi(uint8_t cmd, uint32_t addr, void * buffer);

/**
 * Schreibt einen 512-Byte Sektor auf die Karte
 * \param addr 		Adresse des 512-Byte Blocks
 * \param *buffer 	Zeiger auf den Puffer
 * \return 			0 wenn alles ok ist, 1 wenn prepare_transfer_spi() Fehler meldet, 3 wenn Timeout bis zur Bestaetigung
 */
uint8_t mmc_write_sector_spi(uint32_t addr, void * buffer);
#endif // SPI_AVAILABLE

/**
 * Initialisiere die MMC/SD-Karte
 * \return 0 wenn allles ok, sonst Nummer des Kommandos bei dem abgebrochen wurde
 */
uint8_t mmc_init (void);

#ifdef MMC_INFO_AVAILABLE
/**
 * Liest das CSD-Register (16 Byte) von der Karte
 * \param *buffer	Zeiger auf Puffer von mindestens 16 Byte
 * \return			0, falls alles OK
 */
uint8_t mmc_read_csd(void * buffer);

/**
 * Liefert die Groesse der Karte zurueck
 * \return	Groesse der Karte in Byte. Bei einer 4 GByte-Karte kommt 0xFFFFFFFF zurueck
 */
uint32_t mmc_get_size(void);
#endif // MMC_INFO_AVAILABLE

#ifdef DISPLAY_MMC_INFO
/**
 * Zeigt die Daten der MMC-Karte an
 */
void mmc_display(void);
#endif // DISPLAY_MMC_INFO

#ifdef MMC_WRITE_TEST_AVAILABLE
/**
 * Testet die MMC-Karte. Schreibt nacheinander 2 Sektoren a 512 Byte mit Testdaten voll und liest sie wieder aus
 * !!! Achtung loescht die Karte ab Sektor 0x20000 (^= 64 MB)
 * \param *buffer	Zeiger auf einen 512 Byte grossen Puffer
 * \return 			0, wenn alles ok
 */
uint8_t mmc_test(uint8_t * buffer);
#endif // MMC_WRITE_TEST_AVAILABLE

#endif // MMC_AVAILABLE
#endif // MMC_H_
