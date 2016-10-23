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
 * \file 	mcu/cppsupport.cpp
 * \brief 	AVR-Support for C++ virtual functions
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	23.10.2016
 */

#ifdef MCU
#include <stdlib.h>

extern "C" {
#include "log.h"
}

extern "C" void __cxa_pure_virtual(void) __attribute__ ((__noreturn__));
extern "C" void __cxa_deleted_virtual(void) __attribute__ ((__noreturn__));

void __cxa_pure_virtual() {
	LOG_ERROR("pure virtual method called, abort.");
	abort();
}

void __cxa_deleted_virtual() {
	LOG_ERROR("deleted virtual method called, abort.");
	abort();
}

#endif // MCU
