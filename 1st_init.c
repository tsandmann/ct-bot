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
 * @file 	1st_init.c
 * @brief 	Init-Stuff, der als erstes compiliert und gelinkt werden muss
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	28.06.2007
 */
 
#include <stdint.h>
 
/* fuer EEPROM-Manager */
#ifdef PC
	/* Moeglichst frueh section erzwingen, damit sie VOR .eeprom liegt!!! */
	#ifdef __APPLE__
		/* OS X */
		uint8_t __attribute__ ((section ("__DATA,.s1eeprom"))) _eeprom_start1__ = 0;
		uint8_t __attribute__ ((section ("__DATA,.s2eeprom"))) _eeprom_start2__ = 0;
	#else
		/* Linux und Windows */
		uint8_t __attribute__ ((section (".s1eeprom"))) _eeprom_start1__ = 0;
		uint8_t __attribute__ ((section (".s2eeprom"))) _eeprom_start2__ = 0;
	#endif
#endif
