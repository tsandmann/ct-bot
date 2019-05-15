/*****************************************************************************
*
* AVRPROG compatible boot-loader
* size     : depends on features and startup (minmal features < 512 words)
* by       : Martin Thomas, Kaiserslautern, Germany
*            eversmith@heizung-thomas.de
*            Additional code and improvements contributed by:
*           - Uwe Bonnes
*           - Bjoern Riemer
*           - Olaf Rempel
*
* License  : Copyright (c) 2006 Martin Thomas
*            Free to use. You have to mention the copyright
*            owners in source-code and documentation of derived
*            work. No warranty!
*
* Tested with ATmega8, ATmega16, ATmega32, ATmega128, AT90CAN128, ATmega644, ATmega644P, ATmega1284P
*
* - Initial versions have been based on the Butterfly bootloader-code
*   by Atmel Corporation (Authors: BBrandal, PKastnes, ARodland, LHM)
*
****************************************************************************/

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
 * \file 	bootloader.c
 * \brief 	AVR109 kompatibler Bootloader fuer den c't-Bot
 * \author	Martin Thomas (eversmith@heizung-thomas.de)
 * \author 	Timo Sandmann
 * \date 	17.01.2007
 */

#ifdef MCU
#include "ct-Bot.h"
#ifdef BOOTLOADER_AVAILABLE

/* Device-Type:
   For AVRProg the BOOT-option is prefered
   which is the "correct" value for a bootloader.
   avrdude may only detect the part-code for ISP */
//#define DEVTYPE     DEVTYPE_BOOT
#define DEVTYPE     DEVTYPE_ISP		/**< Device-Typ des emulierten Programmers */

/* Boot Size in Words */
#if defined MCU_ATMEGA644X // => Fuse Bits: low: 0xFF, high: 0xDC, Ext'd: 0xFF
#define BOOTSIZE 1024		// => Linker-Settings: -Wl,--section-start=.bootloader=0xF800
//#warning "Bitte pruefen, ob der Linker auch mit den Optionen: -Wl,--section-start=.bootloader=0xF800 startet"
#elif defined __AVR_ATmega1284P__ // => Fuse Bits: low: 0xFF, high: 0xDC, Ext'd: 0xFF
#define BOOTSIZE 1024		// => Linker-Settings: -Wl,--section-start=.bootloader=0x1F800
//#warning "Bitte pruefen, ob der Linker auch mit den Optionen: -Wl,--section-start=.bootloader=0x1F800 startet"
#endif

#if UART_BAUD != 115200
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wcpp"
#warning "Set UART_BAUD to 115200 for bootloader"
#pragma GCC diagnostic pop
#endif // BAUD

/** Startup-Timeout */
#define START_WAIT

/** character to start the bootloader in mode START_WAIT */
#define START_WAIT_UARTCHAR 'S'

/** wait 5s in START_WAIT mode (10ms steps) */
#define WAIT_VALUE 500

/*
 * enable/disable readout of fuse and lock-bits
 * (AVRPROG has to detect the AVR correctly by device-code
 * to show the correct information).
 */
//#define ENABLEREADFUSELOCK

#define VERSION_HIGH '0'	/**< Versionsnummer */
#define VERSION_LOW  '9'	/**< Versionsnummer */

#include "uart.h" // UART Baudrate
#include <stdint.h>
#include <avr/io.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

/* Chipdefs */
#if defined (SPMCSR)
#define SPM_REG SPMCSR
#elif defined (SPMCR)
#define SPM_REG SPMCR
#else
#error "AVR processor does not provide bootloader support!"
#endif

#define APP_END (FLASHEND - (BOOTSIZE * 2))	/**< Ende des Flash-Bereichs fuer Programm */

#if (SPM_PAGESIZE > UINT8_MAX)
typedef uint16_t pagebuf_t; /**< Seitengroesse */
#else
typedef uint8_t pagebuf_t; /**< Seitengroesse */
#endif

#if defined(__AVR_ATmega644__)
/* Part-Code ISP */
#define DEVTYPE_ISP     0x74
/* Part-Code Boot */
#define DEVTYPE_BOOT    0x73

