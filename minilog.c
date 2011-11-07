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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define LOG_BUFFER_SIZE	64 /**< Groesse des Log-Puffers */

static const char line_str[]	PROGMEM = "[%5u] "; /**< Format-String fuer Zeilennummer */
static const char debug_str[]	PROGMEM	= "DEBUG "; /**< Log-Typ DEBUG */
static const char info_str[]	PROGMEM	= "INFO  "; /**< Log-Typ INFO */
static const char error_str[]	PROGMEM	= "ERROR "; /**< Log-Typ ERROR */
static PGM_P p_log_type;							/**< Zeiger auf Log-Types im Flash */
static PGM_P log_types[] PROGMEM = {debug_str, info_str, error_str}; /**< Log-Typ-Array im Flash */
#ifndef LOG_MMC_AVAILABLE
static char minilog_buffer[LOG_BUFFER_SIZE]; /**< Log-Puffer */
static char * p_buffer = minilog_buffer; /**< Zeiger auf Puffer */
#endif // LOG_MMC_AVAILABLE

#ifdef LOG_MMC_AVAILABLE
#define FILE_NAME "/log.txt"
#define FILE_SIZE (1024 * (1024 / BOTFS_BLOCK_SIZE)) /**< Groesse der Log-Datei in Bloecken */
static botfs_file_descr_t log_file; /**< Log-Datei */
static uint16_t next_line; /**< naechste Zeilennummer */
static uint16_t line_displayed; /**< aktuell auf dem Display angezeigte Zeile */
static char * const file_buffer = GET_MMC_BUFFER(minilog_buffer); /**< Puffer fuer BotFS */
static char * p_buffer = GET_MMC_BUFFER(minilog_buffer); /**< Zeiger auf Puffer */

#ifndef BOT_FS_AVAILABLE
#error "LOG_MMC mit MINILOG geht nur mit BOT_FS_AVAILABLE"
#endif // BOT_FS_AVAILABLE
#endif // LOG_MMC_AVAILABLE


/**
 * Schreibt die Zeilennummer und den Log-Typ in den Puffer
 * \param line		Zeilennummer
 * \param log_type	Log-Typ {DEBUG, INFO, ERROR}
 */
void minilog_begin(uint16_t line, LOG_TYPE log_type) {
	if (log_type != LOG_TYPE_RAW) {
		snprintf_P(p_buffer, 9, line_str, line);
		p_buffer += 8;
		memcpy_P(&p_log_type, &log_types[log_type], sizeof(PGM_P));
		strncpy_P(p_buffer, p_log_type, 7);
		p_buffer += 6;
	}
}

/**
 * Schreibt den Log-Text in den Log-Puffer und versendet / speichert die Daten
 * \param format	Format-String, wie bei printf
 * \param ... 		Variable Argumentenliste, wie bei printf
 */
void minilog_printf(const char * format, ...) {
	va_list	args;
	va_start(args, format);
#ifdef LOG_MMC_AVAILABLE
	p_buffer +=
#endif
	vsnprintf_P(p_buffer, LOG_BUFFER_SIZE - 15, format, args);
	va_end(args);

#ifdef LOG_CTSIM_AVAILABLE
	command_write_data(CMD_LOG, SUB_CMD_NORM, 0, 0, minilog_buffer);
#endif

#ifdef LOG_MMC_AVAILABLE
	/* Zeilenende und 0-Terminierung ergaenzen */
	*p_buffer = '\n';
	++p_buffer;
	/* Puffer-Zeiger fuer naechsten Eintrag weiter setzen */
	const size_t to_end = (size_t) (LOG_BUFFER_SIZE - ((p_buffer - file_buffer) & (LOG_BUFFER_SIZE - 1)));
	memset(p_buffer, 0, to_end);
	p_buffer += to_end;
	if (p_buffer >= &file_buffer[BOTFS_BLOCK_SIZE]) {
		/* Puffer voll -> rausschreiben */
		botfs_write(&log_file, file_buffer);
		p_buffer = file_buffer;
	}
	/* auto scrolling */
	if (next_line == line_displayed + 1) {
		++line_displayed;
	}

	++next_line;
#else
	p_buffer = minilog_buffer;
#endif // LOG_MMC_AVAILABLE
}

