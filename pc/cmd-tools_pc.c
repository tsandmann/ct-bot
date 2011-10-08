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
 * \file 	cmd-tools_pc.c
 * \brief 	Funktionen, die per Commandline-Switch aufgerufen werden koennen
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	19.02.2008
 */

#ifdef PC
#include "ct-Bot.h"
#include "cmd_tools.h"
#include "tcp-server.h"
#include "mini-fat.h"
#include "map.h"
#include "eeprom.h"
#include "command.h"
#include "tcp.h"
#include "bot-logic/bot-logic.h"
#include "botfs.h"
#include "sensor-low.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

//#define DEBUG

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
static pthread_t cmd_thread; /**< Thread fuer die RemoteCall-Auswertung per Kommandozeile */
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE

/**
 * Zeigt Informationen zu den moeglichen Kommandozeilenargumenten an.
 */
static void usage(void) {
	puts("USAGE: ct-Bot [-t host] [-a address] [-T] [-h] [-s] [-M FROM] [-m FILE] [-c FILE ID SIZE] [-e ADDR ID SIZE] [-d ID] [-i] [-f [IMAGE]] [-k IMAGE SOURCEFILE DESTFILE] [-F PATH]");
	puts("\t-t\tHostname oder IP Adresse zu der Verbunden werden soll");
	puts("\t-a\tAdresse des Bots (fuer Bot-2-Bot-Kommunikation), default: 0");
	puts("\t-T\tTestClient");
	puts("\t-s\tServermodus");
#ifdef BOT_FS_AVAILABLE
	puts("\t-M \tKonvertiert eine Bot-Map aus dem BotFS-Image in eine PGM-Datei");
#else
	puts("\t-M FROM \tKonvertiert eine Bot-Map aus Datei FROM in eine PGM-Datei");
#endif
	puts("\t-m FILE\tGibt den Pfad zu einer MiniFat-Datei an, die vom Map-Code verwendet wird (Ex- und Import)");
#ifndef MAP_AVAILABLE
	puts("\t\tACHTUNG, das Programm wurde ohne MAP_AVAILABLE uebersetzt, die Optionen -M / -m stehen derzeit also NICHT zur Verfuegung");
#endif
	puts("\t-c \tErzeugt eine Mini-Fat-Datei fuer den Bot.");
	puts("\t   FILE\tDateiname");
	puts("\t   ID  \tDie ID aus ASCII-Zeichen");
	puts("\t   SIZE\tDie Nutzgroesse der Datei in KByte");
	puts("\t-e \tErzeugt eine Mini-Fat-Datei fuer den Sim (emulierte MMC).");
	puts("\t   ADDR\tStartadresse der Mini-Fat-Datei");
	puts("\t   ID  \tDie ID aus ASCII-Zeichen");
	puts("\t   SIZE\tDie Nutzgroesse der Datei in KByte");
	puts("\t-d \tLoescht eine Mini-Fat-Datei fuer den Sim (emulierte MMC).");
	puts("\t   ID  \tDie ID aus ASCII-Zeichen");
	puts("\t-l \tKonvertiert eine SpeedLog-Datei in eine txt-Datei");
	puts("\t   FILE\tBotFS-Image-Datei");
	puts("\t-E \tInitialisiert das EEPROM mit den Daten der EEP-Datei");
#ifdef BOT_FS_AVAILABLE
	puts("\t-i FILE\tverwendet FILE als Volume-Image fuer BotFS");
	puts("\t-f \tStartet die BotFS-Verwaltung");
	puts("\t   [IMAGE] \tPfad zur Image-Datei");
	puts("\t-k \tKopiert eine Datei auf ein BotFS-Volume");
	puts("\t   IMAGE \tPfad zur Image-Datei");
	puts("\t   SOURCEFILE \tPfad zur Datei, die kopiert werden soll (Quelldatei)");
	puts("\t   DESTFILE \tDateiname der Zieldatei");
	puts("\t-o \tKopiert eine Datei vom BotFS-Volume ins PC-Dateisystem");
	puts("\t   IMAGE \tPfad zur Image-Datei");
	puts("\t   SOURCEFILE \tDateiname der Quelldatei");
	puts("\t   DESTFILE \tPfad zur Datei, auf die kopiert werden soll (Zieldatei)");
	puts("\t-F \tZeigt Informationen ueber Dateien eines FAT16-Dateisystems an");
#endif // BOT_FS_AVAILABLE
	puts("\t   PATH \tPfad zum Dateisystem / Image-Datei");
	puts("\t-h\tZeigt diese Hilfe an");
}

