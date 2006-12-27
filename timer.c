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

/*! @file 	timer.c
 * @brief 	Zeitmanagement
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include "ct-Bot.h"
#include "timer.h"

#ifdef PC
	#include <pthread.h>
	#include "sensor.h"
#endif	/* PC */

#ifdef PC
	/*! Schuetzt die Zeitsynchronisation */
	#define LOCK()		pthread_mutex_lock(&timer_mutex);
	/*! Hebt den Schutz fuer die Zeitsynchronisation wieder auf */
	#define UNLOCK()	pthread_mutex_unlock(&timer_mutex);
#else
	/*! Schuetzt die Zeitsynchronisation */
	#define LOCK()		/* ISR */
	/*! Hebt den Schutz fuer die Zeitsynchronisation wieder auf */
	#define UNLOCK()	/* ISR */
#endif	/* PC */

#ifdef PC
	/*! Schuetzt die Zeitvariablen */
	static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif	/* PC */

#ifdef MCU
	volatile tickCount_t tickCount;		/*!< ein Tick alle 176 us */
#else
	float tickCount=0;
#endif 

#ifdef PC
	inline uint16 timer_get_tickCount16(void){
		uint16 temp;
		LOCK();
		temp = tickCount;
		UNLOCK();
		return temp;
	}
	
	inline uint32 timer_get_tickCount32(void){
		uint32 temp;
		LOCK();
		temp = tickCount;	
		UNLOCK();
		return temp;
	}
#endif

#ifdef TIME_AVAILABLE
	/*!
	 * Diese Funktion liefert den Millisekundenanteil der Systemzeit zurueck.
	 * @return uint16
	 */
	uint16 timer_get_ms(void){
		return ((TIMER_GET_TICKCOUNT_32*(TIMER_STEPS/8))/(1000/8))%1000;
	}
	
	/*!
	 * Diese Funktion liefert den Sekundenanteil der Systemzeit zurueck.
	 * @return uint16
	 */
	uint16 timer_get_s(void){
		return TIMER_GET_TICKCOUNT_32*(TIMER_STEPS/16)/(1000000/16);
	}
	
	/*!
	 *  Liefert die Millisekunden zurueck, die seit old_s, old_ms verstrichen sind 
	 * @param old_s alter Sekundenstand
	 * @param old_ms alter Millisekundenstand
	 */
	uint16 timer_get_ms_since(uint16 old_s, uint16 old_ms){
		return (timer_get_s()-old_s)*1000 + timer_get_ms()-old_ms;
	}
#endif // TIME_AVAILABLE

#ifdef PC
	/*! 
	 * Funktion, die die TickCounts um die vergangene Simulzeit erhoeht
	 */
	void system_time_isr(void){
		LOCK();
		/* TickCounter [176 us] erhoehen */
		static uint16 last_simultime=0;
		if (simultime < last_simultime) last_simultime -= 10000;	// der Sim setzt simultime alle 10s zurueck auf 0
		tickCount += MS_TO_TICKS((float)(simultime - last_simultime));
		last_simultime = simultime;
		UNLOCK();
	}
#endif
