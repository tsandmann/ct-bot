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
 * @file 	delay.h
 * @brief 	Hilfsroutinen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.2005
 */
#ifndef delay_H_
#define delay_H_

/*!
 * Warte 100 ms
 */
#define delay_100ms()	delay(100)

/*!
 * Verzoegert um ms Millisekunden
 * @param ms Anzahl der Millisekunden
 */
void delay(uint16_t ms);

#ifdef MCU
/*!
 * Delay loop using a 16-bit counter so up to 65536 iterations are possible.
 * (The value 65536 would have to be passed as 0.)
 * The loop executes four CPU cycles per iteration, not including the overhead
 * the compiler requires to setup the counter register pair.
 * Thus, at a CPU speed of 1 MHz, delays of up to about 262.1 milliseconds can be achieved.
 * @param __count	1/4 CPU-Zyklen
 */
static inline void _delay_loop_2(uint16_t __count) {
	__asm__ __volatile__(
		"1: sbiw %0,1" "\n\t"
		"brne 1b"
		: "=w" (__count)
		: "0" (__count)
	);
}
#endif	// MCU
#endif	// delay_H_
