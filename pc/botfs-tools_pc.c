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
 * \file 	botfs-tools_pc.c
 * \brief 	Management-Funktionen des Dateisystems BotFS fuer PC
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	20.02.2008
 */

#ifdef PC
#include "ct-Bot.h"

#ifdef BOT_FS_AVAILABLE
#include "botfs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char * volume = NULL; /**< Volume-Image */
static int volume_loaded = 0; /**< Volume geladen? */

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

/**
 * Zeigt die Hilfe an
 */
static void help(void) {
	printf("create volume\terstellt ein neues Volume\n");
	printf("open volume\toeffnet ein Volume\n");
	printf("create file\terstellt eine neue Datei\n");
	printf("delete file\tloescht eine Datei (rm)\n");
	printf("move file\tbenennt eine Datei um oder verschiebt sie (mv)\n");
	printf("write file\tschreibt Text in eine Datei\n");
	printf("read file\tzeigt den Inhalt einer Datei an (cat)\n");
#ifdef BOTFS_STREAM_AVAILABLE
	printf("read lines\tzeigt den Inhalt einer (Text-)Datei zeilenweise an\n");
#endif
	printf("clear file\tleert eine Datei\n");
	printf("ls \t\tzeigt alle Dateien an\n");
	printf("lfree \t\tzeigt die Freelist an\n");
	printf("copy file\tkopiert eine Datei vom PC-Dateisystem auf das Volume (cp)\n");
	printf("extract file\tkopiert eine BotFS-Datei ins PC-Dateisystem\n");
	printf("quit\t\tbeenden (q)\n");
	printf("help\t\tzeigt diese Hilfe an (h)\n");
}

/**
 * Beendet das Programm
 */
static void quit(void) {
	exit(0);
}

/**
 * Laedt ein Volume aus einer angegebenen Image-Datei
 * \param *file	Dateiname des Images
 */
static void load_volume(char * file) {
	/* Dateiname global speichern */
	volume = file;
	uint8_t buffer[BOTFS_BLOCK_SIZE];

	/* Init-Funktion des Treibers aufrufen */
	if (botfs_init(volume, buffer, False) == 0) {
		volume_loaded = 1;
		printf("Volume \"%s\" geladen\n", volume);
		/* Volume-Name aus dem Header lesen und ausgeben */
		botfs_file_descr_t tmp;
		botfs_open(BOTFS_VOLUME_DATA, &tmp, BOTFS_MODE_r, buffer); // Volume-Header ist per Datei adressierbar
		botfs_read(&tmp, buffer);
		botfs_volume_t * vol = (void *) buffer;
		printf("Volumename: \"%s\"\n", vol->data.name);
	} else {
		printf("Volume \"%s\" konnte nicht geladen werden\n", volume);
		volume = NULL;
	}
}

/**
 * Erstellt ein neues Volume
 */
