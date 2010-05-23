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
 * @file 	trace.c
 * @brief 	Trace-Modul
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	01.06.2009
 */

#include "trace.h"

#ifdef CREATE_TRACEFILE_AVAILABLE
#include "fifo.h"
#include "sensor.h"
#include "bot-logic/bot-logik.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static fifo_t trace_fifo;
static uint8_t trace_entries[255];
static char trace_buffer[sizeof(trace_entries)][255];
static uint8_t buf_index;
static long time_overflow;
static pthread_t trace_thread;
static FILE * trace_file = NULL;

void * trace_main(void * data);

/*!
 * Trace-Thread, der die Daten des Puffers in eine Datei schreibt
 * @param *data	Dummy fuer pthraed_create()
 * @return		NULL (macht pthread_create() gluecklich)
 */
void * trace_main(void * data) {
	data = data; // kein warning
	uint8_t index = 0;
	while (42) {
		fifo_get_data(&trace_fifo, &index, sizeof(index)); // blockierend
		fwrite(trace_buffer[index], strlen(trace_buffer[index]), 1, trace_file);
		trace_buffer[index][0] = 0;
	}
}

/*!
 * Initialisiert das Trace-System
 */
void trace_init(void) {
	trace_file = fopen("trace.txt", "wb");
	if (trace_file != NULL) {
		fifo_init(&trace_fifo, trace_entries, sizeof(trace_entries));
		pthread_create(&trace_thread, NULL, trace_main, NULL);
		buf_index = 0;
		time_overflow = 0;
	}
}

/*!
 * Fuegt dem Trace-Puffer die aktuellen Sensordaten hinzu
 */
void trace_add_sensors(void) {
	if (simultime == 0) {
		time_overflow++;
	}
	long time = simultime + time_overflow * 10000;
	int n = snprintf(trace_buffer[buf_index], sizeof(trace_buffer[0]), "time=\t%ld\tencL=\t%+d\tencR=\t%+d\tsensDistL=\t%+d\tsensDistR=\t%+d", time, sensEncL, sensEncR, sensDistL, sensDistR);
	n += snprintf(&trace_buffer[buf_index][n], sizeof(trace_buffer[0]) - n, "\theading=\t%+.12f\tx_enc=\t%+.12f\ty_enc=\t%+.12f\n", heading, x_enc, y_enc);

	fifo_put_data(&trace_fifo, &buf_index, sizeof(buf_index));
	buf_index++;
	buf_index %= sizeof(trace_entries);
}

/*!
 * Fuegt dem Tace-Puffer die aktuellen Aktuatordaten hinzu
 */
void trace_add_actuators(void) {
	long time = simultime + time_overflow * 10000;
	int n = snprintf(trace_buffer[buf_index], sizeof(trace_buffer[0]), "time=\t%ld\tmotorL=\t%+d\tmotorR=\t%+d", time, motor_left, motor_right);

	Behaviour_t * ptr = get_next_behaviour(NULL);
	do {
		if (ptr->active == ACTIVE) {
			n += snprintf(&trace_buffer[buf_index][n], sizeof(trace_buffer[0]) - n, "\tbeh=\t%u", ptr->priority);
		}
	} while ((ptr = get_next_behaviour(ptr)) != NULL);

	snprintf(&trace_buffer[buf_index][n], sizeof(trace_buffer[0]) - n, "\n");

	fifo_put_data(&trace_fifo, &buf_index, sizeof(buf_index));
	buf_index++;
	buf_index %= sizeof(trace_entries);
}

#endif	// CREATE_TRACEFILE_AVAILABLE
