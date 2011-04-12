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
 * \file 	botfs-low_pc.c
 * \brief 	Low-Level-Funktionen des Dateisystems BotFS fuer PC
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.02.2008
 */

#ifdef PC
#include "ct-Bot.h"

#ifdef BOT_FS_AVAILABLE
#include "os_thread.h"
#include "botfs.h"
#include "botfs-low.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define BOTFS_FLUSH_LOG
//#define BOTFS_DEBUG_LOW

#ifndef BOTFS_DEBUG_LOW
#undef PRINT_MSG
#define PRINT_MSG(format, _args...)
#endif

static FILE * image_file; /**< Image-Datei des Volumes */
static botfs_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER; /**< Mutex, um den Dateizugriff zu schuetzen */
#if defined DEBUG_BOTFS || defined DEBUG_BOTFS_LOGFILE
static botfs_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER; /**< Mutes, um das Log zu schuetzen */
FILE * botfs_log_fd; /**< File-Handle fuer Log-Datei */
#endif // defined DEBUG_BOTFS || defined DEBUG_BOTFS_LOGFILE

uint32_t first_block = 0; /**< Adresse des ersten Blocks des Volumes */

/**
 * Laedt das Volume
 * \param *image	Dateiname des Images
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \param create	Soll das Volume erzeugt werden, falls es nicht existiert?
 * \return			0, falls kein Fehler
 */
int8_t botfs_init_low(char * image, void * buffer, uint8_t create) {
	(void) buffer; // kein warning
#ifdef DEBUG_BOTFS_LOGFILE
	botfs_acquire_lock_low(&log_mutex);
	botfs_log_fd = fopen(BOTFS_DEBUG_LOGFILE, "wb");
	botfs_release_lock_low(&log_mutex);
#endif // DEBUG_BOTFS_LOGFILE

	botfs_acquire_lock_low(&file_mutex);
	image_file = fopen(image, "r+b");
	botfs_release_lock_low(&file_mutex);
	if (image_file == NULL) {
		PRINT_MSG("Image-Datei \"%s\" konnte nicht geoeffnet werden", image);
		if (create != 1) {
			return -1;
		}
		PRINT_MSG("Lege neues Images \"%s\" an...", image);
		if (botfs_create_volume(image, BOTFS_DEFAULT_VOL_NAME, BOTFS_DEFAULT_VOL_SIZE) != 0) {
			PRINT_MSG("botfs_create_volume(\"%s\") schlug fehl", image);
			return -2;
		}
		botfs_acquire_lock_low(&file_mutex);
		image_file = fopen(image, "r+b");
		botfs_release_lock_low(&file_mutex);
		if (image_file == NULL) {
			PRINT_MSG("angelegte Image-Datei \"%s\" konnte nicht geoeffnet werden", image);
			return -3;
		}
	}

	PRINT_MSG("botfs_init_low() erfolgreich");
	return 0;
}

/**
 * Ueberprueft ein geladenes Volume
 * \param *image	Dateiname des Images
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes
 * \return 0, falls kein Fehler, sonst Fehlercode (< 0)
 */
int8_t botfs_check_volume_low(char * image, void * buffer) {
	/* Version pruefen */
	botfs_volume_t * volume = buffer;
	if (volume->data.version < BOTFS_MIN_VERSION) {
		printf("Das Volume hat eine zu alte Version!\n");
		printf(" Volume-Version: %u, min. gefordert: %u\n", volume->data.version, BOTFS_MIN_VERSION);
		printf("Bitte Datei \"%s\" loeschen und neu anlegen (lassen).\n", image);
		return -1;
	}

	/* Root-Dir pruefen */
	const char * first_file = botfs_readdir(0, buffer);
	if (first_file == NULL) {
		printf("Fehlerhaftes Volume\n");
		return -2;
	}
	if (strncmp(first_file, "/volumedata", BOTFS_MAX_FILENAME) != 0) {
		printf("Fehler, Volume enhaelt kein Rootverzeichnis\n");
		return -3;
	}

	return 0;
}

