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
 * \file 	ctbot_comp.h
 * \brief 	Compatibility wrapper for SD Fat library Print class with ct-Bot UART functions
 * \author	Timo Sandmann
 * \date 	23.10.2016
 */

#ifndef MCU_SDFAT_CTBOT_COMP_H_
#define MCU_SDFAT_CTBOT_COMP_H_

#ifdef MCU
#include "FatLib/FatLibConfig.h"

#if SDFAT_PRINT_SUPPORT
#include "FatLib/FatVolume.h"
#include "Print.h"

class Uart_Print : public Print {
public:
    virtual size_t write(uint8_t data);
    virtual size_t write(const uint8_t* buffer, size_t size);
};

extern Uart_Print Serial;
#endif // SDFAT_PRINT_SUPPORT

#endif // MCU
#endif /* MCU_SDFAT_CTBOT_COMP_H_ */
