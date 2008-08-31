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

/*!
 * @file 	cmd-tools_pc.c
 * @brief 	Funktionen, die per Commandline-Switch aufgerufen werden koennen
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	19.02.2008
 */

#include "ct-Bot.h"
#ifdef PC
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "tcp-server.h"
#include "mini-fat.h"
#include "map.h"
#include "eeprom.h"
#include "tcp.h"
#include "command.h"

/*!
 * Zeigt Informationen zu den moeglichen Kommandozeilenargumenten an.
 */
static void usage(void) {
	puts("USAGE: ct-Bot [-t host] [-a address] [-T] [-h] [-s] [-M from] [-c FILE ID SIZE] [-e ADDR ID SIZE] [-d ID] [-i]");
	puts("\t-t\tHostname oder IP Adresse zu der Verbunden werden soll");
	puts("\t-a\tAdresse des Bots (fuer Bot-2-Bot-Kommunikation), default: 0");
	puts("\t-T\tTestClient");
	puts("\t-s\tServermodus");
	puts("\t-M from\tKonvertiert eine Bot-map in eine PGM-Datei");
	#ifndef MAP_AVAILABLE
		puts("\t\tACHTUNG, das Programm wurde ohne MAP_AVAILABLE übersetzt, die Option -M steht derzeit also NICHT zur Verfügung");
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
	puts("\t-i \tInitialisiert das EEPROM mit den Daten der EEP-Datei");
	puts("\t-h\tZeigt diese Hilfe an");
}

/*!
 * Behandelt die Kommandozeilen-Argumente
 * @param argc	Anzahl der Argumente
 * @param *argv	Zeiger auf String-Array der Argumente
 */
