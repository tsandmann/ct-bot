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
 * @file 	timer.c
 * @brief 	Zeitmanagement
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#include "ct-Bot.h"
#include "timer.h"
#include "sensor.h"


#ifdef MCU
volatile tickCount_t tickCount;		/*!< ein Tick alle 176 us */
#else // PC
volatile float tickCount = 0;		/*!< ein Tick alle 176 us */
#endif // MCU

#ifdef PC
/*!
 * liefert Ticks in 16 Bit seit Systemstart [176 us]
 */
uint16_t timer_get_tickCount16(void) {
	uint16_t temp;
	temp = tickCount;
	return temp;
}

/*!
 * liefert Ticks in 32 Bit seit Systemstart [176 us]
 */
uint32_t timer_get_tickCount32(void) {
	uint32_t temp;
	temp = tickCount;
	return temp;
}
#endif // PC

#ifdef TIME_AVAILABLE
/*!
 * Diese Funktion liefert den Millisekundenanteil der Systemzeit zurueck.
 * @return Millisekunden der Systemzeit
 */
uint16_t timer_get_ms(void) {
	return (uint16_t) (((TIMER_GET_TICKCOUNT_32 * (TIMER_STEPS / 8)) / (1000 / 8)) % 1000);
}

/*!
 * Diese Funktion liefert den Sekundenanteil der Systemzeit zurueck.
 * @return Sekunden der Systemzeit
 */
uint16_t timer_get_s(void) {
	return (uint16_t) (TIMER_GET_TICKCOUNT_32 * (TIMER_STEPS / 16) / (1000000 / 16));
}

/*!
 * Liefert die Millisekunden zurueck, die seit old_s, old_ms verstrichen sind
 * @param old_s		alter Sekundenstand
 * @param old_ms	alter Millisekundenstand
 */
uint16_t timer_get_ms_since(uint16_t old_s, uint16_t old_ms) {
	return (timer_get_s() - old_s) * 1000 + timer_get_ms() - old_ms;
}
#endif // TIME_AVAILABLE