static void create_volume(void) {
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;
	/* Dateiname der Volume-Images einlesen */
	printf("Volume-Datei (< %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char vol_file[strlen(in_buffer) + 1];
	strcpy(vol_file, in_buffer);

	/* Volume-Name einlesen */
	printf("Volume-Name (< %u Zeichen): ", BOTFS_VOL_NAME_SIZE);
	rv = fgets(in_buffer, BOTFS_VOL_NAME_SIZE + 2, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char vol_name[strlen(in_buffer) + 1];
	strcpy(vol_name, in_buffer);

	/* Volume-Groesse einlesen */
	printf("Groesse in KB: ");
	rv = fgets(in_buffer, 7, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char tmp[strlen(in_buffer) + 1];
	strcpy(tmp, in_buffer);
	uint32_t vol_size = atoi(tmp);

	/* Volume vom Treiber erzeugen lassen */
	printf("erzeuge Volume \"%s\" der Groesse %u KB in Datei \"%s\"...\n", vol_name, vol_size, vol_file);
	if (botfs_create_volume(vol_file, vol_name, vol_size * 1024) == 0) {
		printf("Volume \"%s\" wurde korrekt erzeugt\n", vol_name);
	} else {
		printf("Fehler beim Erzeugen des Volumes\n");
	}

	/* Volume gleich laden */
	load_volume(vol_file);
}

/**
 * Oeffnet ein Volume, Dateiname wird abgefragt
 */
static void open_volume(void) {
	volume_loaded = 0;

	/* Name der Volume-Image-Datei einlesen */
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;
	printf("Volume-Datei (< %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg

	/* Dateiname globel speichern */
	char vol_file[BOTFS_MAX_FILENAME + 2];
	strncpy(vol_file, in_buffer, BOTFS_MAX_FILENAME);

	/* Image-Datei laden */
	load_volume(vol_file);
}

/**
 * Legt eine neue Datei an
 */
static void create_file(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}

	/* Dateiname einlesen */
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;
	printf("Dateiname (<= %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char name[strlen(in_buffer) + 2];
	char * ptr = name;
	if (in_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, in_buffer);

	/* Groesse einlesen */
	printf("Groesse in KB: ");
	rv = fgets(in_buffer, 7, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char tmp[strlen(in_buffer) + 1];
	strcpy(tmp, in_buffer);
	uint32_t size = atoi(tmp);

	/* Treiber-Aufruf */
	uint8_t buffer[BOTFS_BLOCK_SIZE];
	if (botfs_create(name, size * 2, buffer) == 0) {
		printf("Datei \"%s\" wurde korrekt erzeugt\n", name);
	} else {
		printf("Fehler beim Erzeugen der Datei\n");
	}
}

/**
 * Prueft, ob eine Datei vorhanden ist
 * \param *name		Dateiname
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \return			0, falls Datei vorhanden
 */
static int check_filename(char * name, void * buffer) {
    botfs_file_descr_t tmp;
    if (botfs_open(name, &tmp, BOTFS_MODE_r, buffer) != 0) {
		printf("Datei \"%s\" nicht vorhanden\n", name);
		return -1;
	}
    return 0;
}

/**
 * Loescht eine Datei
 */
static void delete_file(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}

	/* Dateiname einlesen */
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;
	printf("Dateiname (< %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char name[strlen(in_buffer) + 2];
	char * ptr = name;
	if (in_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, in_buffer);

	/* Treiber aufrufen */
	uint8_t buffer[BOTFS_BLOCK_SIZE];
    if (check_filename(name, buffer) != 0) {
    	return;
    }
    if (botfs_unlink(name, buffer) == 0) {
		printf("Datei \"%s\" wurde geloescht\n", name);
	} else {
		printf("Fehler beim Loeschen der Datei\n");
	}
}

/**
 * Benennt eine Datei um
 */
static void move_file(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}

	/* alten Dateiname einlesen */
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;
	printf("bisheriger Dateiname (< %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char old_name[strlen(in_buffer) + 2];
	char * ptr = old_name;
	if (in_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, in_buffer);
	uint8_t buffer[BOTFS_BLOCK_SIZE];
    if (check_filename(old_name, buffer) != 0) {
    	return;
    }

	/* neuen Dateinamen einlesen */
	char out_buffer[BOTFS_MAX_FILENAME + 2];
	printf("neuer Dateiname (< %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(out_buffer, BOTFS_MAX_FILENAME, stdin);
	out_buffer[strlen(out_buffer) - 1] = 0; // \n weg
	char new_name[strlen(out_buffer) + 2];
	ptr = new_name;
	if (out_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, out_buffer);

	/* Treiber aufrufen */
	if (botfs_rename(old_name, new_name, buffer) == 0) {
		printf("Datei \"%s\" wurde in \"%s\" umbenannt\n", old_name, new_name);
	} else {
		printf("Fehler beim Umbenennen der Datei\n");
	}
}

/**
 * Schreibt Text aus stdin in eine Datei
 */
static void write_file(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;

	/* Dateiname einlesen */
	printf("Dateiname (<= %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char name[strlen(in_buffer) + 2];
	char * ptr = name;
	if (in_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, in_buffer);
	char buffer[BOTFS_BLOCK_SIZE];
    if (check_filename(name, buffer) != 0) {
    	return;
    }

	/* Datei zum Lesen + Schreiben oeffnen */
	botfs_file_descr_t file;
	if (botfs_open(name, &file, BOTFS_MODE_R, buffer) == 0) {
		printf("Datei \"%s\" geoeffnet, Texteingabe:\n", name);
	} else {
		printf("Fehler beim Oeffnen der Datei\n");
	}

	/* Puffer vorbereiten */
	memset(buffer, 0, BOTFS_BLOCK_SIZE);

	/* Daten einlesen */
	rv = fgets(buffer, BOTFS_BLOCK_SIZE - 2, stdin);

	/* Treiber-Aufruf */
	if (botfs_write(&file, buffer) == 0) {
		printf("Daten wurden korrekt geschrieben\n");
	} else {
		printf("Fehler beim Schreiben der Daten\n");
	}
}

/**
 * Kopiert eine Datei mit Pfad aus stdin in eine BotFS-Datei
 */
static void copy_file(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}

	char file_path[256];
	char * rv;
	/* Quell-Dateiname einlesen */
	printf("Dateiname Quelle: ");
	rv = fgets(file_path, sizeof(file_path), stdin);
	file_path[strlen(file_path) - 1] = 0; // \n weg

	char in_buffer[BOTFS_MAX_FILENAME + 2];
	/* Ziel-Dateiname einlesen */
	printf("Dateiname (<= %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char name[strlen(in_buffer) + 2];
	char * ptr = name;
	if (in_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, in_buffer);

	uint8_t buffer[BOTFS_BLOCK_SIZE];
	if (botfs_copy_file(name, file_path, buffer) == 0) {
		printf("Datei erfolgreich auf BotFS-Volume kopiert\n");
	} else {
		printf("Fehler beim Kopieren der Datei\n");
	}
}

/**
 * Zeigt den (Text-)Inhalt einer Datei auf stdout an
 */
static void read_file(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;

	/* Dateiname einlesen */
	printf("Dateiname (<= %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char name[strlen(in_buffer) + 2];
	char * ptr = name;
	if (in_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, in_buffer);

	/* Datei zum Lesen oeffnen */
	botfs_file_descr_t file;
	uint8_t buffer[BOTFS_BLOCK_SIZE];
    if (check_filename(name, buffer) != 0) {
    	return;
    }
	if (botfs_open(name, &file, BOTFS_MODE_r, buffer) == 0) {
		printf("Datei \"%s\" geoeffnet\n", name);
	} else {
		printf("Fehler beim Oeffnen der Datei\n");
	}

	/* Inhalt ausgeben */
	while (botfs_read(&file, buffer) == 0) {
		unsigned i;
		for (i = 0; i < BOTFS_BLOCK_SIZE; ++i) {
			printf("%c", buffer[i]);
		}
	}
	printf("\n");
}

/**
 * Kopiert eine BotFS-Datei ins PC-Dateisystem
 */
static void extract_file(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;

	/* Dateiname einlesen */
	printf("BottFS-Dateiname (<= %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char from[strlen(in_buffer) + 2];
	char * ptr = from;
	if (in_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, in_buffer);

	/* Quelldatei pruefen */
	uint8_t buffer[BOTFS_BLOCK_SIZE];
    if (check_filename(from, buffer) != 0) {
    	return;
    }

	/* Ziel-Dateiname einlesen */
	char file_path[256];
	printf("Dateiname Ziel: ");
	rv = fgets(file_path, sizeof(file_path), stdin);
	file_path[strlen(file_path) - 1] = 0; // \n weg

	if (botfs_extract_file(file_path, from, buffer) == 0) {
		printf("BotFS-Datei \"%s\" erfolgreich nach \"%s\" kopiert\n", from, file_path);
	} else {
		printf("Fehler beim Kopieren\n");
	}
}


#ifdef BOTFS_STREAM_AVAILABLE
/**
 * Zeigt den (Text-)Inhalt einer Datei zeilenweise auf stdout an
 */
static void read_lines(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}
	char in_buffer[BOTFS_MAX_FILENAME + 2];

	/* Dateiname einlesen */
	printf("Dateiname (<= %lu Zeichen): ", BOTFS_MAX_FILENAME);
	fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char name[strlen(in_buffer) + 2];
	char * ptr = name;
	if (in_buffer[0] != '/') {
		*ptr = '/';
		ptr++;
	}
	strcpy(ptr, in_buffer);
	uint8_t buffer[BOTFS_BLOCK_SIZE];
    if (check_filename(name, buffer) != 0) {
    	return;
    }

	/* Datei zum Lesen oeffnen */
	botfs_file_descr_t file;
	if (botfs_open(name, &file, BOTFS_MODE_r, buffer) == 0) {
		printf("Datei \"%s\" geoeffnet\n", name);
	} else {
		printf("Fehler beim Oeffnen der Datei\n");
	}

	char in[20];
	botfs_stream_t stream;
	botfs_stream_open(&stream, &file, buffer);
	char line[200];
	int i = 1;
	int16_t n;
	do {
		n = botfs_stream_readline(&stream, line, sizeof(line) - 1);
		if (n <= 0) {
			printf("\nEnde\n");
			break;
		}
		printf("%s", line);
		printf(">line is %d bytes long<\n", n);
		++i;
		fgets(in, sizeof(in), stdin);
	} while (strncasecmp(in, "q", 1) != 0);

	printf("\n%d Zeilen\n", i);
}
#endif // BOTFS_STREAM_AVAILABLE

/**
 * Leert eine Datei (wird komplett mit Nullen ueberschrieben)
 */
static void clear_file(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}
	char in_buffer[BOTFS_MAX_FILENAME + 2];
	char * rv;

	/* Dateiname einlesen */
	printf("Dateiname (<= %lu Zeichen): ", BOTFS_MAX_FILENAME);
	rv = fgets(in_buffer, BOTFS_MAX_FILENAME, stdin);
	in_buffer[strlen(in_buffer) - 1] = 0; // \n weg
	char name[strlen(in_buffer) + 1];
	strcpy(name, in_buffer);
	uint8_t buffer[BOTFS_BLOCK_SIZE];
    if (check_filename(name, buffer) != 0) {
    	return;
    }

	/* Datei neu anlegen */
	botfs_file_descr_t file;
	if (botfs_open(name, &file, BOTFS_MODE_W, buffer) == 0) {
		printf("Datei \"%s\" geleert\n", name);
	} else {
		printf("Fehler beim Leeren der Datei\n");
	}
}

/**
 * Zeigt alle Dateien im Root-Verzeichnis an
 */
static void ls(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}
	uint8_t buffer[BOTFS_BLOCK_SIZE];
	char * name;
	uint8_t i = 0;
	while ((name = botfs_readdir(i++, buffer)) != NULL) {
		botfs_file_descr_t file;
		char filename[BOTFS_MAX_FILENAME + 1];
		strncpy(filename, name, BOTFS_MAX_FILENAME);
		if (botfs_open(filename, &file, BOTFS_MODE_r, buffer) != 0) {
			printf("Fehler, konnte Datei \"%s\" nicht oeffnen!\n", filename);
			return;
		}
		const uint16_t size = file.end - file.start + 1;
		if (size >= 2) {
			printf("Eintrag %3u:%15s\tstart:0x%04x\tend:0x%04x\n\t\t\t\tsize: %u KByte\t\t", i, filename,
				file.start, file.end, size / 2);
		} else {
			printf("Eintrag %3u:%15s\tstart:0x%04x\tend:0x%04x\n\t\t\t\tsize: %u  Byte\t\t", i, filename,
				file.start, file.end, size * BOTFS_BLOCK_SIZE);
		}
		const uint32_t used_size = (file.used.end - file.start) * BOTFS_BLOCK_SIZE + file.used.bytes_last_block;
		printf("used-size: %u Byte\n", used_size);
	}
}

/**
 * Zeigt alle Freelist-Eintraege an
 */
static void lfree(void) {
	if (volume_loaded != 1) {
		printf("Kein Volume geladen!\n");
		return;
	}
	uint8_t buffer[BOTFS_BLOCK_SIZE];
	botfs_print_freelist(buffer);
}

/** Kommandos, die akzeptiert werden */
static const char * commands[] = {
	"help", "h",
	"quit", "q",
	"create volume",
	"open volume",
	"ls",
	"lfree",
	"create file",
	"delete file", "rm",
	"move file", "mv",
	"write file",
	"read file", "cat",
	"clear file",
	"copy file", "cp",
	"extract file",
#ifdef BOTFS_STREAM_AVAILABLE
	"read lines",
#endif
};

/** Kommando-Hanlder */
static void (* handler[])(void) = {
	help, help,
	quit, quit,
	create_volume,
	open_volume,
	ls,
	lfree,
	create_file,
	delete_file, delete_file,
	move_file, move_file,
	write_file,
	read_file, read_file,
	clear_file,
	copy_file, copy_file,
	extract_file,
#ifdef BOTFS_STREAM_AVAILABLE
	read_lines,
#endif
};

/**
 * Management-Tools fuer BotFS
 */
void botfs_management(char * volume_file) {
	static const char * title = "BotFS-Verwaltung ";
	const size_t n_commands = sizeof(commands) / sizeof(commands[0]);
	const size_t n_handlers = sizeof(handler) / sizeof(handler[0]);
	if (n_commands != n_handlers) {
		printf("Kommando-Tabelle ungueltig:\n");
		printf("%u Kommandos, aber %u Handler\n", (unsigned) n_commands, (unsigned) n_handlers);
		exit(1);
	}
	if (volume_file != NULL) {
		load_volume(volume_file);
	}
	char in_buffer[32];
	printf("\n%s%u.%u\n", title, BOTFS_VERSION / 10, BOTFS_VERSION % 10);
	printf("\nHilfe mit Kommando \"help\"\n> ");

	/* Hauptschleife */
	while(1) {
		/* Kommando einlesen */
		if (fgets(in_buffer, 30, stdin) == NULL) {
			continue;
		}
		printf("\n");

		/* Schluesselwort suchen */
		unsigned i;
		for (i = 0; i < sizeof(handler) / sizeof(void *); ++i) {
			if (strncmp(commands[i], in_buffer, strlen(commands[i])) == 0) {
				handler[i]();
				break;
			}
		}
		if (i == sizeof(handler) / sizeof(void *)) {
			printf("invalid command\n");
		}
		printf("\n%s%u.%u\n> ", title, BOTFS_VERSION / 10, BOTFS_VERSION % 10);
	}
}

/**
 * \brief Zeigt Informationen ueber jeden Eintrag des Root-Verzeichnisses einer FAT16-Partition an.
 *
 * Insbesondere welche Clusterketten von welcher Datei belegt werden. Dadurch laesst
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
void botfs_read_fat16(const char * path) {
	char data_buffer[BOTFS_BLOCK_SIZE];
	void * buffer = data_buffer;
	FILE * fp = fopen(path, "rb");
	if (fp == NULL) {
		printf("Konnte \"%s\" nicht oeffnen!\n", path);
		return;
	}

	/* MBR lesen */
	fseek(fp, 0, SEEK_SET);
	if (fread(buffer, BOTFS_BLOCK_SIZE, 1, fp) != 1) {
		puts("Fehler beim Lesen");
		fclose(fp);
		return;
	}

	/* Bootsektor der ersten Partition ermitteln.
	 * Trick: Haben wir gar nicht den MBR, sondern den Bootsektor einer
	 * Partition gelesen (s.o.), steht an mbr->part0.first_sect_offset
	 * (Adresse 0x1c6) irgendwelcher Boot-Code. Fuer den (Normal-)Fall,
	 * dass die Partition gar keinen Boot-Code enthaelt, lesen wir hier
	 * Nullen, wodurch sich als Offset des Bootsektors 0 ergibt :-).
	 */
	botfs_mbr_t * p_mbr = buffer;
	uint16_t first_sect = p_mbr->part0.first_sect_offset;
	printf("Offset von Partition 0:\t0x%04x\n", first_sect);
//	char type[11] = "";
//	switch (p_mbr->part0.type) {
//	case 0x01:
//		strcpy(type, "FAT12");
//		break;
//	case 0x04:
//		strcpy(type, "FAT16");
//		break;
//	case 0x06:
//		strcpy(type, "FAT16");
//		break;
//	case 0x07:
//		strcpy(type, "NTFS");
//		break;
//	case 0x0b:
//		strcpy(type, "FAT32");
//		break;
//	case 0x0c:
//		strcpy(type, "FAT32");
//		break;
//	case 0x0e:
//		strcpy(type, "FAT16");
//		break;
//	case 0x82:
//		strcpy(type, "Linux Swap");
//		break;
//	case 0x83:
//		strcpy(type, "Linux");
//		break;
//	}
//	printf("Typ:\t0x%x\n", p_mbr->part0.type);
//	printf("Partitionstyp:\t\"%s\"\n", type);
//	if (strncmp(type, "FAT16", 5) != 0) {
//		printf("Keine FAT16-Partition!\n");
//		fclose(fp);
//		return;
//	}

	/* Bootsektor lesen */
	fseek(fp, first_sect * BOTFS_BLOCK_SIZE, SEEK_SET);
	if (fread(buffer, BOTFS_BLOCK_SIZE, 1, fp) != 1) {
		puts("Fehler beim Lesen");
		fclose(fp);
		return;
	}

	/* Bootsektor auswerten */
	botfs_fat16_bootsector_t * p_bs = buffer;
	char tmp_string[12];
	tmp_string[sizeof(tmp_string) - 1] = 0;
	strncpy(tmp_string, p_bs->volume_name, 11);
	printf("Volumename:\t\t%s\n", tmp_string);
	tmp_string[7] = 0;
	strncpy(tmp_string, p_bs->fat_name, 8);
	printf("Partitionstyp:\t\t%s\n", tmp_string);
	if (strncmp(tmp_string, "FAT16", 5) != 0) {
/** \todo: lt. Spezifikation steht hier nicht zwingend der FAT-Typ */
		puts("Keine FAT16-Partition!");
		fclose(fp);
		return;
	}
	uint16_t fat_offset = p_bs->reserved_sect + first_sect;
	printf("Offset der FAT:\t\t0x%04x\n", fat_offset);
	uint32_t fat_size = p_bs->sect_per_fat * BOTFS_BLOCK_SIZE;
	printf("Groesse der FAT:\t0x%04x\n", p_bs->sect_per_fat);
	uint16_t root_offset = fat_offset + p_bs->fat_copies * p_bs->sect_per_fat;
	printf("Offset des Rootverz.:\t0x%04x\n", root_offset);
	uint8_t n = p_bs->root_dir_entries / (BOTFS_BLOCK_SIZE
		/ sizeof(botfs_fat16_dir_entry_t));
	printf("Groesse des Rootverz.:\t0x%04x\n", n);
	uint16_t data_offset = root_offset + (p_bs->root_dir_entries
		* sizeof(botfs_fat16_dir_entry_t) + (BOTFS_BLOCK_SIZE - 1))
		/ BOTFS_BLOCK_SIZE;
	printf("Erster Datensektor:\t0x%04x\n", data_offset);
	uint8_t sect_per_cluster = p_bs->sect_per_cluster;
	printf("Sektoren / Cluster:\t%u\n", sect_per_cluster);

	/* FAT einlesen */
	uint16_t fat[fat_size];
	fseek(fp, fat_offset * BOTFS_BLOCK_SIZE, SEEK_SET);
	if (fread(&fat, fat_size, 1, fp) != 1) {
		puts("Fehler beim Lesen der FAT");
		fclose(fp);
		return;
	}

	/* Root-Verzeichnis durchsuchen */
	puts("");
	fseek(fp, root_offset * BOTFS_BLOCK_SIZE, SEEK_SET);
	uint8_t i;
	for (i=n; i>0; --i) {
		/* Blockweise Verzeichniseintraege laden */
		if (fread(buffer, BOTFS_BLOCK_SIZE, 1, fp) != 1) {
			puts("Fehler beim Lesen");
			fclose(fp);
			return;
		}
		botfs_fat16_dir_entry_t * p_dir_entry = buffer;
		uint8_t j;
		for (j = BOTFS_BLOCK_SIZE / sizeof(botfs_fat16_dir_entry_t); j > 0; --j, ++p_dir_entry) {
			/* Verzeichniseintraege durchsuchen */
			if (strlen(p_dir_entry->name) > 0 && (uint8_t) p_dir_entry->name[0] != 0xe5 && p_dir_entry->attributes != 0x0f) {
				char name[9];
				name[sizeof(name) - 1] = 0;
				strncpy(name, p_dir_entry->name, sizeof(name) - 1);
				int k;
				for (k = 0; k < 8; ++k) {
					/* delete spaces */
					if (name[k] == 0x20) {
						name[k] = 0;
					}
				}
				char ext[4];
				ext[sizeof(ext) - 1] = 0;
				strncpy(ext, p_dir_entry->extension, sizeof(ext) - 1);
				for (k = 0; k < 3; ++k) {
					/* delete spaces */
					if (ext[k] == 0x20) {
						ext[k] = 0;
					}
				}
				printf("%s.%s:\n", name, ext);
				printf("\tAttribute:\t0x%02x", p_dir_entry->attributes);
				if (((p_dir_entry->attributes >> 4) & 0x01) == 0x01) {
					puts(" -> Verzeichnis");
				} else if (((p_dir_entry->attributes >> 3) & 0x01) == 0x01) {
					puts(" -> Volume\n");
					continue;
				} else {
					puts(" -> Datei");
				}
				uint32_t block = (uint32_t)(p_dir_entry->first_cluster - 2) * sect_per_cluster + data_offset;
				uint32_t start = block * BOTFS_BLOCK_SIZE;
				printf("\tStart:  \t0x%x (Sektor: 0x%x)\n", start, block);
				/* FAT auflisten */
				uint16_t next_cluster = p_dir_entry->first_cluster;
				uint16_t start_cluster = next_cluster;
				uint16_t last_cluster = next_cluster;
				uint32_t size = 0;
				printf("\tCluster:\t0x%04x", next_cluster);
				while (42) {
					next_cluster = fat[next_cluster];
					if (next_cluster != last_cluster + 1) {
						printf(" - 0x%04x\n", last_cluster);
						size += last_cluster - start_cluster + 1;
						start_cluster = next_cluster;
						if (next_cluster < 0xfff0) {
							printf("\tCluster:\t0x%04x", next_cluster);
						} else {
							if (last_cluster != 0) {
								printf("\tGroesse:\t%u KByte\n", size * sect_per_cluster * BOTFS_BLOCK_SIZE / 1024);
							} else {
								printf("\tGroesse:\t0 KByte\n");
							}
							break;
						}
					}
					last_cluster = next_cluster;
				}
				puts("");
			}
		}
	}
	fclose(fp);
 }

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
#pragma GCC diagnostic pop
#endif

#endif // BOT_FS_AVAILABLE
#endif // PC
