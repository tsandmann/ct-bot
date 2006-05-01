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
#endif	/* PC */

#ifdef TIME_AVAILABLE

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

static volatile uint16 time_micro_s=0;	/*!< Mikrosekundenanteil an der Systemzeit */
static volatile uint16 time_ms=0; 	 	/*!< Millisekundenanteil an der Systemzeit */
static volatile uint16 time_s=0;		/*!< Sekundenanteil an der Systemzeit */

/*!
 * Diese Funktion liefert den Mikrosekundenanteil der Systemzeit zurueck.
 * @return uint16
 */
inline uint16 timer_get_us(void) {
	uint16 temp;
	LOCK();
	temp = time_micro_s;
	UNLOCK();
	return temp;
}

/*!
 * Diese Funktion liefert den Millisekundenanteil der Systemzeit zurueck.
 * @return uint16
 */
inline uint16 timer_get_ms(void) {
	uint16 temp;
	LOCK();
	temp = time_ms;
	UNLOCK();
	return temp;
}

/*!
 * Diese Funktion liefert den Sekundenanteil der Systemzeit zurueck.
 * @return uint16
 */
inline uint16 timer_get_s(void) {
	uint16 temp;
	LOCK();
	temp = time_s;
	UNLOCK();
	return temp;
}

/*! Funktion, um die Systemzeit zu berechnen 
 */
void system_time_isr(void){
	LOCK();
	time_micro_s += TIMER_STEPS;	/* Mikrosekundentimer erhoehen */
	if (time_micro_s >= 1000){		/* Eine Mikrosekunde verstrichen? */
		time_ms += (time_micro_s/1000);
		time_micro_s %= 1000;		/* alle vollen Millisekunden vom Mikrosekundenzaehler in den Millisekudnenzaehler verschieben */
		if (time_ms == 1000){		/* Eine Sekunde verstrichen? */
			time_ms=0;
			time_s++;
		}
	}
	UNLOCK();
}

/*!
 *  Liefert die Millisekunden zurueck, die seit old_s, old_ms verstrichen sind 
 * @param old_s alter Sekundenstand
 * @param old_ms alter Millisekundenstand
 */

int16 timer_get_ms_since(uint16 old_s, uint16 old_ms ){
	return timer_get_s()*1000+timer_get_ms() - old_s*1000 - old_ms;
}

#endif
