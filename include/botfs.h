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
 * \file 	botfs.h
 * \brief 	Dateisystem BotFS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.02.2008
 */

#ifndef BOTFS_H_
#define BOTFS_H_

//#define DEBUG_BOTFS /**< schaltet Debug-Ausgaben an */
//#define DEBUG_BOTFS_LOGFILE /**< schaltet Debug-Ausgaben in botfs.log an */

#include "ct-Bot.h"
#include "botfs_config.h"
#include "botfs_types.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

#if defined BOT_FS_AVAILABLE && ! defined SDFAT_AVAILABLE

#ifndef LOG_AVAILABLE
#undef DEBUG_BOTFS
#endif

#if defined DEBUG_BOTFS
#ifdef MCU
#define PRINT_MSG LOG_DEBUG
#else
//#define PRINT_MSG LOG_DEBUG
#define PRINT_MSG(_format, _args...) botfs_log_low(stdout, _format, ## _args)
#endif // MCU
#elif defined DEBUG_BOTFS_LOGFILE
#define PRINT_MSG(_format, _args...) botfs_log_low(botfs_log_fd, _format, ## _args)
extern FILE * botfs_log_fd; /**< File-Handle fuer Log-Datei */
#else
#define PRINT_MSG(_format, _args...)
#endif // DEBUG_BOTFS

extern char * botfs_volume_image_file; /**< Dateiname des BotFS-Volume-Images */

/**
 * Initialisiert ein Volume
 * \param *image	Dateiname des Images
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \param create	Soll das Volume erzeugt werden, falls es nicht existiert?
 * \return			0, falls kein Fehler
 */
int8_t botfs_init(char * image, void * buffer, uint8_t create);

/**
 * Oeffnet eine Datei
 * \param filename	Dateiname
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param mode		Modus, in dem die Datei geoeffnet wird
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_open(const char * filename, botfs_file_descr_t * file, uint8_t mode, void * buffer);

/**
 * Setzt den Dateizeiger an eine neue Position
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param offset	Offset der Dateiposition, an die gesprungen werden soll, in Bloecken
 * \param origin	SEEK_SET, SEEK_CUR oder SEEK_END
 */
static inline void botfs_seek(botfs_file_descr_t * file, int16_t offset, uint8_t origin) {
	/* Basiswert in Abhaengigkeit von origin laden */
	uint16_t tmp;
	switch(origin) {
	case SEEK_SET:
		tmp = (uint16_t) (file->start + 1);
		break;
	case SEEK_CUR:
		tmp = file->pos;
		break;
	case SEEK_END:
		tmp = file->end;
		break;
	default:
		return;
	}

	/* Position speichern */
	file->pos = (uint16_t) ((int32_t) tmp + offset);
}

/**
 * Setzt den Dateizeiger auf den Anfang einer Datei zurueck
 * \param *file	Datei-Deskriptor
 */
static inline void botfs_rewind(botfs_file_descr_t * file) {
	botfs_seek(file, 0, SEEK_SET);
}

/**
 * Liest BOTFS_BLOCK_SIZE Bytes aus einer Datei in einen Puffer
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte, in die Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_read(botfs_file_descr_t * file, void * buffer);

/**
 * Schreibt BOTFS_BLOCK_SIZE Bytes aus einem Puffer in eine Datei
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer mit mindestens BOTFS_BLOCK_SIZE Byte, dessen Daten in die Datei geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_write(botfs_file_descr_t * file, void * buffer);

/**
 * Legt eine neue Datei an
 * \param *filename	Dateiname
 * \param size		Groesse der Datei in Bloecken
 * \param alignment	Ausrichtung des Dateianfangs an einer X-Blockgrenze (normalerweise 0)
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_create(const char * filename, uint16_t size, uint16_t alignment, void * buffer);

/**
 * Entfernt eine Datei
 * \param *filename	Dateiname
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_unlink(const char * filename, void * buffer);

/**
 * Benennt eine Datei um
 * \param *filename	Dateiname
 * \param *new_name	Neuer Dateiname
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_rename(const char * filename, const char * new_name, void * buffer);

/**
 * Schreibt die Information ueber benutzte Bloecke in den Datei-Header
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_flush_used_blocks(botfs_file_descr_t * file, void * buffer);

/**
 * Schliesst eine Datei, d.h. gepufferte Daten werden zurueckgeschrieben
 * \param *file		Zeiger auf Dateideskriptor
 * \param *buffer	Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 */
