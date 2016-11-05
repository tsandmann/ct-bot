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
 * \file 	minilog.c
 * \brief 	vereinfachte Logging-Funktionen
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	01.09.2007
 */

#include "ct-Bot.h"

#if defined LOG_AVAILABLE && defined USE_MINILOG
#include "log.h"
#include "command.h"
#include "botfs.h"
#include "display.h"
#include "sensor.h"
#include "rc5-codes.h"
#include "init.h"
#include "uart.h"
#include "bot-2-sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define LOG_BUFFER_SIZE 65U /**< Groesse des Log-Puffers */

static const char line_str[]	PROGMEM = "[%5u] "; /**< Format-String fuer Zeilennummer */
static const char debug_str[]	PROGMEM	= "DEBUG "; /**< Log-Typ DEBUG */
static const char info_str[]	PROGMEM	= "INFO  "; /**< Log-Typ INFO */
static const char error_str[]	PROGMEM	= "ERROR "; /**< Log-Typ ERROR */
static PGM_P p_log_type;							/**< Zeiger auf Log-Types im Flash */
static PGM_P const log_types[] PROGMEM = {debug_str, info_str, error_str}; /**< Log-Typ-Array im Flash */
static char minilog_buffer[LOG_BUFFER_SIZE]; /**< Log-Puffer */

#ifdef LOG_MMC_AVAILABLE
#define LOG_FILE_NAME "log.txt"
#define LOG_SCROLLBACK 128U /**< Anzahl an Zeilen, die zurueck gescrollt werden kann */
static pFatFile log_file; /**< Log-Datei */
static uint16_t next_line; /**< naechste Zeilennummer */
static uint16_t line_displayed; /**< aktuell auf dem Display angezeigte Zeile */
static uint16_t line_cache[LOG_SCROLLBACK]; /**< Cache fuer Dateiposition pro Zeile */
static uint32_t file_pos_off; /**< letztes Offset in Log-Datei (obere 16 Bit) */

#if ! defined BOT_FS_AVAILABLE || (! defined SDFAT_AVAILABLE && defined MCU)
#error "LOG_MMC mit MINILOG geht nur mit BOT_FS_AVAILABLE"
#endif // BOT_FS_AVAILABLE
#endif // LOG_MMC_AVAILABLE

void minilog_begin(uint16_t line, LOG_TYPE log_type) {
	if (log_type != LOG_TYPE_RAW) {
		char* p_buffer = minilog_buffer + snprintf_P(minilog_buffer, 9, line_str, line);;
		memcpy_P(&p_log_type, &log_types[log_type], sizeof(PGM_P));
		strncpy_P(p_buffer, p_log_type, 7);
	} else {
		*minilog_buffer = 0;
	}
}

void minilog_printf(const char* format, ...) {
	const uint16_t n = strlen(minilog_buffer);
	char* p_buffer = minilog_buffer + n;
	va_list	args;
	va_start(args, format);
	p_buffer += vsnprintf_P(p_buffer, LOG_BUFFER_SIZE - n, format, args);
	va_end(args);
	if (p_buffer > &minilog_buffer[LOG_BUFFER_SIZE - 1]) {
		p_buffer = &minilog_buffer[LOG_BUFFER_SIZE - 1];
	}
	*p_buffer = 0;

#ifdef LOG_CTSIM_AVAILABLE
#ifdef ARM_LINUX_BOARD
	cmd_func_t old_func = cmd_functions;
	set_bot_2_sim();
#endif
	command_write_data(CMD_LOG, SUB_CMD_NORM, 0, 0, minilog_buffer);
#ifdef ARM_LINUX_BOARD
	cmd_functions = old_func;
#endif
#endif // LOG_CTSIM_AVAILABLE

#ifdef LOG_UART_AVAILABLE
	const uint8_t len = (uint8_t) strlen(minilog_buffer);
	uart_write(minilog_buffer, len);
	uart_write((uint8_t *) LINE_FEED, strlen(LINE_FEED));
#endif // LOG_UART_AVAILABLE

#ifdef LOG_RPI_AVAILABLE
	const uint8_t len = (uint8_t) strlen(minilog_buffer);
	command_write_rawdata_to(CMD_LOG, SUB_CMD_NORM, CMD_IGNORE_ADDR, 0, 0, len, minilog_buffer);
#endif // LOG_RPI_AVAILABLE

#ifdef LOG_MMC_AVAILABLE
	/* Zeilenende ergaenzen */
	*p_buffer = '\n';
	int32_t filepos = sdfat_tell(log_file);
	sdfat_write(log_file, minilog_buffer, (uint16_t) (p_buffer - minilog_buffer + 1));
	line_cache[next_line % LOG_SCROLLBACK] = (uint16_t) (filepos);
	file_pos_off = (uint32_t) filepos & 0xffff0000;

	/* auto scrolling */
	if (next_line == line_displayed + 1U) {
		++line_displayed;
	}

	++next_line;

	sdfat_flush(log_file);
#endif // LOG_MMC_AVAILABLE
}

