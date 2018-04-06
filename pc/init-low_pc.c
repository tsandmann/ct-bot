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
 * \file 	init-low_pc.c
 * \brief 	Initialisierungsroutinen fuer PC
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	09.03.2010
 */

#ifdef PC
#include "ct-Bot.h"
#include "init.h"
#include "log.h"
#include "cmd_tools.h"
#include "trace.h"
#include "command.h"
#include "bot-2-atmega.h"
#include "uart.h"
#include "botcontrol.h"
#include "delay.h"
#include "sensor.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef __WIN32__
#include <signal.h>

void sig_handler(int signum);	// ** int ** da SIGINT aus <csignal>

void sig_handler(int signum) {	// ** int ** da SIGINT aus <csignal>
	if (signum == SIGINT) {
		puts("\nCTRL+C pressed, initiating Shutdown...");
		ctbot_shutdown();
	}
}
#endif // ! __WIN32__


//#define DEBUG_INIT_LOW_PC       // Schalter, um auf einmal alle Debugs an oder aus zu machen

#ifndef DEBUG_INIT_LOW_PC
#undef LOG_DEBUG
#define LOG_DEBUG(...) {} /**< Log-Dummy */
#endif

/**
 * Hardwareabhaengige Initialisierungen, die zuerst ausgefuehrt werden sollen
 * \param argc Anzahl der Kommandozeilenparameter
 * \param *argv Zeiger auf Kommandozeilenparameter
 */
void ctbot_init_low_1st(int argc, char * argv[]) {
	/* Kommandozeilen-Argumente auswerten */
	hand_cmd_args(argc, argv);

	printf("c't-Bot\n");

#ifdef CREATE_TRACEFILE_AVAILABLE
	trace_init();
#endif // CREATE_TRACEFILE_AVAILABLE
}

/**
 * Hardwareabhaengige Initialisierungen, die _nach_ der allgemeinen Initialisierung
 * ausgefuehrt werden sollen
 */
void ctbot_init_low_last(void) {
#ifndef __WIN32__
	signal(SIGINT, sig_handler);
#endif // ! __WIN32__

	cmd_init();

#ifdef ARM_LINUX_BOARD
	set_bot_2_atmega();
	LOG_DEBUG("ctbot_init_low_last(): Sending CMD_DONE to ATmega...");
	command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0);
#endif // ARM_LINUX_BOARD
}

/**
 * Beendet die Bot-Instanz
 */
void ctbot_shutdown_low(void) {
#ifdef ARM_LINUX_BOARD
	set_bot_2_atmega();
	command_write(CMD_SHUTDOWN, SUB_CMD_NORM, 0, 0, 0);
	delay(300);
	uart_flush();
	uart_close();
#endif // ARM_LINUX_BOARD
	puts("c't-Bot shutdown. So long, and thanks for all the fish.");
	exit(0);
}
#endif // PC
