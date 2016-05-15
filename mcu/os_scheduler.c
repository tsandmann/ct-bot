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
 * \file 	os_scheduler.c
 * \brief 	Mini-Scheduler fuer BotOS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	02.10.2007
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef OS_AVAILABLE
#include "bot-logic/bot-logic.h"
#include "os_scheduler.h"
#include "ui/available_screens.h"
#include "os_utils.h"
#include "os_thread.h"
#include "sensor.h"
#include "rc5-codes.h"
#include "map.h"
#include "display.h"
#include "log.h"
#include "uart.h"
#include <stdlib.h>
#include <string.h>

// avr-gcc < 4.2.2 ?
#if !(__GNUC__ >= 4 && ((__GNUC_MINOR__ > 2) || (__GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ >= 2)))
#error "Compiler zu alt, mindestens 4.2.2 noetig!"	// fuer __attribute__((OS_task))
#endif

volatile uint8_t os_scheduling_allowed = 1;	/**< sperrt den Scheduler, falls != 1. Sollte nur per os_enterCS() / os_exitCS() veraendert werden! */

uint8_t os_idle_stack[OS_IDLE_STACKSIZE];	/**< Stack des Idle-Threads */

static volatile uint32_t idle_counter[3];	/**< Variable, die im Idle-Thread laufend inkrementiert wird */
#define ZYCLES_PER_IDLERUN	61 /**< Anzahl der CPU-Zyklen fuer einen Durchlauf der Idle-Schleife */

#ifdef OS_KERNEL_LOG_AVAILABLE
typedef struct {
	uint16_t time; /**< Timestamp */
	uint8_t from;  /**< Thread, der unterbrochen wurde */
	uint8_t to;    /**< Thread, der fortgesetzt wurde */
} kernel_log_t; /**< Log-Eintrag fuer Kernel-LOG */
static kernel_log_t kernel_log[OS_KERNEL_LOG_SIZE]; /**< Puffer fuer Kernel-LOG-Eintraege */
static fifo_t kernel_log_fifo; /**< Fifo fuer Kernel-LOG-Eintraege */
static kernel_log_t log_entry; /**< Letzter Kernel-LOG Eintrag */
static uint8_t kernel_log_on = 0; /**< Schalter um das Logging an und aus zu schalten */

/**
 * Initialisiert das Kernel-LOG
 */
void os_kernel_log_init(void) {
	memset(kernel_log, 0, sizeof(kernel_log));
	fifo_init(&kernel_log_fifo, kernel_log, sizeof(kernel_log));
}
#endif // OS_KERNEL_LOG_AVAILABLE

#ifdef MEASURE_UTILIZATION
#define UTIL_LOG_TIME		25  /**< Zeitintervall zwischen zwei Auslastungsberechnungen [ms] */
#define MAX_UTIL_ENTRIES	256 /**< Maximale Anzahl an Auslastungsdatensaetzen (muss 2er-Potenz sein!) */

static uint8_t util_data[MAX_UTIL_ENTRIES][OS_MAX_THREADS]; /**< Puffer fuer Auslastungsstatistik */
static uint16_t missed_deadlines[OS_MAX_THREADS]; /**< Anzahl der nicht eingehaltenen Deadlines */
static uint16_t util_count = 0;		 /**< Anzahl der Eintraege im Puffer */
static uint16_t last_util_time = 0;  /**< Zeitpunkt der letzten Erstellung einer Auslastungsstatistik */
static uint32_t first_util_time = 0; /**< Zeitpunkt der ersten Erstellung einer Auslastungsstatistik */

/**
 * Aktualisiert die Statistikdaten zur CPU-Auslastung
 */
void os_calc_utilization(void) {
	if (timer_ms_passed_16(&last_util_time, UTIL_LOG_TIME)) {
		os_stat_data_t data[OS_MAX_THREADS];
		uint8_t i;

		/* Daten aus den TCBs holen */
		uint16_t sum = 0;
		os_enterCS();
		for (i = 0; i < OS_MAX_THREADS; ++i) {
			data[i] = os_threads[i].statistics;
			sum += data[i].runtime;
			os_threads[i].statistics.runtime = 0;
			os_threads[i].statistics.missed_deadlines = 0;
		}
		os_exitCS();

		if (sum == 0) {
			return; // keine neuen Daten vorhanden
		}

		/* Auslastung in % berechnen */
		for (i = 0; i < OS_MAX_THREADS; ++i) {
			missed_deadlines[i] += data[i].missed_deadlines;
			uint32_t tmp = data[i].runtime;
			tmp *= 100;
			tmp /= sum;
			util_data[util_count][i] = (uint8_t) tmp;
		}
		util_count++;
		util_count &= MAX_UTIL_ENTRIES - 1;
	}
}

