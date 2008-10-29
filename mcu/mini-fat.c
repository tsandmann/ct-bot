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
 * @file 	mcu/mini-fat.c
 * @brief 	Routinen zum Auffinden von markierten Files auf einer MMC-Karte.
 *          Dies ist keine vollstaendige FAT-Unterstuetzung, sondern sucht nur eien Datei, die mit einer Zeichensequenz beginnt.
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @date 	07.11.2006
 */

#include "ct-Bot.h"
#include "eeprom.h"

/* EEPROM-Variable immer deklarieren, damit die Adresse sich nicht veraendert je nach #define */
uint32_t EEPROM eefat[10] = {0};	/*!< EEPROM-Cache fuer FAT-Eintraege */

#define MINI_FAT_CHECK_FRAGMENTATION	/*!< Prueft automatisch, ob eine Mini-FAT-Datei fragmentiert ist */
//#define MINI_FAT_DEBUG				/*!< Schalter fuer Debug-Ausgaben */


#ifdef MCU
#ifdef MMC_AVAILABLE
#ifndef BOT_FS_AVAILABLE
#include <string.h>
#include "mmc.h"
#include "display.h"
#include "ui/available_screens.h"
#include "gui.h"
#include "mini-fat.h"
#include "log.h"
#include "led.h"


#ifndef MINI_FAT_DEBUG
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif


#ifdef DISPLAY_MINIFAT_INFO
/*!
 * Hilfsfunktion, die eine 23-Bit Blockadresse auf dem Display als hex anzeigt.
 * Da display_printf() @MCU maximal mit 16 Bit Zahlen umgehen kann, zerlegt diese Funktion die Adresse ein zwei Teile.
 */
static void display_block(uint32_t addr) {
	uint16_t low  = (uint16_t)addr;
	uint16_t high = (uint16_t)(addr>>16);
	display_printf("0x%02x%04x", high, low);
}

/*!
 * Display-Screen fuer Ausgaben des MiniFAT-Treibers, falls dieser welche erzeugt.
 * Da die MiniFat-Funktionen im Wesentlichen den aktuellen Suchstatus der MMC
 * ausgeben, erfolgt die eigentliche Ausgabe in der jeweiligen Schleife der
 * MiniFAT-Funktion, dieser Screen ist dafuer nur ein Platzhalter
 */
void mini_fat_display(void) {
	display_cursor(1,1);
	display_printf("MiniFAT:");
}
#endif	// DISPLAY_MINIFAT_INFO

/*!
 * @brief			Sucht die Adresse einer MiniFAT-Datei im EERROM
 * @param filename	Datei-ID
 * @param buffer	Zeiger auf 512 Byte großen Speicherbereich (wird ueberschrieben)
 * @return			(Block-)Adresse des ersten Nutzdatenblock der gesuchten Datei oder 0, falls nicht im EEPROM
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			05.03.2007
 * Das Nachschlagen im EEPROM und Durchprobieren der Adressen geht relativ schnell, somit verkuerzt sich die Wartezeit
 * beim Dateioeffnen von einer MMC erheblich durch diese Methode. Die Adressen (32 Bit) liegen in insgesamt 10 Slabs
 * im EEPROM.
 */
static uint32_t mini_fat_lookup_adr(const char * filename, void * buffer) {
	uint8_t i;
	uint32_t * p_eefat = eefat;
	/* EEPROM-Slabs nach gewuenschter Datei-ID durchsuchen */
	for (i=10; i>0; i--) {
		uint32_t block;
		block = ctbot_eeprom_read_dword(p_eefat++);	// Adresse aus dem EEPROM laden
		if (mmc_read_sector(block, buffer) != 0) return 0;
		/* Datei-ID vergleichen */
		if (strcmp_P((char *)buffer, filename) == 0) {
#ifdef DISPLAY_MINIFAT_INFO
			display_cursor(3, 1);
			display_printf("eefound %u:", 11-i);
			display_cursor(3, 13);
			display_block(block);
#endif	// DISPLAY_MINIFAT_INFO
			return ++block;
		}
	}
	return 0;
}

