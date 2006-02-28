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

#ifdef TIME_AVAILABLE

volatile uint16 time_micro_s=0; /*!< Mikrosekundenanteil an der Systemzeit */
volatile uint16 time_ms=0; 	 /*!< Milliekundenanteil an der Systemzeit */
volatile uint16 time_s=0; 		 /*!< Sekundenanteil an der Systemzeit */

/*! Funktion, um die Systemzeit zu berechnen 
 */
void system_time_isr(void){
	time_micro_s += TIMER_STEPS;	/* Mikrosekundentimer erhöhen */
	if (time_micro_s >= 1000){		/* Eine Mikrosekunde verstrichen? */
		time_ms += (time_micro_s/1000);
		time_micro_s %= 1000;		/* alle vollen Millisekunden vom Mikrosekundenzähler in den Millisekudnenzähler verschieben */
		if (time_ms == 1000){		/* Eine Sekunde verstrichen? */
			time_ms=0;
			time_s++;
		}
	}
}
#endif
