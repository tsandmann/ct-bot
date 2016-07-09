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
 * \file 	delay.c
 * \brief 	Hilfsroutinen fuer Wartezeiten
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#ifdef MCU

#include "ct-Bot.h"
#include "delay.h"
#include "timer.h"

/**
 * Verzoegert um ms Millisekunden
 * \param ms Anzahl der Millisekunden
 */
void delay(uint16_t ms) {
	uint32_t start = TIMER_GET_TICKCOUNT_32;
	uint32_t ticksToWait = MS_TO_TICKS((uint32_t) ms);
	uint32_t now;
	do {
		now = TIMER_GET_TICKCOUNT_32;
	} while (now - start < ticksToWait);

}
#endif // MCU
