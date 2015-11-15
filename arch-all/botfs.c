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
 * \file 	botfs.c
 * \brief 	Dateisystem BotFS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.02.2008
 *
 * Notwendige Vorbereitungen zur Verwendung:\n
 * - PC:\n
 *   Keine Vorbereitungen noetig, ist kein Volume vorhanden, wird es automatisch angelegt und dazu eine Datei
 *   mit dem in BOTFS_IMAGE_FILENAME festgeleten Namen angelegt. Achtung, dieses Volume hat dann nicht die fuer
 *   MCU benoetigte Groesse (s.u.). Moechte man Daten zwischen MCU und PC austauschen, kopiert man die Datei von
 *   der MMC / SD-Karte ins Bot-Verzeichnis und ueberschreibt eine evtl. automatisch angelegte Datei.
 *
 * - MCU:\n
 *   Um BotFS auf MCU mit MMC / SD-Karte nutzen zu koennen, muss die Karte einmalig dafuer vorbereitet werden.
 *   Es gibt drei moegliche Varianten:
 *   - Variante 1: Vorgefertigtes Disk-Image auf die SD-Karte uebertragen\n
 *      - + SD-Karte braucht nicht manuell partioniert werden\n
 *      - + Nur ein Arbeitsschritt noetig\n
 *      - - Loescht die komplette SD-Karte\n
 *      - - unter Windows externes Tool noetig\n
 *   - Variante 2: Manuelle Partionierung, anschliessende Einrichtung mit Hilfsprogramm\n
 *      - + volle Kontrolle ueber die Partitionierung\n
 *      - + Partions- und Imagegroesse anpassbar (max. 32 MB)\n
 *      - - mehrere Arbeitsschritte noetig\n
 *      - - Partionierung unter Windows evtl. umstaendlich\n
 *      - - Wert von first_block in pc/botfs-low_pc.c muss angepasst werden, wenn man BotFS-Dateien am PC erzeugen und auf dem echten Bot verwenden moechte\n
 *   - Variante 3: vollstaendig manuelle Einrichtung\n
 *      - + volle Kontrolle ueber alle Schritte\n
 *      - - recht umstaendlich\n
 *      - - Wert von first_block in pc/botfs-low_pc.c muss angepasst werden, wenn man BotFS-Dateien am PC erzeugen und auf dem echten Bot verwenden moechte\n
 * \n
 *   - Durchfuehrung Variante 1:\n
 *     Uebertragen des vorgefertigten Disk-Images (befindet sich in contrib/BotFS/sd.img.zip):
 *     - unter Linux:\n
 *       1. Device-Node des Kartenlesers ausfindig machen, z.B /dev/sdb wie im Folgenden\n
 *       2. Evtl. vorhandene Partitionen auf der SD-Karten unmounten\n
 *       3. \code sudo gunzip -c sd.img.zip | dd of=/dev/sdb bs=4k \endcode
 *          (Achtung, /dev/sdb entsprechend anpassen!)\n
 *       4. \code sync \endcode\n
 *     - unter Mac OS X:\n
 *       1. Disk-Nummer des Kartenlesers ausfinden machen (Festplatten-Dienstprogramm -> Info oder diskutil list), im Folgenden Beispiel disk3\n
 *       2. \code sudo diskutil unmountDisk /dev/rdisk3 \endcode
 *       3. \code sudo gunzip -c sd.img.zip | dd of=/dev/rdisk3 bs=4k \endcode
 *          (Achtung, /dev/rdisk3 entsprechend anpassen!)\n
 *       4. \code sync \endcode\n
 *     - unter Windows:\n
 *       0. physdiskwrite + PhysGUI von http://m0n0.ch/wall/physdiskwrite.php herunterladen und entpacken\n
 *       1. sd.img.zip entpacken, erzeugt die Datei sd.img\n
 *       2. PhysGUI.exe (als Administrator) starten\n
 *       3. Rechtsklick auf den Eintrag des Kartenlesers -> Image laden -> Oeffnen -> sd.img auswaehlen -> OK -> Ja\n
 *       4. Hardawre sicher entfernen -> auswerfen ausfuehren\n
 * \n
 *   - Durchfuehrung Variante 2:\n
 *     - Einrichtung mit Hilfsprogramm BotFS Helper.
 *       Das Programm legt auf einer maximal 32 MB grossen FAT16-Partition einer SD-Karte ein BotFS-Image an.
 *     - Der Sourcecode des Hilfsprogramms ist im ct-Bot Repository unter other/ct-Bot-botfshelper zu finden.\n
 *     - Zunaechst legt man auf der Karte eine FAT16-Partition an, die maximal 32 MByte gross ist. Diese muss die erste
 *       Partition auf der Karte sein, weitere Partitionen koennen problemlos folgen. Auf der SD-Karte muss unbedingt
 *       eine MBR-Partitionstabelle verwendet werden, GPT wird nicht unterstuetzt! Um durch Rechenweise und Aufrundungen
 *       des verwendeten Partitionstools nicht die 32 MByte-Grenze zu ueberschreiten, empfiehlt es sich, eine Partition von
 *       30 MByte anzulegen - wichtig ist, dass die Groesse der erzeugten Partition 32 MByte, also \f$ 32 * 2^{20} \f$ Byte,
 *       nicht ueberschreitet.\n
 *       Auf der angelegten Partition muessen unbedingt alle Dateien (auch evtl. Versteckte) geloescht werden, so dass die
 *       gesamte Partitionsgroesse als freier Speicher verfuegbar ist.\n
 *       Anschliessend ruft man das Hilfsprogramm aus contrib/BotFS wie folgt auf:\n
 *        - Linux:   "./ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "./ct-Bot-botfshelper /media/SD"\n
 *        - Mac:     "./ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "./ct-Bot-botfshelper /Volumes/SD"\n
 *        - Windows: "ct-Bot-botfshelper.exe ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "ct-Bot-botfshelper.exe e:\"\n
 *       Es bietet sich an, auf der SD-Karte auch eine zweite Partition (Groesse und Typ beliebig) anzulegen und dort eine Kopie
 *       der Datei botfs.img zu speichern. Moechte man einmal das komplette Dateisystem fuer den Bot leeren oder wurde es durch
 *       einen Fehler beschaedigt, kopiert man einfach dieses Backup zurueck auf die erste Partition und muss die obigen Schritte
 *       nicht wiederholen.\n
 *       Wenn man BotFS-Dateien am PC erzeugen und auf dem echten Bot verwenden moechte, sollte man den Wert von first_block in
 *       pc/botfs-low_pc.c anpassen, damit das Alignment von Dateien (zur Performanzsteigerung z.B. fuer die Map) stimmt.
 *       first_block muss dazu auf den ersten Datensektor der FAT16-Partition gesetzt werden.
 * \n
 *   - Durchfuehrung Variante 3:\n
 *     Anlegen der Partition wie unter Variante 2.\n
 *     Anschliessend ermittelt man die exakte Groesse der Partition (je nach Betriebssystem findet sich diese in den
 *     Eigenschaften / Informationen des Laufwerks) in Byte und notiert sie. Auf der angelegten Partition muessen unbedingt
 *     alle Dateien (auch evtl. Versteckte) geloescht werden, so dass die gesamte Partitionsgroesse als freier Speicher
 *     verfuegbar ist. Die notierte Groesse teil man noch durch 1024 und erhaelt so die gewuenschte Image-Groesse in KByte.
 *     Nun startet man den fuer PC (mit BOT_FS_AVAILABLE) compilierten Bot-Code mit dem Parameter "-f", also "ct-Bot(.exe) -f",
 *     um die BotFS-Verwaltung aufzurufen. Dort gibt man "create volume" ein und bestaetigt das Kommando mit Enter. Als
 *     Dateinamen waehlt man anschliessend einen Namen wie "botfs.img", eine solche Datei darf aber noch nicht existieren.
 *     Eine komplette Pfadangabe ist auch moeglich, ansonsten wird die Datei im aktuellen Arbeitsverzeichnis erstellt.
 *     Als Volume-Name gibt man dann einen beliebigen ein, wie z.B. "BotFS-Volume", als Groesse danach die eben Ermittelte (in
 *     KByte). Jetzt kann die Verwaltung mit dem Kommando 'q' beendet werden und die erzeugte Datei als "botfs.img" (wichtig:
 *     auf der SD-Karte muss die Datei unbedingt "botfs.img" heissen!) auf die SD-Karte (erste Partition) kopiert werden.\n
 *     Wenn man BotFS-Dateien am PC erzeugen und auf dem echten Bot verwenden moechte, sollte man den Wert von first_block in
 *     pc/botfs-low_pc.c anpassen, damit das Alignment von Dateien (zur Performanzsteigerung z.B. fuer die Map) stimmt.
 *     first_block muss dazu auf den ersten Datensektor der FAT16-Partition gesetzt werden.\n
 * \n
 * Benutzung:\n
 * - Der Map-Code ist jetzt (mit BOT_FS_AVAILABLE) nicht mehr darauf angewiesen, dass eine Map-Datei (MiniFAT) auf der SD-Karte
 *   bereits vorhanden ist. Sobald das BotFS-Volume einmal auf der SD-Karte eingerichtet ist (s.o.), regelt der Map-Code den
 *   Rest automatisch. Insbesondere das Loeschen einer alten Map (beim Start) geht mit BOT_FS_AVAILABLE dann auch deutlich
 *   schneller.
 * - Moechte man Daten zwischen dem echten und einem simulierten Bot austauschen, kann man die "botfs.img"-Datei beliebig
 *   zwischen diesen kopieren, oder den simulierten Bot mit dem Parameter "-i" und der Image-Datei (z.B. auch direkt von der
 *   eingelegten SD-Karte) starten.
 * - In der BotFS-Verwaltung (ueber ct-Bot(.exe) -f [Pfad zur Image-Datei] aufzurufen) zeigt das Kommando "help" eine Uebersicht
 *   aller verfuegbaren Tools an. Wird im Normalbetrieb aber eigentlich nicht benoetigt.
 *
 */

