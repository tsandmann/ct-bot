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
 * \author	Timo Sandmann
 * \date 	23.10.2016
 *
 * For use with Arduino SdFat library by William Greiman (https://github.com/greiman/SdFat)
 */

#ifndef SPIMASTER_SOFT_H_
#define SPIMASTER_SOFT_H_

#ifdef MCU
#include <stdint.h>
#include <avr/io.h>

extern "C" {
#include "timer.h"
#include "os_thread.h"
#include "log.h"
#include "motor.h"
}

/**
 * Software emulation SPI master driver for ATmega using bit banging
 */
class SpiMasterSoft {
protected:
	/**
	 * Initializes the SPI master
	 * \note Sets MOSI and SCK pins as output and MISO as input
	 */
	void init() {
		/* Set MOSI and SCK output, MISO input */
		uint8_t ddrb = DDRB;
		ddrb |=  _BV(DDB5) | _BV(DDB7);
		ddrb = (uint8_t) (ddrb & ~_BV(DDB6));
		DDRB = ddrb;
	}

	/**
	 * Receives a byte from the SPI bus
	 * \return The received data byte
	 * \note 0xff is sent out
	 */
	uint8_t receive() const {
		disable_servo();

		uint8_t clk_high, clk_low, port_data, data;
		__asm__ __volatile__(
			"in	%2, %4		; load PORTB 		\n\t"
			"cbr %2, %9		; PB3 low			\n\t"
			"cbr %2, %7		; CLK low			\n\t"
			"sbr %2, %8		; MOSI high			\n\t"
			"out %4, %2		; CLK low			\n\t"
			"mov %1, %2		; CLK high			\n\t"
			"sbr %1, %7							\n\t"
			"in %3, %5		; load bit 7 		\n\t"
			"out %4, %1		; CLK edge			\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6		; save bit 7  		\n\t"
			"bld %0, 7							\n\t"
			"in %3, %5		; bit 6				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 6							\n\t"
			"in %3, %5		; bit 5				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 5							\n\t"
			"in %3, %5		; bit 4				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 4							\n\t"
			"in %3, %5		; bit 3				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 3							\n\t"
			"in %3, %5		; bit 2				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 2							\n\t"
			"in %3, %5		; bit 1				\n\t"
			"out %4, %1							\n\t"
			"out %4, %2							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 1							\n\t"
			"in %3, %5		; bit 0				\n\t"
			"out %4, %1							\n\t"
			"bst %3, %6							\n\t"
			"bld %0, 0							    "
			: "=&r" (data) /* %0 */, "=&r" (clk_high) /* %1 */, "=&r" (clk_low) /* %2 */, "=&r" (port_data) /* %3 */
			: "M" (_SFR_IO_ADDR(PORTB)) /* %4 */, "M" (_SFR_IO_ADDR(PINB)) /* %5 */, "M" (PB6) /* %6 */, "M" (_BV(PB7)) /* %7 */, "M" (_BV(PB5)) /* %8 */, "M" (_BV(PB3)) /* %9 */
			: "memory"
		);

		return data;
	}

	/**
	 * Receives n byte from the SPI bus
	 * \param[out] buf Pointer to buffer for the received bytes (with space for at least n byte)
	 * \param[in] n Number of bytes to receive
	 */
	void ALWAYS_INLINE receive(uint8_t* buf, size_t n) const {
		if (n != 512) {
			receive_n(buf, n);
		} else {
			receive_512(buf);
		}
	}

	/**
	 * Sends a byte to the SPI bus.
	 * \param[in] data The data byte to send
	 */
	void send(uint8_t data) const {
		disable_servo();

		uint8_t tmp;
		__asm__ __volatile__(
			"in %0, %2		; load PORTB 		\n\t"
			"cbr %0, %6		; PB3 low			\n\t"
			"cbr %0, %5		; CLK low			\n\t"
			"bst %1, 7		; send bit 7			\n\t"
			"bld %0, %3		; data to DO			\n\t"
			"out %2, %0		; send data	 		\n\t"
			"sbi %2, %4		; CLK  high			\n\t"
			"bst %1, 6		; bit 6				\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 5		; bit 5				\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 4		; bit 4				\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 3		; bit 3				\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 2		; bit 2				\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 1		; bit 1				\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"bst %1, 0		; bit 0 				\n\t"
			"bld %0, %3							\n\t"
			"out %2, %0							\n\t"
			"sbi %2, %4							\n\t"
			"sbi %2, %3		; DO high		    	"
			: "=&r" (tmp) /* %0 */
			: "r" (data) /* %1 */, "M" (_SFR_IO_ADDR(PORTB)) /* %2 */, "M" (PB5) /* %3 */, "M" (PB7) /* %4 */, "M" (_BV(PB7)) /* %5 */, "M" (_BV(PB3)) /* %6 */
			: "memory"
		);
	}

	/**
	 * Sends n byte to the SPI bus
	 * \param[in] buf Pointer to buffer for data to send
	 * \param[in] n Number of bytes to send
	 */
	void ALWAYS_INLINE send(const uint8_t* buf, size_t n) const {
		if (n != 512) {
			send_n(buf, n);
		} else {
			send_512(buf);
		}
	}

	/**
	 * Waits until data != 0xff received from the SPI bus or timeout occurs
	 * \param[in] timeout_ms Maximum wait time in ms
	 * \return false, iff timeout; true otherwise
	 */
	bool wait_not_busy(uint16_t timeout_ms) const {
		const auto starttime(TIMER_GET_TICKCOUNT_16);
		auto yield_start_time(starttime);
		const uint16_t timeout_ticks(timeout_ms * (1000U / TIMER_STEPS + 1));
		while (this->receive() != 0xff) {
			const auto now16(TIMER_GET_TICKCOUNT_16);
			if (static_cast<uint16_t>(now16 - starttime) > timeout_ticks) {
				return false;
			}

			if (static_cast<uint16_t>(now16 - yield_start_time) > 1 * (1000U / TIMER_STEPS + 1)) {
				os_exitCS();
//				LOG_DEBUG("wait_not_busy(): yieldtime: %u %u", now16 - yield_start_time, timeout_ms);
				os_enterCS();
				yield_start_time = now16;
			}
		}
//		LOG_DEBUG("wait_not_busy(): took: %u %u", TIMER_GET_TICKCOUNT_16 - starttime, timeout_ms);
		return true;
	}

	/**
	 * Sets the SPI bus speed to fraction of F_CPU: speed = F_CPU / (2 * divisor)
	 * \note Currently not implemented for SpiMasterSoft!
	 */
	void set_speed(uint8_t) {}

private:
	/**
	 * Receives n byte from the SPI bus, used for n != 512
	 * \param[out] buf Pointer to buffer for the received bytes (with space for at least n byte)
	 * \param[in] n Number of bytes to be received
	 */
	void receive_n(uint8_t* buf, size_t n) const {
		for (size_t i(0); i < n; ++i) {
			buf[i] = this->receive();
		}
	}

	/**
	 * Receives 512 byte from the SPI bus, optimized version
	 * \param[out] buf Pointer to buffer for the received bytes (with space for at least 512 byte)
	 */
	void receive_512(void* buf) const {
		disable_servo();

		__asm__ __volatile__(
			"2:								\n\t"
			"in	r20, %0		; load PORTB 	\n\t"
			"cbr r20, %6		; PB3 low		\n\t"
			"cbr r20, %1		; CLK low		\n\t"
			"sbr r20, %5		; MOSI high		\n\t"
			"out %0, r20		; CLK low		\n\t"
			"mov r19, r20	; CLK high		\n\t"
			"sbr r19, %1						\n\t"
			"ldi r18, 2		; load loop-var 	\n\t"
			"clr r24							\n\t"
			"1:								\n\t"
			"in r26, %3		; load bit 7 	\n\t"
			"out %0, r19		; CLK edge		\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2		; store bit 7 	\n\t"
			"bld r25, 7						\n\t"
			"in r26, %3		; bit 6			\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 6						\n\t"
			"in r26, %3		; bit 5			\n\t"
			"out %0, r19						\n\t"
			"out %0 ,r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 5						\n\t"
			"in r26, %3		; bit 4			\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 4						\n\t"
			"in r26, %3		; bit 3			\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 3						\n\t"
			"in r26, %3		; bit 2			\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 2						\n\t"
			"in r26, %3		; bit 1			\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 1						\n\t"
			"in r26, %3		; bit 0			\n\t"
			"out %0, r19						\n\t"
			"out %0, r20						\n\t"
			"bst r26, %2						\n\t"
			"bld r25, 0						\n\t"
			"st Y+, r25	; copy byte to SRAM \n\t"
			"inc r24		; r24++				\n\t"
			"breq 2f		; r24 == 0?			\n\t"
			"rjmp 1b							\n\t"
			"2:								\n\t"
			"dec r18		; r18--				\n\t"
			"breq 3f		; r18 == 0?			\n\t"
			"rjmp 1b							\n\t"
			"3:								\n\t"
			"out %0, r19	; CLK high				"
			:: "M" (_SFR_IO_ADDR(PORTB)) /* %0 */, "M" (_BV(PB7)) /* %1 */, "M" (PB6) /* %2 */, "M" (_SFR_IO_ADDR(PINB)) /* %3 */, "y" (buf) /* %4 */, "M" (_BV(PB5)) /* %5 */, "M" (_BV(PB3)) /* %6 */
			: "r18", "r19", "r20", "r24", "r25", "r26", "memory"
		);
	}

	/**
	 * Sends n byte to the SPI bus, used for n != 512
	 * \param[out] buf Pointer to buffer for data to send
	 * \param[in] n Number of bytes to send
	 */
	void send_n(const uint8_t* buf, size_t n) const {
		for (size_t i(0); i < n; ++i) {
			this->send(buf[i]);
		}
	}

	/**
	 * Sends 512 byte to the SPI bus, optimized version
	 * \param[out] buf Pointer to buffer for the data to send
	 */
	void send_512(const void* buf) const {
		disable_servo();

		__asm__ __volatile__(
			"in r26, %0		; load PORTB 			\n\t"
			"cbr r26, %5		; PB3 low				\n\t"
			"cbr r26, %1		; CLK low				\n\t"
			"ldi r18, 2		; r18 = 2				\n\t"
			"clr r27			; r27 = 0				\n\t"
			"1:										\n\t"
			"ld r25, Z+		; load byte from SRAM 	\n\t"
			"bst r25, 7		; send bit 7				\n\t"
			"bld r26, %2								\n\t"
			"out %0, r26		; send data				\n\t"
			"sbi %0, %3		; CLK high				\n\t"
			"bst r25, 6		; bit 6					\n\t"
			"bld r26, %2								\n\t"
			"out %0, r26								\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 5		; bit 5					\n\t"
			"bld r26, %2								\n\t"
			"out %0, r26								\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 4		; bit 4					\n\t"
			"bld r26, %2								\n\t"
			"out %0, r26								\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 3		; bit 3					\n\t"
			"bld r26, %2								\n\t"
			"out %0, r26								\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 2		; bit 2					\n\t"
			"bld r26, %2								\n\t"
			"out %0, r26								\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 1		; bit 1					\n\t"
			"bld r26, %2								\n\t"
			"out %0, r26								\n\t"
			"sbi %0, %3								\n\t"
			"bst r25, 0		; bit 0					\n\t"
			"bld r26, %2								\n\t"
			"out %0, r26								\n\t"
			"sbi %0, %3								\n\t"
			"inc r27			; r27++					\n\t"
			"breq 2f			; r27 == 0?				\n\t"
			"rjmp 1b									\n\t"
			"2:										\n\t"
			"dec r18			; r18--					\n\t"
			"breq 3f			; r18 == 0?				\n\t"
			"rjmp 1b									\n\t"
			"3:										\n\t"
			"sbi %0, %2		; DO high					"
			:: "M" (_SFR_IO_ADDR(PORTB)) /* %0 */, "M" (_BV(PB7)) /* %1 */, "M" (PB5) /* %2 */,	"M" (PB7) /* %3 */, "z" (buf) /* %4 */, "M" (_BV(PB3)) /* %5 */
			: "r18", "r25", "r26", "r27", "memory"
		);
	}

	/**
	 * Sets SERVO1 to OFF on ATmega1284P
	 */
	void disable_servo() const {
#ifdef __AVR_ATmega1284P__
		servo_set(SERVO1, SERVO_OFF);
#endif
	}
};

#endif // MCU
#endif /* SPIMASTER_SOFT_H_ */
