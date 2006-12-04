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
 * @file 	mmc_vm.h
 * @brief 	Virtual Memory Management mit MMC / SD-Card
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	30.11.2006
 */

#ifndef MMC_VM_H_
#define MMC_VM_H_

#include "ct-Bot.h"  

#ifdef MMC_AVAILABLE
#ifdef MMC_VM_AVAILABLE

uint16 mmc_get_pagefaults(void);

uint32 mmcalloc(uint32 size, uint8 aligned);

uint8* mmc_get_data(uint32 addr);

uint32 mmc_get_end_of_block(uint32 addr);

#endif	// MMC_VM_AVAILABLE
#endif	// MMC_AVAILABLE
#endif	//MMC_VM_H_