/**
 * Behandelt die Kommandozeilen-Argumente
 * \param argc	Anzahl der Argumente
 * \param *argv	Zeiger auf String-Array der Argumente
 */
void hand_cmd_args(int argc, char * argv[]) {
	/* Der Zielhost wird per default durch das Macro IP definiert und
	 * tcp_hostname mit einer Kopie des Strings initialisiert. */
	tcp_hostname = malloc(strlen(IP) + 1);
	if (NULL == tcp_hostname) {
		exit(1);
	}
	strcpy(tcp_hostname, IP);

	int ch;
	/* Die Kommandozeilenargumente komplett verarbeiten */
	while ((ch = getopt(argc, argv, "hsTEt:Mm:c:l:e:d:a:i:fk:o:F:")) != -1) {
		argc -= optind;
		argv += optind;
		switch (ch) {

		case 's': {
			/* Servermodus [-s] wird verlangt */
			printf("ARGV[0]= %s\n", argv[0]);
			tcp_server_init();
			tcp_server_run(1000); // beendet per exit()
			break;
		}

		case 'T': {
			/* Testclient starten */
			tcp_test_client_init();
			tcp_test_client_run(1000); // beendet per exit()
			break;
		}

		case 't': {
			/* Hostname, auf dem ct-Sim laeuft, wurde uebergeben. Der String wird in hostname gesichert. */
			const size_t len = strlen(optarg) + 1;
			if (len > 255) {
				puts("hostname ungueltig");
				exit(1);
			}
			tcp_hostname = realloc(tcp_hostname, len);
			if (tcp_hostname == NULL) {
				exit(1);
			}
			strcpy(tcp_hostname, optarg);
			break;
		}

		case 'a': {
			/* Bot-Adresse wurde uebergeben */
			int addr = atoi(optarg);
			if ((addr >= CMD_SIM_ADDR) && (addr != CMD_BROADCAST)) {
				puts("Unzulaessige Bot-Adresse!");
				exit(1);
			}
			set_bot_address(addr);
			break;
		}

		case 'i': {
			/* angegebenes BotFS Image verwenden */
#ifdef BOT_FS_AVAILABLE
			const size_t len = strlen(optarg) + 1;
			if (len > 1024) {
				puts("Image-Pfad ungueltig");
				exit(1);
			}
			botfs_volume_image_file = malloc(len);
			if (NULL == botfs_volume_image_file) {
				exit(1);
			}
			strcpy(botfs_volume_image_file, optarg);
			printf("Verwende \"%s\" als BotFS-Volume-Image\n", botfs_volume_image_file);
#else
			puts("Fehler, Binary wurde ohne BOT_FS_AVAILABLE compiliert!");
			exit(1);
#endif // BOT_FS_AVAILABLE
			break;
		}

		case 'M': {
			/* Dateiname fuer die Map wurde uebergeben. Der String wird in from gesichert. */
#ifndef MAP_AVAILABLE
			puts("ACHTUNG, das Programm wurde ohne MAP_AVAILABLE uebersetzt, die Option -M steht derzeit also NICHT zur Verfuegung.");
			puts("um dennoch Karten zu konvertieren, bitte im Quelltext in der Datei ct-Bot.h die Kommentarzeichen vor MAP_AVAILABLE entfernen");
			puts("und neu compilieren.");
			exit(1);
#endif
#ifdef MMC_VM_AVAILABLE
			puts("Executable wurde mit MMC_VM_AVAILABLE compiliert.");
			puts("Um Karten des echten Bots einlesen zu koennen, bitte den Code bitte ohne MMC_VM_AVAILABLE neu uebersetzen.");
			exit(1);
#endif
#ifdef MAP_AVAILABLE
			/* Karte in pgm konvertieren */
#ifndef BOT_FS_AVAILABLE
			if (argc != 1) {
				usage();
				exit(1);
			}
			const size_t len = strlen(argv[0]) + 1;
			if (len > 1024) {
				puts("Dateiname ungueltig");
				exit(1);
			}
			printf("Konvertiere Karte %s in PGM %s\n", argv[0], "map.pgm");
			map_read(argv[0]);
#else
			uint8_t buffer[BOTFS_BLOCK_SIZE];
			if (botfs_init(botfs_volume_image_file, buffer, False) != 0) {
				puts("BotFS konnte nicht initialisiert werden");
				exit(1);
			}
			if (map_init() != 0) {
				puts("Map-Subsystem konnte nicht initialisiert werde");
				exit(1);
			}
#endif // BOT_FS_AVAILABLE
			map_to_pgm("map.pgm");
			exit(0);
#endif // MAP_AVAILABLE
		}

		case 'm': {
#ifndef MAP_AVAILABLE
			puts("ACHTUNG, das Programm wurde ohne MAP_AVAILABLE uebersetzt, die Option -m steht derzeit also NICHT zur Verfuegung.");
			puts("um dennoch Karten zu konvertieren, bitte im Quelltext in der Datei ct-Bot.h die Kommentarzeichen vor MAP_AVAILABLE entfernen");
			puts("und neu compilieren.");
			exit(1);
#else
			/* Karte einlesen */
			const size_t len = strlen(optarg) + 1;
			if (len > 1024) {
				puts("Dateiname ungueltig");
				exit(1);
			}
			printf("Lese Karte von \"%s\" ein\n", optarg);
#ifdef BOT_FS_AVAILABLE
			uint8_t buffer[BOTFS_BLOCK_SIZE];
			if (botfs_init(botfs_volume_image_file, buffer, False) != 0) {
				puts("BotFS konnte nicht initialisiert werden");
				exit(1);
			}
#endif // BOT_FS_AVAILABLE
			map_read(optarg);
#endif // MAP_AVAILABLE
			break;
		}

		case 'f': {
			/* BotFS Tool */
#ifdef BOT_FS_AVAILABLE
			char * image = argv[0];
			botfs_management(image);
#else
			puts("Fehler, Binary wurde ohne BOT_FS_AVAILABLE compiliert!");
			exit(1);
#endif // BOT_FS_AVAILABLE
			break;
		}

		case 'k' : {
			/* BotFS file copy */
#ifdef BOT_FS_AVAILABLE
			char * image = optarg;
			char buffer[BOTFS_BLOCK_SIZE];
			if (botfs_init(image, buffer, False) != 0) {
				printf("Konnte BotFS mit Image \"%s\" nicht initialisieren\n", image);
				exit(1);
			}
			if (argc != 2) {
				usage();
				exit(1);
			}
			const char * source_file = argv[0];
			const char * dest_file = argv[1];
			if (botfs_copy_file(dest_file, source_file, buffer) == 0) {
				puts("Datei erfolgreich kopiert");
				exit(0);
			} else {
				puts("Fehler beim Kopieren");
				exit(1);
			}
#else
			puts("Fehler, Binary wurde ohne BOT_FS_AVAILABLE compiliert!");
			exit(1);
#endif // BOT_FS_AVAILABLE
		}

		case 'o' : {
			/* BotFS file extract */
#ifdef BOT_FS_AVAILABLE
			char * image = optarg;
			char buffer[BOTFS_BLOCK_SIZE];
			if (botfs_init(image, buffer, False) != 0) {
				printf("Konnte BotFS mit Image \"%s\" nicht initialisieren\n", image);
				exit(1);
			}
			if (argc != 2) {
				usage();
				exit(1);
			}
			const char * source_file = argv[0];
			const char * dest_file = argv[1];
			if (botfs_extract_file(dest_file, source_file, 0, 0, buffer) == 0) {
				puts("Datei erfolgreich kopiert");
				exit(0);
			} else {
				puts("Fehler beim Kopieren");
				exit(1);
			}
#else
			puts("Fehler, Binary wurde ohne BOT_FS_AVAILABLE compiliert!");
			exit(1);
#endif // BOT_FS_AVAILABLE
		}

		case 'F': {
			/* Fat16-Tool */
#ifdef BOT_FS_AVAILABLE
			botfs_read_fat16(optarg);
			exit(0);
#else
			puts("Fehler, Binary wurde ohne BOT_FS_AVAILABLE compiliert!");
			exit(1);
#endif // BOT_FS_AVAILABLE
		}

		case 'c': {
			/* Datei fuer den Bot (mini-fat) soll erzeugt werden. */
			const int len = strlen(optarg) + 1;
			if (len > 1024) {
				puts("Dateiname ungueltig");
				exit(1);
			}
			if (argc != 2) {
				usage();
				exit(1);
			}
			const size_t id_len = strlen(argv[0]);
			if (id_len >= MMC_FILENAME_MAX) {
				puts("ID zu lang");
				exit(1);
			}
			const int size = atoi(argv[1]);
			printf("Mini-Fat-Datei (%s) mit %d kByte und ID=%s fuer den Bot soll erstellt werden.\n",
				optarg, size, argv[0]);
			create_mini_fat_file(optarg, argv[0], size);
			exit(0);
		}

		case 'e': {
			/* Datei fuer den Sim (mini-fat) soll erzeugt werden. */
			if (argc != 2) {
				usage();
				exit(1);
			}

			const int size = atoi(argv[1]);
			uint32_t addr = atoi(optarg);
			printf("Mini-Fat-Datei mit ID=%s an Adresse 0x%x mit %d kByte auf der emulierten MMC soll erstellt werden.\n",
				argv[0], addr, size);
			create_emu_mini_fat_file(addr, argv[0], size);
			exit(0);
		}

		case 'd': {
			/* Datei fuer den Sim (mini-fat) soll geloescht werden. */
			if (argc != 0) {
				usage();
				exit(1);
			}

			printf("Mini-Fat-Datei mit ID %s auf der emulierten MMC soll geloescht werden.\n", optarg);
			delete_emu_mini_fat_file(optarg);
			exit(0);
		}

		case 'l': {
#ifdef BOT_FS_AVAILABLE
			/* Speedlog-Datei soll in txt konvertiert werden */
			const size_t len = strlen(optarg) + 1;
			if (len > 1024) {
				puts("Dateiname ungueltig");
				exit(1);
			}
			convert_slog_file(optarg);
			exit(0);
#else
			puts("Fehler, Binary wurde ohne BOT_FS_AVAILABLE compiliert!");
			exit(1);
#endif // BOT_FS_AVAILABLE
		}

		case 'E': {
			/* EEPROM-Init */
			printf("EEPROM soll mit den Daten einer eep-Datei initialisiert werden.\n");
			if (init_eeprom_man(1) != 0) {
				puts("Fehler bei EEPROM-Initialisierung!");
			} else {
				puts("done.");
			}
			exit(0);
		}

		case 'h':
		default:
			/* -h oder falscher Parameter, Usage anzeigen */
			usage();
			exit(0);
		}
	}
}

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
/**
 * Liest RemoteCall-Commands von stdin ein
 */
