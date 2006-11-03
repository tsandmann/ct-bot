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

#include "ct-Bot.h"

/*!
 * Makros zur Umrechnung von Ticks in ms und zurueck
 * (ms / ticks evtl. nach uint32 casten, fuer grosse Werte)
 */
#define TICKS_TO_MS(ticks)	((ticks)*(TIMER_STEPS/8)/(1000/8))
#define MS_TO_TICKS(ms)		((ms)*(1000/8)/(TIMER_STEPS/8))

#ifdef TIME_AVAILABLE		
	/*!
	 * Diese Funktion liefert den Millisekundenanteil der Systemzeit zurueck.
	 * @return uint16
	 */
	uint16 timer_get_ms(void);
	
	/*!
	 * Diese Funktion liefert den Sekundenanteil der Systemzeit zurueck.
	 * @return uint16
	 */
	uint16 timer_get_s(void);

	/*!
	 *  Liefert die Millisekunden zurueck, die seit old_s, old_ms verstrichen sind 
	 * @param old_s alter Sekundenstand
	 * @param old_ms alter Millisekundenstand
	 */
	uint16 timer_get_ms_since(uint16 old_s, uint16 old_ms);
#endif // TIME_AVAILABLE
	
#ifdef PC
	/*! 
	 *  Funktion, die die TickCounts um die vergangene Simulzeit erhoeht
	 */
	void system_time_isr(void);

	/*!< liefert Ticks seit Systemstart [176 us] */
	inline uint16 timer_get_tickCount16(void);
	inline uint32 timer_get_tickCount32(void);	
	
	#define TIMER_GET_TICKCOUNT_16 timer_get_tickCount16()
	#define TIMER_GET_TICKCOUNT_32 timer_get_tickCount32()	
#else
	extern volatile uint16 tickCount[2];
	
	#define TIMER_GET_TICKCOUNT_16 tickCount[0]
	#define TIMER_GET_TICKCOUNT_32 *(volatile uint32*)tickCount	
#endif

// Die Werte fuer TIMER_X_CLOCK sind Angaben in Hz

/*!
 * Frequenz von Timer 2 in Hz 
 */
#define TIMER_2_CLOCK	5619	// Derzeit genutzt fuer RC5-Dekodierung

/*!
 * Mikrosekunden, die zwischen zwei Timer-Aufrufen liegen 
 */
//	#define TIMER_STEPS	(1 000 000/TIMER_2_CLOCK)
	#define TIMER_STEPS 176

/*!
 * Initialisiert Timer 2 und startet ihn 
 */
void timer_2_init(void);
#endif