#include "ct-Bot.h"

#ifdef BOT_FS_AVAILABLE
#include "os_thread.h"
#include "botfs.h"
#include "botfs-low.h"
#include "log.h"
#include <string.h>

char * botfs_volume_image_file = BOTFS_IMAGE_FILENAME;	/**< Dateiname des BotFS-Volume-Images */
botfs_volume_data_t botfs_vol_data;						/**< Root-Dir- und Freelist-Adresse */
botfs_mutex_t botfs_mutex = BOTFS_MUTEX_INITIALIZER;	/**< sperrt den Zugriff auf nicht threadsichere Funktionen */
static uint8_t init_state = 0;							/**< Status der Initialisierung (1: korrekt initialisiert) */

/**
 * Initialisiert ein Volume
 * \param *image	Dateiname des Images
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \param create	Soll das Volume erzeugt werden, falls es nicht existiert?
 * \return			0, falls kein Fehler
 */
int8_t botfs_init(char * image, void * buffer, uint8_t create) {
	init_state = 0;
	/* Volume laden */
	PRINT_MSG("botfs_init_low(\"%s\")...", image);
	if (botfs_init_low(image, buffer, create) != 0) {
		PRINT_MSG("botfs_init(): botfs_init_low() schlug fehl");
		return -1;
	}

	/* Header einlesen */
	if (botfs_read_low(BOTFS_HEADER_POS, buffer) != 0) {
		PRINT_MSG("botfs_init(): botfs_read_low(BOTFS_HEADER_POS) schlug fehl");
		return -2;
	}

	/* Root-Dir- und Freelist-Adresse speichern */
	botfs_volume_t * volume = buffer;
	botfs_vol_data = volume->data.ctrldata;
	init_state = 1;

	if (botfs_check_volume_low(image, buffer) != 0) {
		PRINT_MSG("botfs_init(): Volume fehlerhaft");
		init_state = 0;
		return -3;
	}

	PRINT_MSG("botfs_init() erfolgreich");
	return 0;
}

