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
 * @file 	math_utils.c
 * @brief 	Hilfsfunktionen fuer mathematische Dinge, architekturunabhaenigig
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	17.10.2007
 */
#include "ct-Bot.h"
#include "sensor.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/*!
 * Berechnung einer Winkeldifferenz zwischen dem aktuellen Standpunkt und einem anderen Ort
 * @param xDiff	x-Differenz
 * @param yDiff	y-Differenz
 * @return 		Berechnete Winkeldifferenz [Grad]
 */
float calc_angle_diff(int16_t xDiff, int16_t yDiff) {
	float newHeading;
	if (yDiff != 0 && xDiff != 0) {
		newHeading = atan((float)yDiff / (float)xDiff) * (180.0f/M_PI);
		if (xDiff < 0) newHeading += 180;
	} else {
		if (xDiff == 0) newHeading = (yDiff > 0) ? 90 : -90;
		else newHeading = (xDiff > 0) ? 0 : 180;
	}

	float toTurn = newHeading - heading;
	if (toTurn > 180) toTurn -= 360;
	if (toTurn < -180) toTurn += 360;

	return toTurn;
}

/*!
 * Berechnet die Differenz eines Winkels zur aktuellen
 * Botausrichtung.
 * @param angle		Winkel [Grad] zum Vergleich mit heading 
 * @return			Winkeldifferenz [Grad] in Richtung der derzeitigen Botdrehung.
 * 					-1, falls Bot geradeaus faehrt oder steht
 */
int16_t turned_angle(int16_t angle) {
	int16_t diff = 0;
	if (v_enc_left == v_enc_right) {
		/* Drehrichtung nicht ermittelbar */
		return -1;
	}
	if (v_enc_right > v_enc_left) {
		/* Drehung im positiven Sinn */
		diff = (int16_t)heading - angle;
	} else {
		/* Drehung im negativen Sinn */
		diff = angle - (int16_t)heading;
	}
	if (diff < 0) {
		/* Ueberlauf */
		diff += 360;
	}
	return diff;
}

/*!
 * Ermittlung des Quadrat-Abstandes zwischen zwei Punkten
 * @param x1	X-Koordinate des ersten Punktes
 * @param y1	y-Koordinate des ersten Punktes
 * @param x2	X-Koordinate des zweiten Punktes
 * @param y2	Y-Koordiante des zweiten Punktes
 * @return		liefert Quadrat-Abstand zwischen den zwei Punkten 
 */
uint32_t get_dist(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	int16_t xt = x2 - x1;
	int16_t yt = y2 - y1;

	/* Abstandsermittlung nach dem guten alten Pythagoras ohne Ziehen der Wurzel */
	return (int32_t)xt * (int32_t)xt + (int32_t)yt * (int32_t)yt;
}

/*!
 * Ermittelt die Koordinaten eines Punktes, der um dx mm in x- und 
 * dy mm in y-Richtung gegenueber der aktuellen Bot-Position verschoben ist.
 * @param alpha	Winkel zum Punkt [Grad]
 * @param dx	x-Komponente des Verschiebungsvektors [mm]
 * @param dy	y-Komponente des Verschiebungsvektors [mm]
 * @param *x	Zeiger auf x-Koordinate des gesuchten Punktes
 * @param *y	Zeiger auf y-Koordinate des gesuchten Punktes
 */
void calc_point_in_distance(float alpha, int16_t dx, int16_t dy, int16_t * x, int16_t * y) {
	float h = alpha * (M_PI/180.0f);
	float cos_h = cos(h);
	float sin_h = sin(h);

	*x = x_pos + (int16_t)((dx * cos_h) - (dy * sin_h));
	*y = y_pos + (int16_t)((dy * cos_h) + (dx * sin_h));
}
