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
 * \file 	botfs-low.c
 * \brief 	Low-Level-Funktionen des Dateisystems BotFS fuer MCU
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.02.2008
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef BOT_FS_AVAILABLE
#include "botfs.h"
#include "botfs-low.h"
#include "os_thread.h"
#include "mmc.h"
#include "display.h"
#include "log.h"
#include <string.h>
#include <ctype.h>

uint32_t first_block; /**< Adresse des ersten Blocks des Volumes */
static botfs_mutex_t botfs_mutex = BOTFS_MUTEX_INITIALIZER; /**< sperrt den Zugriff auf das Volume */

/**
 * Sucht eine FAT16-Datei und gibt ihren ersten Block zurueck
 * \param *imagename	Dateiname, max. 8 + 3 Zeichen
 * \param *buffer		Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return				Blockadresse, an der die Datei beginnt, 0 falls Fehler
 *
 * Eigentlich muessten wir hier ueberall mit Sektoradressen von 32 Bit arbeiten. Da wir aber davon ausgehen,
 * dass die Image-Datei auf der ersten Partition liegt, reichen 16 Bit aus, um den ersten Sektor der Partition,
 * das Root-Verzeichnis und die FAT zu adressieren.
 */
static uint32_t botfs_get_image_sector(char * imagename, void * buffer) {
	/* Dateinamen anpassen */
	char * pName = imagename;
	char * pExt = &imagename[strlen(imagename)];
	while (*pName != 0) {
		if (*pName == '.') {
			*pName = 0;
			pExt = pName + 1;
		}
		*pName = (char) toupper(*pName);
		pName++;
	}
	pName = imagename;

	/* MBR lesen */
	if (botfs_read_low(0, buffer) != 0) {
		return 0;
	}
	botfs_mbr_t * p_mbr = buffer;

	/* Bootsektor der ersten Partition ermitteln */
	const uint16_t first_sect = (uint16_t) p_mbr->part0.first_sect_offset; // 32
	PRINT_MSG("first_sect=0x%x", first_sect);

	/* Bootsektor lesen */
	if (botfs_read_low(first_sect, buffer) != 0) {
		return 0;
	}
	botfs_fat16_bootsector_t * p_bs = buffer;

	/* Bootsektor auswerten */
	const uint16_t fat_offset = p_bs->reserved_sect + first_sect; // 32
	PRINT_MSG("fat_offset=0x%x", fat_offset);
	const uint16_t root_offset = fat_offset + p_bs->fat_copies * p_bs->sect_per_fat; // 32
	PRINT_MSG("root_offset=0x%x", root_offset);
	const uint8_t n = (uint8_t) (p_bs->root_dir_entries / (BOTFS_BLOCK_SIZE / sizeof(botfs_fat16_dir_entry_t)));
	PRINT_MSG("Groesse des Rootverz.: %u", n);
	const uint16_t data_offset = root_offset + (p_bs->root_dir_entries
		* sizeof(botfs_fat16_dir_entry_t) + (BOTFS_BLOCK_SIZE - 1)) / BOTFS_BLOCK_SIZE;
	PRINT_MSG("Erster Datensektor: 0x%04x", data_offset);
	const uint8_t sect_per_cluster = p_bs->sect_per_cluster;
	PRINT_MSG("sect_per_cluster=%u", sect_per_cluster);

	/* Root-Verzeichnis durchsuchen */
	uint16_t block = root_offset; // 32
	const uint8_t lenName = (uint8_t) strlen(pName);
	uint8_t i;
	for (i = n; i > 0; --i) {
		/* Blockweise Verzeichniseintraege laden */
		if (botfs_read_low(block++, buffer) != 0) {
			return 0;
		}
		botfs_fat16_dir_entry_t * p_dir_entry = buffer;
		uint8_t j;
		for (j = BOTFS_BLOCK_SIZE / sizeof(botfs_fat16_dir_entry_t); j > 0; --j) {
			/* Verzeichniseintraege durchsuchen */
			if (strncmp(p_dir_entry->name, pName, lenName) == 0 && strncmp(p_dir_entry->extension, pExt, strlen(pExt)) == 0) {
				/* Treffer, Datei gefunden */
				const uint32_t file_block = (uint32_t) (p_dir_entry->first_cluster - 2) * sect_per_cluster + data_offset;
				PRINT_MSG("file_block=0x%x", file_block);

				/* FAT auflisten */
				uint16_t next_cluster = p_dir_entry->first_cluster;
				uint16_t last_cluster = next_cluster;
				PRINT_MSG(" Cluster: 0x%04x", next_cluster);
				uint16_t last_fat_block = 0;
				while (42) {
					/* FAT-Eintraege der Datei einlesen, auf Fragmentierung checken */
					const uint16_t fat_block = fat_offset + next_cluster / (512 / sizeof(uint16_t));
//					PRINT_MSG("fat_block=0x%04x", fat_block);
					if (fat_block != last_fat_block) {
						if (botfs_read_low(fat_block, buffer) != 0) {
							return 0;
						}
						last_fat_block = fat_block;
					}
					const uint16_t entry_offset = next_cluster % (512 / sizeof(uint16_t));
//					PRINT_MSG("entry_offset=0x%04x", entry_offset);
					uint16_t * ptr = buffer;
					next_cluster = ptr[entry_offset];
//					PRINT_MSG("next_cluster=0x%04x", next_cluster);
					if (next_cluster != last_cluster + 1) {
						if (next_cluster >= 0xfff0) {
							/* Dateiende */
							break;
						}
						PRINT_MSG("Image-Datei fragmentiert, Abbruch");
						return 0;
					}
					last_cluster = next_cluster;
				}
				return file_block;
			}
			p_dir_entry++;
		}
	}
	return 0;
}

