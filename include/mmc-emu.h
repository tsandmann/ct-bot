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
 * @file 	mmc-emu.h
 * @brief 	MMC / SD-Card Emulation fuer PC
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	10.12.2006
 */

#ifndef MMC_EMU_H_
#define MMC_EMU_H_

#include "ct-Bot.h"  

#define MMC_EMU_SIZE	0x2000000		/*!< Groesse der emulierten Karte in Byte */
#define MMC_EMU_FILE	"mmc_emu.dat"	/*!< Name / Pfad der Datei fuer die Emulation */

#ifdef PC

/*!
 * Checkt Initialisierung der emulierten Karte
 * @return	0, wenn initialisiert
 * @see		mcu/mmc.c
 */
uint8_t mmc_emu_get_init_state(void);

/*! 
 * Initialisiere die emulierte SD/MMC-Karte
 * @return	0 wenn allles ok, sonst 1
 * @see		mcu/mmc.c
 */
uint8_t mmc_emu_init(void);

/*!
 * Liest einen Block von der emulierten Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param buffer 	Puffer von mindestens 512 Byte
 * @return 			0 wenn alles ok ist 
 * @see				mcu/mmc.c
 */	
uint8_t mmc_emu_read_sector(uint32_t addr, uint8_t * buffer);

/*! 
 * Schreibt einen 512-Byte Sektor auf die emulierte Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param buffer 	Zeiger auf den Puffer
 * @return 			0 wenn alles ok ist
 * @see				mcu/mmc.c
 */
uint8_t mmc_emu_write_sector(uint32_t addr, uint8_t * buffer);

/*!
 * Liefert die Groesse der emulierten Karte zurueck
 * @return	Groesse der emulierten Karte in Byte.
 */
uint32_t mmc_emu_get_size(void);

/*!
 * Liest die Groesse einer Datei im MiniFAT-Dateisystem aus 
 * @param file_start	Anfangsblock der Datei (Nutzdaten, nicht Header)
 * @return				Groesse der Datei in Byte, 0 falls Fehler
 */
uint32_t mmc_emu_get_filesize(uint32_t file_start);

/*! 
 * Leert eine Datei im MiniFAT-Dateisystem
 * @param file_start	Anfangsblock der Datei
 */
void mmc_emu_clear_file(uint32_t file_start);

/*!
 * @brief			Sucht die Adresse einer Mini-FAT-Datei im EERROM
 * @param filename	Datei-ID
 * @param buffer	Zeiger auf 512 Byte großen Speicherbereich (wird ueberschrieben)
 * @return			(Byte-)Adresse des ersten Nutzdatenblock der gesuchten Datei oder 0, falls nicht im EEPROM
 * Nur DUMMY fuer MMC-Emulation am PC. Wenn es mal eine EEPROM-Emulation fuer PC gibt, kann man diese Funktion implementieren.
 */
uint32_t mmc_emu_fat_lookup_adr(const char* filename, uint8_t * buffer);

/*!
 * @brief			Speichert die Adresse einer Mini-FAT-Datei in einem EERROM-Slab
 * @param block		(Block-)Adresse der Datei, die gespeichert werden soll
 * Nur DUMMY fuer MMC-Emulation am PC. Wenn es mal eine EEPROM-Emulation fuer PC gibt, kann man diese Funktion implementieren.
 */
void mmc_emu_fat_store_adr(uint32_t block);

/*!
 * @brief			Sucht einen Block auf der MMC-Karte, dessen erste Bytes dem Dateinamen entsprechen
 * @param filename 	String im Flash zur Identifikation
 * @param buffer 	Zeiger auf 512 Byte Puffer im SRAM
 * @param end_addr	Byte-Adresse, bis zu der gesucht werden soll
 * @return			Anfangsblock der Nutzdaten der Datei
 * Achtung das Prinzip geht nur, wenn die Dateien nicht fragmentiert sind 
 */
uint32_t mmc_emu_find_block(const char * filename, uint8_t * buffer, uint32_t end_addr);

/*!
 * Testet VM und MMC / SD-Card Emulation am PC
 */
uint8_t mmc_emu_test(void);

#endif	// PC
#endif /*MMC_H_*/