static inline void botfs_close(botfs_file_descr_t * file, void * buffer) {
	botfs_flush_used_blocks(file, buffer);
}

/**
 * Liest die frei verwendbaren Header-Daten einer Datei aus
 * \param *file			Zeiger auf Datei-Deskriptor
 * \param *headerdata	Zeiger auf die gewuenschten Header-Daten (liegen in buffer[])
 * \param *buffer		Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return				0, falls kein Fehler
 */
int8_t botfs_read_header_data(botfs_file_descr_t * file, uint8_t ** headerdata, void * buffer);

/**
 * Schreib die frei verwendbaren Header-Daten einer Datei in den Header
 * \param *file			Zeiger auf Datei-Deskriptor
 * \param *buffer		Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return				0, falls kein Fehler
 *
 * buffer muss vorher durch botfs_read_header_data() mit den kompletten
 * Header-Daten gefuellt worden sein!
 */
int8_t botfs_write_header_data(botfs_file_descr_t * file, void * buffer);

/**
 * Gibt die Groesse einer Datei zurueck
 * \param *file	Zeiger auf Dateideskriptor
 * \return		Dateigroesse in Bloecken
 */
static inline uint16_t botfs_get_filesize(botfs_file_descr_t * file) {
	return (uint16_t) (file->end - file->start - (uint16_t) (BOTFS_HEADER_SIZE - 1));
}

/**
 * Beendet BotFS sauber
 */
void botfs_close_volume(void);

#ifdef BOTFS_STREAM_AVAILABLE
/**
 * Oeffnet einen Zeichenstrom, so dass er mit botfs_stream_read*() verwendet werden kann
 * @param *stream		Zeiger auf Stream-Objekt, das initialisiert werden soll
 * @param *file			Zeiger auf Deskriptor der Datei, auf dem der Stream aufsetzen soll
 * @param *block_buffer	Zeiger auf Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 */
void botfs_stream_open(botfs_stream_t * stream, botfs_file_descr_t * file, void * block_buffer);

/**
 * Schliesst einen Zeichenstrom (und die darunterliegende Datei)
 * \param *stream	Zeiger auf Stream-Objekt, das geschlossen werden soll
 * \param *buffer	Zeiger auf Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 */
static inline void botfs_stream_close(botfs_stream_t * stream, void * buffer) {
	botfs_close(stream->p_file, buffer);
	stream->p_file = NULL;
}

/**
 * Gibt an, ob ein Stream-Objekt gueltig ist
 * \param *stream	Zeiger auf Stream-Objekt
 * \return			True oder False
 */
static inline uint8_t botfs_stream_valid(botfs_stream_t * stream) {
	return (uint8_t) (stream->p_file != NULL);
}

/**
 * Gibt die aktuelle Byte-Position des Zeichenstroms zurueck
 * \param *stream	Zeiger auf Stream-Objekt
 * \return			Position in Byte
 */
static inline int32_t botfs_stream_tell(botfs_stream_t * stream) {
	return (int32_t) stream->p_file->pos * BOTFS_BLOCK_SIZE + stream->pos;
}

/**
 * Setzt den Zeichenstrom an eine neue Position
 * \param *stream	Zeiger auf Stream-Objekt
 * \param offset	Offset der Position, an die gesprungen werden soll, in Byte
 * \param origin	SEEK_SET, SEEK_CUR oder SEEK_END
 */
