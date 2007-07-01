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

	#ifdef __APPLE__
		/* OS X */
		#define EEPROM __attribute__ ((section ("__DATA,.eeprom"),aligned(1)))	/*!< EEPROM-Section */
	#else
		/* Linux und Windows */
		#define EEPROM __attribute__ ((section (".eeprom"),aligned(1)))			/*!< EEPROM-Section */
	#endif

	#define binary(var,bit) ((var >> bit)&1)	/*!< gibt das Bit "bit" von "var" zurueck */
#endif	// __ASSEMBLER__
#endif	// global_H
