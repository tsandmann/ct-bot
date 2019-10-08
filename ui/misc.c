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
 * \file 	misc.c
 * \brief 	Sonstige Display-Anzeigefunktionen, die in keine andere Datei so richtig passen
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	12.02.2007
 */

#include "ct-Bot.h"
#include "ui/available_screens.h"
#if defined DISPLAY_MISC_AVAILABLE || defined DISPLAY_RESET_INFO_AVAILABLE || defined DISPLAY_RAM_AVAILABLE
#include <stddef.h>
#include "display.h"
#include "gui.h"
#include "rc5-codes.h"
#include "sensor.h"
#include "log.h"
#include "init.h"
#include "os_thread.h"

#ifdef DISPLAY_MISC_AVAILABLE
#include "bot-logic/bot-logic.h"
#include "timer.h"
#include "command.h"
#include "eeprom.h"
#include <stdlib.h>

#ifdef KEYPAD_AVAILABLE
static uint8_t new_address = 0;	/**< True, falls neue Adresse eingegeben wird */

/**
 * Setzt die Bot-Adresse auf einen neuen Wert.
 * Wird von der Keypad-Eingabe aufgerufen, sobald
 * die Eingabe abgeschlossen ist.
 * \param *data	Daten-String
 */
static void change_bot_addr_callback(char * data) {
	new_address = 0;
	uint8_t addr = (uint8_t) atoi(data);
	if (addr != 0) {
		set_bot_address(addr);
	}
}
#endif // KEYPAD_AVAILABLE

/**
 * Zeigt ein paar Infos an, die man nicht naeher zuordnen kann
 */
void misc_display(void) {
	/* Anzeige der Bot-Adresse (aenderbar) */
	display_cursor(1, 1);
	display_puts("bot_addr=");

#ifdef KEYPAD_AVAILABLE
	if (RC5_Code == RC5_CODE_MUTE) {
		gui_keypad_request(change_bot_addr_callback, 1, 1, 10);
#ifdef PC
		display_cursor(1, 10);
#endif
		display_puts("          "); // clean
		new_address = 1;
		RC5_Code = 0;
	}
	if (new_address == 0)
#endif // KEYPAD_AVAILABLE
	{
#ifdef PC
		display_cursor(1, 10);
#endif
		display_printf("0x%x", get_bot_address());
	}

#ifdef BEHAVIOUR_AVAILABLE
	display_cursor(2, 1);
	display_printf("TS=%+4d %+4d", target_speed_l, target_speed_r);
#endif

#ifdef SRF10_AVAILABLE
	display_cursor(2, 15);
	display_printf("US%4u", sensSRF10);
#endif

	display_cursor(3, 1);
	display_printf("RC=%+4d %+4d", sensEncL, sensEncR);

	display_cursor(4, 1);
	display_printf("Speed=%+4d", v_center);

	display_cursor(4, 12);
	display_printf("%4u:%03u", timer_get_s(), timer_get_ms());
}
#endif // DISPLAY_MISC_AVAILABLE

#ifdef MCU
#ifdef DISPLAY_RESET_INFO_AVAILABLE
/**
 * Zeigt Informationen ueber Resets an
 */
void reset_info_display(void) {
	display_cursor(1, 1);
	display_printf("#Resets:%3u | (C)SR:", ctbot_eeprom_read_byte(&resetsEEPROM));

	display_cursor(2, 1);
	display_printf("PORF :%d  WDRF :%d", binary(mcucsr, 0), binary(mcucsr, 3));

	display_cursor(3, 1);
	display_printf("EXTRF:%d  JTRF :%d", binary(mcucsr, 1), binary(mcucsr, 4));

	display_cursor(4, 1);
	display_printf("BORF :%d #SResets:%3u", binary(mcucsr, 2), soft_resets);
}
#endif // DISPLAY_RESET_INFO_AVAILABLE

#ifdef DISPLAY_RAM_AVAILABLE
/*
 * 	*** AVR Data Memory Map ***
 *
 *	-----------------------------------------------------------------
 * 	| regs | i/o | [ext. i/o] | data | bss | heap > | (!) | < stack |
 * 	-----------------------------------------------------------------
 * 	^      ^     ^            ^      ^     ^        ^         ^     ^
 *  |      |     |            |      |     |        |         |     |
 * 	0x0    0x20  0x60         __data_start |        __brkval  |     RAMEND
 *                                   |     |                  |
 *                                   __bss_start              SP
 *                                         |
 *                                         __heap_start
 */

/**
 * Zeigt die aktuelle Speicherbelegung an.
 * Achtung, die Stackgroesse bezieht sich auf den Stack *dieser* Funktion!
 * Die Heapgroesse stimmt nur, wenn es dort keine Luecken gibt (z.b. durch free())
 */
void ram_display(void) {
	unsigned char * sp = (unsigned char *) SP;
	extern unsigned char __data_start;
	extern unsigned char __bss_start;
	extern unsigned char __heap_start;
	extern unsigned char * __brkval;
#ifdef RC5_AVAILABLE
#ifdef LOG_AVAILABLE
	extern unsigned char __data_end;
	extern unsigned char __bss_end;
	if (RC5_Code == RC5_CODE_1) {
		LOG_DEBUG("__data_start = 0x%04x", (uintptr_t)&__data_start);
		LOG_DEBUG("__data_end = 0x%04x", (uintptr_t)&__data_end);
		LOG_DEBUG("__bss_start = 0x%04x", (uintptr_t)&__bss_start);
		LOG_DEBUG("__bss_end = 0x%04x", (uintptr_t)&__bss_end);
		LOG_DEBUG("__heap_start = 0x%04x", (uintptr_t)&__heap_start);
		LOG_DEBUG("__heap_end = 0x%04x", (uintptr_t)__brkval);
		LOG_DEBUG("SP = 0x%04x", (uintptr_t)sp);
		LOG_DEBUG("RAMEND = 0x%04x", RAMEND);
		RC5_Code = 0;
	}
#endif // RC5_AVAILABLE
#endif // LOG_AVAILABLE
	size_t data_size = (size_t) (&__bss_start - &__data_start);
	size_t bss_size = (size_t) (&__heap_start - &__bss_start);
	display_cursor(1, 1);
	display_puts("bss/data:");
	display_cursor(1, 10);
	display_printf("%5u/%5u", bss_size, data_size);
	display_cursor(2, 1);
	display_puts("heap:");
	display_cursor(2, 16);
	size_t heap_size = __brkval == NULL ? 0 : (size_t) (__brkval - &__heap_start);
	display_printf("%5u", heap_size);
	display_cursor(3, 1);
	size_t ram_size = (size_t) ((unsigned char *) (RAMEND + 1) - &__data_start);
	size_t stack_size = (size_t) ((unsigned char *) RAMEND - sp);
	display_puts("stack:");
#ifdef OS_DEBUG
	size_t stack_size_max = ram_size - (os_stack_unused(__brkval + __malloc_margin) + data_size + bss_size + heap_size);
	display_cursor(3, 10);
	display_printf("%5u/%5u", stack_size, stack_size_max);
#else
	display_cursor(3, 16);
	display_printf("%5u", stack_size);
#endif // OS_DEBUG
	display_cursor(4, 1);
	size_t frei = ram_size - (data_size + bss_size + heap_size + stack_size);
	display_puts("free/ram:");
	display_cursor(4, 10);
	display_printf("%5u/%5u", frei, ram_size);
}
#endif // DISPLAY_RAM_AVAILABLE
#endif // MCU
#endif // DISPLAY-Schalter
