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
 * \file 	botfs_pc.c
 * \brief 	PC-only Teile des Dateisystems BotFS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	17.10.2010
 */

#ifdef PC
#include "ct-Bot.h"

#ifdef BOT_FS_AVAILABLE
#include "botfs.h"
#include "botfs-low.h"
#include "log.h"
#include <string.h>

extern botfs_volume_data_t botfs_vol_data; /**< Root-Dir- und Freelist-Adresse */
extern botfs_mutex_t botfs_mutex; /**< sperrt den Zugriff auf nicht threadsichere Funktionen */

/**
 * Erzeugt ein neues Volume
 * \param *image	Dateiname fuer das Volume-Image
 * \param *name		Name des Volumes
 * \param size		Groesse des Volumes in Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_create_volume(const char * image, const char * name, uint32_t size) {
	/* Werte pruefen */
	if (size > BOTFS_MAX_VOLUME_SIZE) {
		return -1;
	}
	if (strlen(name) > BOTFS_MAX_FILENAME) {
		return -2;
	}

	/* Volume erzeugen */
	if (botfs_create_volume_low(image, size) != 0) {
		return -3;
	}

	/* Volume-Daten eintragen */
	botfs_volume_t volume;
	memset(&volume, 0, sizeof(botfs_volume_t));
	/* Volume-Name */
	strncpy(volume.data.name, name, BOTFS_MAX_FILENAME);
	/* Volume-Groesse */
	volume.data.size = size;
	printf("volume.size=%u KB\n", size / 1024);
	/* FAT- und Freelist-Adressen */
	botfs_volume_data_t * volume_data = &volume.data.ctrldata;
	volume_data->rootdir.start = BOTFS_HEADER_POS + (sizeof(botfs_volume_t) / BOTFS_BLOCK_SIZE);
	volume_data->rootdir.used.start = volume_data->rootdir.start;
	printf("rootdir.start=0x%x\n", volume_data->rootdir.start * BOTFS_BLOCK_SIZE);
	volume_data->rootdir.end = volume_data->rootdir.start + BOTFS_DIR_BLOCK_CNT;
	volume_data->rootdir.used.end = volume_data->rootdir.end;
	printf("rootdir.end=0x%x\n", volume_data->rootdir.end * BOTFS_BLOCK_SIZE);
	volume_data->freelist.start = volume_data->rootdir.end + 1;
	volume_data->freelist.used.start = volume_data->freelist.start;
	printf("freelist.start=0x%x\n", volume_data->freelist.start * BOTFS_BLOCK_SIZE);
	volume_data->freelist.end = volume_data->freelist.start + BOTFS_FREEL_BL_CNT;
	volume_data->freelist.used.end = volume_data->freelist.end;
	printf("freelist.end=0x%x\n", volume_data->freelist.end * BOTFS_BLOCK_SIZE);
	volume.data.first_data = volume_data->freelist.end + 1;
	printf("first_data=0x%x\n", volume.data.first_data * BOTFS_BLOCK_SIZE);
	volume_data->blocks_free = (volume.data.size / BOTFS_BLOCK_SIZE)
		- (BOTFS_HEADER_POS + (sizeof(botfs_volume_t) / BOTFS_BLOCK_SIZE) + BOTFS_DIR_BLOCK_CNT
		+ BOTFS_FREEL_BL_CNT);
	printf("blocks_free=0x%x\n", volume_data->blocks_free);
	if (botfs_write_low(BOTFS_HEADER_POS, volume.raw) != 0) {
		return -4;
	}

	/* Root-Verzeichnis erzeugen */
	botfs_file_header_t root_header;
	memset(&root_header, 0, sizeof(botfs_file_header_t));
	if (botfs_write_low(volume_data->rootdir.start, &root_header) != 0) {
		return -5;
	}
	botfs_rootdir_t new_dir;
	memset(&new_dir, 0, sizeof(botfs_rootdir_t));
	botfs_dir_block_t * pDir = new_dir.dirblocks;
	/* erste Datei anlegen, die auf den Volume-Header zeigt */
	pDir->files->descr.start = 0;
	pDir->files->descr.end = (sizeof(botfs_volume_t) / BOTFS_BLOCK_SIZE);
	strcpy(pDir->files->name, BOTFS_VOLUME_DATA);
	/* alle Root-Dir-Bloecke schreiben */
	uint8_t i;
	uint16_t block = volume_data->rootdir.start + 1; // erster Block = "Datei-Header" des Root-Dirs
	for (i = BOTFS_DIR_BLOCK_CNT; i > 0; --i) {
		if (botfs_write_low(block++, pDir) != 0) {
			return -6;
		}
		pDir++;
	}

	/* Freelist erzeugen */
	botfs_file_header_t freel_header;
	memset(&freel_header, 0, sizeof(botfs_file_header_t));
	if (botfs_write_low(volume_data->freelist.start, &freel_header) != 0) {
		return -7;
	}
	botfs_freelist_t new_freelist;
	memset(&new_freelist, 0, sizeof(botfs_freelist_t));
	/* Ersten Frei-Eintrag bauen (umfasst kompletten Datenbereich) */
	botfs_freelist_block_t * pFreeL = new_freelist.freelistblocks;
	printf("1. Freelist-Eintrag:\n");
	pFreeL->freeblocks->block = volume.data.first_data;
	printf("freeblocks->block=0x%x\n", pFreeL->freeblocks->block * BOTFS_BLOCK_SIZE);
	pFreeL->freeblocks->size = volume_data->blocks_free;
	printf("freeblocks->size=%u KB\n", pFreeL->freeblocks->size / 2);
	/* Alle Freelist-Blocke schreiben */
	block = volume_data->freelist.start + 1; // erster Block = "Datei-Header" der Freelist
	for (i = BOTFS_FREEL_BL_CNT; i > 0; --i) {
		if (botfs_write_low(block++, pFreeL) != 0) {
			return -8;
		}
		pFreeL++;
	}

	return 0;
}