/**
 * Loescht alle Statistikdaten zur CPU-Auslastung
 */
void os_clear_utilization(void) {
	uint8_t i;
	os_enterCS();
	for (i = 0; i < OS_MAX_THREADS; ++i) {
		os_threads[i].statistics.runtime = 0;
		os_threads[i].statistics.missed_deadlines = 0;
		missed_deadlines[i] = 0;
	}
	first_util_time = TIMER_GET_TICKCOUNT_32;
	memset((void *)idle_counter, 0, sizeof(idle_counter));
	util_count = 0;
	os_exitCS();
}

/**
 * Gibt die gesammelten Statistikdaten zur CPU-Auslastung per LOG aus
 */
void os_print_utilization(void) {
	uint8_t i;
	for (i = 0; i < util_count; ++i) {
#if (OS_MAX_THREADS == 2)
		LOG_INFO("%3u\t%3u", util_data[i][0], util_data[i][1]);
#elif (OS_MAX_THREADS == 3)
		LOG_INFO("%3u\t%3u\t%3u", util_data[i][0], util_data[i][1], util_data[i][2]);
#elif (OS_MAX_THREADS == 4)
		LOG_INFO("%3u\t%3u\t%3u\t%3u", util_data[i][0], util_data[i][1], util_data[i][2], util_data[i][3]);
#else
#error "Auslastungsstatistik fuer OS_MAX_THREADS > 4 nicht implementiert!"
#endif
	}

	LOG_INFO("Missed deadlines:");
#if (OS_MAX_THREADS == 2)
	LOG_INFO("%5u\t%5u", missed_deadlines[0], missed_deadlines[1]);
#elif (OS_MAX_THREADS == 3)
	LOG_INFO("%5u\t%5u\t%5u", missed_deadlines[0], missed_deadlines[1], missed_deadlines[2]);
#elif (OS_MAX_THREADS == 4)
	LOG_INFO("%5u\t%5u\t%5u\t%5u", missed_deadlines[0], missed_deadlines[1], missed_deadlines[2], missed_deadlines[3]);
#endif

#ifdef LOG_AVAILABLE
	uint32_t runtime = TIMER_GET_TICKCOUNT_32 - first_util_time;
	/* idle_counter kann nicht atomar inkrementiert werden und wir wissen nicht, bei welcher Instruktion
	 * der idle-Thread unterbrochen wurde. Da es aber 3 Kopien des Idle-Zaehlers gibt, muessen immer
	 * mindesten zwei den korrekten Wert enthalten. Genau dieses Paar suchen wir nun und verwenden den Wert. */
	uint32_t idle = idle_counter[0];
	uint32_t idle2 = idle_counter[1];
	uint32_t idle3 = idle_counter[2];
	if (idle != idle2 && idle != idle3) {
		idle = idle2; // idle enthaelt den falschen Wert, also sind idle2 und idle3 korrekt
	}
	uint32_t idle_ticks = (uint32_t) ((float) idle * (ZYCLES_PER_IDLERUN * 1000000.f / F_CPU / TIMER_STEPS));

	uint8_t idle_pc = (uint8_t) (idle_ticks * 100 / runtime);
	LOG_INFO("%u %% idle", idle_pc);
#endif // LOG_AVAILABLE
}
#endif // MEASURE_UTILIZATION

/**
 * Idle-Thread
 */
void os_idle(void) {
	while (42) {
		const uint8_t sreg = SREG;
		__builtin_avr_cli();
		const uint32_t now = tickCount.u32;
		os_delayed_func_t* ptr = (os_delayed_func_t*) os_delayed_next_p; // cast away volatile
		SREG = sreg;

		if (ptr->p_func && ptr->runtime <= now) { // Funktion registriert und Ausfuehrungszeit erreicht
			ptr->p_func(ptr->p_data);
			ptr->p_func = NULL;
			ptr->runtime = (uint32_t) -1;

			/* next Zeiger updaten */
			ptr = os_delayed_func_search_next();
			const uint8_t sreg = SREG;
			__builtin_avr_cli();
			os_delayed_next_p = ptr;
			SREG = sreg;
		}

#ifndef OS_KERNEL_LOG_AVAILABLE
		/* Idle-Counter wird inkrementiert und 3-mal gespeichert.
		 * Durch die 2 Backups gibt es immer mindestens 2 Kopien,
		 * die den korrekten Wert beinhalten. */
		uint32_t tmp = idle_counter[0];
		tmp++;
		idle_counter[0] = tmp;
		idle_counter[1] = tmp;
		idle_counter[2] = tmp;

		// 61 Zyklen pro Durchlauf => 3812 ns @ 16 MHz, 3050 ns @ 20 MHz
#else
		if (kernel_log_fifo.count > 0) {
			kernel_log_t data;
			fifo_get_data(&kernel_log_fifo, &data, sizeof(data));
			LOG_RAW("%u\t%u>%u\n", data.time, data.from, data.to);
		}
#endif // OS_KERNEL_LOG_AVAILABLE
	}
}

