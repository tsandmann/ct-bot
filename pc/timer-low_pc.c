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
 * @file 	timer-low_pc.c
 * @brief 	Timer und Counter
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.2005
 */

#ifdef PC

#include "ct-Bot.h"
#include "timer.h"
#include "sensor.h"

/*!
 * initialisiert Timer 2 und startet ihn
 */
void timer_2_init(void) {
	// Dummy
}

/*!
 * Funktion, die die TickCounts um die vergangene Simulzeit erhoeht
 */
void system_time_isr(void) {
//	LOG_DEBUG("simultime=%d", simultime);
	/* TickCounter [176 us] erhoehen */
	static int last_simultime = -11; // kommt vom Sim zuerst als -1, warum auch immer!?!
	int tmp = simultime - last_simultime;
	if (tmp < 0) tmp += 10000; // der Sim setzt simultime alle 10s zurueck auf 0
	tickCount += MS_TO_TICKS((float) tmp);
	last_simultime = simultime;
}

/*
 * Setzt die Systemzeit zurueck auf 0
 */
void timer_reset(void) {
	tickCount = 0;
}
#endif // PC