/**
 * Kopiert eine Datei vom PC-Dateisystem auf das BotFS-Volume
 * \param *to		Dateiname der Zieldatei auf dem Volume
 * \param *from		Pfadname der Quelldatei
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes
 * \return			0, falls kein Fehler
 */
int8_t botfs_copy_file(const char * to, const char * from, void * buffer) {
	FILE * source = fopen(from, "rb");
	if (source == NULL) {
		printf("Datei \"%s\" nicht vorhanden!\n", from);
		return -1;
	}
	if (fseek(source, 0, SEEK_END) != 0) {
		printf("Fehler beim Dateizugriff\n");
		return -2;
	}
	const long int pos = ftell(source);
	size_t file_size = (pos + 1) / BOTFS_BLOCK_SIZE;
	if (pos == -1) {
		printf("Fehler beim Dateizugriff\n");
		return -3;
	}
	float size_rem = ((float) (pos + 1) / (float) BOTFS_BLOCK_SIZE) - file_size;
	if (size_rem > 0.0f) {
		++file_size;
	}
	printf("Datei wird %u Bloecke gross sein\n", (uint16_t) file_size);

	botfs_file_descr_t file;
	if (botfs_open(to, &file, BOTFS_MODE_r, buffer) == 0) {
		/* Datei bereits vorhanden -> loeschen */
		if (botfs_unlink(to, buffer) != 0) {
			printf("Fehler, bereits existierende Datei konnte nicht geloescht werden\n");
			return -4;
		}
	}

	if (botfs_create(to, file_size, buffer) != 0) {
		printf("Fehler, Datei konnte nicht angelegt werden\n");
		return -5;
	}

	/* Datei zum Lesen + Schreiben oeffnen */
	if (botfs_open(to, &file, BOTFS_MODE_W, buffer) != 0) {
		printf("Fehler beim Oeffnen der Datei\n");
		return -6;
	}

	if (fseek(source, 0, SEEK_SET) != 0) {
		printf("Fehler beim Dateizugriff\n");
		return -7;
	}

	size_t i;
	for (i = 0; i < file_size; ++i) {
		/* Puffer vorbereiten */
		memset(buffer, 0, BOTFS_BLOCK_SIZE);

		/* Datei in Puffer lesen */
		fread(buffer, BOTFS_BLOCK_SIZE, 1, source);

		/* Treiber-Aufruf */
		if (botfs_write(&file, buffer) != 0) {
			printf("Fehler beim Schreiben der Daten\n");
			return -8;
		}
	}

	printf("%u Daten-Bloecke kopiert\n", (uint16_t) file_size);

	fclose(source);
	botfs_close(&file, buffer);

	return 0;
}