static void read_command_thread(void) {
	char input[255];

	while (42) {
		putc('>', stdout);
		if (fgets(input, sizeof(input) - 1, stdin) == NULL) {
			continue;
		}
		if (*input == '\n' || strncmp(input, "list", strlen("list")) == 0) {
			int i = 0;
			while (remotecall_beh_list[i].func != NULL) {
				printf("%s(%s)\n", remotecall_beh_list[i].name, remotecall_beh_list[i].param_info);
				++i;
			}
			continue;
		} else if (strncmp(input, "cancel", strlen("cancel")) == 0) {
			puts("RemoteCall abgebrochen.");
			bot_remotecall_cancel();
			continue;
		}

		char * function = strtok(input, "(\n");
		if (function == NULL || *function == '\n') {
			continue;
		}
		char * param[REMOTE_CALL_MAX_PARAM] = {NULL};
		remote_call_data_t params[REMOTE_CALL_MAX_PARAM] = {{0}};
		int i;
		for (i = 0; i < REMOTE_CALL_MAX_PARAM; ++i) {
			param[i] = strtok(NULL, ",)");
			if (param[i] != NULL && *param[i] != '\n') {
				params[i].s32 = atoi(param[i]);
			}
		}

#ifdef DEBUG
		printf("function=\"%s\"\n", function);
#endif // DEBUG

		for (i = 0; i < REMOTE_CALL_MAX_PARAM; ++i) {
			if (param[i] != NULL && *param[i] != '\n') {
#ifdef DEBUG
				printf("param[%d]=\"%s\"\n", i, param[i]);
				printf("params[%d].s32=%d\tparams[%d].u32=0x%x\n", i, params[0].s32, i, params[0].u32);
#endif // DEBUG
			}
		}

		int8_t result = bot_remotecall(NULL, function, params);
		if (result == 0) {
			printf("RemoteCall \"%s(%d,%d,%d)\" gestartet\n", function, params[0].s32, params[1].s32, params[2].s32);
		} else if (result == -1) {
			puts("Es ist bereits ein RemoteCall aktiv!");
		} else if (result == -2) {
			char func[255] = "bot_";
			strncpy(func + strlen(func), function, sizeof(func) - strlen(func));
			result = bot_remotecall(NULL, func, params);
			if (result == 0) {
				printf("RemoteCall \"%s(%d,%d,%d)\" gestartet\n", func, params[0].s32, params[1].s32, params[2].s32);
			} else {
				printf("RemoteCall \"%s(%d,%d,%d)\" nicht vorhanden\n", function, params[0].s32, params[1].s32, params[2].s32);
			}
		}
	}
}
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE

/**
 * Initialisiert die Eingabekonsole fuer RemoteCalls
 */
void cmd_init(void) {
#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
	pthread_create(&cmd_thread, NULL, (void * (*)(void *)) read_command_thread, NULL);
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE
}

#endif // PC
