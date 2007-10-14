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
 * @file 	os_scheduler.h
 * @brief 	Mini-Scheduler fuer BotOS
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	02.10.2007
 */

#ifndef OS_SCHEDULER_H_
#define OS_SCHEDULER_H_
#include "ct-Bot.h"

#ifdef MCU
#ifdef OS_AVAILABLE

#define OS_TIME_SLICE	10	/*!< Dauer einer Zeitscheibe in ms */

extern volatile uint8_t os_scheduling_allowed;

/*!
 * Aktualisiert den Schedule, prioritaetsbasiert
 * @param tickcount	Wert des Timer-Ticks (32 Bit)
 */
void os_schedule(uint32_t tickcount);

#endif	// OS_AVAILABLE
#endif	// MCU
#endif	// OS_SCHEDULER_H_
