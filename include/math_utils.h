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
 * @brief 	Hilfsfunktionen fuer mathematische Dinge, architekturunabhaengig
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	17.10.2007
 */

#ifndef MATH_UTILS_H_
#define MATH_UTILS_H_

#include "global.h"

#define DEG2RAD (M_PI / 180.0)	/*!< Umrechnungsfaktor von Grad nach Bogenmass */

/*!
 * Rundet float und gibt das Ergebnis als int zurueck.
 * Selbst implementiert, weil es kein roundf() in der avr-libc gibt.
 * @param x	Eingabewert
 * @return	roundf(x)
 */
static inline int16_t iroundf(float x) {
	if (x >= 0.0f) {
		return (int16_t) (x + 0.5f);
	}
	return (int16_t) (x - 0.5f);
}

/*!
 * Wandelt einen Winkel in Grad in Bogenmass um
 * @param degree Winkel [Grad]
 * @return Winkel [Bogenmass]
 */
static inline double rad(double degree) {
	return degree * (M_PI / 180.0);
}

/*!
 * Wandelt einen Winkel in Bogenmass in Grad um
 * @param radian Winkel [Bogenmass]
 * @return Winkel [Grad]
 */
static inline double deg(double radian) {
	return radian / (M_PI / 180.0);
}

#ifdef MCU
#ifndef sqrtf
static inline float sqrtf(float x) __attribute__((__const__));

/*!
 * Berechnet die Quadratwurzel eines float-Wertes
 * @param x	Wert
 * @return	sqrt(x)
 */
static inline float sqrtf(float x) {
	return (float) sqrt((double) x);
}
#endif // !sqrtf

#ifndef sinf
static inline float sinf(float x) __attribute__((__const__));

/*!
 * Berechnet Sinus eines float-Wertes
 * @param x	Wert
 * @return	sin(x)
 */
static inline float sinf(float x) {
	return (float) sin((double) x);
}
#endif // !sinf

#ifndef cosf
static inline float cosf(float x) __attribute__((__const__));

/*!
 * Berechnet Cosinus eines float-Wertes
 * @param x	Wert
 * @return	cos(x)
 */
static inline float cosf(float x) {
	return (float) cos((double) x);
}
#endif // !cosf

#ifndef tanf
static inline float tanf(float x) __attribute__((__const__));

/*!
 * Berechnet Tangens eines float-Wertes
 * @param x	Wert
 * @return	tan(x)
 */
static inline float tanf(float x) {
	return (float) tan((double) x);
}
#endif // !tanf

#ifndef fmodf
static inline float fmodf(float x, float y) __attribute__((__const__));

/*!
 * Berechnet Modulo zweier float-Werte
 * @param x	Dividend
 * @param y	Divisor
 * @return	fmod(x, y)
 */
static inline float fmodf(float x, float y) {
	return (float) fmod((double) x, (double) y);
}
#endif // !fmodf

#ifndef atan2f
static inline float atan2f(float y, float x) __attribute__((__const__));

/*!
 * Berechnet Arcustangens2 zweier float-Werte
 * @param y	Y
 * @param x	X
 * @return	atan2(y, x)
 */
static inline float atan2f(float y, float x) {
	return (float) atan2((double) y, (double) x);
}
#endif // !atan2f
#endif // MCU

/*!
 * Berechnung einer Winkeldifferenz zwischen dem aktuellen Standpunkt und einem anderen Ort
 * @param xDiff	x-Differenz
 * @param yDiff	y-Differenz
 * @return 		Berechnete Winkeldifferenz [Bogenmass]
 */
float calc_angle_diff_rad(int16_t xDiff, int16_t yDiff);


/*!
 * Berechnung einer Winkeldifferenz zwischen dem aktuellen Standpunkt und einem anderen Ort
 * @param xDiff	x-Differenz
 * @param yDiff	y-Differenz
 * @return 		Berechnete Winkeldifferenz [Grad]
 */
static inline float calc_angle_diff(int16_t xDiff, int16_t yDiff) {
	return deg(calc_angle_diff_rad(xDiff, yDiff));
}

/*!
 * Ermittelt das Vorzeichnen eines 8 Bit Integers. Interpretiert 0 immer als positiv.
 * @param z	Zahl, deren VZ gewuenscht ist
 * @return	1, falls z >= 0, -1 sonst
 */
static inline int8_t sign8(int8_t z) {
	return (int8_t) ((z & 0x80) ? -1 : 1);
}

/*!
 * Ermittelt das Vorzeichnen eines 16 Bit Integers. Interpretiert 0 immer als positiv.
 * @param z	Zahl, deren VZ gewuenscht ist
 * @return	1, falls z >= 0, -1 sonst
 */
static inline int8_t sign16(int16_t z) {
	return (int8_t) ((z & 0x8000) ? -1 : 1);
}

/*!
 * Ermittelt das Vorzeichnen eines 32 Bit Integers. Interpretiert 0 immer als positiv.
 * @param z	Zahl, deren VZ gewuenscht ist
 * @return	1, falls z >= 0, -1 sonst
 */
static inline int8_t sign32(int32_t z) {
	return (int8_t) ((z & 0x80000000) ? -1 : 1);
}

/*!
 * Berechnet die Differenz eines Winkels zur aktuellen Botausrichtung
 * @param angle		Winkel [Grad] zum Vergleich mit heading
 * @return			Winkeldifferenz [Grad] in Richtung der derzeitigen Botdrehung.
 * 					-1, falls Bot geradeaus faehrt oder steht
 */
int16_t turned_angle(int16_t angle);

/*!
 * Ermittlung des Quadrat-Abstands zwischen zwei Punkten
 * @param x1	X-Koordinate des ersten Punktes
 * @param y1	y-Koordinate des ersten Punktes
 * @param x2	X-Koordinate des zweiten Punktes
 * @param y2	Y-Koordiante des zweiten Punktes
 * @return		liefert Quadrat-Abstand zwischen den zwei Punkten
 */
int32_t get_dist(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

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
	__asm__ __volatile__(
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
#endif // MCU
}

/*!
 * Multipliziert zwei vorzeichenlose 8 Bit Werte
 * @param a	Faktor 1 (8 Bit unsigned)
 * @param b	Faktor 2 (8 Bit unsigned)
 * @return	a * b (16 Bit unsigned)
 */
static inline uint16_t mul8(uint8_t a, uint8_t b) {
#ifdef MCU
	uint16_t result;
	__asm__ __volatile__(
		"mul %1,%2	\n\t"
		"movw %0,r0	\n\t"
		"clr r1			"
		: "=&r"	(result)
		: "d" (a), "d" (b)
		: "r0"
	);
	return result;
#else
	return a * b;
#endif // MCU
}

#ifdef BPS_AVAILABLE
/*!
 * Berechnet den Standort via Rueckwaertseinschnitt nach Cassini, wenn drei angepeilte Positionen bekannt sind.
 * @param a			Koordinaten von Bake A [mm]
 * @param m			Koordinaten von Bake M [mm]
 * @param b			Koordinaten von Bake B [mm]
 * @param angle_am	Winkel zwischen A und M [Grad] aus Peilung, > 0
 * @param angle_mb	Winkel zwischen M und B [Grad] aus Peilung, > 0
 * @return			Koordinaten des berechneten Standorts [mm], oder {INT16_MAX, INT16_MAX} falls Fehler
 */
position_t calc_resection(position_t a, position_t m, position_t b, float angle_am, float angle_mb);
#endif // BPS_AVAILABLE

#endif	/*MATH_UTILS_H_*/