/**
 * Leert eine Datei, indem sie mit Nullen ueberschrieben wird
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
static int8_t clear_file(botfs_file_descr_t * file, void * buffer) {
	/* Puffer leeren */
	memset(buffer, 0, BOTFS_BLOCK_SIZE);

	/* Alle belegten Dateibloecke nullen, Header nicht */
	uint16_t block = file->used.start <= file->start ? file->start + BOTFS_HEADER_SIZE : file->used.start;
	for (; block <= file->used.end; ++block) {
		if (botfs_write_low(block, buffer) != 0) {
			return -3;
		}
	}

	/* Datei-Header-Nutzdaten nullen */
	uint8_t * ptr;
	if (botfs_read_header_data(file, &ptr, buffer) != 0) {
		return -4;
	}
	memset(ptr, 0, BOTFS_HEADER_DATA_SIZE);
	if (botfs_write_header_data(file, buffer) != 0) {
		return -5;
	}

	/* Used-Blocks zuruecksetzen */
	file->used.start = UINT16_MAX;
	file->used.end = 0;
	file->used.bytes_last_block = 0;
	return botfs_flush_used_blocks(file, buffer);
}

/**
 * Sucht einen Dateieintrag im Root-Verzeichnis.
 * In buffer befindet sich anschliessend der Root-Dir-Block zum Dateieintrag.
 * \param *filename	gesuchter Dateiname, falls der fuehrende / fehlt, wird
 * 					er automatisch ergaenzt
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			Zeiger auf Dateieintrag (aus buffer) oder NULL
 */
static botfs_file_t * search_file(const char * filename, void * buffer) {
	if (filename == NULL) {
		return NULL;
	}
	botfs_dir_block_t * root_block = buffer;
	uint8_t i;
	botfs_rewind(&botfs_vol_data.rootdir);
	for (i = BOTFS_DIR_BLOCK_CNT; i > 0; --i) {
		/* alle Root-Dir-Bloecke durchsuchen */
		if (botfs_read(&botfs_vol_data.rootdir, buffer) != 0) {
			return NULL;
		}
		botfs_file_t * ptr = root_block->files;
		uint8_t j;
		for (j = BOTFS_FILE_DESC_CNT; j > 0; --j) {
			/* alle Dateieintraege durchsuchen */
			if ((strncmp(filename, ptr->name, BOTFS_MAX_FILENAME) == 0) ||
				(filename[0] != '/' && strncmp(filename, &ptr->name[1], BOTFS_MAX_FILENAME - 1) == 0)) { // Dateiname ohne fuehrenden / angegeben
				/* Datei gefunden */
				return ptr;
			}
			ptr++;
		}
	}
	/* Fehler */
	return NULL;
}

/**
 * Vergleich die Groesse eines Freelisteintrags mit x
 * \param *ptr	Zeiger auf Freelisteintrag
 * \param x		Vergleichswert
 * \return		(ptr->size >= x)
 */
static uint8_t test_size(botfs_freelist_entry_t * ptr, uint16_t x) {
	if (ptr->size >= x) {
		return 1;
	}
	return 0;
}

