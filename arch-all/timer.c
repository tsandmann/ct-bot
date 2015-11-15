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
 * \file 	timer.c
 * \brief 	Zeitmanagement
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#include "ct-Bot.h"
#include "timer.h"
#include "sensor.h"

#ifdef MCU
volatile tickCount_t tickCount; /**< ein Tick alle 176 us */
#else // PC
#ifdef ARM_LINUX_BOARD
uint_fast32_t tickCount = 0; /**< ein Tick alle 176 us */
#else
double tickCount = 0; /**< ein Tick alle 176 us */
#endif // ARM_LINUX_BOARD
#endif // MCU

#ifdef PC
/**
 * liefert Ticks in 16 Bit seit Systemstart [176 us]
 */
uint16_t timer_get_tickCount16(void) {
	uint16_t temp;
	temp = (uint16_t) tickCount;
	return temp;
}

/**
 * liefert Ticks in 32 Bit seit Systemstart [176 us]
 */
uint32_t timer_get_tickCount32(void) {
	uint32_t temp;
	temp = (uint32_t) tickCount;
	return temp;
}
#endif // PC

/**
 * Diese Funktion liefert den Millisekundenanteil der Systemzeit zurueck.
 * \return Millisekunden der Systemzeit
 */
uint16_t timer_get_ms(void) {
	return (uint16_t) (((float) TIMER_GET_TICKCOUNT_32 * ((float) TIMER_STEPS / 1000.0f))) % 1000;
}

/**
 * Diese Funktion liefert den Sekundenanteil der Systemzeit zurueck.
 * \return Sekunden der Systemzeit
 */
uint16_t timer_get_s(void) {
	return (uint16_t) ((float) TIMER_GET_TICKCOUNT_32 * ((float) TIMER_STEPS / 1000000.0f));
}
