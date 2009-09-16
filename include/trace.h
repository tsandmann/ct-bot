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
 * @file 	trace.h
 * @brief 	Trace-Modul
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	01.06.2009
 */

#ifndef TRACE_H_
#define TRACE_H_
#include "ct-Bot.h"

#ifdef CREATE_TRACEFILE_AVAILABLE


/*!
 * Initialisiert das Trace-System
 */
void trace_init(void);

/*!
 * Fuegt dem Trace-Puffer die aktuellen Sensordaten hinzu
 */
void trace_add_sensors(void);

/*!
 * Fuegt dem Tace-Puffer die aktuellen Aktuatordaten hinzu
 */
void trace_add_actuators(void);

#endif	// CREATE_TRACEFILE_AVAILABLE
#endif /* TRACE_H_ */
