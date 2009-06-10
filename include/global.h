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
 * @file 	global.h
 * @brief 	Allgemeine Definitionen und Datentypen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
 */

#ifndef global_H
#define global_H

#ifndef __ASSEMBLER__
#include <stdint.h>

#ifndef MCU
#ifndef PC
#define PC		/*!< Zielplattform PC */
#endif
#endif

#ifndef WIN32
#define bool byte;	/*!< True/False-Aussage */
#endif

#ifdef DOXYGEN		/*!< Nur zum Generieren von Doku!!!! */
#define PC			/*!< Zielplattform PC */
#define MCU			/*!< Zielplattform MCU */
#define WIN32		/*!< System Windows */
#define __linux__	/*!< System Linux */
#endif	// DOXYGEN

#define byte	uint8_t		/*!< vorzeichenlose 8-Bit-Zahl */
#define uint8	uint8_t		/*!< vorzeichenlose 8-Bit-Zahl */
#define int8	int8_t		/*!< vorzeichenbehaftete 8-Bit-Zahl */
#define uint16	uint16_t	/*!< vorzeichenlose 16-Bit-Zahl */
#define int16	int16_t		/*!< vorzeichenbehaftete 16-Bit-Zahl */
#define uint32	uint32_t	/*!< vorzeichenlose 32-Bit-Zahl */
#define	int32	int32_t		/*!< vorzeichenbehaftete 32-Bit-Zahl */

#define True	1			/*!< Wahr */
#define False	0			/*!< Falsch */

#define On		1			/*!< An */
#define Off		0			/*!< Aus */

#ifdef PC
#if defined WIN32
#define LITTLE_ENDIAN	1234
#define BIG_ENDIAN	4321
#define BYTE_ORDER	LITTLE_ENDIAN
#elif defined __linux__
#include <endian.h>
#else
#include <machine/endian.h>
#endif	// WIN32
#endif	// PC

#define binary(var,bit) ((var >> bit)&1)	/*!< gibt das Bit "bit" von "var" zurueck */

#ifdef WIN32
#define LINE_FEED "\n\r"	/*!< Linefeed fuer Windows */
#else
#define LINE_FEED "\n"		/*!< Linefeed fuer nicht Windows */
#endif

#ifdef MCU
#include <avr/interrupt.h>
#ifdef SIGNAL
#define NEW_AVR_LIB	/*!< AVR_LIB-Version */
#else
#include <avr/signal.h>
#endif

#if defined __AVR_ATmega644__ || defined __AVR_ATmega644P__
#define MCU_ATMEGA644X	/*!< ATmega644-Familie (ATmega644 oder ATmega644P) */
#endif
#endif	// MCU

/*!
 * 2D-Position. Ist effizienter, als Zeiger auf X- und Y-Anteil
 */
typedef struct {
	int16_t x; /*!< X-Anteil der Position */
	int16_t y; /*!< Y-Anteil der Position */
} __attribute__ ((packed)) position_t;

#else	// __ASSEMBLER__

#if defined __APPLE__ || defined __linux__ || defined __WIN32__
#ifndef PC
#define PC
#endif	// PC
#endif	// Plattform

#endif	// __ASSEMBLER__
#endif	// global_H
