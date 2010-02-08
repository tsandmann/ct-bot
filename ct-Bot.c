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
 * @file 	ct-Bot.c
 * @brief 	Bot-Hauptprogramm
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.2005
 */

#include "ct-Bot.h"

#ifdef MCU
#include <avr/io.h>
#include <avr/wdt.h>
#else	// PC
#include "tcp.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#endif	// MCU

#include "bot-2-sim.h"
#include "display.h"
#include "led.h"
#include "ena.h"
#include "shift.h"
#include "delay.h"
#include "uart.h"
#include "adc.h"
#include "timer.h"
#include "sensor.h"
#include "log.h"
#include "motor.h"
#include "sensor-low.h"
#include "bot-logic/bot-logik.h"
#include "mouse.h"
#include "command.h"
#include "ir-rc5.h"
#include "rc5.h"
#include "timer.h"
#include "mmc.h"
#include "mmc-emu.h"
#include "mmc-vm.h"
#include "gui.h"
#include "ui/available_screens.h"
#include "os_thread.h"
#include "map.h"
#include "cmd_tools.h"
#include "eeprom.h"
#include "i2c.h"
#include "twi.h"
#include "sp03.h"
#include "trace.h"

//#define DEBUG_TIMES	/*!< Gibt Debug-Infos zum Timing aus (PC) */

/*!
 * Der Mikrocontroller und der PC-Simulator brauchen ein paar Einstellungen,
 * bevor wir loslegen koennen.
 */
