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
 * \file 	global.h
 * \brief 	Allgemeine Definitionen und Datentypen
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#if ! defined MCU && ! defined PC
#define PC
#endif

#ifndef __ASSEMBLER__
#ifdef __WIN32__
/* Prototypes, die in den MinGW-Includes fehlen -> keine Warnings */
#include <stddef.h>
#include <stdarg.h>
#ifndef _FILE_DEFINED
#define _FILE_DEFINED
typedef struct _iobuf {
	char * _ptr;
	int _cnt;
	char * _base;
	int _flag;
	int _file;
	int _charbuf;
	int _bufsiz;
	char * _tmpfname;
} FILE;
#endif // ! _FILE_DEFINED
FILE * fopen64(const char *, const char *);
long long ftello64 (FILE *);
int getc (FILE *);
int getchar (void);
int putc (int, FILE *);
int putchar(int);
#ifndef __VALIST
#define __VALIST __gnuc_va_list
#endif
#endif // __WIN32__

#include <stdint.h>
#include <math.h>

#ifdef DOXYGEN
#define PC
#define MCU
#define WIN32
#define __linux__
#endif // DOXYGEN

#define True	1			/**< wahr */
#define False	0			/**< falsch */

#ifdef PC
#if defined WIN32
#define LITTLE_ENDIAN	1234
#define BIG_ENDIAN	4321
#define BYTE_ORDER	LITTLE_ENDIAN
#elif defined __linux__
#include <endian.h>
#else
#include <machine/endian.h>
#endif // WIN32

#ifndef BYTE_ORDER
#if (! defined __BYTE_ORDER) || (! defined __LITTLE_ENDIAN) || (! defined __BIG_ENDIAN)
#if defined __DARWIN_BYTE_ORDER && defined __DARWIN_LITTLE_ENDIAN && defined __DARWIN_BIG_ENDIAN
#define BYTE_ORDER __DARWIN_BYTE_ORDER
#define LITTLE_ENDIAN __DARWIN_LITTLE_ENDIAN
#define BIG_ENDIAN __DARWIN_BIG_ENDIAN
#else
#error "Unable to detect byte order, check include/global.h"
#endif
#else
#define BYTE_ORDER __BYTE_ORDER
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN
#endif // __BYTE_ORDER
#endif // BYTE_ORDER

#if BYTE_ORDER == BIG_ENDIAN
#warning "Big endian byte order detected which isn't fully supported"
#endif

#endif // PC

#define binary(var, bit) ((var >> bit) & 1)	/**< gibt das Bit "bit" von "var" zurueck */

#ifdef WIN32
#define LINE_FEED "\r\n"	/**< Linefeed fuer Windows */
#else
#define LINE_FEED "\n"		/**< Linefeed fuer nicht Windows */
#endif

#ifdef MCU
#include "builtins.h"

#include <avr/interrupt.h>
#ifdef SIGNAL
#define NEW_AVR_LIB	/**< neuere AVR_LIB-Version */
#else // ! SIGNAL
#include <avr/signal.h>
#endif // SIGNAL

#if defined __AVR_ATmega644__ || defined __AVR_ATmega644P__
#define MCU_ATMEGA644X /**< ATmega644-Familie (ATmega644 oder ATmega644P) */
#endif
#endif // MCU

#if ! defined DOXYGEN && ! defined PC
#define PACKED __attribute__ ((packed)) /**< packed-Attribut fuer Strukturen und Enums (nur MCU) */
#else // PC || Doxygen
#define PACKED
#endif

#ifndef DOXYGEN
#if (defined __x86_64__ || defined __i386__) && ! defined __clang__
#define PACKED_FORCE __attribute__ ((gcc_struct, packed)) /**< erzwungenes packed-Attribut fuer Strukturen und Enums (x86) */
#else
#define PACKED_FORCE __attribute__ ((packed)) /**< erzwungenes packed-Attribut fuer Strukturen und Enums (nicht x86) */
#endif // __x86_64__ || __i386__
#else // Doxygen
#define PACKED_FORCE
#endif

#ifdef MCU
#include <avr/pgmspace.h>
#else // PC
#include <string.h>
#define PROGMEM /**< Attribut fuer Programmspeicher, fuer PC nicht noetig */
#define PGM_P const char * /**< Zeiger auf Programmspeicher, auf PC Zeiger auf const char */
#define strcmp_P strcmp /**< strcmp fuer PROGMEM-Daten, fuer PC Weiterleitung auf strcmp() */
#define strcasecmp_P strcasecmp /**< strcasemp fuer PROGMEM-Daten, fuer PC Weiterleitung auf strcasecmp() */
#define strncasecmp_P strncasecmp /**< strncasemp fuer PROGMEM-Daten, fuer PC Weiterleitung auf strncasecmp() */
#define strchr_P strchr /**< strchr fuer PROGMEM-Daten, fuer PC Weiterleitung auf strchr() */
#define strlen_P strlen /**< strlen fuer PROGMEM-Daten, fuer PC Weiterleitung auf strlen() */
#define memcpy_P memcpy /**< memcpy fuer PROGMEM-Daten, fuer PC Weiterleitung auf memcpy() */
#define strncpy_P strncpy /**< strncpy fuer PROGMEM-Daten, fuer PC Weiterleitung auf strncpy() */
#define snprintf_P snprintf /**< snprintf fuer PROGMEM-Daten, fuer PC Weiterleitung auf snprintf() */
#define vsnprintf_P vsnprintf /**< vsnprintf fuer PROGMEM-Daten, fuer PC Weiterleitung auf vsnprintf() */
#define pgm_read_byte(_addr) (*(_addr)) /**< liest ein Byte aus dem Programmspeicher (PROGMEM), fuer PC nicht noetig */
#define pgm_read_word(_addr) (*(_addr)) /**< liest ein Word aus dem Programmspeicher (PROGMEM), fuer PC nicht noetig */
#define display_flash_puts display_puts /**< Ausgabe eines Strings aus PROGMEM auf dem Display, fuer PC einfach display_puts() */
#endif // MCU

/** 2D-Position. Ist effizienter, als Zeiger auf X- und Y-Anteil */
typedef struct {
	int16_t x; /**< X-Anteil der Position */
	int16_t y; /**< Y-Anteil der Position */
} PACKED position_t;

/** Repraesentation eines Bits, dem ein Byte-Wert zugewiesen werden kann */
typedef union {
	uint8_t byte;
	unsigned bit:1;
} bit_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846 /** pi */
#endif
#ifndef M_PI_2
#define M_PI_2	(M_PI / 2.0) /**< pi / 2 */
#endif

#else // __ASSEMBLER__

#if defined __APPLE__ || defined __linux__ || defined __WIN32__
#ifndef PC
#define PC
#endif // PC
#endif // Plattform

#endif // ! __ASSEMBLER__
#endif // GLOBAL_H_
