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
 * \file 	spimaster.h
 * \brief 	SPI master driver
 * \author	Timo Sandmann
 * \date 	23.10.2016
 *
 * For use with Arduino SdFat library by William Greiman (https://github.com/greiman/SdFat)
 */

#ifndef SPIMASTER_H_
#define SPIMASTER_H_

#ifdef MCU
#include <stdint.h>
#include <avr/io.h>

extern "C" {
#include "timer.h"
}

/**
 * SPI master driver for ATmega using SPI module of the controller
 */
class SpiMaster {
protected:
	/**
	 * Initializes the SPI module as a master
	 * \note Sets SS, MOSI and SCK pins as output and MISO as input, SS is driven high (permanently)
	 */
	void init() {
		/* Set SS, MOSI and SCK output, MISO input */
		PORTB |= _BV(PB4); // SS high
		uint8_t ddrb = DDRB;
		ddrb |= _BV(DDB5) | _BV(DDB7) | _BV(DDB4);
		ddrb = (uint8_t) (ddrb & ~_BV(DDB6));
		DDRB = ddrb;
	}

	/**
	 * Receives a byte from the SPI bus
	 * \return The received data byte
	 * \note Blocking until data is received, 0xff is sent out
	 */
	uint8_t __attribute__((always_inline)) receive() const {
		SPDR = 0xff;
		while (! (SPSR & _BV(SPIF))) {}
		return SPDR;
	}

	/**
	 * Receives n byte from the SPI bus
	 * \param[out] buf Pointer to buffer for the received bytes (with space for at least n byte)
	 * \param[in] n Number of bytes to be received
	 */
	void receive(uint8_t* buf, size_t n) const {
		if (n-- == 0) {
			return;
		}
		SPDR = 0xff; // start 1st SPI-transfer
		if (m_divisor == 2) {
			int16_t i(n);
			uint8_t tmp;
			__asm__ __volatile__(
				"adiw %2,1		; 2 nop		\n\t"
				"sbiw %2,1		; 2 nop		\n\t"
				"adiw %2,1		; 2 nop		\n\t"
				"sbiw %2,1		; 2 nop		\n\t"
				"1:							\n\t"
				"adiw %2,1	 	; 2 nop		\n\t" // wait 16 cycles for SPI reception to finish
				"sbiw %2,1		; 2 nop		\n\t"
				"adiw %2,1		; 2 nop		\n\t"
				"sbiw %2,1		; 2 nop		\n\t"
				"nop			; 1 nop		\n\t"
				"in %0,%3		; tmp		\n\t" // load from SPDR
				"out %3,__zero_reg__		\n\t" // start next SPI-transfer
				"st Z+,%0	 	; tmp		\n\t" // save to *buffer
				"sbiw %A1,1		; i			\n\t" // i--
				"sbrs %B1,7		; i			\n\t" // i == 0?
				"rjmp 1b						"
				: "=&r" (tmp) /* %0 */
				: "w" (i) /* %1 */, "z" (buf) /* %2 */, "M" (_SFR_IO_ADDR(SPDR)) /* %3 */
				: "memory"
			);
		} else {
			for (size_t i(0); i < n; ++i) {
				while (! (SPSR & _BV(SPIF))) {}
				const uint8_t b(SPDR);
				SPDR = 0xff;
				buf[i] = b;
			}
			while (! (SPSR & _BV(SPIF))) {}
			buf[n] = SPDR;
		}
	}

	/**
	 * Sends a byte to the SPI bus.
	 * \param[in] data The data byte to send
	 * \note Blocking until byte is sent out
	 */
	void __attribute__((always_inline)) send(uint8_t data) const {
		SPDR = data;
		while (!( SPSR & _BV(SPIF))) {}
	}

	/**
	 * Sends n byte to the SPI bus
	 * \param[in] buf Pointer to buffer for data to send
	 * \param[in] n Number of bytes to send
	 */
	void send(const uint8_t* buf, size_t n) const {
		if (n == 0) {
			return;
		}
		if (m_divisor == 2) {
			int16_t i(n - 1);
			uint8_t tmp;
			__asm__ __volatile__(
				"1:							\n\t"
				"ld %0,Z	 	; tmp	2	\n\t" // load from *buffer
				"out %3,%0					\n\t" // start next SPI-transfer
				"adiw %2,1 		; 2 nop		\n\t" // wait 17 cycles for SPI transfer to finish
				"sbiw %2,1		; 2 nop		\n\t"
				"adiw %2,1 		; 2 nop		\n\t"
				"sbiw %2,1		; 2 nop		\n\t"
				"adiw %2,1		; buf++ 2	\n\t"
				"sbiw %A1,1		; i		2	\n\t"
				"sbrs %B1,7		; i		1	\n\t"
				"rjmp 1b		;		2		"
				: "=&r" (tmp) /* %0 */
				: "w" (i) /* %1 */, "z" (buf) /* %2 */, "M" (_SFR_IO_ADDR(SPDR)) /* %3 */
				: "memory"
			);
		} else {
			SPDR = buf[0];
			if (n > 1) {
				uint8_t b(buf[1]);
				size_t i(2);
				while (1) {
					while (! (SPSR & _BV(SPIF))) {}
					SPDR = b;
					if (i == n) {
						break;
					}
					b = buf[i++];
				}
			}
		}
		while (! (SPSR & _BV(SPIF))) {}
	}

	/**
	 * Waits until data != 0xff received from the SPI bus or timeout occurs
	 * \param[in] timeout_ms Maximum wait time in ms
	 * \return false, iff timeout; true otherwise
	 */
	bool wait_not_busy(uint16_t timeout_ms) const {
		const auto starttime(TIMER_GET_TICKCOUNT_16);
		const uint16_t timeout_ticks(timeout_ms * (1000U / TIMER_STEPS + 1));
		if (m_divisor == 2) {
			bool ret;
			__asm__ __volatile__(
				"ldi %A0,0				; 	 		\n\t"
				"ldi %B0,lo8(-1)		; 	 		\n\t"
				"jmp 2f					; 			\n\t"
				"1:						;			\n\t"
				"cli					; 1			\n\t" // wait 16 cycles for SPI reception to finish
				"lds %B0,tickCount+1	; 2			\n\t"
				"lds %A0,tickCount		; 2			\n\t"
				"sei					; 1			\n\t"
				"sub %A0,%A1			; 1			\n\t"
				"sbc %B0,%B1			; 1			\n\t"
				"cp %A2,%A0				; 1			\n\t"
				"cpc %B2,%B0			; 1			\n\t"
				"ldi %A0,0				; 1  		\n\t"
				"brlo 3f				; 1			\n\t"
				"ldi %B0,lo8(-1)		; 1 		\n\t"
				"in %A0,%3				;			\n\t"
				"2:						;			\n\t"
				"out %3,%B0				;			\n\t"
				"cpi %A0,lo8(-1)		; ==0xff? 1 \n\t"
				"brne 1b				; 2			\n\t"
				"ldi %A0,1				;   		\n\t"
				"3:						;   			"
				: "=&w" (ret) /* %0 */
				: "d" (starttime) /* %1 */, "d" (timeout_ticks) /* %2 */, "M" (_SFR_IO_ADDR(SPDR)) /* %3 */
				:
			);
			return ret;
		} else {
			while (receive() != 0xff) {
				if (static_cast<uint16_t>(TIMER_GET_TICKCOUNT_16 - starttime) > timeout_ticks) {
					return false;
				}
			}
			return true;
		}
	}

	/**
	 * Sets the SPI bus speed to fraction of F_CPU: speed = F_CPU / (2 * divisor)
	 * \param[in] divisor Devides (F_CPU / 2) to get SPI bus speed
	 */
	void set_speed(uint8_t divisor) {
		uint8_t b(2);
		uint8_t r(0);
		m_divisor = divisor;

		/* see AVR processor documentation */
		for (; divisor > b && r < 7; b <<= 1, r += r < 5 ? 1 : 2) {} // well, monsters live here
		SPCR = _BV(SPE) | _BV(MSTR) | (r >> 1);
		SPSR = r & 1 ? 0 : _BV(SPI2X);
	}

private:
	void receive_sector(void* buf);
	void send_sector(const void* buf);

	uint8_t m_divisor;
};

#endif // MCU
#endif /* SPIMASTER_H_ */
