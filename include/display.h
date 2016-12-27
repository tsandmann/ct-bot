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
 * \file 	display.h
 * \brief 	Routinen zur Displaysteuerung
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#ifdef MCU
#include <avr/pgmspace.h>
#endif

#define DISPLAY_LENGTH 20 /**< Wieviele Zeichen passen in eine Zeile */
#define DISPLAY_BUFFER_SIZE	(DISPLAY_LENGTH + 1) /**< Puffergroesse fuer eine Zeile [Byte] */

#define DISPLAY_SCREEN_TOGGLE 42 /**< Screen-Nummer, die zum Wechseln verwendet wird */
extern uint8_t display_screen;   /**< Welcher Screen soll gezeigt werden? */

/**
 * Initialisiert das Display
 */
void display_init(void);

/**
 * Loescht das ganze Display
 */
void display_clear(void);

/**
 * Positioniert den Cursor
 * \param row		Zeile
 * \param column	Spalte
 */
void display_cursor(int16_t row, int16_t column);

#ifdef DISPLAY_MCU_AVAILABLE
/**
 * Schreibt ein Zeichen auf das Display
 * \param data 	Das Zeichen
 */
void display_data(const char data);
#endif // #ifdef DISPLAY_MCU_AVAILABLE

#ifdef PC
/**
 * Schreibt einen String auf das Display.
 * \param *format 	Format, wie beim printf
 * \param ... 		Variable Argumentenliste, wie beim printf
 * \return			Anzahl der geschriebenen Zeichen
 */
uint8_t display_printf(const char * format, ...);

/**
 * Gibt einen String auf dem Display aus
 * \param *text	Zeiger auf den auszugebenden String
 * \return		Anzahl der geschriebenen Zeichen
 */
uint8_t display_puts(const char * text);
#else // MCU
/**
 * Schreibt einen String auf das Display, der im Flash gespeichert ist
 * \param *format 	Format, wie beim printf, Zeiger aber auf String im Flash
 * \param ... 		Variable Argumentenliste, wie beim printf
 * \return			Anzahl der geschriebenen Zeichen
 * Ganz genauso wie das "alte" display_printf(...) zu benutzen, das Makro
 * display_printf(format, args...) erledigt alles automatisch, damit der String
 * im Flash verbleibt und erst zur Laufzeit temporaer (jeweils nur eine Zeile) geladen wird.
 */
uint8_t display_flash_printf(const char * format, ...);

/**
 * Schreibt einen String auf das Display, der String verbleibt im Flash
 * \param _format	Format, wie beim printf
 * \param ... 		Variable Argumentenliste, wie beim printf
 * \return			Anzahl der geschriebenen Zeichen
 */
#define display_printf(_format, ...) display_flash_printf(PSTR(_format), ## __VA_ARGS__)

/**
 * Gibt einen String (im Flash) auf dem Display aus
 * \param *text	Zeiger auf den auszugebenden String
 * \return 		Anzahl der geschriebenen Zeichen
 */
uint8_t display_flash_puts(const char * text);

/**
 * Gibt einen String auf dem Display aus, der String verbleibt im Flash
 * \param *text	Zeiger auf den auszugebenden String
 * \return		Anzahl der geschriebenen Zeichen
 */
#define display_puts(_text) display_flash_puts(PSTR(_text))
#endif // PC

#endif // DISPLAY_H_
