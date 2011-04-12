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
 * \file 	botfs_config.h
 * \brief 	Konfig-Optionen fuer Dateisystem BotFS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	17.10.2010
 *
 * Konfiguration derzeit:
 * max. Groesse des Volumes:	32 MByte
 * max. Anzahl an Dateien:		256
 * max. Dateinamenlaenge:		116 Zeichen
 * max. Dateigroesse:			16 MByte
 *
 * moechte man da etwas aendern, muss man die Groesse der Datentypen beachten / anpassen!
 */

#ifndef BOTFS_CONFIG_H_
#define BOTFS_CONFIG_H_

#define BOTFS_HEADER_POS	1			/**< Blockadresse des Volume-Headers (erster Block ist Header der Volume-Header-Datei) */
#define BOTFS_BLOCK_SIZE	512U		/**< Groesse eines Blocks (kleinste adressierbare Einheit) in Byte */
#define BOTFS_ROOT_SIZE		32768U		/**< Groesse des Root-Verzeichnisses in Byte */
#define BOTFS_MAX_FILENAME	(128 - (sizeof(botfs_file_descr_t) + 1))	/**< max. Dateinamenlaenge in Zeichen (116 Zeichen + 0-Byte + 11 Byte Dateidescr. = 128 Byte Daten pro Datei) */
#define BOTFS_MAX_VOLUME_SIZE	(65536 * BOTFS_BLOCK_SIZE)				/**< max. Groesse des Volumes in Byte */
#define BOTFS_DIR_BLOCK_CNT	(BOTFS_ROOT_SIZE / BOTFS_BLOCK_SIZE)		/**< Anzahl der Root-Dir-Bloecke */
#define BOTFS_FILE_DESC_CNT	(BOTFS_BLOCK_SIZE / sizeof(botfs_file_t))	/**< Anzahl der Datei-Deskriptoren pro Block */
#define BOTFS_MAX_FILE_CNT	((BOTFS_ROOT_SIZE / BOTFS_BLOCK_SIZE) * BOTFS_FILE_DESC_CNT) /**< max. Anzahl an Dateien */
#define BOTFS_FREEL_SIZE	BOTFS_MAX_FILE_CNT									/**< Groesse der Freelist in Eintraegen */
#define BOTFS_FREEL_BL_SIZE	(BOTFS_BLOCK_SIZE / sizeof(botfs_freelist_entry_t))	/**< Anzahl der Freelisteintraege pro Block */
#define BOTFS_FREEL_BL_CNT	(BOTFS_FREEL_SIZE / BOTFS_FREEL_BL_SIZE)			/**< Anzahl der Freelist-Bloecke */
#define BOTFS_HEADER_SIZE	(sizeof(botfs_file_header_t) / BOTFS_BLOCK_SIZE)	/**< Groesse des Datei-Headers in Bloecken */
#define BOTFS_HEADER_DATA_SIZE	(BOTFS_BLOCK_SIZE - (sizeof(uint8_t) + sizeof(botfs_file_used_t))) /**< Groesse des Datei-Header-Datenfelds in Byte */
#define BOTFS_VOLUME_DATA		"/volumedata"	/**< "Dateiname" der Root-Verzeichnis-Daten */
#define BOTFS_VERSION			3				/**< BotFS-Version */
#define BOTFS_MIN_VERSION		3				/**< BotFS-Version eines Volumes, die dieser Code mindestens braucht */
#define BOTFS_VOL_NAME_SIZE		32				/**< Maximale Laenge des Volume-Namens */
#define BOTFS_DEFAULT_VOL_NAME	"BotFS-Volume"	/**< Volume-Name, falls es automatisch angelegt wird */
#define BOTFS_DEFAULT_VOL_SIZE	(8 * (1 << 20))	/**< Volume-Groesse, falls es automatisch angelegt wird in Byte */
#define BOTFS_IMAGE_FILENAME	"botfs.img"		/**< Dateiname des BotFS-Volume-Images */

#define BOTFS_MODE_r	'r'	/**< nur lesen */
#define BOTFS_MODE_R	'R'	/**< lesen und schreiben */
#define BOTFS_MODE_W	'W'	/**< lesen und schreiben, Datei wird aber beim Oeffnen geleert */

#define BOTFS_DEBUG_LOGFILE	"botfs_log.txt" /**< Logfile, falls DEBUG_BOTFS_LOGFILE */


//#define BOTFS_STREAM_AVAILABLE /**< Stream-Funktionen aktivieren */

#endif // BOTFS_CONFIG_H_
