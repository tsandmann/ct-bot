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
 * \file 	mini-fat.h
 * \brief 	Routinen zum Auffinden von markierten Files auf einer MMC-Karte.
 *
 * Dies ist keine vollstaendige FAT-Unterstuetzung, sondern sucht nur eien Datei, die mit einer Zeichensequenz beginnt.
 *
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * \date 	07.11.2006
 */

#ifndef MINIFAT_H_
#define MINIFAT_H_

#define MMC_FILENAME_MAX	255 /*!< Maximale Dateienamenlaenge in Zeichen [1;255] */
#define MMC_HEADER_DATA_SIZE (512 - sizeof(file_len_t) - (MMC_FILENAME_MAX + 1)) /*!< Header-Daten-Groesse [Byte] */

/*! Datentyp fuer Mini-Fat-Dateilaenge */
typedef union {
	uint32_t u32;	/*!< Laenge in 32 Bit */
	uint8_t u8[4];	/*!< Laenge in 4 "einzelnen" Bytes */
} file_len_t;

/*! Datentyp fuer Mini-Fat Dateiheader */
typedef struct {
	const char filename[MMC_FILENAME_MAX + 1];
	file_len_t length;
	uint8_t data[MMC_HEADER_DATA_SIZE];
} PACKED_FORCE mini_fat_header_t;

#ifdef MCU
#if defined MMC_AVAILABLE && ! defined BOT_FS_AVAILABLE
#include "mmc.h"
#include <string.h>
#include <avr/pgmspace.h>

/*!
 * Sucht einen Block auf der MMC-Karte, dessen erste Bytes dem Dateinamen entsprechen
 * \param filename	String im Flash zur Identifikation
 * \param buffer 	Zeiger auf 512 Byte Puffer im SRAM
 * \param end_addr	Byte-Adresse, bis zu der gesucht werden soll
 * \return			Anfangsblock der Nutzdaten der Datei
 * Achtung das Prinzip geht nur, wenn die Dateien nicht fragmentiert sind
 */
uint32_t mini_fat_find_block_P(const char * filename, void * buffer, uint32_t end_addr);

/*!
 * Sucht einen Block auf der MMC-Karte, dessen erste Bytes dem Dateinamen entsprechen
 * \param filename	String zur Identifikation
 * \param buffer 	Zeiger auf 512 Byte Puffer im SRAM
 * \return			Anfangsblock der Nutzdaten der Datei
 * Achtung das Prinzip geht nur, wenn die Dateien nicht fragmentiert sind
 */
#define mini_fat_find_block(filename, buffer) mini_fat_find_block_P(PSTR(filename), buffer, mmc_get_size());

/*!
 * Leert eine Datei im MiniFAT-Dateisystem auf der MMC/SD-Karte
 * \param file_start	Anfangsblock der Datei
 * \param *buffer		Zeiger auf 512 Byte Puffer im SRAM, wird geloescht!
 */
void mini_fat_clear_file(uint32_t file_start, void * buffer);

/*!
 * Liest den Mini-Fat-Header aus einer Mini-Fat-Datei
 * \param address Block-Adresse der Mini-Fat-Datei (erster Datenblock, nicht Header)
 * \param *buffer Puffer von 512 Byte (wird ueberschrieben)
 * \return Zeiger auf den Header (im Puffer)
 */
static inline mini_fat_header_t * mini_fat_read_header(uint32_t address, void * buffer) {
	mmc_read_sector(address - 1, buffer);
	mini_fat_header_t * header = buffer;
	return header;
}

/*!
 * Liest die zusaetzlichen Daten des Mini-Fat-Headers aus
 * \param address Block-Adresse der Mini-Fat-Datei (erster Datenblock, nicht Header)
 * \param *buffer Puffer von 512 Byte (wird ueberschrieben)
 * \return Zeiger auf die Headerdaten
 */
static inline uint8_t * mini_fat_read_header_data(uint32_t address, void * buffer) {
	mini_fat_header_t * header = mini_fat_read_header(address, buffer);
	return header->data;
}

