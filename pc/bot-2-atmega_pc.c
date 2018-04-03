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
 * \file 	bot-2-atmega_pc.c
 * \brief 	Verbindung ARM-Board zu ATmega
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	04.05.2013
 */

#ifdef PC
#define _POSIX_C_SOURCE 200809L // nanosleep

#include "ct-Bot.h"

#ifdef ARM_LINUX_BOARD
#include "bot-2-atmega.h"
#include "bot-2-sim.h"
#include "command.h"
#include "uart.h"
#include "log.h"
#include "sensor.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>

//#define DEBUG_BOT_2_ATMEGA // Schalter, um auf einmal alle Debugs an oder aus zu machen

#define UART_TIMEOUT 20 /**< Timeout fuer UART-Kommunikation in ms */

#ifndef DEBUG_BOT_2_ATMEGA
#undef LOG_DEBUG
#define LOG_DEBUG(...) {} /**< Log-Dummy */
#endif

static struct timeval atmega_last_receive; /**< Zeitpunkt des letzten Datenempfangs vom ATmega zu Debugging-Zwecken */


/**
 * Fuehrt einen Reset auf dem ATmega aus
 * \return Fehlercode: 0 wenn alles OK
 */
int8_t atmega_reset(void) {
#ifdef BOT_RESET_GPIO
	uart_close();
	if (uart_init_baud(UART_LINUX_PORT, B115200) != 0) {
		LOG_ERROR("atmega_reset(): Konnte UART nicht initialisieren");
		return -1;
	}
	uart_flush();

	/* perform reset on ATmega */
	int8_t fd = open(BOT_RESET_GPIO, O_WRONLY | O_SYNC);
	if (fd == -1) {
		LOG_ERROR("Unable to open Reset-GPIO \"%s\"", BOT_RESET_GPIO);
		uart_close();
		return -2;
	}

	if (write(fd, "1", 1) != 1) {
		LOG_ERROR("Unable to write on Reset-GPIO \"%s\"", BOT_RESET_GPIO);
		close(fd);
		uart_close();
		return -3;
	}
	fsync(fd);

    struct timespec s;
    s.tv_sec = 0;
    s.tv_nsec = 500000000L;
    if (nanosleep(&s, NULL)) {
    	LOG_ERROR("nanosleed() failed: \"%s\"", strerror(errno));
    	close(fd);
    	uart_close();
    	return -4;
    }

	if (write(fd, "0", 1) != 1) {
		LOG_ERROR("Unable to write on Reset-GPIO \"%s\"", BOT_RESET_GPIO);
		close(fd);
		uart_close();
		return -5;
	}
	fsync(fd);
	close(fd);

	s.tv_nsec = 500000000L;
	if (nanosleep(&s, NULL)) {
		LOG_ERROR("nanosleed() failed: \"%s\"", strerror(errno));
    	uart_close();
    	return -6;
	}

	/* exit bootloader on ATmega immediately */
	const char buf[] = "SE";
	uart_write(buf, sizeof(buf) - 1);
	uart_flush();
	s.tv_nsec = 200000000L;
	if (nanosleep(&s, NULL)) {
		LOG_ERROR("nanosleed() failed: \"%s\"", strerror(errno));
	}
	uart_close();
#endif // BOT_RESET_GPIO

	return 0;
}

/**
 * Initialisiert die Verbindung
 * \return Fehlercode: 0 korrekt initialisiert
 */
int8_t bot_2_atmega_init(void) {
	LOG_DEBUG("bot_2_atmega_init(): Fuehre ATmega-Reset aus.");
	if (atmega_reset() != 0) {
		LOG_ERROR("bot_2_atmega_init(): Fehler beim ATmega-Reset");
	}

	LOG_DEBUG("bot_2_atmega_init(): Initialisiere UART %s mit %d Baud", UART_LINUX_PORT, UART_BAUD);
	if (uart_init(UART_LINUX_PORT) != 0) {
		LOG_ERROR("bot_2_atmega_init(): Konnte UART nicht initialisieren");
		return -1;
	}

	gettimeofday(&atmega_last_receive, NULL);

	return 0;
}

/**
 * Setzt den aktiven Kommunikationskanal auf UART
 */
void set_bot_2_atmega(void) {
	cmd_functions.write = uart_write;
	cmd_functions.read = uart_read;
	cmd_functions.crc_check = uart_check_crc;
	cmd_functions.crc_calc = uart_calc_crc;
}

/**
 * Empfaengt alle Daten vom ATmega
 */
void bot_2_atmega_listen(void) {
	set_bot_2_atmega();
	LOG_DEBUG("bot_2_atmega_listen(): Receiving until CMD_DONE from ATmega...");

	struct timeval start, now;
	gettimeofday(&start, NULL);

	while (receive_until_frame(CMD_DONE) != 0) {
		gettimeofday(&now, NULL);
		const uint64_t t = (now.tv_sec - start.tv_sec) * 1000000UL + now.tv_usec - start.tv_usec;
		if (t > UART_TIMEOUT * 1000UL) {
			LOG_ERROR("bot_2_atmega_listen(): Timeout, t = %llu us", t);
			break;
		}
	}
//	gettimeofday(&now, NULL);
//	const uint64_t t = (now.tv_sec - start.tv_sec) * 1000000UL + now.tv_usec - start.tv_usec;
//	LOG_INFO("bot_2_atmega_listen(): receive took %llu us", t);
	gettimeofday(&atmega_last_receive, NULL);
//	uint64_t t = (atmega_last_receive.tv_sec - start.tv_sec) * 1000000UL + atmega_last_receive.tv_usec - start.tv_usec;
//	LOG_INFO("bot_2_atmega_listen(): t = %llu us", t);
	LOG_DEBUG("bot_2_atmega_listen() done.");
}

#ifdef DISPLAY_AVAILABLE
/**
 * Display Screen fuer Inhalte vom ATmega
 */
void atmega_display(void) {
	// leer, wird von command_evaluate() gefuellt
}
#endif // DISPLAY_AVAILABLE

#endif // ARM_LINUX_BOARD
#endif // PC
