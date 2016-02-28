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
 * \file 	bot-2-linux.c
 * \brief 	Verbindung zwischen c't-Bot und Linux-Board (z.B. BeagleBoard oder Raspberry Pi)
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.08.2009
 */

#include "ct-Bot.h"
#if defined MCU && defined BOT_2_RPI_AVAILABLE

#include "command.h"
#include "bot-2-linux.h"
#include "uart.h"
#include "sensor.h"
#include "mouse.h"
#include "display.h"
#include "led.h"
#include "timer.h"
#include "delay.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


#define CRC_ERROR_TIME 2000UL

/**
 * Initialisiert die Kommunikation mit dem Linux-Board
 */
void bot_2_linux_init(void) {
	LED_set(0);
	display_clear();
	display_cursor(1, 1);
	display_printf("*** Waiting for");
	display_cursor(2, 1);
	display_printf("*** connection...");
	if (((PINB >> 2) & 1) == 1) { // error sensor
		LED_on(LED_GRUEN | LED_ORANGE);
	} else {
		LED_on(LED_ROT);
		display_cursor(3, 1);
		display_printf("Low battery!");
	}
}

/**
 * Empfaengt alle Kommondos vom Linux-Board.
 * Die Funktion nimmt die Daten vom Linux-Board entgegen und
 * wertet sie aus. Dazu nutzt sie die Funktion command_evaluate().
 * \todo add Timeout
 */
void bot_2_linux_listen(void) {
	static uint32_t last_crc_error = 0 - MS_TO_TICKS(CRC_ERROR_TIME);

	const uint32_t now = TIMER_GET_TICKCOUNT_32;
	if (now < last_crc_error + MS_TO_TICKS(CRC_ERROR_TIME)) {
		LED_on(LED_TUERKIS);
	} else {
		LED_off(LED_TUERKIS);
	}

	uint16_t i = 0;
	while (42) {
		if (uart_data_available() >= sizeof(command_t)) {
			const int8_t result = command_read();
			if (result == 0) {
				command_evaluate();
			} else if (result == -20) {
				/* CRC Fehler */
				last_crc_error = TIMER_GET_TICKCOUNT_32;
			}

			if (received_command.request.command == CMD_DONE) {
				return;
			}
		}
		++i;
		if (i % 5000 < 2500) {
			display_cursor(4, 20);
			display_puts("-");
		} else {
			display_cursor(4, 20);
			display_puts("|");
		}
	}
}

/**
 * Diese Funktion informiert den Steuercode auf dem Linux-Board ueber alle Sensor- und Aktuator-Werte
 */
void bot_2_linux_inform(void) {
	command_write_to(CMD_SENS_IR, SUB_CMD_NORM, CMD_IGNORE_ADDR, sensDistL, sensDistR, 0);
	command_write_to(CMD_SENS_ENC, SUB_CMD_NORM, CMD_IGNORE_ADDR, sensEncL, sensEncR, 0);
	command_write_to(CMD_SENS_BORDER, SUB_CMD_NORM, CMD_IGNORE_ADDR, sensBorderL, sensBorderR, 0);
	command_write_to(CMD_SENS_LINE, SUB_CMD_NORM,  CMD_IGNORE_ADDR, sensLineL, sensLineR, 0);
	command_write_to(CMD_SENS_LDR, SUB_CMD_NORM, CMD_IGNORE_ADDR, sensLDRL, sensLDRR, 0);
#ifdef BPS_AVAILABLE
	command_write_to(CMD_SENS_BPS, SUB_CMD_NORM, CMD_IGNORE_ADDR, (int16_t) sensBPS, 0, 0);
#endif
	command_write_to(CMD_SENS_TRANS, SUB_CMD_NORM, CMD_IGNORE_ADDR, sensTrans, 0, 0);
	command_write_to(CMD_SENS_DOOR, SUB_CMD_NORM, CMD_IGNORE_ADDR, sensDoor, 0, 0);
	command_write_to(CMD_SENS_ERROR, SUB_CMD_NORM, CMD_IGNORE_ADDR, sensError, 0, 0);
	command_write_to(CMD_SENS_RC5, SUB_CMD_NORM, CMD_IGNORE_ADDR, (int16_t) RC5_Code, 0, 0);

#ifdef MOUSE_AVAILABLE
	command_write_to(CMD_SENS_MOUSE, SUB_CMD_NORM, CMD_IGNORE_ADDR, sensMouseDX, sensMouseDY, 0);
#endif

	const uint32_t now = timer_get_tickcount_32();
	const uint16_t time_low = (uint16_t) now;
	const uint16_t time_high = (uint16_t) (now >> 16);
	command_write_to(CMD_DONE, SUB_CMD_NORM, CMD_IGNORE_ADDR, (int16_t) time_low, (int16_t) time_high, 0);
}


#ifdef DISPLAY_AVAILABLE
/**
 * Display Screen fuer Inhalte vom Linux-Board
 */
void linux_display(void) {
	// leer, wird von command_evaluate() gefuellt
}
#endif // DISPLAY_AVAILABLE

#endif // MCU && BOT_2_RPI_AVAILABLE