/*!
 * Schreibt einen Mini-Fat-Header zurueck
 * \param address Block-Adresse der Mini-Fat-Datei (erster Datenblock, nicht Header)
 * \param *header Zeiger auf Header, 512 Byte gross
 */
static inline void mini_fat_write_header(uint32_t address, mini_fat_header_t * header) {
	mmc_write_sector(address - 1, header);
}

/*!
 * Schreibt Header-Daten in einen Mini-Fat-Header
 * \param address Block-Adresse der Mini-Fat-Datei (erster Datenblock, nicht Header)
 * \param *header Zeiger auf die Header-Daten, die geschrieben werden sollen
 * \param *buffer Puffer von 512 Byte (wird ueberschrieben, falls header nicht auf buffer->data zeigt)
 */
static inline void mini_fat_write_header_data(uint32_t address, const uint8_t * const header, void * const buffer) {
	mini_fat_header_t * const ptr = buffer;
	if (header != ptr->data) {
		/* Headerdaten liegen nicht im Puffer -> dorthin kopieren */
		mini_fat_read_header_data(address, buffer);
		memcpy(ptr->data, header, MMC_HEADER_DATA_SIZE);
	}
	mini_fat_write_header(address, buffer);
}

/*!
 * Liest die Groesse einer Datei im MiniFAT-Dateisystem auf der MMC/SD-Karte aus
 * \param file_start	Anfangsblock der Datei (Nutzdaten, nicht Header)
 * \param *buffer		Zeiger auf 512 Byte Puffer im SRAM, wird veraendert!
 * \return				Groesse der Datei in Byte, 0 falls Fehler
 */
static inline uint32_t mini_fat_get_filesize(uint32_t file_start, void * buffer) {
	mini_fat_header_t * header = mini_fat_read_header(file_start, buffer);
	file_len_t length = header->length;
	return length.u32;
}
#endif // MMC_AVAILABLE && ! BOT_FS_AVAILABLE

#else // ! MCU

/*!
 * Erzeugt eine Datei, die an den ersten 3 Byte die ID- enthaelt. dann folgen 512 - sizeof(id) nullen
 * Danach kommen so viele size kByte Nullen
 * \param filename Der Dateiname der zu erzeugenden Datei
 * \param id_string Die ID des Files, wie sie zu beginn steht
 * \param size kByte Nutzdaten, die der MCU spaeter beschreiben darf
 */
void create_mini_fat_file(const char * filename, const char * id_string, uint32_t size);

/*!
 * Erzeugt eine Mini-Fat-Datei in einer emulierten MMC
 * \param addr			Die Adresse auf der emulierten Karte, an der die Datei beginnen soll
 * \param id_string 	Die ID der Datei, wie sie zu Beginn in der Datei steht
 * \param size 			KByte Nutzdaten, die die Datei umfasst
 *
 * Erzeugt eine Datei, die an den ersten Bytes die ID enthaelt. Dann folgen 512 - sizeof(id) Nullen
 * Danach kommen size * 1024 Nullen
 */
void create_emu_mini_fat_file(uint32_t addr, const char * id_string, uint32_t size);

/*!
 * Loescht eine Mini-Fat-Datei in einer emulierten MMC
 * \param id_string 	Die ID der Datei, wie sie zu Beginn in der Datei steht
 */
void delete_emu_mini_fat_file(const char * id_string);
#endif // MCU

#ifdef DISPLAY_MINIFAT_INFO
/*!
 * Display-Screen fuer Ausgaben des MiniFAT-Treibers, falls dieser welche erzeugt.
 * Da die MiniFat-Funktionen im Wesentlichen den aktuellen Suchstatus der MMC
 * ausgeben, erfolgt die eigentliche Ausgabe in der jeweiligen Schleife der
 * MiniFAT-Funktion, dieser Screen ist dafuer nur ein Platzhalter
 */
void mini_fat_display(void);
#endif // DISPLAY_MINIFAT_INFO
#endif // MINIFAT_H_
