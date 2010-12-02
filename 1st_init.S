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
 * \file 	1st_init.S
 * \brief 	Init-Stuff, der als erstes uebersetzt und gelinkt werden muss
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	04.07.2007
 *
 * Hinweis:
 * Sollte der Build-Prozess an dieser Datei scheitern, sind vermutlich die
 * Projekteinstellungen falsch oder die Eclipse- / CDT-Version ist zu alt.
 */

#ifdef PC
#include "ct-Bot.h"

#ifdef EEPROM_EMU_AVAILABLE
#if defined (__APPLE__)

/* OS X */
.globl __eeprom_start1__
	.section __eeprom,__s1data
__eeprom_start1__:
	.space 1
	
.globl __eeprom_start2__
	.section __eeprom,__s2data
__eeprom_start2__:
	.space 1

#elif defined (__linux__)

/* Linux */
#ifndef __arm__
/* PC-Linux */
.globl _eeprom_start1__
	.section	.s1eeprom,"aw",@progbits
	.type		_eeprom_start1__, @object
	.size		_eeprom_start1__, 1
_eeprom_start1__:
	.zero	1
.globl _eeprom_start2__
	.section	.s2eeprom,"aw",@progbits
	.type		_eeprom_start2__, @object
	.size		_eeprom_start2__, 1
_eeprom_start2__:
	.zero	1
#else
/* ARM-Linux */
	.globl _eeprom_start1__
		.section	.s1eeprom,"aw"
		.type		_eeprom_start1__,object
		.size		_eeprom_start1__, 1
	_eeprom_start1__:
		.zero	1
	.globl _eeprom_start2__
		.section	.s2eeprom,"aw"
		.type		_eeprom_start2__,object
		.size		_eeprom_start2__, 1
	_eeprom_start2__:
		.zero	1	
#endif /* __arm__ */
#elif defined (__WIN32__)

/* Windows */
.globl __eeprom_start1__
	.section .s1eeprom,"w"
__eeprom_start1__:
	.space 1
	
.globl __eeprom_start2__
	.section .s2eeprom,"w"
__eeprom_start2__:
	.space 1

#endif

.text

#endif // EEPROM_EMU_AVAILABLE
#endif // PC
