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
 * @file 	display.h 
 * @brief 	Routinen zur Displaysteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.2005
 */

#ifndef display_H_
#define display_H_

#ifdef MCU
	#include <avr/pgmspace.h>
#else
	#define PROGMEM			/*!< Alibideklaration hat keine Funktion, verhindert aber eine Warning */
#endif

#define DISPLAY_LENGTH	20	/*!< Wieviele Zeichen passen in eine Zeile */

#define DISPLAY_SCREEN_TOGGLE	42	/*!< Screen-Nummer, die zum wechseln verwendet wird */
uint8 display_screen;				/*!< Welcher Screen soll gezeigt werden? */

/*! 
 * @brief	Initialisiert das Display
 */
void display_init(void);

/*!
 * @brief	Loescht das ganze Display
 */
void display_clear(void);

/*!
 * @brief			Positioniert den Cursor
 * @param row		Zeile
 * @param column	Spalte
 */
void display_cursor (uint8 row, uint8 column);

#ifdef PC
	/*!
	 * @brief			Schreibt einen String auf das Display.
	 * @param format 	Format, wie beim printf
	 * @param ... 		Variable Argumentenliste, wie beim printf
	 */
	void display_printf(char *format, ...);
#else
	/*!
	 * @brief			Schreibt einen String auf das Display, der im Flash gespeichert ist
	 * @param format 	Format, wie beim printf
	 * @param ... 		Variable Argumentenliste, wie beim printf
	 * Ganz genauso wie das "alte" display_printf(...) zu benutzen, das Makro 
	 * display_printf(format, args...) erledigt alles automatisch, damit der String
	 * im Flash verbleibt und erst zur Laufzeit temporaer (jeweils nur eine Zeile) geladen wird.
	 */
	void display_flash_printf(const char* format, ...);
	
	/*!
	 * @brief			Schreibt einen String auf das Display, der String verbleibt im Flash
	 * @param format	Format, wie beim printf
	 * @param args 		Variable Argumentenliste, wie beim printf
	 */
	#define display_printf(format, args...) {		\
		static const char data[] PROGMEM = format;	\
		display_flash_printf(data, ## args);		\
	}
#endif	// PC

#endif	// display_H_