#define SIG_BYTE1	0x1E
#define SIG_BYTE2	0x96
#define SIG_BYTE3	0x09

#define UART_BAUD_HIGH	UBRR0H
#define UART_BAUD_LOW	UBRR0L
#define UART_STATUS	UCSR0A
#define UART_TXREADY	UDRE0
#define UART_RXREADY	RXC0
#define UART_DOUBLE	U2X0
#define UART_CTRL	UCSR0B
#define UART_CTRL_DATA	((1<<TXEN0) | (1<<RXEN0))
#define UART_CTRL2	UCSR0C
//#define UART_CTRL2_DATA	((1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0))
#define UART_CTRL2_DATA 0x86	// just 8N1
#define UART_DATA	UDR0

#elif defined(__AVR_ATmega644P__)
/* Part-Code ISP */
#define DEVTYPE_ISP     0x74
/* Part-Code Boot */
#define DEVTYPE_BOOT    0x73

#define SIG_BYTE1	0x1E
#define SIG_BYTE2	0x96
#define SIG_BYTE3	0x0A

#define UART_BAUD_HIGH	UBRR0H
#define UART_BAUD_LOW	UBRR0L
#define UART_STATUS	UCSR0A
#define UART_TXREADY	UDRE0
#define UART_RXREADY	RXC0
#define UART_DOUBLE	U2X0
#define UART_CTRL	UCSR0B
#define UART_CTRL_DATA	((1<<TXEN0) | (1<<RXEN0))
#define UART_CTRL2	UCSR0C
//#define UART_CTRL2_DATA	((1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0))
#define UART_CTRL2_DATA 0x86	// just 8N1
#define UART_DATA	UDR0

#elif defined __AVR_ATmega1284P__
/* Part-Code ISP */
#define DEVTYPE_ISP     0x74
/* Part-Code Boot */
#define DEVTYPE_BOOT    0x73

#define SIG_BYTE1	0x1E
#define SIG_BYTE2	0x97
#define SIG_BYTE3	0x05

#define UART_BAUD_HIGH	UBRR0H
#define UART_BAUD_LOW	UBRR0L
#define UART_STATUS	UCSR0A
#define UART_TXREADY	UDRE0
#define UART_RXREADY	RXC0
#define UART_DOUBLE	U2X0
#define UART_CTRL	UCSR0B
#define UART_CTRL_DATA	((1<<TXEN0) | (1<<RXEN0))
#define UART_CTRL2	UCSR0C
//#define UART_CTRL2_DATA	((1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0))
#define UART_CTRL2_DATA 0x86 // just 8N1
#define UART_DATA	UDR0

#else
#error "no definition for MCU available"
#endif
// end Chipdefs

static uint8_t gBuffer[SPM_PAGESIZE];	/**< Puffer */

/* all inline! Sonst stimmt die Startadresse der bl_main nicht */
inline static void ALWAYS_INLINE sendchar(uint8_t data) {
	while (!(UART_STATUS & (1<<UART_TXREADY)));
	UART_DATA = data;
}

inline static uint8_t ALWAYS_INLINE recvchar(void) {
	while (!(UART_STATUS & (1<<UART_RXREADY)));
	return UART_DATA;
}

inline static void ALWAYS_INLINE eraseFlash(void) {
	// erase only main section (bootloader protection)
	uint32_t addr = 0;
	while (APP_END > addr) {
		boot_page_erase(addr);		// Perform page erase
		boot_spm_busy_wait();		// Wait until the memory is erased.
		addr += SPM_PAGESIZE;
	}
	boot_rww_enable();
}

inline static void ALWAYS_INLINE recvBuffer(pagebuf_t size) {
	pagebuf_t cnt;
	uint8_t *tmp = gBuffer;

	for (cnt = 0; cnt < sizeof(gBuffer); cnt++)
		*tmp++ = (uint8_t) ((cnt < size) ? recvchar() : 0xff);
}

