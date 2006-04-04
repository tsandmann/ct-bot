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

volatile int16 clock_motor_control_l, clock_motor_control_r;  // Zaehler für Abtastzeit der Motorticks

#ifdef TIME_AVAILABLE
	/*!
	 * Diese Funktion liefert den Mikrosekundenanteil der Systemzeit zurueck.
	 * @return uint16
	 */
	inline uint16 timer_get_us(void);
	
	/*!
	 * Diese Funktion liefert den Millisekundenanteil der Systemzeit zurueck.
	 * @return uint16
	 */
	inline uint16 timer_get_ms(void);
	
	/*!
	 * Diese Funktion liefert den Sekundenanteil der Systemzeit zurueck.
	 * @return uint16
	 */
	inline uint16 timer_get_s(void);

	/*! Funktion, um die Systemzeit zu berechnen 
	 */
	void system_time_isr(void);
#endif

// Die Werte fuer TIMER_X_CLOCK sind Angaben in Hz

/*!
 * Frequenz von Timer 2 in Hz 
 */
#define TIMER_2_CLOCK	5619	// Derzeit genutzt fuer RC5-Dekodierung

/*!
 * Mikrosekunden die zwischen zwei Timer-Aufrufen liegen 
 */
#ifdef MCU
	#define TIMER_STEPS	(1000000/TIMER_2_CLOCK)
#else
	/* Auf dem PC koppeln wir das an die Zeitbasis des Simulators, derzeit 10ms also 10000 Microsekunden */
	#define TIMER_STEPS	10000
#endif
/*!
 * Initialisiert Timer 2 und startet ihn 
 */
void timer_2_init(void);
#endif