#ifdef LOG_MMC_AVAILABLE
/**
 * Initialisierung fuer MMC-Logging
 */
void log_mmc_init(void) {
	int8_t res;
	if ((res = botfs_open(FILE_NAME, &log_file, BOTFS_MODE_W, file_buffer)) != 0) {
		if (res == -1) {
			botfs_create(FILE_NAME, FILE_SIZE, 0, file_buffer);
			if (botfs_open(FILE_NAME, &log_file, BOTFS_MODE_W, file_buffer) != 0) {
				return;
			}
		}
	}
}

/**
 * Schreibt den aktuellen Inhalt des Log-Puffers auf die MMC
 */
void log_flush(void) {
	if (p_buffer == file_buffer) {
		/* Puffer leer */
		return;
	}
	const size_t to_end = (size_t) (&file_buffer[BOTFS_BLOCK_SIZE] - p_buffer);
	memset(p_buffer, 0, to_end);
	botfs_write(&log_file, file_buffer);
	botfs_flush_used_blocks(&log_file, file_buffer);
	botfs_seek(&log_file, -1, SEEK_CUR);
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
		line_displayed = 0;
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

/**
 * Display-Handler fuer MMC-LOG
 */
void log_mmc_display(void) {
	if (next_line == 0) {
		display_cursor(1, 1);
		display_puts("LOG leer");
		return;
	}

	/* Tasten auswerten */
	keyhandler();

	uint16_t file_pos = 0xffff;
	/* anzuzeigende Zeile derzeit nicht im Puffer? */
	if ((next_line - 1) / (BOTFS_BLOCK_SIZE / LOG_BUFFER_SIZE) != (line_displayed + 0) / (BOTFS_BLOCK_SIZE / LOG_BUFFER_SIZE)) {
		/* Log-Puffer zurueckschreiben */
		log_flush();
		file_pos = log_file.pos;

		/* anzuzeigende Zeile in Puffer laden */
		botfs_seek(&log_file, (int16_t) (line_displayed / (BOTFS_BLOCK_SIZE / LOG_BUFFER_SIZE)), SEEK_SET);
		if (botfs_read(&log_file, file_buffer) != 0) {
			return;
		}
	}
	char * ptr = file_buffer;

	/* Offset im Puffer berechnen */
	ptr += (line_displayed % (BOTFS_BLOCK_SIZE / LOG_BUFFER_SIZE)) * LOG_BUFFER_SIZE;
	const char tmp = ptr[13];
	if (tmp == ' ') {
		/* erste Zeile hinter Log-Typ abbrechen */
		ptr[13] = 0;
	}

	/* new-line weg */
	char * nl = strchr(ptr + 14, '\n');
	if (nl != NULL) {
		*nl = 0;
	}

	/* Zeile 1 (Line und Typ) */
	display_cursor(1, 1);
	display_printf("%-20s:", ptr);
	ptr[13] = tmp;
	ptr += 14;

	/* Zeile 2 bis 4 */
	uint8_t i;
	for (i = 2; i <= 4; ++i) {
		display_cursor(i, 1);
		display_printf("%-20s", ptr);
		ptr += 20;
	}
	if (nl != NULL) {
		*nl = '\n';
	}

	/* aktuelle Zeile / Anzahl Zeilen anzeigen */
	if (next_line != 0) {
		display_cursor(4, 10);
		display_printf("%5u/%5u", line_displayed + 1, next_line);
	}

	if (file_pos != 0xffff) {
		/* Puffer wiederherstellen */
		log_file.pos = file_pos;
		botfs_read(&log_file, file_buffer);
		botfs_seek(&log_file, -1, SEEK_CUR);
	}
}
#endif // DISPLAY_AVAILABLE
#endif // LOG_MMC_AVAILABLE

#endif // LOG_AVAILABLE && USE_MINILOG
