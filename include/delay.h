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
 * \file 	delay.h
 * \brief 	Hilfsroutinen fuer Wartezeiten
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */
#ifndef DELAY_H_
#define DELAY_H_

/**
 * Verzoegert um ms Millisekunden
 * \param ms Anzahl der Millisekunden
 */
void delay(uint16_t ms);

#ifdef MCU
#include "builtins.h"

/**
 * Verzoegert um us Mikrosekunden
 * \param us Anzahl der Mikrosekunden
 */
static inline ALWAYS_INLINE void delay_us(const uint32_t us) {
	const uint32_t cycles = F_CPU / 1000000UL * us;
	__builtin_avr_delay_cycles(cycles);
}
#endif // MCU
#endif // DELAY_H_
