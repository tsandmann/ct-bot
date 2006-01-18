/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	timer.h
 * @brief 	Timer und Zaehler
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef TIMER_H_
#define TIMER_H_


// Die Werte fuer TIMER_X_CLOCK sind Angaben in Hz

/*!
 * Frequenz von Timer 2 in Hz 
 */
#define TIMER_2_CLOCK	5619	// Derzeit genutzt fuer RC5-Dekodierung

/*!
 * Initialisiert Timer 2 und startet ihn 
 */
void timer_2_init(void);
#endif