/*!
 * @brief			Speichert die Adresse einer MiniFAT-Datei in einem EERROM-Slab
 * @param block		(Block-)Adresse der Datei, die gespeichert werden soll
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			05.03.2007
 * Gespeichert wird die Adresse des 0. Blocks der Datei (man gibt aber die Adresse des ersten Nutzdatenblocks an, wie sie
 * z.B. mini_fat_find_block() liefert) in einem EEPROM-Slab. Derzeit gibt es 10 Slabs, sind alle belegt (d.h. != 0), speichert
 * diese Funktion die uebergebene Adresse nicht.
 */
static void mini_fat_store_adr(uint32_t block) {
	uint8_t i;
	uint32_t * p_eefat = eefat;
	block--;	// Block mit der Datei-ID speichern, nicht ersten Nutzdatenblock
	/* freien Block im EEPROM suchen */
	for (i=9; i>0; i--) {
		uint32_t tmp = ctbot_eeprom_read_dword(p_eefat++);
		if (tmp == 0) {	// hier noch ist Platz :-)
#ifdef DISPLAY_MINIFAT_INFO
			display_cursor(3, 1);
			display_printf("eestore %u:", 11-i);
			display_cursor(3, 13);
			display_block(block);
#endif	// DISPLAY_MINIFAT_INFO
			p_eefat--;
			break;	// fertig
		}
	}
	ctbot_eeprom_write_dword(p_eefat, block);	// Adresse speichern
}

#ifdef MINI_FAT_CHECK_FRAGMENTATION
/*! Partitionseintrag im MBR */
typedef struct {
	uint8_t state;
	uint8_t begin_head;
	uint16_t begin_sect;
	uint8_t type;
	uint8_t end_head;
	uint16_t end_sect;
	uint32_t first_sect_offset;
	uint32_t sectors;
} __attribute__ ((packed)) part_entry_t;

/*! Master-Boot-Record */
typedef struct {
	uint8_t bootcode[446];
	part_entry_t part0;
	part_entry_t part1;
	part_entry_t part2;
	part_entry_t part3;
	uint16_t executable_mark;
} __attribute__ ((packed)) mbr_t;

/*! FAT16-Bootsektor */
typedef struct {
	uint8_t jump_code[3];
	char oem_name[8];
	uint16_t bytes_per_sect;
	uint8_t sect_per_cluster;
	uint16_t reserved_sect;
	uint8_t fat_copies;
	uint16_t root_dir_entries;
	uint16_t sectors_smaller_32m;
	uint8_t media_descr;
	uint16_t sect_per_fat;
	uint16_t sect_per_track;
	uint16_t num_heads;
	uint32_t num_hidden_sect;
	uint32_t num_sect;
	uint16_t logic_drive_num;
	uint8_t ext_signature;
	uint32_t serial;
	char volume_name[11];
	char fat_name[8];
	uint8_t exec_code[448];
	uint16_t executable_mark;
} __attribute__ ((packed)) fat16_bootsector_t;

/*! FAT16-Verzeichniseintrag */
typedef struct {
	char name[8];
	char extension[3];
	uint8_t attributes;
	uint8_t unused[10];
	uint16_t time;
	uint16_t date;
	uint16_t first_cluster;
	uint32_t size;
} __attribute__ ((packed)) fat16_dir_entry_t;

/*!
 * Prueft, ob eine Mini-FAT-Datei fragmentiert ist.
 * @param block		Adresse des ersten Nutzdatenblocks einer Datei (nicht Header!)
 * @param *buffer	Zeiger auf 512 Byte grossen Puffer (enthaelt anschliessend den Dateiheader)
 * @return			Adresse des ersten Nutzdatenblocks der Datei
 * 					oder 0xffffffff, falls Datei fragmentiert
 */
