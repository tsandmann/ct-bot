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
 * @file 	minilog.c
 * @brief 	Logging-Funktionen fuer MCU
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	01.09.2007
 */

#include "ct-Bot.h"
#include "log.h"
#include "command.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef LOG_AVAILABLE
#ifdef USE_MINILOG

#define LOG_BUFFER_SIZE	50		/*!< Groesse des Log-Buffers */

static const char line_str[]	PROGMEM = "[%5u] ";		/*!< Format-String fuer Zeilennummer */
static const char debug_str[]	PROGMEM	= "DEBUG\t";	/*!< Log-Typ DEBUG */
static const char info_str[]	PROGMEM	= "INFO \t";	/*!< Log-Typ INFO */
static const char error_str[]	PROGMEM	= "ERROR\t";	/*!< Log-Typ Error */
static PGM_P p_log_type;								/*!< Zeiger auf Log-Types im Flash */
static PGM_P log_types[] PROGMEM = {debug_str, info_str, error_str};	/*!< Log-Typ-Array im Flash */
static char minilog_buffer[LOG_BUFFER_SIZE];			/*!< Log-Puffer */

/*!
 * Schreibt die Zeilennummer und den Log-Typ in den Puffer
 * @param line		Zeilennummer
 * @param log_type	Log-Typ {DEBUG, INFO, ERROR}
 */
void minilog_begin(uint16_t line, LOG_TYPE log_type) {
	snprintf_P(minilog_buffer, 9, line_str, line);
	memcpy_P(&p_log_type, &log_types[log_type], sizeof(PGM_P));
	snprintf_P(minilog_buffer+8, 7, p_log_type);
}

/*!
 * Schreibt den Log-Text in den Log-Puffer und versendet die Daten
 * @param format	Format-String, wie bei printf
  * @param ... 		Variable Argumentenliste, wie beim printf
 */
void minilog_printf(const char * format, ...) {
	va_list	args;
	va_start(args, format);
	vsnprintf_P(minilog_buffer+14, LOG_BUFFER_SIZE-15, format, args);
	va_end(args);
	command_write_data(CMD_LOG, SUB_CMD_NORM, NULL, NULL, minilog_buffer);
}
#endif	// USE_MINILOG
#endif	// LOG_AVAILABLE
