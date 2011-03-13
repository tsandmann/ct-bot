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
 * \file 	builtins.h
 * \brief 	Compiler-Builtins mit Fallback fuer aeltere avr-libc Versionen
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	13.03.2011
 */

#ifndef BUILTINS_H_
#define BUILTINS_H_

#ifdef MCU
#include <avr/version.h>

#if __AVR_LIBC_VERSION__ >= 10701UL
#include <avr/builtins.h>

#else // __AVR_LIBC_VERSION__ < 10701UL

/* Code aus avr/builtins.h von avr-libc 1.7.1 */
#ifndef _AVR_BUILTINS_H_
#define _AVR_BUILTINS_H_

#ifndef __HAS_DELAY_CYCLES
#define __HAS_DELAY_CYCLES 1
#endif

/**
 * Enables interrupts by setting the global interrupt mask.
 */
extern void __builtin_avr_sei(void);

/**
 * Disables all interrupts by clearing the global interrupt mask.
 */
extern void __builtin_avr_cli(void);

/**
 * Emits a sequence of instructions causing the CPU to spend
 * \c __n cycles on it.
 */
extern void __builtin_avr_delay_cycles(unsigned long __n);

#endif // _AVR_BUILTINS_H_
#endif // __AVR_LIBC_VERSION__

#endif // MCU
#endif // BUILTINS_H_