inline static uint16_t ALWAYS_INLINE writeFlashPage(uint16_t waddr, pagebuf_t size) {
	uint32_t pagestart = (uint32_t)waddr<<1;
	uint32_t baddr = pagestart;
	uint16_t data;
	uint8_t *tmp = gBuffer;

	do {
		data = *tmp++;
		data |= (uint16_t) (*tmp++ << 8);
		boot_page_fill(baddr, data);	// call asm routine.

		baddr += 2;			// Select next word in memory
		size -= 2;			// Reduce number of bytes to write by two
	} while (size);				// Loop until all bytes written

	boot_page_write(pagestart);
	boot_spm_busy_wait();
	boot_rww_enable();			// Re-enable the RWW section

	return (uint16_t) (baddr >> 1);
}

inline static uint16_t ALWAYS_INLINE writeEEpromPage(uint16_t address, pagebuf_t size) {
	uint8_t *tmp = gBuffer;

	do {
		EEARL = (uint8_t) address; // Setup EEPROM address
		EEARH = (uint8_t) (address >> 8);
		EEDR = *tmp++;
		address++; // Select next byte

#if defined MCU_ATMEGA644X || defined __AVR_ATmega1284P__
		EECR |= (1<<EEMPE); // Write data into EEPROM
		EECR |= (1<<EEPE);
#else
		EECR |= (1 << EEMWE);
		EECR |= (1 << EEWE);
#endif
		eeprom_busy_wait();

		size--;				// Decreas number of bytes to write
	} while (size);			// Loop until all bytes written

	return address;
}

inline static uint16_t ALWAYS_INLINE readFlashPage(uint16_t waddr, pagebuf_t size) {
	uint32_t baddr = (uint32_t)waddr<<1;
	uint16_t data;

	do {
#if defined(RAMPZ)
		data = pgm_read_word_far(baddr);
#else
		data = pgm_read_word_near(baddr);
#endif
		sendchar((uint8_t) data); // send LSB
		sendchar((uint8_t) (data >> 8)); // send MSB
		baddr += 2;			// Select next word in memory
		size -= 2;			// Subtract two bytes from number of bytes to read
	} while (size);			// Repeat until all block has been read

	return (uint16_t) (baddr >> 1);
}

inline static uint16_t ALWAYS_INLINE readEEpromPage(uint16_t address, pagebuf_t size) {
	do {
		EEARL = (uint8_t) address; // Setup EEPROM address
		EEARH = (uint8_t) (address >> 8);
		EECR |= (1<<EERE);		// Read EEPROM
		address++;			// Select next EEPROM byte

		sendchar(EEDR);			// Transmit EEPROM data to PC

		size--;				// Decrease number of bytes to read
	} while (size);				// Repeat until all block has been read

	return address;
}

#if defined(ENABLEREADFUSELOCK)
static uint8_t ALWAYS_INLINE read_fuse_lock(uint16_t addr) {
	uint8_t mode = (1<<BLBSET) | (1<<SPMEN);
	uint8_t retval;

	__asm__ __volatile__
	(
			"movw r30, %3\n\t" /* Z to addr */
			"sts %0, %2\n\t" /* set mode in SPM_REG */
			"lpm\n\t" /* load fuse/lock value into r0 */
			"mov %1,r0\n\t" /* save return value */
			: "=m" (SPM_REG),
			"=r" (retval)
			: "r" (mode),
			"r" (addr)
			: "r30", "r31", "r0"
	);
	return retval;
}
#endif

inline static void ALWAYS_INLINE send_boot(void) {
	sendchar('A');
	sendchar('V');
	sendchar('R');
	sendchar('B');
	sendchar('O');
	sendchar('O');
	sendchar('T');
}

static void (*jump_to_app)(void) = 0x0000;

void bootloader_main(void) __attribute__ ((section (".bootloader")));

/** Der eigentliche Bootloader. Die Section "bootloader" muss dort beginnen,
 * wohin die MCU beim Booten springt (=> Fuse Bits). Deshalb die Linkereinstellungen
 * anpassen, wie oben beschrieben!
 */