static uint32_t check_fragmentation(uint32_t block, void * buffer) {
	LOG_DEBUG("block=0x%04x", block - 1);
	/* MBR lesen */
	mmc_read_sector(0, buffer);
	mbr_t * p_mbr = buffer;
	uint16_t first_sect = p_mbr->part0.first_sect_offset;
	LOG_DEBUG("first_sect=0x%04x", first_sect);

	/* Bootsektor von Partition 1 lesen */
	mmc_read_sector(first_sect, buffer);
	fat16_bootsector_t * p_bs = buffer;
	p_bs->fat_name[7] = 0;
	if (strcmp_P(p_bs->fat_name, PSTR("FAT16  ")) != 0) {
#ifdef DISPLAY_MINIFAT_INFO
		display_cursor(2, 1);
		display_printf("Kein FAT16");
#endif
		LOG_ERROR("Keine FAT16-Partition");
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
			LED_on(LED_TUERKIS);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
		return 0xffffffff;
	}
	uint16_t fat_offset = p_bs->reserved_sect + first_sect;
	LOG_DEBUG("fat_offset=0x%04x", fat_offset);
	uint16_t root_offset = fat_offset + p_bs->fat_copies * p_bs->sect_per_fat;
	LOG_DEBUG("root_offset=0x%04x", root_offset);
	uint16_t data_offset = root_offset + (p_bs->root_dir_entries * sizeof(fat16_dir_entry_t) + 511) / 512;
	LOG_DEBUG("data_offset=0x%04x", data_offset);
	uint8_t sect_per_cluster = p_bs->sect_per_cluster;
	LOG_DEBUG("sect_per_cluster=%u", sect_per_cluster);

	/* 1. Cluster der Datei berechnen */
	uint16_t first_cluster = (block - 1 - data_offset) / sect_per_cluster + 2;
	LOG_DEBUG("first_cluster=0x%04x", first_cluster);

	/* 1. FAT-Eintrag der Datei einlesen */
	uint16_t fat_block = fat_offset + first_cluster / (512 / sizeof(uint16_t));
	LOG_DEBUG("fat_block=0x%04x", fat_block);
	mmc_read_sector(fat_block, buffer);
	uint16_t entry_offset = first_cluster % (512 / sizeof(uint16_t));
	LOG_DEBUG("entry_offset=0x%04x", entry_offset);
	uint16_t * ptr = buffer;
	uint16_t entry = ptr[entry_offset];

	mmc_read_sector(block - 1, buffer);	// Dateiheader wieder in Puffer laden

	LOG_DEBUG("Fat-Eintrag=0x%04x", entry);
	if (entry < 0xfff8 && entry != first_cluster + 1) {
		/* Datei ist fragmentiert */
#ifdef DISPLAY_MINIFAT_INFO
		display_cursor(2, 1);
		display_printf("Datei fragmentiert  ");
#endif
		LOG_ERROR("Datei ist fragmentiert!");
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
			LED_on(LED_TUERKIS);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
		return 0xffffffff;
	}
	/* alles gut */
#ifdef DISPLAY_MINIFAT_INFO
		display_cursor(2, 1);
		display_printf("Datei ok");
#endif
	LOG_DEBUG("Datei ist nicht fragmentiert");
	return block;
}
#else
/*!
 * Keine Ueberpruefung auf Fragmentierung gewuescht, daher nur Dummy
 * @param block		Adresse des ersten Nutzdatenblocks einer Datei (nicht Header!)
 * @param *buffer	Zeiger auf 512 Byte grossen Puffer
 * @return			Adresse des ersten Nutzdatenblocks der Datei
 */
static uint32_t check_fragmentation(uint32_t block, void * buffer) {
	return block;
}
#endif	// MINI_FAT_CHECK_FRAGMENTATION

/*!
 * @brief			Sucht einen Block auf der MMC-Karte, dessen erste Bytes dem Dateinamen entsprechen
 * @param filename 	String im Flash zur Identifikation
 * @param buffer 	Zeiger auf 512 Byte Puffer im SRAM
 * @param end_addr	Byte-Adresse, bis zu der gesucht werden soll
 * @return			Anfangsblock der Nutzdaten der Datei
 * Achtung das Prinzip geht nur, wenn die Dateien nicht fragmentiert sind
 */
