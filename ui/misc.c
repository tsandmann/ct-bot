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
 * @file 	misc.c
 * @brief 	Sonstige Display-Anzeigefunktionen, die in keine andere Datei so richtig passen
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007
 */

#include "ct-Bot.h"
#include "global.h"
#include "ui/available_screens.h"
#include "display.h"
#include "bot-logic/bot-logik.h"
#include "timer.h"
#include "log.h"
#include "rc5-codes.h"
#include "command.h"
#include "gui.h"
#include "eeprom.h"
#include <stdlib.h>

uint8_t EEPROM resetsEEPROM = 0;	/*!< Reset-Counter-Wert im EEPROM */

#ifdef MISC_DISPLAY_AVAILABLE
static uint8_t new_address = 0;	/*!< True, falls neue Adresse eingegeben wird */

/*!
 * Setzt die Bot-Adresse auf einen neuen Wert.
 * Wird von der Keypad-Eingabe aufgerufen, sobald
 * die Eingabe abgeschlossen ist.
 * @param *data	Daten-String
 */
void change_bot_addr_callback(char * data) {
	new_address = 0;
	uint8_t addr = atoi(data);
	if (addr != 0) {
		set_bot_address(addr);
	}
}

/*!
 * @brief	Zeigt ein paar Infos an, die man nicht naeher zuordnen kann
 */
void misc_display(void){
	#ifdef TIME_AVAILABLE
		display_cursor(1,1);
		display_printf("Zeit: %04u:%03u", timer_get_s(), timer_get_ms());
	#else
		/* Anzeige der Bot-Adresse (aenderbar) */
		display_cursor(1,1);
		display_printf("bot_addr=");

	#ifdef KEYPAD_AVAILABLE
		if (RC5_Code == RC5_CODE_MUTE) {
			gui_keypad_request(change_bot_addr_callback, 1, 10);
			#ifdef PC
				display_cursor(1,10);
			#endif
			display_printf("          ");	// clean
			new_address = 1;
			RC5_Code = 0;
		}
	#endif	// KEYPAD_AVAILABLE
		if (new_address == 0) {
			#ifdef PC
				display_cursor(1,10);
			#endif
			display_printf("0x%x", get_bot_address());
		}
	#endif	// TIME_AVAILABLE

	#ifdef BEHAVIOUR_AVAILABLE
		display_cursor(2,1);
		display_printf("TS=%+4d %+4d",target_speed_l,target_speed_r);
	#endif

	#ifdef SRF10_AVAILABLE
		display_cursor(2,15);
		display_printf("US%4u",sensSRF10);
	#endif

	display_cursor(3,1);
	display_printf("RC=%+4d %+4d",sensEncL,sensEncR);

	display_cursor(4,1);
	display_printf("Speed= %04d",(int16)v_center);
}
#endif	// MISC_DISPLAY_AVAILABLE

#ifdef RESET_INFO_DISPLAY_AVAILABLE
uint8 reset_flag;	/*!< Nimmt den Status von MCU(C)SR bevor dieses Register auf 0x00 gesetzt wird */

/*!
 * @brief Zeigt Informationen ueber Resets an
 */
void reset_info_display(void){
	display_cursor(1,1);
	display_printf("#Resets:%3u | (C)SR:", ctbot_eeprom_read_byte(&resetsEEPROM));

	display_cursor(2,1);
	display_printf("PORF :%d  WDRF :%d",binary(reset_flag,0),binary(reset_flag,3));

	display_cursor(3,1);
	display_printf("EXTRF:%d  JTRF :%d",binary(reset_flag,1),binary(reset_flag,4));

	display_cursor(4,1);
	display_printf("BORF :%d",binary(reset_flag,2));
}
#endif	// RESET_INFO_DISPLAY_AVAILABLE

#ifdef RAM_DISPLAY_AVAILABLE
/*
 * 	*** AVR Data Memory Map ***
 *
 *	-----------------------------------------------------------------
 * 	| regs | i/o | [ext. i/o] | data | bss | heap > | (!) | < stack |
 * 	-----------------------------------------------------------------
 * 	^      ^     ^            ^      ^     ^        ^     ^         ^
 *  |      |     |            |      |     |        |     |         |
 * 	0x0    0x20  0x60         __data_start |        __brkval        RAMEND
 *                                   |     |              |
 *                                   __bss_start          SP
 *                                         |
 *                                         __heap_start
 */

/*!
 * @brief	Zeigt die aktuelle Speicherbelegung an.
 * 			Achtung, die Stackgroesse bezieht sich auf den Stack *dieser* Funktion!
 * 			Die Heapgroesse stimmt nur, wenn es dort keine Luecken gibt (z.b. durch free())
 */
void ram_display(void) {
	unsigned char * sp = (unsigned char *)SP;
	extern unsigned char __data_start;
	extern unsigned char __bss_start;
	extern unsigned char __heap_start;
	extern unsigned char * __brkval;
	#ifdef LOG_AVAILABLE
		extern unsigned char __data_end;
		extern unsigned char __bss_end;
		if (RC5_Code == RC5_CODE_1) {
			LOG_DEBUG("__data_start = 0x%04x", &__data_start);
			LOG_DEBUG("__data_end = 0x%04x", &__data_end);
			LOG_DEBUG("__bss_start = 0x%04x", &__bss_start);
			LOG_DEBUG("__bss_end = 0x%04x", &__bss_end);
			LOG_DEBUG("__heap_start = 0x%04x", &__heap_start);
			LOG_DEBUG("__heap_end = 0x%04x", __brkval);
			LOG_DEBUG("SP = 0x%04x", sp);
			LOG_DEBUG("RAMEND = 0x%04x", RAMEND);
			RC5_Code = 0;
		}
	#endif	// LOG_AVAILABLE
	size_t data_size = &__bss_start - &__data_start;
	size_t bss_size = &__heap_start - &__bss_start;
	display_cursor(1,1);
	display_printf("bss/data:%4u/%4u B", bss_size, data_size);
	display_cursor(2,1);
	display_printf("heap:");
	display_cursor(2,15);
	size_t heap_size = __brkval - &__heap_start;
	display_printf("%4u B", heap_size);
	display_cursor(3,1);
	display_printf("stack:");
	display_cursor(3,15);
	size_t stack_size = (unsigned char *)RAMEND - sp;
	display_printf("%4u B", stack_size);
	display_cursor(4,1);
	size_t ram_size = (unsigned char *)(RAMEND+1) - &__data_start;
	size_t frei = ram_size - (data_size + bss_size + heap_size + stack_size);
	display_printf("free/all:%4u/%4u B", frei, ram_size);
}
#endif	// RAM_DISPLAY_AVAILABLE
