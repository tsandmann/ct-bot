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
 * @file 	os_utils.h
 * @brief 	Hilfsfunktionen fuer BotOS
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	02.10.2007
 */

#ifndef _OS_UTILS_H
#define _OS_UTILS_H

#include "ct-Bot.h"
#ifdef MCU
#ifdef OS_AVAILABLE

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/*!
 * Setzt die Variable var auf den Wert x und gibt den alten Wert von var zurueck.
 * Die Operation ist atomar also interrupt- und threadsicher
 * @param *var	Zeiger auf die Variable (8 Bit)
 * @param x		Wert, der anschliessend in var stehen soll
 * @return		alter Wert von var
 */
static inline uint8_t test_and_set(uint8_t * var, uint8_t x) {
	uint8_t sreg = SREG;
	__asm__ __volatile__("cli":::"memory");
	uint8_t old = *var;
	*var = x;
	__asm__ __volatile__("":::"memory");
	SREG = sreg;
	return old;
}

#endif	// OS_AVAILABLE
#endif	// MCU
#endif	// _OS_UITLS_H