/**
 * Vergleicht den Start-Block eines Freelisteintrags mit x
 * \param *ptr	Zeiger auf Freelisteintrag
 * \param x		Vergleichswert
 * \return		(ptr->size > 0 && ptr->block == x)
 */
static uint8_t test_block_start(botfs_freelist_entry_t * ptr, uint16_t x) {
	if (ptr->size > 0 && ptr->block == x) {
		return 1;
	}
	return 0;
}

/**
 * Vergleicht den End-Block eines Freelisteintrags mit x
 * \param *ptr	Zeiger auf Freelisteintrag
 * \param x		Vergleichswert
 * \return		(ptr->size > 0 && ptr->size + ptr->block == x)
 */
static uint8_t test_block_end(botfs_freelist_entry_t * ptr, uint16_t x) {
	if (ptr->size > 0 && ptr->size + ptr->block == x) {
		return 1;
	}
	return 0;
}

/**
 * Sucht einen Freelist-Eintrag, fuer den die Vergleichsfunktion test mit Parameter value
 * den Wert result liefert.
 * \param *test		Zeiger auf Vergleichsfunktion
 * \param value		Wert des Freelist-Eintrags, der getestet werden soll von test()
 * \param result	Vergleichswert fuer test(), dem value entsprechen soll
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			Zeiger auf gefundenen Freelist-Eintrag oder NULL, falls kein Treffer
 */
static botfs_freelist_entry_t * search_freelist(uint8_t (* test)(botfs_freelist_entry_t *, uint16_t), uint16_t value,
		uint8_t result, void * buffer) {
	botfs_rewind(&botfs_vol_data.freelist);
	botfs_freelist_block_t * freelist = buffer;
	uint8_t i;
	for (i = BOTFS_FREEL_BL_CNT; i > 0; --i) {
		if (botfs_read(&botfs_vol_data.freelist, buffer) != 0) {
			PRINT_MSG("search_freelist(): read()-error");
			return NULL;
		}
		botfs_freelist_entry_t * pFree = freelist->freeblocks;
		uint8_t j;
		for (j = BOTFS_FREEL_BL_SIZE; j > 0; --j) {
			if (test(pFree, value) == result) {
				return pFree;
			}
			pFree++;
		}
	}
	return NULL;
}

/**
 * Fuegt einen Bereich zur Freelist hinzu
 * \param start		Erster Block der freizugebenden Daten
 * \param end		Letzter Block der freizugebenden Daten
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
static int8_t add_to_freelist(uint16_t start, uint16_t end, void * buffer) {
	/* Auf Freiblock direkt nach dieser Datei pruefen */
	botfs_freelist_entry_t * pFree = search_freelist(test_block_start, end + 1, 1, buffer);
	if (pFree != NULL) {
		PRINT_MSG("add_to_freelist(): Freiblock folgt auf Datei");
		/* Datei mit Freiblock dahinter verschmelzen */
		end += pFree->size;
		/* Freiblock loeschen */
		pFree->size = 0;
		/* Block zurueckschreiben */
		botfs_seek(&botfs_vol_data.freelist, -1, SEEK_CUR);
		if (botfs_write(&botfs_vol_data.freelist, buffer) != 0) {
			PRINT_MSG("add_to_freelist(): add_to_freelist(): write()-error");
			return -11;
		}
	}
	/* Auf Freiblock direkt vor dieser Datei pruefen */
	pFree = search_freelist(test_block_end, start, 1, buffer);
	if (pFree != NULL) {
		PRINT_MSG("add_to_freelist(): Datei folgt auf Freiblock");
		/* Freiblock auf Datei ausdehnen */
		pFree->size += end - start + 1;
		/* Block zurueckschreiben */
		botfs_seek(&botfs_vol_data.freelist, -1, SEEK_CUR);
		if (botfs_write(&botfs_vol_data.freelist, buffer) != 0) {
			PRINT_MSG("add_to_freelist(): write()-error");
			return -12;
		}
		return 0;
	}

	/* Kein Freelist-Eintrag passt, Neuen erzeugen */
	PRINT_MSG("add_to_freelist(): erzeuge neuen Freelist-Eintrag");
	pFree = search_freelist(test_size, 1, 0, buffer);
	if (pFree == NULL) {
		return -13;
	}
	pFree->block = start;
	pFree->size = end - start + 1;
	PRINT_MSG("add_to_freelist(): block=0x%04x, size=0x%04x", pFree->block, pFree->size);
	/* Block zurueckschreiben */
	botfs_seek(&botfs_vol_data.freelist, -1, SEEK_CUR);
	if (botfs_write(&botfs_vol_data.freelist, buffer) != 0) {
		PRINT_MSG("add_to_freelist(): write()-error");
		return -14;
	}

	return 0;
}

