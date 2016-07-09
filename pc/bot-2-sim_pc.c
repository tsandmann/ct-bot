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
 * \file 	bot-2-sim_pc.c
 * \brief 	Verbindung c't-Bot zu c't-Sim
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#ifdef PC

#include "ct-Bot.h"

#ifdef BOT_2_SIM_AVAILABLE
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
#include "bot-logic.h"
#include "motor.h"
#include "bot-2-bot.h"
#include "bot-2-atmega.h"
#include "log.h"
#include "led.h"


//#define DEBUG_BOT_2_SIM       // Schalter, um auf einmal alle Debugs an oder aus zu machen

#ifndef DEBUG_BOT_2_SIM
#undef LOG_DEBUG
#define LOG_DEBUG(...) {} /**< Log-Dummy */
#endif

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

#ifdef WIN32
/* These are winbase.h definitions, but to avoid including
 * tons of Windows related stuff, it is reprinted here */

typedef struct _FILETIME {
	unsigned long dwLowDateTime;
	unsigned long dwHighDateTime;
} FILETIME;

void __stdcall GetSystemTimeAsFileTime(FILETIME*);

void gettimeofday_win(struct timeval * p, void * tz /* IGNORED */) {
	(void) tz; // kein warning
	union {
		long long ns100; // time since 1 Jan 1601 in 100ns units
		FILETIME ft;
	} _now;

	GetSystemTimeAsFileTime(&(_now.ft));
	p->tv_usec = (long) ((_now.ns100 / 10LL) % 1000000LL);
	p->tv_sec = (long) ((_now.ns100 - (116444736000000000LL)) / 10000000LL);
	return;
}
#endif // WIN32

/**
 * Empfaengt alle Kommondos vom Sim
 */
void bot_2_sim_listen(void) {
	set_bot_2_sim();

#ifndef ARM_LINUX_BOARD
	while (receive_until_frame(CMD_DONE) != 0) {}
#else
	if (tcp_client_connected() && tcp_data_available() >= (int) sizeof(command_t)) {
		LOG_DEBUG("Data from Sim available");
		if (command_read() == 0) {
			LOG_DEBUG("Sim command read");
			command_evaluate();
		}
	}
#endif // ARM_LINUX_BOARD
}

/**
 * Setzt den aktiven Kommunikationskanal auf TCP/IP
 */
void set_bot_2_sim(void) {
	cmd_functions.write = tcp_write;
	cmd_functions.read = tcp_read;
	cmd_functions.crc_check = tcp_check_crc;
	cmd_functions.crc_calc = tcp_calc_crc;
}

/**
 * Initialisiert die Kommunikation mit dem Sim
 */
void bot_2_sim_init(void) {
	/* Bot beim Sim anmelden */
#ifdef ARM_LINUX_BOARD
	if (! tcp_client_connected()) {
		return;
	}
#endif // ARM_LINUX_BOARD
	register_bot();
	flushSendBuffer();

#ifndef ARM_LINUX_BOARD
	receive_until_frame(CMD_DONE);
	command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0);
#endif // ARM_LINUX_BOARD

#ifdef BOT_2_BOT_AVAILABLE
	receive_until_frame(CMD_DONE);
	/* hello (bot-)world! */
	if (get_bot_address() <= 127) {
		command_write_to(BOT_CMD_WELCOME, SUB_CMD_NORM, CMD_BROADCAST, 0, 0, 0);
	}
	command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0);
#endif // BOT_2_BOT_AVAILABLE
}

/**
 * Diese Funktion informiert den Sim ueber alle Sensor und Aktuator-Werte.
 * Dummy fuer PC-Code
 */
void bot_2_sim_inform(void) {
	if (! tcp_client_connected()) {
		return;
	}
#ifdef ARM_LINUX_BOARD
	cmd_func_t old_func = cmd_functions;
	set_bot_2_sim();

	command_write(CMD_AKT_MOT, SUB_CMD_NORM, speed_l, speed_r, 0);
	command_write(CMD_SENS_IR, SUB_CMD_NORM, sensDistL, sensDistR, 0);
	command_write(CMD_SENS_ENC, SUB_CMD_NORM, sensEncL, sensEncR, 0);
	command_write(CMD_SENS_BORDER, SUB_CMD_NORM, sensBorderL, sensBorderR, 0);
	command_write(CMD_SENS_LINE, SUB_CMD_NORM, sensLineL, sensLineR, 0);
	command_write(CMD_SENS_LDR, SUB_CMD_NORM , sensLDRL, sensLDRR, 0);
#ifdef BPS_AVAILABLE
	command_write(CMD_SENS_BPS, SUB_CMD_NORM , (int16_t) sensBPS, 0, 0);
#endif // BPS_AVAILABLE

#ifdef LED_AVAILABLE
	command_write(CMD_AKT_LED, SUB_CMD_NORM, led, 0, 0);
#endif
	command_write(CMD_SENS_TRANS, SUB_CMD_NORM, sensTrans, 0, 0);
	command_write(CMD_SENS_DOOR, SUB_CMD_NORM, sensDoor, 0, 0);
	command_write(CMD_SENS_ERROR, SUB_CMD_NORM, sensError, 0, 0);

#ifdef MOUSE_AVAILABLE
	command_write(CMD_SENS_MOUSE, SUB_CMD_NORM, sensMouseDX, sensMouseDY, 0);
#endif

	command_write(CMD_DONE, SUB_CMD_NORM, 0, 0, 0);
	cmd_functions = old_func;
#endif // ARM_LINUX_BOARD
}

#endif // BOT_2_SIM_AVAILABLE
#endif // PC
