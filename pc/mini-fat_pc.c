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
 * \file 	mini-fat_pc.c
 * \brief 	Routinen zum Erstellen von markierten Files fuer eine MMC-Karte.
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	04.01.2007
 */

#ifdef PC

#include "ct-Bot.h"
#include "mini-fat.h"
#include "mmc-vm.h"
#include "mmc-emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * Erzeugt eine Datei, die an den ersten Bytes die ID enthaelt. Dann folgen 512 - sizeof(id) nullen
 * Danach kommen so viele size kByte Nullen
 * \param filename Der Dateiname der zu erzeugenden Datei
 * \param id_string Die ID des Files, wie sie zu beginn steht
 * \param size kByte Nutzdaten, die der MCU spaeter beschreiben darf
 */
void create_mini_fat_file(const char * filename, const char * id_string, uint32_t size){
	printf("Erstelle eine Mini-Fat-Datei mit dem Namen %s\n", filename);
	FILE *fp = fopen(filename, "wb");
	if (fp == NULL){
		printf("Datei konnte nicht zum Schreiben geoeffnet werden! Abbruch!\n");
		return;
	}

	/* Dateiparameter vorbereiten */
	mini_fat_header_t header = {"", {0}, {0}};
	strncpy((char * ) header.filename, id_string, MMC_FILENAME_MAX);
	header.length.u32 = size * 1024 - 512; // im ersten Block stehen interne Daten

	printf("Schreibe ID: \"%s\"\n", header.filename);
	/* Datei-Header schreiben */
	fwrite(&header, sizeof(mini_fat_header_t), 1, fp);

	printf("Erzeuge Speicherplatz fuer %u kByte Nutzdaten\n",size);
	fseek(fp, size * 1024 - 1, SEEK_SET); // ans Dateiende springen
	putc(0, fp); // eine Null schreiben
	fclose(fp);
}

/**
 * Erzeugt eine Mini-Fat-Datei in einer emulierten MMC
 * \param addr			Die Adresse auf der emulierten Karte, an der die Datei beginnen soll
 * \param id_string 	Die ID der Datei, wie sie zu Beginn in der Datei steht
 * \param size 			KByte Nutzdaten, die die Datei umfasst
 * Erzeugt eine Datei, die an den ersten Bytes die ID enthaelt. Dann folgen 512 - sizeof(id) Nullen
 * Danach kommen size * 1024 Nullen
 */
void create_emu_mini_fat_file(uint32_t addr, const char * id_string, uint32_t size) {
#ifndef MMC_VM_AVAILABLE
	/* keine warnings */
	(void) addr;
	(void) id_string;
	(void) size;

	printf("Bitte mit MMC_VM_AVAILABLE compilieren, Abbruch!\n");
	return;
#else
	/* some checks */
	if (mmc_emu_get_init_state() != 0) mmc_emu_init();
	if (mmc_emu_get_init_state() != 0) {
		printf("Fehler beim Initialisieren der emulierten MMC, Abbruch\n");
		return;
	}

	if (mmc_emu_get_size() < addr+size) {
		printf("Emulierte MMC zu klein fuer Adresse 0x%x und Groesse %d, Abbruch\n", addr, size);
		return;
	}

	uint32_t old_addr = mmc_fopen(id_string);
	if (old_addr != 0) {
		printf("Mini-Fat-Datei existiert bereits an Adresse 0x%x, Abbruch!\n", old_addr-512);
		return;
	}

	if (addr % 512 != 0) {
		printf("Adresse ist nicht 512 Byte aligned, Abbruch!\n");
		return;
	}

	printf("Erstelle eine Mini-Fat-Datei in %s an Adresse 0x%x mit dem Namen %s\n", MMC_EMU_FILE, addr, id_string);

	uint8_t * p_buffer = calloc(512, 1);
	if (p_buffer == NULL) return;

	/* Dateiparameter vorbereiten */
	uint8_t id_len = strlen(id_string) >= MMC_FILENAME_MAX ? 254 : strlen(id_string);
	file_len_t length = {size * 1024 - 512};	// im ersten Block stehen interne Daten
	strncpy((char *) p_buffer, id_string, id_len);

	p_buffer[256] = length.u8[3];
	p_buffer[257] = length.u8[2];
	p_buffer[258] = length.u8[1];
	p_buffer[259] = length.u8[0];

	/* Datei-Header schreiben */
	if (mmc_emu_write_sector(addr/512, p_buffer) != 0) {
		printf("Datei konnte nicht korrekt geschrieben werden!\n");
		return;
	}

	printf("done.\n");

#endif // MMC_VM_AVAILABLE
}

/**
 * Loescht eine Mini-Fat-Datei in einer emulierten MMC
 * \param id_string 	Die ID der Datei, wie sie zu Beginn in der Datei steht
 */
void delete_emu_mini_fat_file(const char * id_string) {
#ifndef MMC_VM_AVAILABLE
	(void) id_string; // kein warning
	printf("Bitte mit MMC_VM_AVAILABLE compilieren, Abbruch!\n");
	return;
#else
	/* some checks */
	if (mmc_emu_get_init_state() != 0) mmc_emu_init();
	if (mmc_emu_get_init_state() != 0) {
		printf("Fehler beim Initialisieren der emulierten MMC, Abbruch\n");
		return;
	}

	uint32_t addr = mmc_fopen(id_string);
	if (addr == 0) {
		printf("Mini-Fat-Datei %s existiert nicht, Abbruch!\n", id_string);
		return;
	}

	uint8_t* p_buffer = calloc(512, 1);

	/* Datei leeren und Header loeschen */
	mmc_clear_file(addr);

	if (mmc_emu_write_sector(addr-1, p_buffer) != 0) {
		printf("Datei konnte nicht korrekt geloescht werden!\n");
		return;
	}

	printf("done.\n");

#endif // MMC_VM_AVAILABLE
}

#endif // PC