/**
 * Legt eine neue Datei an
 * \param *filename	Dateiname
 * \param size		Groesse der Datei in Bloecken
 * \param alignment	Ausrichtung des Dateianfangs an einer X-Blockgrenze (normalerweise 0)
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_create(const char * filename, uint16_t size, uint16_t alignment, void * buffer) {
	botfs_acquire_lock_low(&botfs_mutex);
	/* Datei bereits vorhanden? */
	if (search_file(filename, buffer) != NULL) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_create(): file existing");
		return -1;
	}
	++size;	// Header braucht einen Block
	PRINT_MSG("botfs_create(): creating file \"%s\" with size 0x%x blocks", filename, size);

	alignment = alignment > 0 ? alignment - 1 : 0;

	/* freien Speicherbereich suchen */
	botfs_freelist_entry_t * pFree = search_freelist(test_size, size + alignment, 1, buffer);
	if (pFree == NULL) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_create(): no space left");
		return -2;
	}

	PRINT_MSG("botfs_create(): pFree->block=0x%x", pFree->block);
	PRINT_MSG("botfs_create():  sector=0x%x", get_sector(pFree->block));

	/* Alignment des ersten Datenblocks berechnen */
	uint16_t offset = (uint16_t) (((get_sector(pFree->block + BOTFS_HEADER_SIZE) + alignment) & ~alignment) - get_sector(pFree->block + BOTFS_HEADER_SIZE));
	PRINT_MSG("botfs_create(): alignment-offset=0x%x", offset);

	/* Freelist aktualisieren */
	pFree->size -= size + offset;
	uint16_t start = pFree->block + offset;
	PRINT_MSG("botfs_create(): start=0x%x", start);
	PRINT_MSG("botfs_create(): first_data=0x%x", start + BOTFS_HEADER_SIZE);
	pFree->block += size + offset;
	uint16_t end = pFree->block - 1;
	PRINT_MSG("botfs_create(): end=0x%x", end);

	/* Freelist-Block zurueckschreiben */
	botfs_seek(&botfs_vol_data.freelist, -1, SEEK_CUR);
	if (botfs_write(&botfs_vol_data.freelist, buffer) != 0) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_create(): can't write freelist");
		return -3;
	}

	/* Alingment-Offset als frei markieren */
	if (offset != 0) {
		PRINT_MSG("botfs_create(): freeing 0x%x - 0x%x", start - offset, start - 1);
		if (add_to_freelist(start - offset, start - 1, buffer) != 0) {
			botfs_release_lock_low(&botfs_mutex);
			PRINT_MSG("botfs_create(): can't update freelist");
			// Freelist muesste man hier eigentlich wieder zuruecksetzen
			return -4;
		}
	}

	/* leeren Root-Dir-Eintrag suchen */
	botfs_file_t * pFile = search_file("", buffer);
	if (pFile == NULL) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_create(): no dir-entry left");
		// Freelist muesste man hier eigentlich wieder zuruecksetzen
		return -4;
	}
	char * ptr = pFile->name;
	if (filename[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strncpy(ptr, filename, BOTFS_MAX_FILENAME - 1);
	pFile->descr.start = start;
	pFile->descr.end = end;

	/* Root-Dir-Block zurueckschreiben */
	botfs_seek(&botfs_vol_data.rootdir, -1, SEEK_CUR);
	if (botfs_write(&botfs_vol_data.rootdir, buffer) != 0) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_create(): can't write rootdir");
		// Freelist muesste man hier eigentlich wieder zuruecksetzen
		return -5;
	}

	/* Datei-Header erzeugen */
	botfs_file_header_t * pHeader = buffer;
	memset(pHeader, 0, sizeof(botfs_file_header_t));
	/* Datei als komplett belegt kennzeichnen */
	pHeader->used_blocks.start = start + BOTFS_HEADER_SIZE;
	pHeader->used_blocks.end = end;
	pHeader->used_blocks.bytes_last_block = BOTFS_BLOCK_SIZE;
	PRINT_MSG("botfs_create(): writing file-header...");
	if (botfs_write_low(start, buffer) != 0) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_create(): can't write file-header");
		// Freelist muesste man hier eigentlich wieder zuruecksetzen
		return -6;
	}
	botfs_release_lock_low(&botfs_mutex);
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
int8_t botfs_open(const char * filename, botfs_file_descr_t * file, uint8_t mode, void * buffer) {
	botfs_acquire_lock_low(&botfs_mutex);
	/* Datei suchen */
	botfs_file_t * ptr = search_file(filename, buffer);
	if (ptr == NULL) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_open(): file not found");
		return -1;
	}

	/* Datei-Deskriptor laden und updaten */
	*file = ptr->descr;
	file->mode = mode;
	file->pos = file->start + BOTFS_HEADER_SIZE;

	/* Datei-Header laden */
	if (botfs_read_low(file->start, buffer) != 0) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_open(): read_low()-error");
		return -2;
	}

	/* benutzte Bloecke lt. Header in den Datei-Deskriptor uebernehmen */
	botfs_file_header_t * ptr_head = buffer;
	file->used = ptr_head->used_blocks;
//	PRINT_MSG("botfs_open(): start=0x%x end=0x%x", file->used.start, file->used.end);

	int8_t tmp = 0;
	if (mode == BOTFS_MODE_W) {
		PRINT_MSG("botfs_open(): Datei wird geleert...");
		tmp = clear_file(file, buffer);
	}
	botfs_release_lock_low(&botfs_mutex);
	return tmp;
}