/**
 * Erzeugt eine neue Image-Datei fuer ein Volume
 * \param *image	Name der Image-Datei
 * \param size		Groesse der Datei in Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_create_volume_low(const char * image, uint32_t size) {
	/* Datei erzeugen */
	botfs_acquire_lock_low(&file_mutex);
	image_file = fopen(image, "w+b");
	if (image_file == NULL) {
		botfs_release_lock_low(&file_mutex);
		return -1;
	}

	/* Dateigroesse anpassen */
	if (fseek(image_file, size - 1, SEEK_SET) != 0) {
		botfs_release_lock_low(&file_mutex);
		return -2;
	}
	if (putc(0, image_file) != 0) {
		botfs_release_lock_low(&file_mutex);
		return -3;
	}
	if (fflush(image_file) != 0) {
		botfs_release_lock_low(&file_mutex);
		return -4;
	}
	botfs_release_lock_low(&file_mutex);
	return 0;
}

/**
 * Liest einen BOTFS_BLOCK_SIZE Byte grossen Block
 * \param block		Blockadresse der Daten
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes, in den die Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_read_low(uint16_t block, void * buffer) {
	PRINT_MSG("botfs_read_low with block=%d, &buffer=0x%lx", block, (size_t) buffer);
	botfs_acquire_lock_low(&file_mutex);
	if (fseek(image_file, block * BOTFS_BLOCK_SIZE, SEEK_SET) != 0) {
		botfs_release_lock_low(&file_mutex);
		return -1;
	}
	if (fread(buffer, BOTFS_BLOCK_SIZE, 1, image_file) != 1) {
		botfs_release_lock_low(&file_mutex);
		return -2;
	}
	botfs_release_lock_low(&file_mutex);
	return 0;
}

/**
 * Schreibt einen BOTFS_BLOCK_SIZE Byte grossen Block
 * \param block		Blockadresse der Daten
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes, dessen Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_write_low(uint16_t block, void * buffer) {
	PRINT_MSG("botfs_write_low with block=%d, &buffer=0x%lx", block, (size_t) buffer);
	botfs_acquire_lock_low(&file_mutex);
	if (fseek(image_file, block * BOTFS_BLOCK_SIZE, SEEK_SET) != 0) {
		botfs_release_lock_low(&file_mutex);
		return -1;
	}
	if (fwrite(buffer, BOTFS_BLOCK_SIZE, 1, image_file) != 1) {
		botfs_release_lock_low(&file_mutex);
		return -2;
	}
	if (fflush(image_file) != 0) {
		botfs_release_lock_low(&file_mutex);
		return -3;
	}
	botfs_release_lock_low(&file_mutex);
	return 0;
}

/**
 * Schliesst das BotFS-Volume (beendet BotFS sauber)
 */
void botfs_close_volume_low(void) {
	botfs_acquire_lock_low(&file_mutex);
	fclose(image_file);
	botfs_release_lock_low(&file_mutex);
}

#if defined DEBUG_BOTFS || defined DEBUG_BOTFS_LOGFILE
static char log_buf[1024 * 1024]; /**< Log-Puffer */
static char * ptr = log_buf; /**< aktuelle Position im Log-Puffer */

/**
 * Schreibt den Log-Puffer raus
 * \param *fd	File-Handle fuer Ausgabe
 */
void botfs_flush_log(FILE * fd) {
	if (fd == NULL) {
		return;
	}
	/* write buffer back to file */
	botfs_acquire_lock_low(&log_mutex);
	fwrite(log_buf, ptr - log_buf, 1, fd);
	fflush(fd);
	ptr = log_buf;
	botfs_release_lock_low(&log_mutex);
}

/**
 * Schreibt eine Log-Message
 * \param *fd		File-Handle fuer Ausgabe
 * \param *format	Format-String (wie bei printf)
 */
void botfs_log_low(FILE * fd, const char * format, ...) {
	if (ptr > log_buf + 2 * sizeof(log_buf) / 3) {
		botfs_flush_log(fd);
	}
	va_list	args;
	va_start(args, format);

	/* log time */
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	botfs_acquire_lock_low(&log_mutex);
	ptr += strftime(ptr, 30, "%Y-%m-%d %H:%M:%S ", timeinfo);

	/* write message */
	ptr += vsnprintf(ptr, 512, format, args);

	/* new line */
	*ptr = '\n';
	ptr++;

	botfs_release_lock_low(&log_mutex);
	va_end(args);

#ifdef BOTFS_FLUSH_LOG
	botfs_flush_log(fd);
#endif
}
#endif // defined DEBUG_BOTFS || defined DEBUG_BOTFS_LOGFILE

#endif // BOT_FS_AVAILABLE
#endif // PC
