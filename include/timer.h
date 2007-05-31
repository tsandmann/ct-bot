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
 * @file 	timer.h
 * @brief 	Timer und Zaehler
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "ct-Bot.h"
#include <stdint.h>

/*!
 * Makro zur Umrechnung von Ticks in ms
 * (ms / ticks evtl. nach uint32 casten, fuer grosse Werte)
 */
#define TICKS_TO_MS(ticks)	((ticks)*(TIMER_STEPS/8)/(1000/8))

/*!
 * Makro zur Umrechnung von ms in Ticks
 * (ms / ticks evtl. nach uint32 casten, fuer grosse Werte)
 */
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

	/*! liefert Ticks in 16 Bit seit Systemstart [176 us] */
	inline uint16 timer_get_tickCount16(void);
	/*! liefert Ticks in 32 Bit seit Systemstart [176 us] */
	inline uint32 timer_get_tickCount32(void);	
	
	#define TIMER_GET_TICKCOUNT_8  (uint8)timer_get_tickCount16()	/*!< Zeit in 8 Bit */
	#define TIMER_GET_TICKCOUNT_16 timer_get_tickCount16()			/*!< Zeit in 16 Bit */
	#define TIMER_GET_TICKCOUNT_32 timer_get_tickCount32()			/*!< Zeit in 32 Bit */
#else
	/*! Union fuer TickCount in 8, 16 und 32 Bit */
	typedef union {
		uint32 u32;		/*!< 32 Bit Integer */
		uint16 u16;		/*!< 16 Bit Integer */
		uint8 u8;		/*!< 8 Bit Integer */
	} tickCount_t;
	extern volatile tickCount_t tickCount;			/*!< ein Tick alle 176 us */
	
	#define TIMER_GET_TICKCOUNT_8  tickCount.u8		/*!< Zeit in 8 Bit */
	#define TIMER_GET_TICKCOUNT_16 tickCount.u16	/*!< Zeit in 16 Bit */
	#define TIMER_GET_TICKCOUNT_32 tickCount.u32	/*!< Zeit in 32 Bit */
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
 * @brief				Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 * 
 * Die Funktion aktualisiert den Timestamp, der die alte Zeit zum Vergleich speichert, automatisch, 
 * falls ms Millisekunden vergangen sind.
 * Man verwendet sie z.B. wie folgt:
 * static uint32 old_time;
 * ...
 * if (timer_ms_passed(&old_time, 50)) {
 * 		// wird alle 50 ms ausgefuehrt //
 * }
 */
#ifndef DOXYGEN
	static inline uint8_t __attribute__((always_inline)) timer_ms_passed(void* old_ticks, uint32_t ms) {
#else
	static inline uint8_t timer_ms_passed(void* old_ticks, uint32_t ms) {
#endif
	
	/* 8 Bit Version */
	if (MS_TO_TICKS(ms) < UINT8_MAX) {
		register uint8_t ticks = TIMER_GET_TICKCOUNT_8;
		if ((uint8_t)(ticks - *(uint8*)old_ticks) > MS_TO_TICKS(ms)) {
			*(uint8_t*)old_ticks = ticks;
			return True;
		}
		return False;
		
	/* 16 Bit Version */		
	} else if (MS_TO_TICKS(ms) < UINT16_MAX) { 
		register uint16_t ticks = TIMER_GET_TICKCOUNT_16;
		if ((uint16_t)(ticks - *(uint16_t*)old_ticks) > MS_TO_TICKS(ms)) {
			*(uint16_t*)old_ticks = ticks;
			return True;
		}
		return False;
	
	/* 32 Bit Version */
	} else {
		register uint32_t ticks = TIMER_GET_TICKCOUNT_32;
		if ((uint32_t)(ticks - *(uint32_t*)old_ticks) > MS_TO_TICKS(ms)) {
			*(uint32_t*)old_ticks = ticks;
			return True;
		}
		return False;			
	}	
}

/*!
 * Initialisiert Timer 2 und startet ihn 
 */
void timer_2_init(void);
#endif