/**
 * Entfernt eine Datei
 * \param *filename	Dateiname
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_unlink(const char * filename, void * buffer) {
	botfs_acquire_lock_low(&botfs_mutex);
	/* Datei suchen */
	botfs_file_t * ptr = search_file(filename, buffer);
	if (ptr == NULL) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_unlink(): Datei nicht vorhanden");
		return -1;
	}

	/* Root-Dir Eintrag loeschen */
	ptr->name[0] = 0;
	ptr->name[1] = 0;
	botfs_seek(&botfs_vol_data.rootdir, -1, SEEK_CUR); // Dateizeiger stand auf naechstem Dir-Block => -1
	if (botfs_write(&botfs_vol_data.rootdir, buffer) != 0) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_unlink(): Fehler beim Schreiben des Root-Blocks");
		return -2;
	}
	const uint16_t start = ptr->descr.start;
	const uint16_t end = ptr->descr.end;

	/* Freelist aktualisieren */
	int8_t res = add_to_freelist(start, end, buffer);

	botfs_release_lock_low(&botfs_mutex);

	PRINT_MSG("botfs_unlink(): Datei \"%s\" wurde geloescht", filename);
	return res;
}

/**
 * Benennt eine Datei um
 * \param *filename	Dateiname
 * \param *new_name	Neuer Dateiname
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 *
 * Datei new_name wird geloescht, falls sie bereits existiert.
 */
int8_t botfs_rename(const char * filename, const char * new_name, void * buffer) {
	botfs_acquire_lock_low(&botfs_mutex);
	/* Datei suchen */
	botfs_file_t * ptr = search_file(filename, buffer);
	botfs_release_lock_low(&botfs_mutex);
	if (ptr == NULL) {
		PRINT_MSG("botfs_rename(): Datei nicht vorhanden");
		return -1;
	}

	/* Ziel loeschen, falls vorhanden */
	if (botfs_unlink(new_name, buffer) < -1) {
		PRINT_MSG("botfs_rename(): Fehler beim Loeschen der vorhandenen Zieldatei");
		return -2;
	}

	/* Root-Dir Eintrag updaten */
	botfs_acquire_lock_low(&botfs_mutex);
	ptr = search_file(filename, buffer);
	char * pName = ptr->name;
	if (new_name[0] != '/') {
		/* fehlenden fuehrenden / ergaenzen */
		*pName = '/';
		pName++;

	}
	strncpy(pName, new_name, BOTFS_MAX_FILENAME - 1);
	botfs_seek(&botfs_vol_data.rootdir, -1, SEEK_CUR); // Dateizeiger stand auf naechstem Dir-Block => -1
	if (botfs_write(&botfs_vol_data.rootdir, buffer) != 0) {
		botfs_release_lock_low(&botfs_mutex);
		PRINT_MSG("botfs_rename(): Fehler beim Schreiben des Root-Blocks");
		return -3;
	}
	botfs_release_lock_low(&botfs_mutex);
	return 0;
}

/**
 * Prueft eine Dateiposition auf Gueltigkeit
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param pos		Zu ueberpruefende Position (in Bloecken)
 */
static int8_t check_pos(botfs_file_descr_t * file, uint16_t pos) {
	if (pos > file->start && pos <= file->end) {
		return 0;
	}
	PRINT_MSG("check_pos(): start=0x%x, end=0x%x, pos=0x%x", file->start, file->end, pos);
	return -1;
}

/**
 * Liest BOTFS_BLOCK_SIZE Bytes aus einer Datei in einen Puffer
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte, in die Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_read(botfs_file_descr_t * file, void * buffer) {
	if (init_state != 1) {
		return -99;
	}
	/* Positions-Check */
	if (check_pos(file, file->pos) != 0) {
		PRINT_MSG("botfs_read(): Position 0x%x ungueltig!", file->pos);
		return -1;
	}

	/* Daten lesen und Dateizeiger anpassen */
	if (botfs_read_low(file->pos++, buffer) != 0) {
		PRINT_MSG("botfs_read(): botfs_read_low() meldet Fehler");
		return -2;
	}

	return 0;
}

/**
 * Schreibt BOTFS_BLOCK_SIZE Bytes aus einem Puffer in eine Datei
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer mit mindestens BOTFS_BLOCK_SIZE Byte, dessen Daten in die Datei geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_write(botfs_file_descr_t * file, void * buffer) {
	if (init_state != 1) {
		return -99;
	}
	/* Schreibschutz pruefen */
	if (file->mode == BOTFS_MODE_r) {
		PRINT_MSG("botfs_write(): Fehler, Datei ist schreibgeschuetzt geoeffnet!");
		return -1;
	}

	/* Positions-Check */
	if (check_pos(file, file->pos) != 0) {
		PRINT_MSG("botfs_write(): Dateiposition unzulaessig");
		return -2;
	}

	/* Daten schreiben und Dateizeiger anpassen */
	if (botfs_write_low(file->pos, buffer) != 0) {
		PRINT_MSG("botfs_write(): write_low() schlug fehl");
		return -3;
	}

	/* Info ueber benutzte Bloecke updaten */
	if (file->pos < file->used.start) {
		file->used.start = file->pos;
//		PRINT_MSG("botfs_write(): Bloecke 0x%x bis 0x%x als benutzt vermerkt", file->used.start, file->used.end);
	}
	if (file->pos > file->used.end) {
		file->used.end = file->pos;
		file->used.bytes_last_block = BOTFS_BLOCK_SIZE;
//		PRINT_MSG("botfs_write(): Bloecke 0x%x bis 0x%x als benutzt vermerkt", file->used.start, file->used.end);
	}

	file->pos++;
	return 0;
}