uint32_t mini_fat_find_block_P(const char * filename, void * buffer, uint32_t end_addr) {
	end_addr >>= 9;	// letzte Blockadresse ermitteln

	/* zunaechst im EEPROM-FAT-Cache nachschauen */
	uint32_t block = mini_fat_lookup_adr(filename, buffer);
	if (block != 0)	return check_fragmentation(block, buffer);

#ifdef DISPLAY_MINIFAT_INFO
	display_cursor(2, 1);
	char name[7];
	strncpy_P(name, filename, sizeof(name) - 1);
	display_printf("Find %s: ", name);
#endif	// DISPLAY_MINIFAT_INFO

	/* sequenziell die Karte durchsuchen */
	for (block=0; block<end_addr; block++) {
#ifdef DISPLAY_MINIFAT_INFO
		display_cursor(2, 13);
		display_block(block);
#endif	// DISPLAY_MINIFAT_INFO
		uint8_t result = mmc_read_sector(block, buffer);
		if (result != 0) {
#ifdef DISPLAY_MINIFAT_INFO
			display_cursor(4, 10);
			display_printf("Fehler %u", result);
#endif	// DISPLAY_MINIFAT_INFO
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
			LED_on(LED_TUERKIS);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
			return 0xffffffff;
		}
		if (strcmp_P((char *)buffer, filename) == 0) {
			/* gefunden, Adresse ins EEPROM schreiben */
			mini_fat_store_adr(++block);
#ifdef DISPLAY_MINIFAT_INFO
			display_cursor(4, 1);
			display_printf("Found:");
			display_cursor(4, 7);
			display_block(block);
#endif	// DISPLAY_MINIFAT_INFO
			return check_fragmentation(block, buffer);
		}
	}
	return 0xffffffff;
}

/*!
 * Liest die Groesse einer Datei im MiniFAT-Dateisystem auf der MMC/SD-Karte aus
 * @param file_start	Anfangsblock der Datei (Nutzdaten, nicht Header)
 * @param *buffer		Zeiger auf 512 Byte Puffer im SRAM, wird veraendert!
 * @return				Groesse der Datei in Byte, 0 falls Fehler
 */
uint32_t mini_fat_get_filesize(uint32_t file_start, void * buffer) {
	file_len_t length;
	if (mmc_read_sector(file_start-1, buffer) != 0) return 0;
	/* Dateilaenge aus Block 0, Byte 256 bis 259 der Datei lesen */
	uint8_t i;
	uint8_t * ptr = buffer;
	for (i=0; i<4; i++) {
		length.u8[i] = ptr[259-i];
	}
	return length.u32;
}

/*!
 * Leert eine Datei im MiniFAT-Dateisystem auf der MMC/SD-Karte
 * @param file_start	Anfangsblock der Datei
 * @param *buffer		Zeiger auf 512 Byte Puffer im SRAM, wird geloescht!
 */
void mini_fat_clear_file(uint32_t file_start, void * buffer) {
#ifdef DISPLAY_MINIFAT_INFO
	/* Zum Mini-FAT-Screen springen */
	uint8_t i;
	for (i=0; i<sizeof(screen_functions)/sizeof(screen_functions[0]); i++) {
		if (screen_functions[i] == mini_fat_display) break;
	}
	display_screen = i;
	display_clear();
	mini_fat_display();
	display_cursor(2, 1);
	display_printf("Start:");
	display_cursor(2, 7);
	display_block(file_start);
#endif	// DISPLAY_MINIFAT_INFO
	uint32_t length = mini_fat_get_filesize(file_start, buffer) >> 9;
#ifdef DISPLAY_MINIFAT_INFO
	display_cursor(3, 1);
	display_printf("End:");
	display_cursor(3, 5);
	display_block(file_start+length-1);
#endif	// DISPLAY_MINIFAT_INFO
	memset(buffer, 0, 512);	// Puffer leeren
	/* Alle Bloecke der Datei mit dem 0-Puffer ueberschreiben */
	uint32_t addr;
	for (addr=file_start; addr<file_start+length; addr++) {
		uint8_t result = mmc_write_sector(addr, buffer);
		if (result != 0) {
#ifdef DISPLAY_MINIFAT_INFO
			display_cursor(4, 10);
			display_printf("Fehler %u", result);
#endif	// DISPLAY_MINIFAT_INFO
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
			LED_on(LED_TUERKIS);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
			return;
		}
#ifdef DISPLAY_MINIFAT_INFO
		display_cursor(4, 1);
		display_block(addr);
#endif	// DISPLAY_MINIFAT_INFO
	}
}
#endif	// BOT_FS_AVAILABLE
#endif	// MMC_AVAILABLE
#endif	// MCU
