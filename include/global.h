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
 * @brief 	Allgemeine Definitionen
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
		#define bool byte;				/*!< True/False-Aussage */
	#endif

	#ifdef DOXYGEN		/*!< Nur zum Generieren von Doku!!!! */
		#define PC		/*!< Zielplattform PC */
		#define MCU		/*!< Zielplattform MCU */
		#define WIN32	/*!< System Windows */
		#define __linux__	/*!< System Linux */
	#endif
	
	#define byte	uint8_t				/*!< vorzeichenlose 8-Bit-Zahl */
	#define uint8	uint8_t				/*!< vorzeichenlose 8-Bit-Zahl */
	#define int8	int8_t				/*!< vorzeichenbehaftete 8-Bit-Zahl */
	#define uint16	uint16_t			/*!< vorzeichenlose 16-Bit-Zahl */
	#define int16	int16_t				/*!< vorzeichenbehaftete 16-Bit-Zahl */
	#define uint32	uint32_t			/*!< vorzeichenlose 32-Bit-Zahl */
	#define	int32	int32_t				/*!< vorzeichenbehaftete 32-Bit-Zahl */
	
	#define True                  1		/*!< Wahr */
	#define False                 0		/*!< Falsch */
	
	#define On                    1		/*!< An */
	#define Off                   0		/*!< Aus */

	#ifdef MCU
		#define EE_SECTION	__attribute__ ((section (".eeprom"),aligned (1)))	/*!< Shortcut fuer EEPROM-Section */
	#else
		#define EE_SECTION														/*!< Shortcut fuer EEPROM-Section */
	#endif	// MCU

	#define binary(var,bit) ((var >> bit)&1)	/*!< gibt das Bit "bit" von "var" zurueck */
	
	
	/* old avr-libc support */
	#ifndef __CONCAT
		#define __CONCATenate(left, right) left ## right
		#define __CONCAT(left, right) __CONCATenate(left, right)
	#endif
	
	#ifndef INT8_MAX
		#define INT8_MAX 0x7f
		#define INT8_MIN (-INT8_MAX - 1)
		#define UINT8_MAX (__CONCAT(INT8_MAX, U) * 2U + 1U)
		
		#if __USING_MINT8	
			#define INT16_MAX 0x7fffL
			#define INT16_MIN (-INT16_MAX - 1L)
			#define UINT16_MAX (__CONCAT(INT16_MAX, U) * 2UL + 1UL)
			
			#define INT32_MAX 0x7fffffffLL
			#define INT32_MIN (-INT32_MAX - 1LL)
			#define UINT32_MAX (__CONCAT(INT32_MAX, U) * 2ULL + 1ULL)
		#else	// !__USING_MINT8
			#define INT16_MAX 0x7fff
			#define INT16_MIN (-INT16_MAX - 1)
			#define UINT16_MAX (__CONCAT(INT16_MAX, U) * 2U + 1U)
			#define INT32_MAX 0x7fffffffL
			#define INT32_MIN (-INT32_MAX - 1L)
			#define UINT32_MAX (__CONCAT(INT32_MAX, U) * 2UL + 1UL)
		#endif	// __USING_MINT8
	#endif	// INT8_MAX	

#endif	// __ASSEMBLER__
#endif	// global_H