/**
 * Schreibt die Information ueber benutzte Bloecke in den Datei-Header
 * \param *file		Zeiger auf Datei-Deskriptor
 * \param *buffer	Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_flush_used_blocks(botfs_file_descr_t * file, void * buffer) {
	if (init_state != 1) {
		return -99;
	}
	/* update file header */
	if (botfs_read_low(file->start, buffer) != 0) {
		PRINT_MSG("botfs_flush_used_blocks(): Datei-Header konnte nicht gelesen werden");
		return -4;
	}
	botfs_file_header_t * pHeader = buffer;
	pHeader->used_blocks = file->used;
	int8_t res = botfs_write_low(file->start, buffer);
	if (res != 0) {
		PRINT_MSG("botfs_flush_used_blocks(): Datei-Header konnte nicht gespeichert werden");
		return -5;
	}
//	PRINT_MSG("botfs_flush_used_blocks(): Benutzte Bloecke im Header gespeichert");
//	PRINT_MSG("botfs_flush_used_blocks: start=0x%x end=0x%x bytes_last_block=%u", file->used.start, file->used.end, file->used.bytes_last_block);

	return 0;
}

/**
 * Liest die frei verwendbaren Header-Daten einer Datei aus
 * \param *file			Zeiger auf Datei-Deskriptor
 * \param *headerdata	Zeiger auf die gewuenschten Header-Daten (liegen in buffer[])
 * \param *buffer		Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return				0, falls kein Fehler
 */
int8_t botfs_read_header_data(botfs_file_descr_t * file, uint8_t ** headerdata, void * buffer) {
	if (init_state != 1) {
		return -99;
	}
	if (botfs_read_low(file->start, buffer) != 0) {
		return -1;
	}
	botfs_file_header_t * ptr = buffer;
	if (headerdata != NULL) {
		*headerdata = ptr->headerdata;
	}
	return 0;
}

/**
 * Schreib die frei verwendbaren Header-Daten einer Datei in den Header
 * \param *file			Zeiger auf Datei-Deskriptor
 * \param *buffer		Puffer mit mindestens BOTFS_BLOCK_SIZE Byte
 * \return				0, falls kein Fehler
 *
 * buffer muss vorher durch botfs_read_header_data() mit den kompletten
 * Header-Daten gefuellt worden sein!
 */
int8_t botfs_write_header_data(botfs_file_descr_t * file, void * buffer) {
	if (init_state != 1) {
		return -99;
	}
	return botfs_write_low(file->start, buffer);
}

/**
 * Beendet BotFS sauber
 */
void botfs_close_volume(void) {
	botfs_close_volume_low();
	init_state = 0;
}

#ifdef BOTFS_STREAM_AVAILABLE
/**
 * Oeffnet einen Zeichenstrom, so dass er mit botfs_stream_read*() verwendet werden kann
 * \param *stream	Zeiger auf Stream-Objekt, das initialisiert werden soll
 * \param *file		Zeiger auf Deskriptor der Datei, auf dem der Stream aufsetzen soll
 * \param *buffer	Zeiger auf Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 */
