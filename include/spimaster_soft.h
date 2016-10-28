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
 * \file 	spimaster_soft.h
 * \brief 	Software SPI master driver
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	23.10.2016
 *
 * For use with Arduino SdFat library by William Greiman (https://github.com/greiman/SdFat).
 */

#ifndef SPIMASTER_SOFT_H_
#define SPIMASTER_SOFT_H_

#ifdef MCU
#include <stdint.h>
#include <avr/io.h>

extern "C" {
#include "timer.h"
}

class SpiMasterSoft {
protected:
	void init() {
		/* Set SS, MOSI and SCK output, MISO input */
		PORTB |= _BV(PB4); // SS high
		uint8_t ddrb = DDRB;
		ddrb |=  _BV(DDB5) | _BV(DDB7) | _BV(DDB4);
		ddrb = (uint8_t) (ddrb & ~_BV(DDB6));
		DDRB = ddrb;
	}

	uint8_t receive() const {
		uint8_t clk_high, clk_low, port_data, data;
		__asm__ __volatile__(
			"in	%2, %4		; PORTB einlesen	\n\t"
			"cbr %2, %7		; CLK low			\n\t"
			"out %4, %2		; CLK auf low		\n\t"
			"mov %1, %2		; CLK high			\n\t"
			"sbr %1, %7							\n\t"
			"in %3, %5		; Bit 7 lesen		\n\t"
			"out %4, %1		; CLK Flanke		\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6		; Bit 7 speichern	\n\t"
			"bld %0, 7							\n\t"
			"in %3, %5		; Bit 6				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 6							\n\t"
			"in %3, %5		; Bit 5				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 5							\n\t"
			"in %3, %5		; Bit 4				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 4							\n\t"
			"in %3, %5		; Bit 3				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 3							\n\t"
			"in %3, %5		; Bit 2				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 2							\n\t"
			"in %3, %5		; Bit 1				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 1							\n\t"
			"in %3, %5		; Bit 0				\n\t"
			"out %4, %1							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 0							    "
			: "=&r" (data) /* %0 */, "=&r" (clk_high) /* %1 */, "=&r" (clk_low) /* %2 */, "=&r" (port_data) /* %3 */
			: "M" (_SFR_IO_ADDR(PORTB)) /* %4 */, "M" (_SFR_IO_ADDR(PINB)) /* %5 */, "M" (PB6) /* %6 */, "M" (_BV(PB7)) /* %7 */
			: "memory"
		);

		return data;
	}

	void __attribute__((always_inline)) receive(uint8_t* buf, size_t n) const {
		if (n != 512) {
			receive_n(buf, n);
		} else {
			receive_512(buf);
		}
	}

	void send(uint8_t data) const {
		uint8_t tmp;
		__asm__ __volatile__(
			"in %0, %2		; PORTB einlesen	\n\t"
			"cbr %0, %5		; CLK auf low		\n\t"
			"bst %1, 7		; Bit 7	der Daten	\n\t"
			"bld %0, %3		; Daten nach DO		\n\t"
			"out %2, %0		; Daten senden		\n\t"
			"sbi %2, %4		; CLK auf high		\n\t"
			"bst %1, 6		; Bit 6	der Daten	\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 5		; Bit 5	der Daten	\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 4		; Bit 4	der Daten	\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 3		; Bit 3	der Daten	\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 2		; Bit 2	der Daten	\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 1		; Bit 1	der Daten	\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 0		; Bit 0 der Daten	\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"sbi %2, %3		; DO auf high		    "
			: "=&r" (tmp) /* %0 */
			: "r" (data) /* %1 */, "M" (_SFR_IO_ADDR(PORTB)) /* %2 */, "M" (PB5) /* %3 */, "M" (PB7) /* %4 */, "M" (_BV(PB7)) /* %5 */
			: "memory"
		);
	}

	void __attribute__((always_inline)) send(const uint8_t* buf, size_t n) const {
		if (n != 512) {
			send_n(buf, n);
		} else {
			send_512(buf);
		}
	}

	bool wait_not_busy(uint16_t timeout_ms) const {
		const auto starttime(TIMER_GET_TICKCOUNT_16);
		const uint16_t timeout_ticks(timeout_ms * (1000U / TIMER_STEPS + 1));
		while (this->receive() != 0xff) {
			if (static_cast<uint16_t>(tickCount.u16 - starttime) > timeout_ticks) {
				return false;
			}
		}
		return true;
	}

	void set_speed(uint8_t) {}

private:
	void receive_n(uint8_t* buf, size_t n) const {
		for (size_t i(0); i < n; ++i) {
			buf[i] = this->receive();
		}
	}

	void receive_512(void* buf) const {
		__asm__ __volatile__(
			"2:									\n\t"
			"in	r20, %0		; PORTB einlesen	\n\t"
			"cbr r20, %1	; CLK low			\n\t"
			"out %0, r20	; CLK auf low		\n\t"
			"mov r19, r20	; CLK high			\n\t"
			"sbr r19, %1						\n\t"
			"ldi r18, 2		; load loop-var 	\n\t"
			"clr r24							\n\t"
			"1:									\n\t"
			"in r26, %3		; Bit 7 lesen		\n\t"
			"out %0, r19	; CLK Flanke		\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2	; Bit 7 speichern	\n\t"
			"bld r25, 7							\n\t"
			"in r26, %3		; Bit 6				\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 6							\n\t"
			"in r26, %3		; Bit 5				\n\t"
			"out %0, r19						\n\t"
			"out %0 ,r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 5							\n\t"
			"in r26, %3		; Bit 4				\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 4							\n\t"
			"in r26, %3		; Bit 3				\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 3							\n\t"
			"in r26, %3		; Bit 2				\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 2							\n\t"
			"in r26, %3		; Bit 1				\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 1							\n\t"
			"in r26, %3		; Bit 0				\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 0							\n\t"
			"st Y+, r25		; copy byte to SRAM \n\t"
			"inc r24		; r24++				\n\t"
			"breq 2f		; r24 == 0?			\n\t"
			"rjmp 1b							\n\t"
			"2:									\n\t"
			"dec r18		; r18--				\n\t"
			"breq 3f		; r18 == 0?			\n\t"
			"rjmp 1b							\n\t"
			"3:									\n\t"
			"out %0, r19	; CLK auf high			"
			:: "M" (_SFR_IO_ADDR(PORTB)) /* %0 */, "M" (_BV(PB7)) /* %1 */, "M" (PB6) /* %2 */, "M" (_SFR_IO_ADDR(PINB)) /* %3 */, "y" (buf) /* %4 */
			: "r18", "r19", "r20", "r24", "r25", "r26", "memory"
		);
	}

	void send_n(const uint8_t* buf, size_t n) const {
		for (size_t i(0); i < n; ++i) {
			this->send(buf[i]);
		}
	}

	void send_512(const void* buf) const {
		__asm__ __volatile__(
			"in r26, %0		; PORTB einlesen		\n\t"
			"cbr r26, %1	; CLK auf low			\n\t"
			"ldi r18, 2		; r18 = 2				\n\t"
			"clr r27		; r27 = 0				\n\t"
			"1:										\n\t"
			"ld r25, Y+		; read byte from SRAM 	\n\t"
			"bst r25, 7		; Bit 7					\n\t"
			"bld r26, %2							\n\t"
			"out %0, r26	; Daten senden			\n\t"
			"sbi %0, %3		; CLK auf high			\n\t"
			"bst r25, 6		; Bit 6					\n\t"
			"bld r26, %2							\n\t"
			"out %0, r26							\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 5		; Bit 5					\n\t"
			"bld r26, %2							\n\t"
			"out %0, r26							\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 4		; Bit 4					\n\t"
			"bld r26, %2							\n\t"
			"out %0, r26							\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 3		; Bit 3					\n\t"
			"bld r26, %2							\n\t"
			"out %0, r26							\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 2		; Bit 2					\n\t"
			"bld r26, %2							\n\t"
			"out %0, r26							\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 1		; Bit 1					\n\t"
			"bld r26, %2							\n\t"
			"out %0, r26							\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 0		; Bit 0					\n\t"
			"bld r26, %2							\n\t"
			"out %0, r26							\n\t"
			"sbi %0, %3								\n\t"
			"inc r27		; r27++					\n\t"
			"breq 2f		; r27 == 0?				\n\t"
			"rjmp 1b								\n\t"
			"2:										\n\t"
			"dec r18		; r18--					\n\t"
			"breq 3f		; r18 == 0?				\n\t"
			"rjmp 1b								\n\t"
			"3:										\n\t"
			"sbi %0, %2		; DO auf high				"
			:: "M" (_SFR_IO_ADDR(PORTB)) /* %0 */, "M" (_BV(PB7)) /* %1 */, "M" (PB5) /* %2 */,
			"M" (PB7) /* %3 */, "y" (buf) /* %4 */
			: "r18", "r25", "r26", "r27", "memory"
		);
	}
};

#endif // MCU
#endif /* SPIMASTER_SOFT_H_ */