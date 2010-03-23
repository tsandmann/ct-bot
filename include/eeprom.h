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
 * @file 	eeprom.h
 * @brief 	EEPROM-Zugriff
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	30.08.2008
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include <stddef.h>
#include "ct-Bot.h"

/*! Aufteilung eines Words in zwei Bytes */
typedef union {
	uint16_t word;
	struct {
		uint8_t byte_0;
		uint8_t byte_1;
	} bytes;
} eeprom_word_t;

/*! Aufteilung eines DWords in vier Bytes */
typedef union {
	uint32_t dword;
	struct {
		uint8_t byte_0;
		uint8_t byte_1;
		uint8_t byte_2;
		uint8_t byte_3;
	} bytes;
} eeprom_dword_t;

#ifdef MCU
#include <avr/io.h>

/*! EEPROM-Section */
#define EEPROM __attribute__ ((section (".eeprom"), aligned(1)))

/*!
 * prueft, ob das EEPROM bereit ist
 * @return	1, falls EEPROM bereit, 0 sonst
 */
#if defined MCU_ATMEGA644X || defined __AVR_ATmega1284P__
#define EEWE	EEPE
#define EEMWE	EEMPE
#endif	// MCU_ATMEGA644X || ATmega1284P

#define eeprom_is_ready() bit_is_clear(EECR, EEWE)

/*!
 * Wartet, bis das EEPROM bereit ist
 */
#define eeprom_busy_wait() do {} while (!eeprom_is_ready())

/*!
 * Liest ein Byte aus dem EEPROM
 * @param *address	Adresse des zu lesenden Bytes im EEPROM
 * @return			Das zu lesende Byte
 */
static inline uint8_t ctbot_eeprom_read_byte(const uint8_t * address) {
	eeprom_busy_wait();
#if E2END <= 0xFF
    EEARL = (uint8_t)address;
#else
    EEAR = (uint16_t)address;
#endif
    uint8_t result;
    __asm__ __volatile__(
        "/* START EEPROM READ CRITICAL SECTION */	\n\t"
        "sbi %1, %2									\n\t"
        "in %0, %3									\n\t"
        "/* END EEPROM READ CRITICAL SECTION */			"
        : "=r" (result)
        : "i" (_SFR_IO_ADDR(EECR)),
          "i" (EERE),
          "i" (_SFR_IO_ADDR(EEDR))
    );
    return result;
}

/*!
 * Schreibt ein Byte in das EEPROM
 * @param *address	Adresse des Bytes im EEPROM
 * @param value		Das zu schreibende Byte
 */
static inline void ctbot_eeprom_write_byte(uint8_t * address, const uint8_t value) {
	eeprom_busy_wait();

#if	defined(EEPM0) && defined(EEPM1)
	EECR = 0;	// Set programming mode: erase and write
#elif defined(EEPM0) || defined(EEPM1)
#error "Unknown EECR register, eeprom_write_byte() has become outdated."
#endif

#if	E2END <= 0xFF
	EEARL = (unsigned)address;
#else
	EEAR = (unsigned)address;
#endif
	EEDR = value;

	__asm__ __volatile__(
		"/* START EEPROM WRITE CRITICAL SECTION */	\n\t"
		"in	r0, %[__sreg]							\n\t"
		"cli										\n\t"
		"sbi	%[__eecr], %[__eemwe]				\n\t"
		"sbi	%[__eecr], %[__eewe]				\n\t"
		"out	%[__sreg], r0						\n\t"
		"/* END EEPROM WRITE CRITICAL SECTION */		"
		:
		: [__eecr]  "i" (_SFR_IO_ADDR(EECR)),
		  [__sreg]  "i" (_SFR_IO_ADDR(SREG)),
		  [__eemwe] "i" (EEMWE),
		  [__eewe]  "i" (EEWE)
		: "r0"
	);
}

#else	// PC

#ifdef EEPROM_EMU_AVAILABLE
#ifdef __APPLE__
/* OS X */
#define EEPROM __attribute__ ((section ("__eeprom, __data"), aligned(1)))	/*!< EEPROM-Section */
#else
/* Linux und Windows */
#define EEPROM __attribute__ ((section (".eeprom"), aligned(1)))			/*!< EEPROM-Section */
#endif
#else
/* keine EEPROM-Emulation */
#define EEPROM
#endif	// EEPROM_EMU_AVAILABLE

/*!
 * Liest ein Byte aus dem EEPROM
 * @param *address	Adresse des zu lesenden Bytes im EEPROM
 * @return			Das zu lesende Byte
 */
uint8_t ctbot_eeprom_read_byte(const uint8_t * address);

/*!
 * Schreibt ein Byte in das EEPROM
 * @param *address	Adresse des Bytes im EEPROM
 * @param value		Das zu schreibende Byte
 */