static void init(void) {
#ifdef CREATE_TRACEFILE_AVAILABLE
	trace_init();
#endif	// CREATE_TRACEFILE_AVAILABLE

#ifdef MCU
	PORTA = 0;
	DDRA  = 0; // Alles Eingang -> alles Null
	PORTB = 0;
	DDRB  = 0;
	PORTC = 0;
	DDRC  = 0;
	PORTD = 0;
	DDRD  = 0;

	wdt_disable(); // Watchdog aus!
#ifdef OS_AVAILABLE
	os_create_thread((void *) SP, NULL); // Hauptthread anlegen
#ifdef OS_DEBUG
	extern unsigned char * __brkval;
	extern size_t __malloc_margin;
	malloc(0);	// initialisiert __brkval
	os_mask_stack(__brkval, (size_t) ((unsigned char *) SP - __brkval) - __malloc_margin);
#endif	// OS_DEBUG
#ifdef OS_KERNEL_LOG_AVAILABLE
	os_kernel_log_init();
#endif	// OS_KERNEL_LOG_AVAILABLE
#endif	// OS_AVAILABLE
	timer_2_init();

	/* Ist das ein Power on Reset? */
#ifdef MCU_ATMEGA644X
	if ((MCUSR & 1) == 1) {
		MCUSR = (uint8_t)(MCUSR & ~1); // Bit loeschen
#else
	if ((MCUCSR & 1) == 1) {
		MCUCSR = (uint8_t)(MCUCSR & ~1); // Bit loeschen
#endif	// MCU_ATMEGA644X
		delay(100);
		__asm__ __volatile__("jmp 0");	// reboot
	}

	delay(100);
#ifdef RESET_INFO_DISPLAY_AVAILABLE
#ifdef MCU_ATMEGA644X
	reset_flag = (uint8_t) (MCUSR & 0x1F); // Lese Grund fuer Reset und sichere Wert
	MCUSR = 0; // setze Register auf 0x00 (loeschen)
#else
	reset_flag = (uint8_t) (MCUCSR & 0x1F); // Lese Grund fuer Reset und sichere Wert
	MCUCSR = 0; // setze Register auf 0x00 (loeschen)
#endif	// MCU_ATMEGA644X
	uint8_t resets = (uint8_t) (ctbot_eeprom_read_byte(&resetsEEPROM) + 1);
	ctbot_eeprom_write_byte(&resetsEEPROM, resets);
#endif	// RESET_INFO_DISPLAY_AVAILABLE
#endif	// MCU

#ifdef UART_AVAILABLE
	uart_init();
#endif
#ifdef BOT_2_SIM_AVAILABLE
	bot_2_sim_init();
#ifdef PC
	os_create_thread(NULL, read_command_thread);
#endif // PC
#endif // BOT_2_SIM_AVAILABLE
#ifdef DISPLAY_AVAILABLE
	display_init();
#endif
#ifdef LED_AVAILABLE
	LED_init();
#endif
	motor_init();
#ifdef ENA_AVAILABLE
	ENA_init();
#endif
#ifdef MMC_AVAILABLE
	mmc_init();
#endif
	bot_sens_init();
#ifdef BEHAVIOUR_AVAILABLE
	bot_behave_init();
#endif
#ifdef RC5_AVAILABLE
	ir_init(&RC5_PORT, &RC5_DDR, RC5_PIN);
#endif
#ifdef BPS_AVAILABLE
	ir_init(&BPS_PORT, &BPS_DDR, BPS_PIN);
#endif
#ifdef MOUSE_AVAILABLE
	mouse_sens_init();
#endif
#ifdef MAP_AVAILABLE
	map_init();
#endif
#ifdef LOG_MMC_AVAILABLE
	log_mmc_init();
#endif
#ifdef I2C_AVAILABLE
	i2c_init(42); // 160 kHz
#endif
#ifdef TWI_AVAILABLE
	Init_TWI();
#endif
#ifdef DISPLAY_AVAILABLE
	gui_init();
#endif
#if defined OS_AVAILABLE && defined MCU
#ifdef OS_DEBUG
	os_mask_stack(os_idle_stack, OS_IDLE_STACKSIZE);
#endif
	os_create_thread(&os_idle_stack[OS_IDLE_STACKSIZE - 1], os_idle);
#endif	// OS_AVAILABLE
}

#ifdef MCU
int main(int argc, char * argv[]) __attribute__((OS_main)); // kein Caller, Interrupts disabled
#endif	// MCU

/*!
 * Hauptprogramm des Bots. Diese Schleife kuemmert sich um seine Steuerung.
 */
int main(int argc, char * argv[]) {
	/* keine warnings */
	argc = argc;
	argv = argv;
	static uint16_t comm_ticks = 0;
	static uint8_t uart_gui = 0;

#ifdef PC
#ifdef DEBUG_TIMES
	/* zum Debuggen der Zeiten */
	struct timeval start, stop;
#endif	// DEBUG_TIMES

	/* PC-EEPROM-Init vor hand_cmd_args() */
	if (init_eeprom_man(0) != 0) {
		LOG_ERROR("EEPROM-Manager nicht korrekt initialisiert!");
	}

	/* Kommandozeilen-Argumente auswerten */
	hand_cmd_args(argc, argv);

	printf("c't-Bot\n");
#endif	// PC

	/* Alles initialisieren */
	init();

#ifdef WELCOME_AVAILABLE
	display_cursor(1, 1);
	display_printf("c't-Roboter");

#ifdef LOG_AVAILABLE
	LOG_INFO("Hallo Welt!");
#endif
#ifdef SP03_AVAILABLE
	sp03_say("I am Robi %d", sensError);
#endif
#endif	// WELCOME_AVAILABLE

#ifdef  TEST_AVAILABLE_MOTOR
	uint16_t calls = 0;	// Im Testfall zaehle die Durchlaeufe
#endif

	/* Hauptschleife des Bots */
	for (;;) {
#ifdef BOT_2_SIM_AVAILABLE
		/* Daten vom Sim empfangen */
		bot_2_sim_listen();
#endif	// BOT_2_SIM_AVAILABLE

		/* Sensordaten aktualisieren / auswerten */
		bot_sens();

#ifdef CREATE_TRACEFILE_AVAILABLE
		trace_add_sensors();
#endif	// CREATE_TRACEFILE_AVAILABLE

#if defined PC && defined DEBUG_TIMES
		/* Zum Debuggen der Zeiten */
		GETTIMEOFDAY(&start, NULL);
		int t1 = (start.tv_sec - stop.tv_sec) * 1000000 + start.tv_usec - stop.tv_usec;
		printf("Done-Token (%d) in nach %d usec ", received_command.data_l, t1);
#endif	// DEBUG_TIMES

#ifdef TEST_AVAILABLE_MOTOR
		/* Testprogramm, das den Bot erst links-, dann rechtsrum dreht */
		if (calls < 1001) {
			calls++;
			if (calls == 1) {
				motor_set(BOT_SPEED_SLOW, -BOT_SPEED_SLOW);
			} else if (calls == 501) {
				motor_set(-BOT_SPEED_SLOW, BOT_SPEED_SLOW);
			} else if (calls == 1001) {
				motor_set(BOT_SPEED_STOP, BOT_SPEED_STOP);
			}
		} else
#endif	// TEST_AVAILABLE_MOTOR
		{
#ifdef BEHAVIOUR_AVAILABLE
			/* hier drin steckt der Verhaltenscode */
			bot_behave();
#endif	// BEHAVIOUR_AVAILABLE
		}

		/* jeweils alle 100 ms kommunizieren Bot, User und Sim */
		if (timer_ms_passed_16(&comm_ticks, 50) || RC5_Code != 0) {
			if (uart_gui == 0 || RC5_Code != 0) {
#ifdef DISPLAY_AVAILABLE
				/* GUI-Behandlung starten */
				gui_display(display_screen);
#endif	// DISPLAY_AVAILABLE
				uart_gui = 1; // bot-2-sim ist erst beim naechsten Mal dran
			} else {
#ifdef BOT_2_SIM_AVAILABLE
				/* Den Sim ueber Sensoren und Aktuatoren informieren */
				bot_2_sim_inform(); // NOP auf PC
#endif	// BOT_2_SIM_AVAILABLE
				uart_gui = 0; // naechstes Mal wieder mit GUI anfangen
			}
		}

#ifdef PC
#ifdef CREATE_TRACEFILE_AVAILABLE
		trace_add_actuators();
#endif	// CREATE_TRACEFILE_AVAILABLE

		/* Sim ueber naechsten Schleifendurchlauf / Bot-Zyklus informieren */
		command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0); // flusht auch den Sendepuffer

#ifdef DEBUG_TIMES
		/* Zum Debuggen der Zeiten */
		GETTIMEOFDAY(&stop, NULL);
		int t2 = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
		printf("Done-Token (%d) out after %d usec\n", simultime, t2);
#endif	// DEBUG_TIMES
#endif	// PC

		/* Ausgabemoeglichkeit der Positionsdaten (z.B. zur Analyse der Genauigkeit): */
//		LOG_INFO("%f\t%f\t%f\t%f\t%f\t%f", x_enc, x_mou, y_enc, y_mou, heading_enc, heading_mou);

#ifdef OS_AVAILABLE
		/* Rest der Zeitscheibe (OS_TIME_SLICE ms) schlafen legen */
		os_thread_yield();
#endif	// OS_AVAILABLE
	}
}