void hand_cmd_args(int argc, char * argv[]) {
	int ch;						/*!< Kommandozeilen-Parameter */
	char * from = NULL;			/*!< Speichert Argument zwischen */

	/* Der Zielhost wird per default durch das Macro IP definiert und
	 * tcp_hostname mit einer Kopie des Strings initialisiert. */
	tcp_hostname = malloc(strlen(IP) + 1);
	if (NULL == tcp_hostname) exit(1);
	strcpy(tcp_hostname, IP);

	/* Die Kommandozeilenargumente komplett verarbeiten */
	while ((ch = getopt(argc, argv, "hsTit:M:c:l:e:d:a:")) != -1) {
		argc -= optind;
		argv += optind;
		switch (ch) {

		case 's': {
			/* Servermodus [-s] wird verlangt */
			printf("ARGV[0]= %s\n", argv[0]);
			tcp_server_init();
			tcp_server_run(1000);	// beendet per exit()
		}

		case 'T': {
			/* Testclient starten */
			tcp_test_client_init();
			tcp_test_client_run(1000);	// beendet per exit()
		}

		case 't': {
			/* Hostname, auf dem ct-Sim laeuft, wurde uebergeben. Der String wird in hostname gesichert. */
			tcp_hostname = realloc(tcp_hostname, strlen(optarg) + 1);
			if (tcp_hostname == NULL) exit(1);
			strcpy(tcp_hostname, optarg);
			break;
		}

		case 'a': {
			/* Bot-Adresse wurde uebergeben */
			int addr = atoi(optarg);
			if ((addr >= CMD_SIM_ADDR) && (addr != CMD_BROADCAST)) {
				printf("Unzulaessige Bot-Adresse!\n");
				exit(1);
			}
			set_bot_address(addr);
			break;
		}

		case 'M': {
			/* Dateiname fuer die Map wurde uebergeben. Der String wird in from gesichert. */
			#ifndef MAP_AVAILABLE
				puts("ACHTUNG, das Programm wurde ohne MAP_AVAILABLE übersetzt, die Option -M steht derzeit also NICHT zur Verfügung.");
				puts("um dennoch Karten zu konvertieren, bitte im Quelltext in der Datei ct-Bot.h die Kommentarzeichen vor MAP_AVAILABLE entfernen");
				puts("und neu compilieren.");
				exit(1);
			#endif
			#ifdef MMC_VM_AVAILABLE
				printf("Executable wurde mit MMC_VM_AVAILABLE compiliert.\n");
				printf("Um Karten des echten Bots einlesen zu koennen, bitte den Code bitte ohne MMC_VM_AVAILABLE neu uebersetzen.\n");
				exit(1);
			#endif
			#ifdef MAP_AVAILABLE
				/* Karte in pgm konvertieren */
				int len = strlen(optarg);
				from = malloc(len + 1);
				if (NULL == from) exit(1);
				strcpy(from, optarg);
				printf("Konvertiere Karte %s in PGM %s\n",from,"map.pgm");
				map_read(from);
				map_to_pgm("map.pgm");
				exit(0);
			#endif	// MAP_AVAILABLE
		}

		case 'c': {
			/* Datei fuer den Bot (mini-fat) soll erzeugt werden. */
			int len = strlen(optarg);
			from = malloc(len + 1);
			if (NULL == from) exit(1);
			strcpy(from, optarg);
			printf("optind= %d argc=%d\n", optind, argc);
			if (argc != 2) {
				usage();
				exit(1);
			}
			char * id = malloc(strlen(argv[0]));
			strcpy(id, argv[0]);
			char * s = malloc(strlen(argv[1]));
			strcpy(s, argv[1]);
			int size = atoi(s);
			printf(
					"Mini-Fat-Datei (%s) mit %d kByte und ID=%s fuer den Bot soll erstellt werden.\n",
					from, size, id);
			create_mini_fat_file(from, id, size);
			exit(0);
		}

		case 'e': {
			/* Datei fuer den Sim (mini-fat) soll erzeugt werden. */
			int len = strlen(optarg);
			from = malloc(len + 1);
			if (NULL == from) exit(1);
			strcpy(from, optarg);
			printf("optind= %d argc=%d\n", optind, argc);
			if (argc != 2) {
				usage();
				exit(1);
			}

			char * id = malloc(strlen(argv[0]));
			strcpy(id, argv[0]);
			char * s = malloc(strlen(argv[1]));
			strcpy(s, argv[1]);
			int size = atoi(s);
			uint32_t addr = atoi(from);
			printf(
					"Mini-Fat-Datei mit ID=%s an Adresse 0x%x mit %d kByte auf der emulierten MMC soll erstellt werden.\n",
					id, addr, size);
			create_emu_mini_fat_file(addr, id, size);
			exit(0);
		}

		case 'd': {
			/* Datei fuer den Sim (mini-fat) soll geloescht werden. */
			int len = strlen(optarg);
			from = malloc(len + 1);
			if (NULL == from) exit(1);
			strcpy(from, optarg);
			printf("optind= %d argc=%d\n", optind, argc);
			if (argc != 0) {
				usage();
				exit(1);
			}

			printf(
					"Mini-Fat-Datei mit ID %s auf der emulierten MMC soll geloescht werden.\n",
					from);
			delete_emu_mini_fat_file(from);
			exit(0);
		}

		case 'l': {
			/* Speedlog-Datei soll in txt konvertiert werden */
			int len = strlen(optarg);
			from = malloc(len + 1);
			if (NULL == from) exit(1);
			strcpy(from, optarg);
			convert_slog_file(from);
			exit(0);
		}

		case 'i': {
			/* EEPROM-Init */
			printf("EEPROM soll mit den Daten einer eep-Datei initialisiert werden.\n");
			if (init_eeprom_man(1) != 0) {
				printf("Fehler bei EEPROM-Initialisierung!\n");
			} else {
				printf("done.\n");
			}
			exit(0);
		}

		case 'h':
		default:
			/* -h oder falscher Parameter, Usage anzeigen */
			usage();
			exit(1);
		}
	}
}
#endif	// PC