/**
 * Kopiert eine Datei vom BotfS-Volume ins PC-Dateisystem
 * \param *to		Pfadname der Zieldatei
 * \param *from		Dateiname der Quelldatei auf dem Volume
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes
 * \return			0, falls kein Fehler
 */
int8_t botfs_extract_file(const char * to, const char * from, void * buffer) {
	FILE * dest = fopen(to, "wb");
	if (dest == NULL) {
		printf("Zeildatei \"%s\" konnte nicht angelegt werden!\n", to);
		return -1;
	}

	/* Quelldatei oeffnen */
	botfs_file_descr_t file;
	if (botfs_open(from, &file, BOTFS_MODE_r, buffer) != 0) {
		printf("Fehler, Quelldatei konnte nicht geoeffnet werden\n");
		return -2;
	}

	/* Dateigroesse bestimmen */
	const uint16_t file_size = botfs_get_filesize(&file);
	printf("Datei ist %u Bloecke gross\n", file_size);

	size_t i;
	for (i = 0; i < file_size; ++i) {
		/* Datei in Puffer lesen */
		if (botfs_read(&file, buffer) != 0) {
			printf("Fehler beim Lesen aus der Quelldatei\n");
			return -3;
		}

		/* Dateibloecke in Zieldatei schreiben */
		if (fwrite(buffer, BOTFS_BLOCK_SIZE, 1, dest) != 1) {
			printf("Fehler beim Schreiben in die Zieldatei\n");
			return -4;
		}
	}

	printf("%u Daten-Bloecke kopiert\n", file_size);

	fclose(dest);
	botfs_close(&file, buffer);

	return 0;
}

/**
 * Gibt Root-Dir-Eintraege zurueck
 * \param offset	Nummer des Eintrags
 * \param *buffer	Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return			Zeiger auf Dateiname (aus buffer)
 */
char * botfs_readdir(uint8_t offset, void * buffer) {
	botfs_acquire_lock_low(&botfs_mutex);
	botfs_dir_block_t * root_block = buffer;
	uint8_t count = 0;
	uint8_t i;
	botfs_rewind(&botfs_vol_data.rootdir);
	for (i = 0; i < BOTFS_DIR_BLOCK_CNT; ++i) {
		/* alle Root-Dir-Bloecke durchgehen */
		if (botfs_read(&botfs_vol_data.rootdir, buffer) != 0) {
			botfs_release_lock_low(&botfs_mutex);
			return NULL;
		}
		botfs_file_t * ptr = root_block->files;
		uint8_t j;
		for (j = 0; j < BOTFS_FILE_DESC_CNT; ++j) {
			/* alle Dateieintraege durchsuchen */
			if (strlen(ptr->name) > 0) {
				if (count++ == offset) {
					botfs_release_lock_low(&botfs_mutex);
					return ptr->name;
				}
			}
			ptr++;
		}
	}
	botfs_release_lock_low(&botfs_mutex);
	return NULL;
}

/**
 * Gibt alle Freelist-Eintraege aus
 * \param *buffer Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 */
void botfs_print_freelist(void * buffer) {
	botfs_acquire_lock_low(&botfs_mutex);
	botfs_rewind(&botfs_vol_data.freelist);
	botfs_freelist_block_t * freelist = buffer;
	uint8_t i;
	for (i = 0; i < BOTFS_FREEL_BL_CNT; ++i) {
		if (botfs_read(&botfs_vol_data.freelist, buffer) != 0) {
			botfs_release_lock_low(&botfs_mutex);
			printf("Fehler beim Lesen der Freelist\n");
			return;
		}
		botfs_freelist_entry_t * pFree = freelist->freeblocks;
		uint8_t j;
		for (j = 0; j < BOTFS_FREEL_BL_SIZE; ++j) {
			if (pFree->size != 0) {
				printf("Block %u, Eintrag %u: start: 0x%04x\tsize: 0x%04x\n", i, j, pFree->block, pFree->size);
			}
			pFree++;
		}
	}
	botfs_release_lock_low(&botfs_mutex);
}

#endif // BOT_FS_AVAILABLE
#endif // PC
