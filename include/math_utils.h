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
 * @return 		Berechnete Winkeldifferenz [Grad]
 */
float calc_angle_diff(int16_t xDiff, int16_t yDiff);

/*!
 * Ermittelt das Vorzeichnen eines 8 Bit Integers. Interpretiert 0 immer als positiv.
 * @param z	Zahl, deren VZ gewuenscht ist
 * @return	1, falls z >= 0, -1 sonst
 */
static inline int16_t sign8(int8_t z) {
	return (z & 0x80) ? -1 : 1;
}

/*!
 * Ermittelt das Vorzeichnen eines 16 Bit Integers. Interpretiert 0 immer als positiv.
 * @param z	Zahl, deren VZ gewuenscht ist
 * @return	1, falls z >= 0, -1 sonst
 */
static inline int16_t sign16(int16_t z) {
	return (z & 0x8000) ? -1 : 1;
}

/*!
 * Ermittelt das Vorzeichnen eines 32 Bit Integers. Interpretiert 0 immer als positiv.
 * @param z	Zahl, deren VZ gewuenscht ist
 * @return	1, falls z >= 0, -1 sonst
 */
static inline int16_t sign32(int32_t z) {
	return (z & 0x80000000) ? -1 : 1;
}

/*!
 * Berechnet die Differenz eines Winkels zur aktuellen
 * Botausrichtung.
 * @param angle		Winkel [Grad] zum Vergleich mit heading
 * @return			Winkeldifferenz [Grad] in Richtung der derzeitigen Botdrehung.
 * 					-1, falls Bot geradeaus faehrt oder steht
 */
int16_t turned_angle(int16_t angle);

/*!
 * Ermittlung des Quadrat-Abstandes zwischen zwei Punkten
 * @param x1	X-Koordinate des ersten Punktes
 * @param y1	y-Koordinate des ersten Punktes
 * @param x2	X-Koordinate des zweiten Punktes
 * @param y2	Y-Koordiante des zweiten Punktes
 * @return		liefert Quadrat-Abstand zwischen den zwei Punkten
 */
uint32_t get_dist(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

/*!
 * Ermittelt die Koordinaten eines Punktes, der um dx mm in x- und
 * dy mm in y-Richtung gegenueber der aktuellen Bot-Position verschoben ist.
 * @param alpha	Winkel zum Punkt [Grad]
 * @param dx	x-Komponente des Verschiebungsvektors [mm]
 * @param dy	y-Komponente des Verschiebungsvektors [mm]
 * @return		Gesuchter Punkt
 */
position_t calc_point_in_distance(float alpha, int16_t dx, int16_t dy);

/*!
 * Multipliziert zwei vorzeichenbehaftete 8 Bit Werte
 * @param a	Faktor 1 (8 Bit signed)
 * @param b	Faktor 2 (8 Bit signed)
 * @return	a * b (16 Bit signed)
 */
static inline int16_t muls8(int8_t a, int8_t b) {
#ifdef MCU
	int16_t result;
	asm volatile(
		"muls %1,%2	\n\t"
		"movw %0,r0	\n\t"
		"clr r1			"
		: "=&r"	(result)
		: "d" (a), "d" (b)
		: "r0"
	);
	return result;
#else
	return a * b;
#endif
}

#endif	/*MATH_UTILS_H_*/
