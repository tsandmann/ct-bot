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
 * @file 	math_utils.h
 * @brief 	Hilfsfunktionen fuer mathematische Dinge, architekturunabhaenigig
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	17.10.2007
 */

#ifndef MATH_UTILS_H_
#define MATH_UTILS_H_

/*!
 * Rundet float und gibt das Ergebnis als int zurueck.
 * Selbst implementiert, weil es kein roundf() in der avr-libc gibt.
 * @param x	Eingabewert
 * @return	roundf(x)
 */
static inline int16_t iroundf(float x) {
	if (x >= 0) {
		return (int16_t) (x + 0.5);
	}
	return (int16_t) (x - 0.5);
}

/*!
 * Berechnung deiner Winkeldifferenz zwischen dem aktuellen Standpunkt und einem anderen Ort
 * @param xDiff	x-Differenz
 * @param yDiff	y-Differenz
 * @return 		berechnete Winkeldifferenz
 */
float calc_angle_diff(float xDiff, float yDiff);

#endif	/*MATH_UTILS_H_*/