static inline void botfs_stream_seek(botfs_stream_t * stream, int32_t offset, uint8_t origin) {
	stream->block_in_buf = (uint16_t) ~0U;
	int16_t blk_offset = (int16_t) (offset / BOTFS_BLOCK_SIZE);
	botfs_seek(stream->p_file, blk_offset, origin);

	/* Basiswert in Abhaengigkeit von origin laden */
	const int16_t offset16 = (int16_t) (offset - blk_offset * BOTFS_BLOCK_SIZE);
	int16_t tmp;
	switch (origin) {
	case SEEK_SET:
		tmp = 0;
		break;
	case SEEK_CUR:
		tmp = (int16_t) stream->pos;
		break;
	case SEEK_END:
		tmp = BOTFS_BLOCK_SIZE - 1;
		break;
	default:
		return;
	}

	/* Position speichern */
	stream->pos = tmp + offset16;
}

/**
 * Liest Zeichen aus einem Strom in einen Puffer, bis ein Trennzeichen gefunden wird,
 * oder die maximale Anzahl zu lesender Zeichen erreicht ist.
 * \param *stream	Zeiger auf Zeichenstrom, aus dem gelesen werden soll
 * \param *to		Zeiger auf Ausgabepuffer fuer mindestens count + 1 Byte
 * \param count 	Anzahl der maximal zu lesenden Zeichen, <= BOTFS_BLOCK_SIZE
 * \param delim 	Zeichen, bis zu dem gelesen wird
 * \return 			Anzahl der in buffer geschriebenen Zeichen, < 0 fuer Fehler
 */
int16_t botfs_stream_read_until(botfs_stream_t * stream, char * to, int16_t count, const char delim);

/**
 * Liest eine Zeile aus einem Strom in einen Puffer oder die maximale Anzahl
 * zu lesender Zeichen erreicht ist. Zeilenende muss ’\n' sein.
 * \param *stream	Zeiger auf Zeichenstrom, aus dem gelesen werden soll
 * \param *to		Zeiger auf Ausgabepuffer fuer mindestens count + 1 Byte
 * \param count 	Anzahl der maximal zu lesenden Zeichen, <= BOTFS_BLOCK_SIZE
 * \return 			Anzahl der in buffer geschriebenen Zeichen, < 0 fuer Fehler
 */
static inline int16_t botfs_stream_readline(botfs_stream_t * stream, char * to, int16_t count) {
	return botfs_stream_read_until(stream, to, count, '\n');
}
#endif // BOTFS_STREAM_AVAILABLE

#ifdef BOTFS_COPY_AVAILABLE
/**
 * Kopiert eine bestehende BotFS-Datei in eine neue BotFS-Datei
 * \param *src			Zeiger auf Dateideskriptor der Quelldatei
 * \param *dest			Name der Zieldatei
 * \param src_offset	Block-Offset, ab dem aus der Quelldatei kopiert werden soll (normalerweise 0)
 * \param dest_offset	Block-Offest, an dem der kopierte Inhalt in der Zieldatei beginnen soll (normalerweise 0)
 * \param dest_tail		Freier Speicherplatz, der am Ende der Zieldatei reserviert wird in Bloecken (normalerweise 0)
 * \param dest_align	Ausrichtung der neuen Zieldatei an einer X-Blockgrenze (normalerweise 0)
 * \param *buffer		Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return				0, falls kein Fehler
 */
int8_t botfs_copy(botfs_file_descr_t * src, const char * dest, uint16_t src_offset, uint16_t dest_offset, uint16_t dest_tail, uint16_t dest_align,
	void * buffer);
#endif // BOTFS_COPY_AVAILABLE

#ifdef PC
/** FAT */
typedef struct {
	botfs_dir_block_t dirblocks[BOTFS_DIR_BLOCK_CNT]; /**< Root-Verzeichnis-Bloecke */
} PACKED_FORCE botfs_rootdir_t;

