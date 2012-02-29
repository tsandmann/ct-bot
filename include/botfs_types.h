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
 * \file 	botfs_types.h
 * \brief 	Datentypen fuer Dateisystem BotFS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	17.10.2010
 */

#ifndef BOTFS_TYPES_H_
#define BOTFS_TYPES_H_

/** allgemein Datentypen, unabhaengig von BOT_FS_AVAILABLE */

/** Partitionseintrag im MBR */
typedef struct {
	uint8_t state;
	uint8_t begin_head;
	uint16_t begin_sect;
	uint8_t type;
	uint8_t end_head;
	uint16_t end_sect;
	uint32_t first_sect_offset;
	uint32_t sectors;
} PACKED botfs_part_entry_t;

/** Master-Boot-Record */
typedef struct {
	uint8_t bootcode[446];
	botfs_part_entry_t part0;
	botfs_part_entry_t part1;
	botfs_part_entry_t part2;
	botfs_part_entry_t part3;
	uint16_t executable_mark;
} PACKED botfs_mbr_t;

/** FAT16-Bootsektor */
typedef struct {
	uint8_t jump_code[3];			// 0-2
	char oem_name[8];				// 3-10
	uint16_t bytes_per_sect;		// 11-12
	uint8_t sect_per_cluster;		// 13-13
	uint16_t reserved_sect;			// 14-15
	uint8_t fat_copies;				// 16-16
	uint16_t root_dir_entries;		// 17-18
	uint16_t sectors_smaller_32m;	// 19-20
	uint8_t media_descr;			// 21-21
	uint16_t sect_per_fat;			// 22-23
	uint16_t sect_per_track;		// 24-25
	uint16_t num_heads;				// 26-27
	uint32_t num_hidden_sect;		// 28-31
	uint32_t num_sect;				// 32-35
	uint16_t logic_drive_num;		// 36-37
	uint8_t ext_signature;			// 38-38
	uint32_t serial;				// 39-42
	char volume_name[11];			// 43-53
	char fat_name[8];				// 54-61
	uint8_t exec_code[448];			// 62-62
	uint16_t executable_mark;		// 510-512
} PACKED botfs_fat16_bootsector_t;

/** FAT16-Verzeichniseintrag */
typedef struct {
	char name[8];
	char extension[3];
	uint8_t attributes;
	uint8_t unused[10];
	uint16_t time;
	uint16_t date;
	uint16_t first_cluster;
	uint32_t size;
} PACKED botfs_fat16_dir_entry_t;

#ifdef BOT_FS_AVAILABLE
/** BotFS-spezifische Datentypen */

/** Benutzte Bloecke einer Datei */
typedef struct {
	uint16_t start;				/**< Adresse des ersten belegten Blocks (auf Volume, nicht relativ zur Datei) */
	uint16_t end;				/**< Adresse des letzten belegten Blocks (auf Volume, nicht relativ zur Datei) */
	uint16_t bytes_last_block;	/**< Anzahl der Bytes in Block end */
} PACKED botfs_file_used_t;

/**
 * Datei-Deskriptor
 * Erweiterungsmoeglichkeit (fuer groessere Volumes): 32 Bit Startadresse, 16 Bit Offset Ende, 16 Bit Offset Position */
typedef struct {
	uint16_t start;			/**< Blockadresse des ersten Blocks der Datei (Header, nicht Nutzdaten) */
	uint16_t end;			/**< Blockadresse des letzten Blocks der Datei */
	uint16_t pos;			/**< Blockadresse der aktuellen Position */
	uint8_t mode;			/**< Modus, in dem die Datei geoeffnet wurde */
	botfs_file_used_t used;	/**< Benutzte Bloecke dieser Datei */
} PACKED botfs_file_descr_t;
#define BOTFS_FD_INITIALIZER { 0, 0, 0, 0, { 0, 0, 0 } } /**< Initialisiert einen botfs_file_descr_t Typ */

/** Datei-Header */
typedef	struct {
	uint8_t attributes;							/**< Datei-Attribute */
	botfs_file_used_t used_blocks;				/**< Benutzte Bloecke einer Datei */
	uint8_t headerdata[BOTFS_HEADER_DATA_SIZE];	/**< Header-Daten zur freien Verwendung */
} PACKED botfs_file_header_t;

/** Hauptdaten des Volumes (FAT- und Freelist-Adresse) */
typedef struct {
	botfs_file_descr_t rootdir;		/**< Datei-Deskriptor des Root-Verzeichnisses */
	botfs_file_descr_t freelist;	/**< Datei-Deskriptor der Freelist */
} PACKED botfs_volume_data_t;

/** Volume-Header */
typedef union {
	struct {
		uint8_t reserved[32];			/**< unbenutzt */
		uint16_t version;				/**< BotFS-Version */
		uint32_t size;					/**< Groesse des Volumes in Byte */
		botfs_volume_data_t ctrldata;	/**< Interne Daten des Volumes */
		uint16_t first_data;			/**< Blockadresse des ersten Datenblocks */
		char name[BOTFS_VOL_NAME_SIZE];	/**< Name des Volumes */
	} PACKED data;
	uint8_t raw[BOTFS_BLOCK_SIZE];			/**< Raw-Daten (erweitern Header auf Blockgroesse) */
} botfs_volume_t;

/** Dateieintrag */
typedef struct {
	botfs_file_descr_t descr;			/**< Dateideskriptor */
	char name[BOTFS_MAX_FILENAME + 1];	/**< Dateiname */
} PACKED botfs_file_t;

/** Root-Dir-Block */
typedef union {
	botfs_file_t files[BOTFS_FILE_DESC_CNT]; /**< Dateieintraege des Root-Dir-Blocks */
} PACKED botfs_dir_block_t;

/** Freelist-Eintrag */
typedef struct {
	uint16_t block;		/**< Blockadresse des Freibereichs */
	uint16_t size;		/**< Groesse des Freibereichs in Bloecken */
} PACKED botfs_freelist_entry_t;

/** Freelist-Block */
typedef struct {
	botfs_freelist_entry_t freeblocks[BOTFS_FREEL_BL_SIZE]; /**< Freelist-Eintraege */
} PACKED botfs_freelist_block_t;

/** Freelist */
typedef struct {
	botfs_freelist_block_t freelistblocks[BOTFS_FREEL_BL_CNT]; /**< Freelist-Bloecke */
} PACKED botfs_freelist_t;

#ifdef BOTFS_STREAM_AVAILABLE
/** Stream-Deskriptor */
typedef struct {
	botfs_file_descr_t * p_file; /**< Zeiger auf Dateideskriptor, zu dem der Stream gehoert */
	int16_t pos;			/**< aktuelle Position [Byte] im aktuellen Block */
	char * p_buf;			/**< Zeiger auf den Puffer mit dem aktuellen Block */
	uint16_t block_in_buf;	/**< Adresse des Blocks, der im Puffer ist */
} PACKED botfs_stream_t;

#define BOTFS_STREAM_INITIALIZER {NULL, 0, NULL, (uint16_t) ~0U} /**< Initialisiert ein Stream-Objekt */
#endif // BOTFS_STREAM_AVAILABLE
#endif // BOT_FS_AVAILABLE
#endif // BOTFS_TYPES_H_
