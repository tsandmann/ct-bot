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
 * \file 	display_pc.c
 * \brief 	Routinen zur Displaysteuerung
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#ifdef PC
#include "ct-Bot.h"

#ifdef DISPLAY_AVAILABLE
#include "display.h"
#include "command.h"
#include "bot-2-sim.h"
#include "bot-2-atmega.h"
#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

uint8_t display_screen = 0; /**< zurzeit aktiver Displayscreen */

static char display_buf[DISPLAY_BUFFER_SIZE]; /**< Pufferstring fuer Displayausgaben */
static int16_t last_row = 0;
static int16_t last_column = 0;

#if defined ARM_LINUX_BOARD && defined ARM_LINUX_DISPLAY
static FILE* fp = NULL;
#endif

/**
 * Loescht das ganze Display
 */
void display_clear(void) {
#if defined ARM_LINUX_BOARD && defined ARM_LINUX_DISPLAY
	if (fp) {
		fprintf(fp, "\033[2J");
		fflush(fp);
	}
#endif // ARM_LINUX_BOARD && ARM_LINUX_DISPLAY

	command_write(CMD_AKT_LCD, SUB_LCD_CLEAR, 0, 0, 0);

#if defined ARM_LINUX_BOARD && defined BOT_2_SIM_AVAILABLE && defined DISPLAY_REMOTE_AVAILABLE
	set_bot_2_sim();
	command_write(CMD_AKT_LCD, SUB_LCD_CLEAR, 0, 0, 0);
	set_bot_2_atmega();
#endif // ARM_LINUX_BOARD && BOT_2_SIM_AVAILABLE && DISPLAY_REMOTE_AVAILABLE
}

/**
 * Positioniert den Cursor
 * \param row		Zeile
 * \param column	Spalte
 */
void display_cursor(int16_t row, int16_t column) {
#if defined ARM_LINUX_BOARD && defined ARM_LINUX_DISPLAY
	if (fp) {
		fprintf(fp, "\033[%d;%dH", row, column);
		fflush(fp);
	}
#endif // ARM_LINUX_BOARD && ARM_LINUX_DISPLAY

	last_row = row - 1;
	last_column = column - 1;
	command_write(CMD_AKT_LCD, SUB_LCD_CURSOR, last_column, last_row, 0);

#if defined ARM_LINUX_BOARD && defined BOT_2_SIM_AVAILABLE && defined DISPLAY_REMOTE_AVAILABLE
	set_bot_2_sim();
	command_write(CMD_AKT_LCD, SUB_LCD_CURSOR, last_column, last_row, 0);
	set_bot_2_atmega();
#endif // ARM_LINUX_BOARD && BOT_2_SIM_AVAILABLE && DISPLAY_REMOTE_AVAILABLE
}

/**
 * Initialisiert das Display
 */
void display_init(void) {
#if defined ARM_LINUX_BOARD && defined ARM_LINUX_DISPLAY
	if (strcmp("stdout", ARM_LINUX_DISPLAY) == 0) {
		fp = stdout;
	} else {
		fp = fopen(ARM_LINUX_DISPLAY, "r");
	}
	if (fp) {
		if (fp != stdout) {
			fclose(fp);
		}

		if (strcmp("stdout", ARM_LINUX_DISPLAY) == 0) {
			fp = stdout;
		} else {
			fp = fopen(ARM_LINUX_DISPLAY, "w");
		}
		if (! fp) {
			LOG_ERROR("display_init(): Konnte \"%s\" nicht oeffnen.", ARM_LINUX_DISPLAY);
		}
	} else {
		LOG_ERROR("display_init(): Konnte \"%s\" nicht oeffnen.", ARM_LINUX_DISPLAY);
	}
#endif // ARM_LINUX_BOARD && ARM_LINUX_DISPLAY

	display_clear();
}

/**
 * Schreibt einen String auf das Display.
 * \param *format	Format, wie beim printf
 * \param ... 		Variable Argumentenliste, wie beim printf
 * \return			Anzahl der geschriebenen Zeichen
 */
uint8_t display_printf(const char * format, ...) {
	va_list	args;

	va_start(args, format);
	uint8_t len = vsnprintf(display_buf, DISPLAY_BUFFER_SIZE, format, args);
	va_end(args);

	display_puts(display_buf);

	return len;
}

/**
 * Gibt einen String auf dem Display aus
 * \param *text	Zeiger auf den auszugebenden String
 * \return		Anzahl der geschriebenen Zeichen
 */
uint8_t display_puts(const char * text) {
	uint8_t len = strlen(text);
	if (len > DISPLAY_LENGTH) {
		len = DISPLAY_LENGTH;
	}
	command_write_rawdata(CMD_AKT_LCD, SUB_LCD_DATA, last_column, last_row, len, text);

#if defined ARM_LINUX_BOARD && defined ARM_LINUX_DISPLAY
	if (fp) {
		fprintf(fp, text);
		fflush(fp);
	}
#endif // ARM_LINUX_BOARD && ARM_LINUX_DISPLAY

#if defined ARM_LINUX_BOARD && defined BOT_2_SIM_AVAILABLE && defined DISPLAY_REMOTE_AVAILABLE
	set_bot_2_sim();
	command_write_rawdata(CMD_AKT_LCD, SUB_LCD_DATA, last_column, last_row, len, text);
	set_bot_2_atmega();
#endif // ARM_LINUX_BOARD && BOT_2_SIM_AVAILABLE && DISPLAY_REMOTE_AVAILABLE

	last_column += len;

	return len;
}
#endif // DISPLAY_AVAILABLE
#endif // PC
