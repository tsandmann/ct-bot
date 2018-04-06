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
#include "map.h"
#include "eeprom.h"
#include "command.h"
#include "tcp.h"
#include "bot-logic.h"
#include "sensor-low.h"
#include "uart.h"

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
	puts("USAGE: ct-Bot [-t host] [-a address] [-T] [-s] [-u RUNS] [-M FILE] [-m FILE] [-h]");
	puts("\t-t\tHostname oder IP Adresse zu der Verbunden werden soll");
	puts("\t-a\tAdresse des Bots (fuer Bot-2-Bot-Kommunikation), default: 0");
	puts("\t-T\tTestClient");
	puts("\t-s\tServermodus");
#ifdef ARM_LINUX_BOARD
	puts("\t-u RUNS\tUART-Test");
#endif
#ifdef MAP_AVAILABLE
	puts("\t-M FILE\tKonvertiert eine Bot-Map aus Datei FILE in eine PGM-Datei");
	puts("\t-m FILE\tGibt den Pfad zu einer Datei FILE an, die vom Map-Code verwendet wird (Ex- und Import)");
#else
	puts("\t\tACHTUNG, das Programm wurde ohne MAP_AVAILABLE uebersetzt, die Optionen -M / -m stehen derzeit also NICHT zur Verfuegung");
#endif
	puts("\t-h\tZeigt diese Hilfe an");
}

/**
 * Behandelt die Kommandozeilen-Argumente
 * \param argc	Anzahl der Argumente
 * \param *argv	Zeiger auf String-Array der Argumente
 */
void hand_cmd_args(int argc, char * argv[]) {	// explizit ** int **
	/* Der Zielhost wird per default durch das Macro IP definiert und
	 * tcp_hostname mit einer Kopie des Strings initialisiert. */
	tcp_hostname = malloc(strlen(IP) + 1);
	if (NULL == tcp_hostname) {
		exit(1);
	}
	strcpy(tcp_hostname, IP);

	int ch;	// explizit ** int **
	/* Die Kommandozeilenargumente komplett verarbeiten */
	while ((ch = getopt(argc, argv, "hsTu:Et:M:m:c:l:e:d:a:i:fk:o:F:")) != -1) {
		argc -= optind;
		argv += optind;

		switch (ch) {
		case 's': {
#ifdef BOT_2_SIM_AVAILABLE
			/* Servermodus [-s] wird verlangt */
			printf("ARGV[0]= %s\n", argv[0]);
			tcp_server_init();
			tcp_server_run(1000); // beendet per exit()
#else
			puts("Fehler, Binary wurde ohne BOT_2_SIM_AVAILABLE compiliert!");
			exit(1);
#endif // BOT_2_SIM_AVAILABLE
			break;
		}

		case 'T': {
#ifdef BOT_2_SIM_AVAILABLE
			/* Testclient starten */
			tcp_test_client_init();
			tcp_test_client_run(1000); // beendet per exit()
#else
			puts("Fehler, Binary wurde ohne BOT_2_SIM_AVAILABLE compiliert!");
			exit(1);
#endif // BOT_2_SIM_AVAILABLE
			break;
		}

		case 'u': {
#ifdef ARM_LINUX_BOARD
			long long int n = atoll(optarg);	// ** int ** da aus <cstdlib>
			uart_test((uint32_t) n);
#endif
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
			int addr = atoi(optarg);	// ** int ** da aus <cstdlib>
			if ((addr >= CMD_SIM_ADDR) && (addr != CMD_BROADCAST)) {
				puts("Unzulaessige Bot-Adresse!");
				exit(1);
			}
			set_bot_address(addr);
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
#ifdef MAP_AVAILABLE
			/* Karte in pgm konvertieren */
			const size_t len = strlen(optarg);
			if (! len) {
				puts("Dateiname ungueltig");
				usage();
				exit(1);
			}
			printf("Konvertiere Karte \"%s\" in PGM \"%s\"\n", optarg, "map.pgm");
			map_init();
			map_load_from_file(optarg);
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
			const size_t len = strlen(optarg);
			if (! len) {
				puts("Dateiname ungueltig");
				usage();
				exit(1);
			}
			printf("Lese Karte von \"%s\" ein\n", optarg);
			map_init();
			map_load_from_file(optarg);
#endif // MAP_AVAILABLE
			break;
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
			int16_t i = 0;
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
		int16_t i;
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
