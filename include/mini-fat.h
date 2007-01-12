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

/*! @file 	mini-fat.h
 * @brief 	Routinen zum Auffinden von markierten Files auf einer MMC-Karte. 
 *          Dies ist keine vollstaendige FAT-Unterstuetzung, sondern sucht nur eien Datei, die mit einer 3-Zeichen Sequenz beginnt. 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @date 	07.11.06
*/

#ifndef MINIFAT_H_
#define MINIFAT_H_

#include "ct-Bot.h"

#define MMC_FILENAME_MAX	255		/*!< Maximale Dateienamenlaenge in Zeichen [1;255] */

typedef union{
	uint32 u32;		// Laenge in 32 Bit
	uint8 u8[4];	// laenge in 4 "einzelnen" Bytes
} file_len_t;

#ifdef MINI_FAT_AVAILABLE

/*!
 * Sucht einen Block auf der MMC-Karte, dessen erste drei Bytes dem key entsprechen
 * liefert dann den folgenden Block zurueck.
 * Achtung das prinzip geht nur, wenn die Dateien nicht fragmentiert sind
 * @param key 3 Byte zur Identifikation
 * @param buffer Zeiger auf 512 Byte Puffer im SRAM
 */
uint32 mini_fat_find_block(const char key[3], uint8* buffer);

#endif

#ifdef PC
/*! Erzeugt eine Datei, die an den ersten 3 Byte die ID- enthaelt. dann folgen 512 - sizeof(id) nullen
 * Danach kommen so viele size kByte Nullen
 * @param filename Der Dateiname der zu erzeugenden Datei
 * @param id_string Die ID des Files, wie sie zu beginn steht
 * @param size kByte Nutzdaten, die der MCU spaeter beschreiben darf
 */
void create_mini_fat_file(const char* filename, const char* id_string, uint32 size);
#endif

#endif /*MINIFAT_H_*/