void botfs_stream_open(botfs_stream_t * stream, botfs_file_descr_t * file, void * block_buffer) {
	stream->p_file = file;
	botfs_rewind(file);
	stream->p_buf = (char *) block_buffer;
	stream->block_in_buf = (uint16_t) ~0U;
	stream->pos = 0;
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
int16_t botfs_stream_read_until(botfs_stream_t * stream, char * to, int16_t count, const char delim) {
	if (count > BOTFS_BLOCK_SIZE) {
		return -1;
	}

	int16_t i = 0, n = 0, to_blockend = 0;
	char * ptr;
	do {
		if (stream->block_in_buf != stream->p_file->pos) {
			if (stream->p_file->pos > stream->p_file->used.end) {
				*to = 0; // EOF
				PRINT_MSG("botfs_stream_read_until(): Dateiende erreicht");
				PRINT_MSG("botfs_stream_read_until(): pos=0x%x, used.end=0x%x", stream->p_file->pos, stream->p_file->used.end);
				return 0;
			}
			if (botfs_read(stream->p_file, stream->p_buf) != 0) {
				PRINT_MSG("botfs_stream_read_until(): botfs_read() lieferte Fehler");
				return -2;
			}
			botfs_seek(stream->p_file, -1, SEEK_CUR);
			stream->block_in_buf = stream->p_file->pos;
			PRINT_MSG("botfs_stream_read_until(): Block 0x%x read", stream->block_in_buf);
		} else {
			PRINT_MSG("botfs_stream_read_until(): Block cached");
		}

		count -= i;
		to_blockend = BOTFS_BLOCK_SIZE - stream->pos;
		char * p_start = stream->p_buf + stream->pos;
		ptr = p_start;
		for (i = 0; i < count && i < to_blockend; ++i) {
			if (*ptr == delim) {
				++i;
				break;
			} else if (*ptr == 0) {
				break;
			}
			++ptr;
		}
		n += i;
		memcpy(to, p_start, (size_t) i);
		to += i;
		*to = 0;

		if (*ptr != delim && count > to_blockend) {
			stream->pos = 0;
			botfs_seek(stream->p_file, 1, SEEK_CUR); // naechster Block
		} else {
			break;
		}
	} while (42);

	/* Windows Linefeed (CR) entfernen, falls vorhanden */
	to -= 2;
	if (*to == '\r') {
		*to = '\n';
		++to;
		*to = 0;
		--n;
	}

	stream->pos += i;

	return n;
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
		void * buffer) {
	if (init_state != 1) {
		return -99;
	}
	PRINT_MSG("botfs_copy(0x%x, \"%s\", 0x%x, 0x%x, 0x%x)", src->start, dest, src_offset, dest_offset, dest_tail);

	uint32_t dest_size = botfs_get_filesize(src);
	dest_size -= src_offset;
	dest_size += dest_offset;
	dest_size += dest_tail;
	if (dest_size > BOTFS_MAX_FILE_SIZE) {
		PRINT_MSG("botfs_copy(): Zieldatei zu gross");
		return -1;
	}
	const uint16_t dest_size_16 = (uint16_t) dest_size;
	PRINT_MSG("botfs_copy(): dest_size_16=0x%x", dest_size_16);

	if (botfs_create(dest, dest_size_16, dest_align, buffer) != 0) {
		PRINT_MSG("botfs_copy(): Zieldatei konnte nicht angelegt werden");
		return -2;
	}

	botfs_file_descr_t dest_file;
	if (botfs_open(dest, &dest_file, BOTFS_MODE_W, buffer) != 0) {
		PRINT_MSG("botfs_copy(): Zieldatei konnte nicht geoeffnet werden");
		return -3;
	}

	const uint16_t src_skip_end = src->used.end != 0 ? src->end - src->used.end : 0;
	uint16_t src_skip = src->used.start == UINT16_MAX ? botfs_get_filesize(src) : src->used.start - (src->start + BOTFS_HEADER_SIZE);
	PRINT_MSG("botfs_copy(): src_skip=0x%x", src_skip);
	PRINT_MSG("botfs_copy(): src_offset=0x%x", src_offset);
	if (src_offset > src_skip) {
		src_skip = src_offset;
		PRINT_MSG("botfs_copy(): src_skip=0x%x", src_skip);
	}
	PRINT_MSG("botfs_copy(): src_skip_end=0x%x", src_skip_end);
	PRINT_MSG("botfs_copy(): dest_offset=0x%x", dest_offset);

	const uint16_t to_copy = botfs_get_filesize(src) - src_skip - src_skip_end;
	PRINT_MSG("botfs_copy(): filesize(src)=0x%x", botfs_get_filesize(src));
	PRINT_MSG("botfs_copy(): to_copy=0x%x", to_copy);

	botfs_seek(src, (int16_t) src_skip, SEEK_SET);
	botfs_seek(&dest_file, (int16_t) (dest_offset + src_skip), SEEK_SET);
	uint16_t i;
	for (i = 0; i < to_copy; ++i) {
		if (botfs_read(src, buffer) != 0) {
			PRINT_MSG("botfs_copy(): Fehler beim Lesen aus der Quelldatei, i=0x%x", i);
			return -4;
		}
		if (botfs_write(&dest_file, buffer) != 0) {
			PRINT_MSG("botfs_copy(): Fehler beim Schreiben in die Zieldatei, i=0x%x", i);
			return -5;
		}
	}
	if (to_copy > 0) {
		PRINT_MSG("botfs_copy(): Bloecke 0x%x bis 0x%x kopiert", src_skip, src->pos - 1 - src->start);
	}

	if (botfs_read_header_data(src, NULL, buffer) != 0) {
		PRINT_MSG("botfs_copy(): Fehler beim Lesen der Headerdaten");
		return -6;
	}

	if (botfs_write_header_data(&dest_file, buffer) != 0) {
		PRINT_MSG("botfs_copy(): Fehler beim Schreiben der Headerdaten");
		return -7;
	}

	if (dest_file.used.start != UINT16_MAX && dest_file.used.end != 0) {
		PRINT_MSG("botfs_copy(): dest_file->used=[0x%x; 0x%x]", dest_file.used.start - (uint16_t) (dest_file.start + BOTFS_HEADER_SIZE),
			botfs_get_filesize(&dest_file) - (dest_file.end - dest_file.used.end));
	}

	botfs_close(&dest_file, buffer);

	return 0;
}
#endif // BOTFS_COPY_AVAILABLE
#endif // BOT_FS_AVAILABLE