void ctbot_eeprom_write_byte(uint8_t * address, const uint8_t value);

/*!
 * Diese Funktion initialisiert die EEPROM-Emulation. Sie sorgt fuer die Erstellung der
 * pc_eeprom.bin, falls nicht vorhanden und erstellt ueber eine Hilfsfunktion eine Adress-
 * konvertierungstabelle fuer die EEPROM-Adressen, wenn die benoetigten Daten vorliegen.
 * Statusinformationen werden ueber DEBUG_INFO angezeigt.
 * @param init	gibt an, ob das EEPROM mit Hilfer einer eep-Datei initialisiert werden soll (0 nein, 1 ja)
 * @return		0: alles ok, 1: Fehler
 */
uint8_t init_eeprom_man(uint8_t init);

#endif	// MCU

/*!
 * Liest die zwei Bytes eines Words aus dem EEPROM.
 * @param *address	Adresse des Words im EEPROM
 * @return			Das zu lesende DWord
 */
static inline uint16_t ctbot_eeprom_read_word(const uint16_t * address) {
	eeprom_word_t data;
	size_t eeprom_addr = (size_t)address;

	data.bytes.byte_0 = ctbot_eeprom_read_byte((const uint8_t *)eeprom_addr++);
	data.bytes.byte_1 = ctbot_eeprom_read_byte((const uint8_t *)eeprom_addr);
	return data.word;
}

/*!
 * Liest die vier Bytes eines DWords aus dem EEPROM.
 * @param *address	Adresse des DWords im EEPROM
 * @return			Das zu lesende DWord
 */
static inline uint32_t ctbot_eeprom_read_dword(const uint32_t * address) {
	eeprom_dword_t data;
	size_t eeprom_addr = (size_t)address;

	data.bytes.byte_0 = ctbot_eeprom_read_byte((const uint8_t *)eeprom_addr++);
	data.bytes.byte_1 = ctbot_eeprom_read_byte((const uint8_t *)eeprom_addr++);
	data.bytes.byte_2 = ctbot_eeprom_read_byte((const uint8_t *)eeprom_addr++);
	data.bytes.byte_3 = ctbot_eeprom_read_byte((const uint8_t *)eeprom_addr);
	return data.dword;
}

/*!
 * Liest size Bytes aus dem EEPROM ins RAM
 * @param *dst	Zeiger auf Puffer fuer die Daten im RAM
 * @param *src	Adresse der Daten im EEPROM
 * @param size	Anzahl der zu lesenden Bytes
 */
static inline void ctbot_eeprom_read_block(void * dst, const void * src, size_t size) {
	uint8_t * p_dst = dst;
	const uint8_t * p_src = src;
    while (size--) {
    	*p_dst++ = ctbot_eeprom_read_byte(p_src++);
    }
}


/*!
 * Schreibt die zwei Bytes eines Words ins EEPROM.
 * @param *address	Adresse des Words im EEPROM
 * @param value		Neuer Wert des Words
 */
static inline void ctbot_eeprom_write_word(uint16_t * address, const uint16_t value) {
	eeprom_word_t data;
	data.word = value;
	size_t eeprom_addr = (size_t)address;

	ctbot_eeprom_write_byte((uint8_t *)eeprom_addr++, data.bytes.byte_0);
	ctbot_eeprom_write_byte((uint8_t *)eeprom_addr, data.bytes.byte_1);
}

/*!
 * Schreibt die vier Bytes eines DWords ins EEPROM.
 * @param *address	Adresse des DWords im EEPROM
 * @param value		Neuer Wert des DWords
 */
static inline void ctbot_eeprom_write_dword(uint32_t * address, const uint32_t value) {
	eeprom_dword_t data;
	data.dword = value;
	size_t eeprom_addr = (size_t)address;

	ctbot_eeprom_write_byte((uint8_t *)eeprom_addr++, data.bytes.byte_0);
	ctbot_eeprom_write_byte((uint8_t *)eeprom_addr++, data.bytes.byte_1);
	ctbot_eeprom_write_byte((uint8_t *)eeprom_addr++, data.bytes.byte_2);
	ctbot_eeprom_write_byte((uint8_t *)eeprom_addr, data.bytes.byte_3);
}

/*!
 * Schreibt size Bytes vom RAM ins EEPROM
 * @param *dst	Zeiger auf Puffer fuer die Daten im RAM
 * @param *src	Adresse der Daten im EEPROM
 * @param size	Anzahl der zu schreibenden Bytes
 */
static inline void ctbot_eeprom_write_block(void * dst, const void * src, size_t size) {
	uint8_t * p_dst = dst;
	const uint8_t * p_src = src;
    while (size--) {
    	ctbot_eeprom_write_byte(p_dst++, *p_src++);
    }
}

#endif	/* EEPROM_H_ */