/**
 * Erzeugt ein neues Volume
 * \param *image	Dateiname fuer das Volume-Image
 * \param *name		Name des Volumes
 * \param size		Groesse des Volumes in Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_create_volume(const char * image, const char * name, uint32_t size);

/**
 * Kopiert eine Datei vom PC-Dateisystem auf das BotFS-Volume
 * \param *to		Dateiname der Zieldatei auf dem Volume
 * \param *from		Pfadname der Quelldatei
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes
 * \return			0, falls kein Fehler
 */
int8_t botfs_copy_file(const char * to, const char * from, void * buffer);

/**
 * Kopiert eine Datei vom BotfS-Volume ins PC-Dateisystem
 * \param *to			Pfadname der Zieldatei
 * \param *from			Dateiname der Quelldatei auf dem Volume
 * \param src_offset	Offset / Bloecken, ab dem aus der Quelldatei kopiert werden soll (im Normalfall 0)
 * \param dest_offset	Offset / Bloecken, ab dem in der Zieldatei der Inhalt beginnen soll (im Normalfall 0)
 * \param *buffer		Puffer fuer mindestens BOTFS_BLOCK_SIZE Bytes
 * \return				0, falls kein Fehler
 */
int8_t botfs_extract_file(const char * to, const char * from, uint32_t src_offset, uint32_t dest_offset, void * buffer);

/**
 * Management-Tools fuer BotFS
 */
void botfs_management(char * volume_file);

/**
 * Gibt Root-Dir-Eintraege zurueck
 * \param offset	Nummer des Eintrags
 * \param *buffer	Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return			Zeiger auf Dateiname (aus buffer)
 */
char * botfs_readdir(uint8_t offset, void * buffer);

/**
 * Gibt alle Freelist-Eintraege aus
 * \param *buffer Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 */
void botfs_print_freelist(void * buffer);

/**
 * Zeigt Informationen ueber jeden Eintrag des Root-Verzeichnisses einer FAT16-Partition
 * an. Insbesondere welche Clusterketten von welcher Datei belegt werden. Dadurch laesst
 * sich ablesen, ob und wie stark eine Datei fragmentiert ist, und an welchem Sektor des
 * Mediums sie beginnt.
 * \param *path	Pfad zum Device, dessen (erste) FAT16-Partitiion untersucht werden
 * 				soll (z.B. /dev/hdb (Linux) oder /dev/rdisk1 (Mac OS X).
 * 				Alternativ Pfad zu einer Image-Datei, die den Partitionsinhalt
 * 				enthaelt (z.B. von "dd" erzeugt).
 *
 * Hinweis: Gibt man einen Pfad zum Medium an, wird immer die erste Partition untersucht.
 * Wenn man statt des Mediums den Namen einer Partition angibt (hdb1 oder rdisk1s1),
 * funktioniert das Tool ebenfalls, alle ausgegebenen Adressen sind aber relativ zum Anfang
 * der Partition! Das funktioniert allerdings nur dann, wenn die Partition keinen Boot-Code
 * enthaelt!
 */
void botfs_read_fat16(const char * path);
#endif // PC

#endif // BOT_FS_AVAILABLE && ! SDFAT_AVAILABLE

#if defined BOT_FS_AVAILABLE && defined SDFAT_AVAILABLE
#include "sdcard_wrapper.h"

/**
 * Initialisiert ein Volume
 * \param *image
 * \param *buffer
 * \param create
 * \return 0, falls kein Fehler
 */
static inline int8_t botfs_init(char* image, void* buffer, uint8_t create) {
	(void) image;
	(void) buffer;
	(void) create;
	return 0;
}