/**
 * Aktualisiert den Schedule, prioritaetsbasiert
 * \param tickcount	Wert des Timer-Ticks (32 Bit)
 */
void os_schedule(uint32_t tickcount) {
	/* Mutex abfragen / setzen => Scheduler ist nicht reentrant! */
	if (test_and_set((uint8_t *) &os_scheduling_allowed, 0) != 1) {
		os_scheduling_allowed = 2;
		return;
	}

	/* Naechsten lauffaehigen Thread mit hoechster Prioritaet suchen.
	 * Das ist zwar in O(n), aber wir haben nur eine sehr beschraenkte Anzahl an Threads! */
	uint8_t i;
	Tcb_t * ptr = os_threads;
	// os_scheduling_allowed == 0, sonst waeren wir nicht hier
	for (i = os_scheduling_allowed; i < OS_MAX_THREADS; ++i, ++ptr) {
		if (ptr->stack != NULL) {
			/* Es existiert noch ein Thread in der Liste */
			if (ptr->wait_for->value == 0 && ptr->nextSchedule <= tickcount) {
				/* Der Thread ist nicht blockiert */
				if (ptr != os_thread_running) {
#ifdef OS_KERNEL_LOG_AVAILABLE
					if (kernel_log_on != 0) {
						log_entry.time = (uint16_t) tickcount;
						log_entry.from = (uint8_t) (os_thread_running - os_threads);
						log_entry.to = (uint8_t) (ptr - os_threads);
						fifo_put_data(&kernel_log_fifo, &log_entry, sizeof(log_entry));
					}
#endif // OS_KERNEL_LOG_AVAILABLE

#ifdef MEASURE_UTILIZATION
					os_thread_running->statistics.runtime += (uint16_t) ((uint16_t) tickcount - os_thread_running->lastSchedule);
#endif
					/* switch Thread */
					ptr->lastSchedule = (uint16_t) tickcount;
					//-- hier laeuft noch der alte Thread (SP zeigt auf os_thread_running's Stack) --//
					os_switch_thread(os_thread_running, ptr);
					//-- jetzt laeuft bereits der neue Thread (SP zeigt auf ptr's Stack) --//
					// => return fuehrt den NEUEN Thread weiter aus -> (noch) KEIN Ruecksprung zum Caller
					break;
				} else {
					/* aktiver Thread darf weiterlaufen */
					// => return fuehrt den ALTEN Thread weiter aus -> Funktionsaufruf mit "normaler" Rueckkehr
					break;
				}
			}
		}
	}

	/* Mutex freigeben */
	os_scheduling_allowed = 1;
	/* Ruecksprung dorthin, wo der (neu) geschedulte Thread vor SEINER Unterbrechung war,
	 * also nicht zwangsweise in die Funktion, die (direkt) vor dem Scheduler-Aufruf
	 * ausgefuehrt wurde! */
}

/**
 * Berechnet CPU und UART Auslastung
 * @param cpu Zeiger auf Ausgabeparameter fuer CPU-Auslastung
 * @param uart_in Zeiger auf Ausgabeparameter fuer UART-Auslastung eingehend
 * @param uart_out Zeiger auf Ausgabeparameter fuer UART-Auslastung ausgehend
 */