/**
 * Laedt das Volume
 * \param *image	Dateiname des Images
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \param create	wird ignoriert
 * \return			0, falls kein Fehler
 */
int8_t botfs_init_low(char * image, void * buffer, uint8_t create) {
	(void) create;
	first_block = botfs_get_image_sector(image, buffer);
	if (first_block == 0) {
		return -1;
	}
	PRINT_MSG("block=0x%x", first_block);
	PRINT_MSG("botfs_init_low() erfolgreich");
	return 0;
}

/**
 * Wartet, bis ein Mutex verfuegbar ist und sperrt es dann
 * \param *p_mutex Zeiger auf das gewuenschte Mutex
 */
void botfs_acquire_lock_low(uint8_t * p_mutex) {
	os_enterCS();
	while (*p_mutex == 1) {
		os_exitCS();
		PRINT_MSG("thread 0x%x waiting", os_thread_running);
		os_thread_sleep(1);
		os_enterCS();
	}
	*p_mutex = 1;
	os_exitCS();
}

/**
 * Liest einen BOTFS_BLOCK_SIZE Byte grossen Block
 * \param block		Blockadresse der Daten
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes, in den die Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_read_low(uint16_t block, void * buffer) {
	int8_t tmp;
	/* spinlock */
	botfs_acquire_lock_low(&botfs_mutex);
	/* MMC-read */
	tmp = (int8_t) mmc_read_sector(get_sector(block), buffer);
	botfs_release_lock_low(&botfs_mutex);
	return tmp;
}

/**
 * Schreibt einen BOTFS_BLOCK_SIZE Byte grossen Block
 * \param block		Blockadresse der Daten
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes, dessen Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_write_low(uint16_t block, void * buffer) {
	int8_t tmp;
	/* spinlock */
	botfs_acquire_lock_low(&botfs_mutex);
	/* MMC-write */
	tmp = (int8_t) mmc_write_sector(get_sector(block), buffer);
	botfs_release_lock_low(&botfs_mutex);
	return tmp;
}

/**
 * Schliesst das BotFS-Volume (beendet BotFS sauber)
 */
void botfs_close_volume_low(void) {
	// nichts zu tun
}
#endif // BOT_FS_AVAILABLE
#endif // MCU
