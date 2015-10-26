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
 * @file 	bot-2-sim_pc.c
 * @brief 	Verbindung c't-Bot zu c't-Sim
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.2005
 */

#ifdef PC

#include "ct-Bot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "bot-2-sim.h"
#include "eeprom.h"
#include "command.h"
#include "tcp.h"
#include "display.h"
#include "sensor.h"
#include "bot-logic/bot-logic.h"
#include "motor.h"
#include "bot-2-bot.h"


/* Linux with glibc:
 * _REENTRANT to grab thread-safe libraries
 */
#ifdef __linux__
#define _REENTRANT
#endif

/* Hack for LinuxThreads */
#ifdef __linux__
#define _P __P
#endif

#define low_init tcp_init	/*!< Low-Funktion zum Initialisieren */

#ifdef WIN32
/* These are winbase.h definitions, but to avoid including
 * tons of Windows related stuff, it is reprinted here */

typedef struct _FILETIME {
	unsigned long dwLowDateTime;
	unsigned long dwHighDateTime;
} FILETIME;

void __stdcall GetSystemTimeAsFileTime(FILETIME*);

void gettimeofday_win(struct timeval * p, void * tz /* IGNORED */) {
	tz = tz; // kein warning
	union {
		long long ns100; // time since 1 Jan 1601 in 100ns units
		FILETIME ft;
	} _now;

	GetSystemTimeAsFileTime(&(_now.ft));
	p->tv_usec = (long) ((_now.ns100 / 10LL) % 1000000LL);
	p->tv_sec = (long) ((_now.ns100 - (116444736000000000LL)) / 10000000LL);
	return;
}
#endif	// WIN32

/*!
 * Schleife, die Kommandos empfaengt und bearbeitet, bis ein Kommando vom Typ frame kommt
 * @param frame	Kommando zum Abbruch
 * @return		Fehlercode
 */
int8_t receive_until_Frame(uint8_t frame) {
	int8_t result;
	for (;;) {
		result = command_read();
		if (result != 0) {
			/* Fehler werden in command_read() ausgegeben
			 * -1 kommt auch, wenn das Paket nicht fuer unsere Adresse war */
			return result;
		} else {
			command_evaluate();
		}

		if (received_command.request.command == frame) {
			return 0;
		}
	}

	return 0;
}

/*!
 * Empfaengt alle Kommondos vom Sim
 */
void bot_2_sim_listen(void) {
	while (receive_until_Frame(CMD_DONE) != 0) {}
}

/*!
 * Ein wenig Initialisierung kann nicht schaden
 */
void bot_2_sim_init(void) {
	low_init();

	command_init();
	flushSendBuffer();

	receive_until_Frame(CMD_DONE);
	command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0);
#ifdef BOT_2_BOT_AVAILABLE
	receive_until_Frame(CMD_DONE);
	/* hello (bot-)world! */
	if (get_bot_address() <= 127) {
		command_write_to(BOT_CMD_WELCOME, SUB_CMD_NORM, CMD_BROADCAST, 0, 0, 0);
	}
	command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0);
#endif // BOT_2_BOT_AVAILABLE
}

#endif // PC
