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
 * \file 	display.c
 * \brief 	Routinen zur Displaysteuerung
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.05
 */

#ifdef MCU

#include "ct-Bot.h"
#ifdef DISPLAY_AVAILABLE

#include <avr/io.h>
#include <stdio.h>
#include "command.h"

#include "display.h"
#include "delay.h"
#include "shift.h"
#include "os_thread.h"

uint8_t display_screen = 0;	/**< zurzeit aktiver Displayscreen */

#define DISPLAY_CLEAR 0x01		/**< Kommando zum Loeschen */
#define DISPLAY_CURSORHOME 0x02	/**< Kommando fuer den Cursor */
#define DISPLAY_MODE 0x0E		/**< Display on, Cursor on, Blink off */
//#define DISPLAY_MODE 0x0F		/**< Display on, Cursor on, Blink on */

#define DISPLAY_OUT 			0x07		/**< Output-Pins Display */
#define DISPLAY_IN 				(1<<5)		/**< Input-Pins Display */

#define DISPLAY_PORT			PORTC		/**< Port an dem das Display haengt */
#define DISPLAY_DDR				DDRC		/**< Port an dem das Display haengt */
#define DPC (uint8_t) (DISPLAY_PORT & ~DISPLAY_OUT)	/**< Port des Displays */
//#define DRC (DDRC & ~DISPLAY_PINS)

//#define DISPLAY_READY_PINR	PINC		/**< Port an dem das Ready-Flag des Display haengt */
#define DISPLAY_READY_DDR		DDRC		/**< Port an dem das Ready-Flag des Display haengt */
#define DISPLAY_READY_PIN		(1<<5)		/**< Pin  an dem das Ready-Flag des Display haengt */

/**
 * RS-Leitung
 * legt fest, ob die Daten an das Display in den Textpuffer (RS=1) kommen
 * oder als Steuercode interpretiert werden (RS=0)
 */
#define DISPLAY_RS				(1<<0)

/**
 * RW-Leitung
 * legt fest, ob zum Display geschrieben wird (RW=0)
 * oder davon gelesen wird (RW=1)
 */
#define DISPLAY_RW				(1<<1)

/**
 * Enable Leitung
 * schaltet das Interface ein (E=1).
 * Nur wenn Enable auf High-Pegel liegt, laesst sich das Display ansprechen
 */
#define DISPLAY_EN				(1<<2)

/*
 * Im Moment der Low-High-Flanke von ENABLE liest das Dislplay
 * die Werte von RS und R/W ein. Ist zu diesem Zeitpunkt R/W=0,
 * dann liest das Display mit der folgenden High-Low-Flanke von ENABLE
 * den Datenbus ein (Schreibzyklus).
 * War aber R/W=1, dann legt das Display ein Datenword auf den
 * Datenbus (Lese-Zyklus), solange bis die High-Low-Flanke von ENABLE
 * das Interface wieder deaktiviert.
 */

#ifdef DISPLAY_REMOTE_AVAILABLE
static uint8_t remote_column = 0; /**< Spalte der aktuellen Cursorposition fuer Remote-LCD */
static uint8_t remote_row = 0;    /**< Zeile der aktuellen Cursorposition fuer Remote-LCD */
#endif

#ifdef DISPLAY_MCU_AVAILABLE
/**
 * Uebertraegt ein Kommando an das Display
 * \param cmd Das Kommando
 */
static void display_cmd(uint8_t cmd) {
	/* Kommando cmd an das Display senden */
	shift_data_out(cmd, SHIFT_LATCH, SHIFT_REGISTER_DISPLAY);

	/* 47 us warten */
	delay_us(47);
	DISPLAY_PORT = DPC;	// Alles zurueck setzen ==> Fallende Flanke von Enable

	if (cmd == DISPLAY_CLEAR) {
		/* 1.52 ms warten */
		delay_us(1520);
	}
}

/**
 * Schreibt ein Zeichen auf das Display
 * \param data Das Zeichen
 */
void display_data(const char data) {
	/* Zeichen aus data in den Displayspeicher schreiben */
	shift_data_out((uint8_t) data, SHIFT_LATCH, SHIFT_REGISTER_DISPLAY | DISPLAY_RS);

	/* 47 us warten */
	delay_us(47);
	DISPLAY_PORT = DPC;	// Alles zurueck setzen ==> Fallende Flanke von Enable
}
#endif // DISPLAY_MCU_AVAILABLE

/**
 * Initialisiert das Display
 */
void display_init(void) {
#ifdef DISPLAY_MCU_AVAILABLE
	DISPLAY_DDR |= DISPLAY_OUT; // Ausgaenge
	DISPLAY_DDR = (uint8_t) (DISPLAY_DDR & ~DISPLAY_IN); // Eingaenge

	delay(12); // Display steht erst 10ms nach dem Booten bereit

	/* Register in 8-Bit-Modus 3x uebertragen, dazwischen warten */
	uint8_t i;
	for (i = 3; i > 0; --i) {
		shift_data_out(0x38, SHIFT_LATCH, SHIFT_REGISTER_DISPLAY);
		DISPLAY_PORT = DPC;
		delay(5);
	}

	display_cmd(DISPLAY_MODE); // Display an und Cursor-Modus setzen
	display_cmd(DISPLAY_CLEAR); // Display loeschen, Cursor Home
#endif // DISPLAY_MCU_AVAILABLE
}

/**
 * \brief	Loescht das ganze Display
 */