/**
 * Oeffnet eine Datei
 * \param filename	Dateiname
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param mode		Modus, in dem die Datei geoeffnet wird
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_open(const char* filename, botfs_file_descr_t* p_file, uint8_t mode, void* buffer);

/**
 * Setzt den Dateizeiger an eine neue Position
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param offset	Offset der Dateiposition, an die gesprungen werden soll, in Bloecken
 * \param origin	SEEK_SET, SEEK_CUR oder SEEK_END
 */
static inline void botfs_seek(botfs_file_descr_t* p_file, int16_t offset, uint8_t origin) {
	sdfat_seek(*p_file, offset, origin);
}

/**
 * Setzt den Dateizeiger auf den Anfang einer Datei zurueck
 * \param *file	Datei-Deskriptor
 */
static inline void botfs_rewind(botfs_file_descr_t* p_file) {
	sdfat_rewind(*p_file);
}

/**
 * Liest BOTFS_BLOCK_SIZE Bytes aus einer Datei in einen Puffer
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte, in die Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
static inline int8_t botfs_read(botfs_file_descr_t* p_file, void* buffer) {
	return sdfat_read(*p_file, buffer, BOTFS_BLOCK_SIZE) == BOTFS_BLOCK_SIZE ? 0 : 1;
}

/**
 * Schreibt BOTFS_BLOCK_SIZE Bytes aus einem Puffer in eine Datei
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer mit mindestens BOTFS_BLOCK_SIZE Byte, dessen Daten in die Datei geschrieben werden
 * \return			0, falls kein Fehler
 */
static inline int8_t botfs_write(botfs_file_descr_t* p_file, void* buffer) {
	int16_t res = sdfat_write(*p_file, buffer, BOTFS_BLOCK_SIZE);
	if (res != BOTFS_BLOCK_SIZE) {
//		LOG_ERROR("botfs_write() failed with %d", res);
		return 1;
	}
	return 0;
}

/**
 * Legt eine neue Datei an
 * \param *filename	Dateiname
 * \param size		Groesse der Datei in Bloecken
 * \param alignment	Ausrichtung des Dateianfangs an einer X-Blockgrenze (normalerweise 0)
 * \param *buffer
 * \return			0, falls kein Fehler
 */
int8_t botfs_create(const char* filename, uint16_t size, uint16_t alignment, void* buffer);

/**
 * Entfernt eine Datei
 * \param *filename	Dateiname
 * \param *buffer
 * \return			0, falls kein Fehler
 */
static inline int8_t botfs_unlink(const char* filename, void* buffer) {
	(void) buffer;
	return  (int8_t) sdfat_c_remove(filename);
}

/**
 * Benennt eine Datei um
 * \param *filename	Dateiname
 * \param *new_name	Neuer Dateiname
 * \param *buffer
 * \return			0, falls kein Fehler
 */
static inline int8_t botfs_rename(const char* filename, const char* new_name, void* buffer) {
	(void) buffer;
	return (int8_t) sdfat_c_rename(filename, new_name);
}

/**
 * Schliesst eine Datei, d.h. gepufferte Daten werden zurueckgeschrieben
 * \param *file		Zeiger auf Dateideskriptor
 * \param *buffer
 */
static inline void botfs_close(botfs_file_descr_t* p_file, void* buffer) {
	(void) buffer;
	sdfat_free(*p_file);
}

/**
 * Gibt die Groesse einer Datei zurueck
 * \param *file	Zeiger auf Dateideskriptor
 * \return		Dateigroesse in Bloecken
 */
static inline uint16_t botfs_get_filesize(botfs_file_descr_t* p_file) {
	return (uint16_t) ((uint32_t) sdfat_get_filesize(*p_file) / BOTFS_BLOCK_SIZE);
}

/**
 * Beendet BotFS sauber
 */
static inline void botfs_close_volume(void) {
	sdfat_c_sync_vol();
}
#endif // BOT_FS_AVAILABLE && SDFAT_AVAILABLE
#endif // BOTFS_H_
