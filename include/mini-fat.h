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
 * @file 	mini-fat.h
 * @brief 	Routinen zum Auffinden von markierten Files auf einer MMC-Karte. 
 *          Dies ist keine vollstaendige FAT-Unterstuetzung, sondern sucht nur eien Datei, die mit einer 3-Zeichen Sequenz beginnt. 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @date 	07.11.2006
 */

#ifndef MINIFAT_H_
#define MINIFAT_H_

#include "ct-Bot.h"

#define MMC_FILENAME_MAX	255		/*!< Maximale Dateienamenlaenge in Zeichen [1;255] */

/*! Datentyp fuer Mini-Fat-Dateilaenge */
typedef union {
	uint32 u32;		/*!< Laenge in 32 Bit */
	uint8 u8[4];	/*!< Laenge in 4 "einzelnen" Bytes */
} file_len_t;

/*!
 * @brief			Sucht die Adresse einer Mini-FAT-Datei im EERROM
 * @param filename	Datei-ID
 * @param buffer	Zeiger auf 512 Byte großen Speicherbereich (wird ueberschrieben)
 * @return			(Byte-)Adresse des ersten Nutzdatenblock der gesuchten Datei oder 0, falls nicht im EEPROM
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			05.03.2007
 * Das Nachschlagen im EEPROM und durchprobieren der Adressen geht relativ schnell, somit verkuerzt sich die Wartezeit
 * beim Dateioeffnen von einer MMC erheblich durch diese Methode. Die Adressen (32 Bit) liegen in insgesamt 10 Slabs
 * im EEPROM.
 */
uint32 mini_fat_lookup_adr(const char* filename, uint8* buffer);

/*!
 * @brief			Speichert die Adresse einer Mini-FAT-Datei in einem EERROM-Slab
 * @param adr		(Block-)Adresse der Datei, die gespeichert werden soll
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			05.03.2007
 * Gespeichert wird die Adresse des 0. Blocks der Datei (man gibt aber die Adresse des ersten Nutzdatenblocks an, wie sie
 * z.B. mini_fat_find_block() liefert) in einem EEPROM-Slab. Derzeit gibt es 10 Slabs, sind alle belegt (d.h. != 0), speichert
 * diese Funktion die uebergebene Adresse nicht.
 */
void mini_fat_store_adr(uint32 adr);

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
/*! 
 * Erzeugt eine Datei, die an den ersten 3 Byte die ID- enthaelt. dann folgen 512 - sizeof(id) nullen
 * Danach kommen so viele size kByte Nullen
 * @param filename Der Dateiname der zu erzeugenden Datei
 * @param id_string Die ID des Files, wie sie zu beginn steht
 * @param size kByte Nutzdaten, die der MCU spaeter beschreiben darf
 */
void create_mini_fat_file(const char* filename, const char* id_string, uint32 size);

/*! 
 * @brief				Erzeugt eine Mini-Fat-Datei in einer emulierten MMC
 * @param addr			Die Adresse auf der emulierten Karte, an der die Datei beginnen soll
 * @param id_string 	Die ID der Datei, wie sie zu Beginn in der Datei steht
 * @param size 			KByte Nutzdaten, die die Datei umfasst
 * Erzeugt eine Datei, die an den ersten Bytes die ID enthaelt. Dann folgen 512 - sizeof(id) Nullen
 * Danach kommen size * 1024 Nullen
 */
void create_emu_mini_fat_file(uint32_t addr, const char* id_string, uint32_t size);

/*! 
 * @brief				Loescht eine Mini-Fat-Datei in einer emulierten MMC
 * @param id_string 	Die ID der Datei, wie sie zu Beginn in der Datei steht
 */
void delete_emu_mini_fat_file(const char* id_string);

/*! 
 * Konvertiert eine (binaere) mini-fat-Datei ("AVR-Endian") mit Speed-Log-Daten in eine Textdatei.
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			10.02.2007  
 * @param filename 	Der Dateiname der mini-fat-Datei
 */
void convert_slog_file(const char* input_file);
#endif

#endif /*MINIFAT_H_*/