void display_clear(void) {
#ifdef DISPLAY_MCU_AVAILABLE
	display_cmd(DISPLAY_CLEAR); // Display loeschen, Cursor Home
#endif
#ifdef DISPLAY_REMOTE_AVAILABLE
#ifndef BOT_2_RPI_AVAILABLE
	command_write(CMD_AKT_LCD, SUB_LCD_CLEAR, 0, 0, 0);
#else
	command_write_to(CMD_AKT_LCD, SUB_LCD_CLEAR, CMD_IGNORE_ADDR, 0, 0, 0);
#endif // BOT_2_RPI_AVAILABLE
#endif
}

/**
 * Positioniert den Cursor
 * \param row		Zeile
 * \param column	Spalte
 */
void display_cursor(int16_t row, int16_t column) {
	const uint8_t r = (uint8_t) (row - 1);
	const uint8_t c = (uint8_t) (column - 1);

#ifdef DISPLAY_MCU_AVAILABLE
	switch (r) {
	case 0:
		display_cmd((uint8_t) (0x80 + c));
		break;
	case 1:
		display_cmd((uint8_t) (0xc0 + c));
		break;
	case 2:
		display_cmd((uint8_t) (0x94 + c));
		break;
	case 3:
		display_cmd((uint8_t) (0xd4 + c));
		break;
	default:
		break;
	}
#endif // DISPLAY_MCU_AVAILABLE

#ifdef DISPLAY_REMOTE_AVAILABLE
	remote_column = c;
	remote_row = r;
#endif

	(void) r;
	(void) c;
}

/**
 * Schreibt einen String auf das Display, der im Flash gespeichert ist
 * \param *format 	Format, wie beim printf, Zeiger aber auf String im Flash
 * \param ... 		Variable Argumentenliste, wie beim printf
 * \return			Anzahl der geschriebenen Zeichen
 * Ganz genauso wie das "alte" display_printf(...) zu benutzen, das Makro
 * define display_printf(format, args...) erledigt alles automatisch, damit der String
 * im Flash verbleibt und erst zur Laufzeit temporaer (jeweils nur eine Zeile) geladen wird.
 */
uint8_t display_flash_printf(const char * format, ...) {
	char display_buf[DISPLAY_BUFFER_SIZE]; /**< Pufferstring fuer Displayausgaben */
	va_list	args;

	/* Sicher gehen, dass der zur Verfuegung stehende Puffer nicht ueberlaeuft */
	va_start(args, format);
	uint8_t len = (uint8_t) vsnprintf_P(display_buf, DISPLAY_BUFFER_SIZE, format, args);
	va_end(args);
	if (len > DISPLAY_LENGTH) {
		len = DISPLAY_LENGTH;
	}

#ifdef DISPLAY_MCU_AVAILABLE
	/* Ausgeben bis Puffer leer ist */
	char* ptr = display_buf;
	uint8_t i;
	for (i = len; i > 0; --i) {
		display_data(*ptr++);
	}
#endif // DISPLAY_MCU_AVAILABLE

#ifdef DISPLAY_REMOTE_AVAILABLE
	const int16_t c = remote_column;
	const int16_t r = remote_row;
	remote_column = (uint8_t) (remote_column + len);
#ifndef BOT_2_RPI_AVAILABLE
	command_write_rawdata(CMD_AKT_LCD, SUB_LCD_DATA, c, r, len, display_buf);
#else
	command_write_rawdata_to(CMD_AKT_LCD, SUB_LCD_DATA, CMD_IGNORE_ADDR, c, r, len, display_buf);
#endif // BOT_2_RPI_AVAILABLE

#endif // DISPLAY_REMOTE_AVAILABLE
	return len;
}

/**
 * Gibt einen String (im Flash) auf dem Display aus
 * \param *text	Zeiger auf den auszugebenden String
 * \return Anzahl der geschriebenen Zeichen
 */
uint8_t display_flash_puts(const char * text) {
	uint8_t len = 0;
#if defined DISPLAY_MCU_AVAILABLE || defined DISPLAY_REMOTE_AVAILABLE
	const char* ptr = text;
	char tmp;
	while ((tmp = (char) pgm_read_byte(ptr++)) != 0 && len < DISPLAY_LENGTH) {
#ifdef DISPLAY_MCU_AVAILABLE
		display_data(tmp);
#endif
		++len;
	}

#ifdef DISPLAY_REMOTE_AVAILABLE
	const int16_t c = remote_column;
	const int16_t r = remote_row;
	remote_column = (uint8_t) (remote_column + len);

	os_enterCS();
#ifndef BOT_2_RPI_AVAILABLE
	command_write_to_internal(CMD_AKT_LCD, SUB_LCD_DATA, CMD_SIM_ADDR, c, r, len);
#else
	command_write_to_internal(CMD_AKT_LCD, SUB_LCD_DATA, CMD_IGNORE_ADDR, c, r, len);
#endif // BOT_2_RPI_AVAILABLE
	ptr = text;
	uint8_t i;
	for (i = 0; i < len; ++i) {
		tmp = (char) pgm_read_byte(ptr++);
		cmd_functions.write(&tmp, 1);
	}
	os_exitCS();
#endif // DISPLAY_REMOTE_AVAILABLE
#endif // DISPLAY_MCU_AVAILABLE || DISPLAY_REMOTE_AVAILABLE

	(void) ptr;
	(void) tmp;

	return len;
}

#endif // DISPLAY_AVAILABLE
#endif // MCU