void bootloader_main(void) {
	uint16_t address = 0;
	uint8_t device = 0, val;

	// Set baud rate
	UART_BAUD_HIGH = UBRRH_VALUE;
	UART_BAUD_LOW = UBRRL_VALUE;

#if USE_2X
	UART_STATUS = (1 << UART_DOUBLE);
#endif

	UART_CTRL = UART_CTRL_DATA;
	UART_CTRL2 = UART_CTRL2_DATA;

	uint16_t cnt = 0;

	while (1) {
		if (UART_STATUS & (1 << UART_RXREADY)) {
			if (UART_DATA == START_WAIT_UARTCHAR) {
				break;
			}
		}

		if (cnt++ >= WAIT_VALUE) {
			jump_to_app();			// Jump to application sector
		}

		_delay_ms(10);
	}
	send_boot();

	for(;;) {
		val = recvchar();
		// Autoincrement?
		if (val == 'a') {
			sendchar('Y');			// Autoincrement is quicker

		// write address
        } else if (val == 'A') {
			address = recvchar();		// read address 8 MSB
			address = (address<<8) | recvchar();
			sendchar('\r');

		// Buffer load support
		} else if (val == 'b') {
			sendchar('Y');					// Report buffer load supported
			sendchar((sizeof(gBuffer) >> 8) & 0xFF);	// Report buffer size in bytes
			sendchar(sizeof(gBuffer) & 0xFF);

		// Start buffer load
		} else if (val == 'B') {
			pagebuf_t size;
			size = (pagebuf_t) (recvchar() << 8); // Load high byte of buffersize
			size |= recvchar();				// Load low byte of buffersize
			val = recvchar();				// Load memory type ('E' or 'F')
			recvBuffer(size);

			if (device == DEVTYPE) {
				if (val == 'F') {
					address = writeFlashPage(address, size);

				} else if (val == 'E') {
					address = writeEEpromPage(address, size);
				}
				sendchar('\r');

			} else {
				sendchar(0);
			}

		// Block read
		} else if (val == 'g') {
			pagebuf_t size;
			size = (pagebuf_t) (recvchar() << 8); // Load high byte of buffersize
			size |= recvchar();				// Load low byte of buffersize
			val = recvchar();				// Get memtype

			if (val == 'F') {
				address = readFlashPage(address, size);

			} else if (val == 'E') {
				address = readEEpromPage(address, size);
			}

		// Chip erase
 		} else if (val == 'e') {
			if (device == DEVTYPE)
				eraseFlash();

			sendchar('\r');

		// Exit upgrade
		} else if (val == 'E') {
			sendchar('\r');
			jump_to_app();		// Fertig, nun main() starten

		// Enter programming mode
		} else if (val == 'P') {
			sendchar('\r');

		// Leave programming mode
		} else if (val == 'L') {
			sendchar('\r');

		// return programmer type
		} else if (val == 'p') {
			sendchar('S');		// always serial programmer

#ifdef ENABLEREADFUSELOCK
#warning "Extension 'ReadFuseLock' enabled"
			// read "low" fuse bits
		} else if (val == 'F') {
			sendchar(read_fuse_lock(GET_LOW_FUSE_BITS));

			// read lock bits
		} else if (val == 'r') {
			sendchar(read_fuse_lock(GET_LOCK_BITS));

			// read high fuse bits
		} else if (val == 'N') {
			sendchar(read_fuse_lock(GET_HIGH_FUSE_BITS));

			// read extended fuse bits
		} else if (val == 'Q') {
			sendchar(read_fuse_lock(GET_EXTENDED_FUSE_BITS));
#endif

		// Return device type
		} else if (val == 't') {
			sendchar(DEVTYPE);
			sendchar(0);

		// clear and set LED ignored
        	} else if ((val == 'x') || (val == 'y')) {
			recvchar();
			sendchar('\r');

		// set device
		} else if (val == 'T') {
			device = recvchar();
			sendchar('\r');

		// Return software identifier
		} else if (val == 'S') {
			send_boot();

		// Return Software Version
		} else if (val == 'V') {
			sendchar(VERSION_HIGH);
			sendchar(VERSION_LOW);

		// Return Signature Bytes (it seems that
		// AVRProg expects the "Atmel-byte" 0x1E last
		// but shows it first in the dialog-window)
		} else if (val == 's') {
			sendchar(SIG_BYTE3);
			sendchar(SIG_BYTE2);
			sendchar(SIG_BYTE1);

		/* ESC */
		} else if (val != 0x1b) {
			sendchar('?');
		}
	}
}
#endif // BOOTLOADER_AVAILABLE
#endif // MCU
