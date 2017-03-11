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
 * \file 	botcontrol.c
 * \brief 	High-level Steuerungsroutinen, z.B. Funktionen fuer die Bot-Hauptschleife
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.05.2011
 */

#include "ct-Bot.h"

#include "botcontrol.h"
#include "bot-2-sim.h"
#include "sensor-low.h"
#include "sensor.h"
#include "command.h"
#include "trace.h"
#include "timer.h"
#include "gui.h"
#include "display.h"
#include "os_thread.h"
#include "log.h"
#include "motor.h"
#include "map.h"
#include "init.h"
#include "ena.h"
#include "bot-2-atmega.h"
#include "bot-2-linux.h"

//#define DEBUG_TIMES /**< Gibt Debug-Infos zu Timing / Auslastung aus */

#if defined PC && defined DEBUG_TIMES
struct timeval init_start, init_stop; /**< Zeit von Beginn und Ende des Verhaltensdurchlaufs */
#endif // PC && DEBUG_TIMES

/**
 * Fuehrt die Verarbeitung in der Hauptschlaufe vor dem
 * Verhaltenscode durch. Dazu gehoert beispielsweise, die Sensoren
 * abzufragen und auf Pakete des Simulators zu reagieren.
 */
void pre_behaviour(void) {
#ifdef ARM_LINUX_BOARD
	/* Daten vom ATmega empfangen */
	bot_2_atmega_listen();
#endif // ARM_LINUX_BOARD

#ifdef BOT_2_SIM_AVAILABLE
	/* Daten vom Sim empfangen */
	bot_2_sim_listen();
#ifdef ARM_LINUX_BOARD
	set_bot_2_atmega();
#endif // ARM_LINUX_BOARD
#endif // BOT_2_SIM_AVAILABLE

	/* Sensordaten aktualisieren / auswerten */
	bot_sens();

#if defined PC && defined DEBUG_TIMES
	/* Zum Debuggen der Zeiten */
	GETTIMEOFDAY(&init_start, NULL);
	int t1 = (init_start.tv_sec - init_stop.tv_sec) * 1000000 + init_start.tv_usec - init_stop.tv_usec;
	LOG_DEBUG("Done-Token (%d) in nach %d usec", received_command.data_l, t1);
#endif // PC && DEBUG_TIMES

#ifdef RC5_AVAILABLE
	rc5_control(); // Abfrage der IR-Fernbedienung
#endif // RC5_AVAILABLE

#ifdef BOT_2_RPI_AVAILABLE
	bot_2_linux_inform();
#endif // BOT_2_RPI_AVAILABLE
}

/**
 * Fuehrt die Verarbeitung in der Hauptschleife nach dem
 * Verhaltenscode durch. Dazu gehoert beispielsweise, die
 * Bildschirmanzeige zu steuern und den Simulator ueber den aktuellen
 * Zustand zu informieren.
 */
void post_behaviour(void) {
	static uint16_t comm_ticks = 0;
	static uint8_t uart_gui = 0;
#if defined MCU && defined DEBUG_TIMES
	static uint32_t log_ticks = 0;
#endif

#ifdef BOT_2_RPI_AVAILABLE
	bot_2_linux_listen();
#endif // BOT_2_RPI_AVAILABLE

#ifdef CREATE_TRACEFILE_AVAILABLE
	trace_add_sensors();
#endif // CREATE_TRACEFILE_AVAILABLE

	/* jeweils alle 100 ms kommunizieren Bot, User und Sim */
	if (timer_ms_passed_16(&comm_ticks, 50)
#ifdef RC5_AVAILABLE
		|| RC5_Code != 0
#endif
	) {
		if (uart_gui == 0
#ifdef RC5_AVAILABLE
			|| RC5_Code != 0
#endif
		) {
#ifdef DISPLAY_AVAILABLE
			/* GUI-Behandlung starten */
			gui_display(display_screen);
#endif // DISPLAY_AVAILABLE
			uart_gui = 1; // bot-2-sim ist erst beim naechsten Mal dran
		} else {
#ifdef BOT_2_SIM_AVAILABLE
			/* Den Sim ueber Sensoren und Aktuatoren informieren */
			bot_2_sim_inform(); // NOP auf PC
#endif // BOT_2_SIM_AVAILABLE
			uart_gui = 0; // naechstes Mal wieder mit GUI anfangen
		}
	}

#if defined MCU && defined DEBUG_TIMES
	if (timer_ms_passed_32(&log_ticks, 1000UL)) {
		uint8_t cpu, uart_in, uart_out;
		os_get_utilizations(&cpu, &uart_in, &uart_out);
		LOG_INFO("CPU=%3u%% UART_IN=%3u%% UART_OUT=%3u%%", cpu, uart_in, uart_out);
	}
#endif // MCU && DEBUG_TIMES

#ifdef PC
#ifdef CREATE_TRACEFILE_AVAILABLE
	trace_add_actuators();
#endif // CREATE_TRACEFILE_AVAILABLE

	/* Sim oder ATmega ueber naechsten Schleifendurchlauf / Bot-Zyklus informieren */
#ifdef ARM_LINUX_BOARD
	set_bot_2_atmega();
#endif // ARM_LINUX_BOARD
	command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0); // flusht auch den Sendepuffer

#ifdef DEBUG_TIMES
	/* Zum Debuggen der Zeiten */
	GETTIMEOFDAY(&init_stop, NULL);
	int t2 = (init_stop.tv_sec - init_start.tv_sec) * 1000000 + init_stop.tv_usec - init_start.tv_usec;
	LOG_DEBUG("Done-Token (%d) out after %d usec", simultime, t2);
#endif // DEBUG_TIMES
#endif // PC

#if defined OS_AVAILABLE
	/* Rest der Zeitscheibe (OS_TIME_SLICE ms) schlafen legen */
	os_thread_yield();
#endif // OS_AVAILABLE
}

#ifdef BOOTLOADER_AVAILABLE
void bootloader_main(void);
#endif // BOOTLOADER_AVAILABLE

/**
 * Faehrt den Bot sauber herunter
 */
void ctbot_shutdown(void) {
	LOG_INFO("Shutting c't-Bot down...");

	motor_set(BOT_SPEED_STOP, BOT_SPEED_STOP);

#ifdef MAP_AVAILABLE
	map_flush_cache();
#endif

#ifdef LOG_MMC_AVAILABLE
	log_flush();
#endif

#ifdef BOT_FS_AVAILABLE
	botfs_close_volume();
#endif

#ifdef DISPLAY_AVAILABLE
	display_clear();
	display_cursor(1, 1);
	display_puts("SYSTEM HALTED.");
#endif // DISPLAY_AVAILABLE

	ENA_off(0xff);

	ctbot_shutdown_low();

#ifdef BOOTLOADER_AVAILABLE
	bootloader_main();
#endif // BOOTLOADER_AVAILABLE
}
