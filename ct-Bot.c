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
#include "init.h"
#include "display.h"
#include "log.h"
#include "sp03.h"
#include "bot-2-sim.h"
#include "sensor-low.h"
#include "trace.h"
#include "motor.h"
#include "bot-logic/bot-logik.h"
#include "sensor.h"
#include "gui.h"
#include "command.h"
#include "os_thread.h"


//#define DEBUG_TIMES	/*!< Gibt Debug-Infos zum Timing aus (PC) */


/*!
 * Hauptprogramm des Bots. Diese Schleife kuemmert sich um seine Steuerung.
 */
int main(int argc, char * argv[]) {
	static uint16_t comm_ticks = 0;
	static uint8_t uart_gui = 0;

	/* Alles initialisieren */
	ctbot_init(argc, argv);

#ifdef WELCOME_AVAILABLE
	display_cursor(1, 1);
	display_printf("c't-Roboter");

#ifdef LOG_AVAILABLE
	LOG_INFO("Hallo Welt!");
#endif
#ifdef SP03_AVAILABLE
	sp03_say("I am Robi %d", sensError);
#endif
#endif // WELCOME_AVAILABLE

#ifdef  TEST_AVAILABLE_MOTOR
	uint16_t calls = 0;	// Im Testfall zaehle die Durchlaeufe
#endif

	/* Hauptschleife des Bots */
	for (;;) {
#ifdef BOT_2_SIM_AVAILABLE
		/* Daten vom Sim empfangen */
		bot_2_sim_listen();
#endif // BOT_2_SIM_AVAILABLE

		/* Sensordaten aktualisieren / auswerten */
		bot_sens();

#if defined PC && defined DEBUG_TIMES
		/* Zum Debuggen der Zeiten */
		struct timeval start, stop;
		GETTIMEOFDAY(&start, NULL);
		int t1 = (start.tv_sec - stop.tv_sec) * 1000000 + start.tv_usec - stop.tv_usec;
		printf("Done-Token (%d) in nach %d usec\n", received_command.data_l, t1);
#endif // PC && DEBUG_TIMES

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
#endif // TEST_AVAILABLE_MOTOR
		{
#ifdef BEHAVIOUR_AVAILABLE
			/* hier drin steckt der Verhaltenscode */
			bot_behave();
#endif // BEHAVIOUR_AVAILABLE
		}

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

#ifdef PC
#ifdef CREATE_TRACEFILE_AVAILABLE
		trace_add_actuators();
#endif // CREATE_TRACEFILE_AVAILABLE

		/* Sim ueber naechsten Schleifendurchlauf / Bot-Zyklus informieren */
		command_write(CMD_DONE, SUB_CMD_NORM, simultime, 0, 0); // flusht auch den Sendepuffer

#ifdef DEBUG_TIMES
		/* Zum Debuggen der Zeiten */
		GETTIMEOFDAY(&stop, NULL);
		int t2 = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
		printf("Done-Token (%d) out after %d usec\n", simultime, t2);
#endif // DEBUG_TIMES
#endif // PC

#ifdef OS_AVAILABLE
		/* Rest der Zeitscheibe (OS_TIME_SLICE ms) schlafen legen */
		os_thread_yield();
#endif // OS_AVAILABLE
	}
}