void os_get_utilizations(uint8_t* cpu, uint8_t* uart_in, uint8_t* uart_out) {
	static uint32_t last_time = 0;
	static uint32_t last_idle = 0;
#ifdef UART_AVAILABLE
	static uint32_t last_uart_in = 0;
	static uint32_t last_uart_out = 0;
#endif

	/* CPU-Auslastung berechnen (Durchschnitt seit letztem Aufruf) */
	const uint32_t time = TIMER_GET_TICKCOUNT_32;

	/* idle_counter kann nicht atomar inkrementiert werden und wir wissen nicht, bei welcher Instruktion
	 * der idle-Thread unterbrochen wurde. Da es aber 3 Kopien des Idle-Zaehlers gibt, muessen immer
	 * mindesten zwei den korrekten Wert enthalten. Genau dieses Paar suchen wir nun und verwenden den Wert. */
	uint32_t idle = idle_counter[0];
	uint32_t idle2 = idle_counter[1];
	uint32_t idle3 = idle_counter[2];
	if (idle != idle2 && idle != idle3) {
		idle = idle2; // idle enthaelt den falschen Wert, also sind idle2 und idle3 korrekt
	}

	const uint32_t time_diff = time - last_time;
	const uint32_t idle_diff = idle - last_idle;
	last_time = time;
	last_idle = idle;
	const uint32_t idle_ticks = (uint32_t) ((float) idle_diff * (ZYCLES_PER_IDLERUN * 1000000.f / F_CPU / TIMER_STEPS));
	const uint8_t idle_pc = (uint8_t) (idle_ticks * 100 / time_diff);

	*cpu = (uint8_t) (100 - idle_pc);
#ifdef UART_AVAILABLE
	const uint8_t sreg = SREG;
	__builtin_avr_cli();
	const uint32_t uart_in_tmp = uart_infifo.written;
	const uint32_t uart_out_tmp = uart_outfifo.written;
	SREG = sreg;
	const uint32_t uart_diff_in = uart_in_tmp - last_uart_in;
	last_uart_in = uart_in_tmp;
	const uint32_t uart_diff_out = uart_out_tmp - last_uart_out;
	last_uart_out = uart_out_tmp;
	const uint16_t uart_max = UART_BAUD / 10;

	*uart_in = (uint8_t) ((float) uart_diff_in / (float) time_diff / (176.f / 1000000.f / 100.f) / uart_max); // %
	*uart_out = (uint8_t) ((float) uart_diff_out / (float) time_diff / (176.f / 1000000.f / 100.f) / uart_max); // %
#else
	*uart_in = 0;
	*uart_out = 0;
#endif // UART_AVAILABLE
}

#ifdef DISPLAY_OS_AVAILABLE
/**
 * Handler fuer OS-Display
 */
void os_display(void) {
	uint8_t i;
#ifndef OS_KERNEL_LOG_AVAILABLE
	static char display_buf[20];

	uint8_t cpu_pc, uart_pc_in, uart_pc_out;
	os_get_utilizations(&cpu_pc, &uart_pc_in, &uart_pc_out);

	/* Balken fuer Auslastung ausgeben */
	for (i = 0; i < cpu_pc / 5; ++i) {
		display_buf[i] = (char) 0xff; // schwarzes Feld
	}

	/* Spaces fuer Idle ausgeben */
	for (; i < 20; ++i) {
		display_buf[i] = 0x10; // Space
	}
	display_cursor(1, 1);
	display_printf("%s", display_buf);

	display_cursor(2, 1);
	display_printf("CPU%3u%% I/O%3u%%|%3u%%", cpu_pc, uart_pc_in, uart_pc_out);
#else
	display_cursor(1, 1);
	display_puts("4: Kernel-LOG ");
	if (kernel_log_on == 0) {
		display_puts("an ");
	} else {
		display_puts("aus");
	}
#endif // OS_KERNEL_LOG_AVAILABLE

	display_cursor(3, 1);
	display_puts("dump Stacks: 1: Main");
	display_cursor(4, 6);
#ifdef MAP_AVAILABLE
	display_puts("2: Idle 3: Map");
#else
	display_puts("2: Idle");
#endif

#ifdef OS_DEBUG
	/* Debug-Info zum freien Stackspeicher ausgeben */
	os_print_stackusage();

	extern unsigned char * __brkval;

	/* Idle-Thread suchen */
	Tcb_t * pIdle = os_threads;
	for (i = 0; i < OS_MAX_THREADS - 1; ++i) {
		if ((pIdle + 1)->stack != NULL) {
			pIdle++;
		}
	}
#endif // OS_DEBUG

#ifdef RC5_AVAILABLE
	/* Keyhandler */
	switch (RC5_Code) {
	case RC5_CODE_1:
#ifdef OS_DEBUG
		os_stack_dump(&os_threads[0], (unsigned char *) RAMEND, (uint16_t) ((unsigned char *) RAMEND - __brkval));
#endif
		RC5_Code = 0;
		break;

	case RC5_CODE_2:
#ifdef OS_DEBUG
		os_stack_dump(pIdle, &os_idle_stack[OS_IDLE_STACKSIZE - 1], OS_IDLE_STACKSIZE);
#endif
		RC5_Code = 0;
		break;

#ifdef MAP_AVAILABLE
	case RC5_CODE_3:
#ifdef OS_DEBUG
		os_stack_dump(&os_threads[1], &map_update_stack[MAP_UPDATE_STACK_SIZE - 1], MAP_UPDATE_STACK_SIZE);
#endif
		RC5_Code = 0;
		break;
#endif // MAP_AVAILABLE

#ifdef OS_KERNEL_LOG_AVAILABLE
	case RC5_CODE_4:
		kernel_log_on = (uint8_t) ~kernel_log_on;
		RC5_Code = 0;
		break;
#endif // OS_KERNEL_LOG_AVAILABLE
	}
#endif // RC5_AVAILABLE
}
#endif // DISPLAY_OS_AVAILABLE

#endif // OS_AVAILABLE
#endif // MCU