#ifdef LOG_MMC_AVAILABLE
void log_mmc_init(void) {
	if (sdfat_open(LOG_FILE_NAME, &log_file, 0x1 | 0x2 | 0x10 | 0x40)) {
#ifdef PC
		printf("log_mmc_init(): sdfat_open(%s) failed.", LOG_FILE_NAME);
#endif
	}
}

void log_flush(void) {
	sdfat_flush(log_file);
}

#ifdef DISPLAY_AVAILABLE
/**
 * \brief Wertet die Tastenkommandos aus
 *
 * - Taste Pause: eine Zeile zurueck
 * - Taste Stopp: eine Zeile vor
 * - Taste >>: zum Ende
 * - Taste <<: zum Anfang
 */
static void keyhandler(void) {
	switch (RC5_Code) {
#ifdef RC5_CODE_UP
	/* zurueck scrollen */
	case RC5_CODE_UP:
		if (line_displayed > 0) {
			--line_displayed;
		}
		RC5_Code = 0;
		break;
#endif // RC5_CODE_UP

#ifdef RC5_CODE_DOWN
	/* vor scrollen */
	case RC5_CODE_DOWN:
		if (next_line != 0 && line_displayed < next_line - 1) {
			++line_displayed;
		}
		RC5_Code = 0;
		break;
#endif // RC5_CODE_DOWN

#ifdef RC5_CODE_LEFT
	/* zum Anfang scrollen */
	case RC5_CODE_LEFT:
		line_displayed = (next_line - 1) & ~(LOG_SCROLLBACK - 1U);
		RC5_Code = 0;
		break;
#endif // RC5_CODE_LEFT

#ifdef RC5_CODE_RIGHT
	/* zum Ende scrollen */
	case RC5_CODE_RIGHT:
		line_displayed = next_line - 1;
		RC5_Code = 0;
		break;
#endif // RC5_CODE_RIGHT
	}
}

void log_mmc_display(void) {
	static uint16_t last_line = 0xffff;

	/* Tasten auswerten */
	keyhandler();

	if (next_line == 0) {
		display_cursor(1, 1);
		display_puts("LOG leer");
		return;
	}

	if (line_displayed != last_line) {
		/* anzuzeigende Zeile in Puffer laden */
		int32_t file_pos = sdfat_tell(log_file);
		sdfat_seek(log_file, (int32_t) (line_cache[(line_displayed % LOG_SCROLLBACK)] | file_pos_off), SEEK_SET);
		int16_t res = sdfat_read(log_file, minilog_buffer, LOG_BUFFER_SIZE);
		sdfat_seek(log_file, file_pos, SEEK_SET);
		if (res < 1) {
			printf("sdfat_read() failed: %d\n", res);
			return;
		}

		/* new-line weg */
		char* ptr = minilog_buffer;
		char* nl = strchr(ptr + 14, '\n');
		if (nl != NULL) {
			*nl = 0;
		} else {
			minilog_buffer[LOG_BUFFER_SIZE - 1] = 0;
		}

		/* Zeilen ausgeben */
		display_clear();
		uint8_t i;
		for (i = 1; i <= 4 && ptr < nl; ++i) {
			display_cursor(i, 1);
			display_printf("%-20s", ptr);
			ptr += 20;
		}
	}

	/* aktuelle Zeile / Anzahl Zeilen anzeigen */
	if (next_line != 0) {
		display_cursor(4, 10);
		display_printf("%5u/%5u", line_displayed + 1, next_line);
	}

	last_line = line_displayed;
}
#endif // DISPLAY_AVAILABLE
#endif // LOG_MMC_AVAILABLE

#endif // LOG_AVAILABLE && USE_MINILOG
