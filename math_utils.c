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
 * @return 		berechnete Winkeldifferenz
 */
float calc_angle_diff(float xDiff, float yDiff) {
	float newHeading;
	if(fabs(yDiff)>0.001 && fabs(xDiff)>0.001) {
		newHeading = atan(yDiff/xDiff)*(180.0f/M_PI);
		if (xDiff<0) newHeading += 180;
	} else {
		if(fabs(xDiff) <= 0.001) newHeading = (yDiff>0) ? 90 : -90;
		else newHeading = (xDiff>0) ? 0 : 180;
	}

	float toTurn = newHeading-heading;
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
	if ((int16_t)v_enc_left == (int16_t)v_enc_right) {
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
 * Ermittlung des Quadrat-Abstandes zwischen 2 Koordinaten
 * @param x1 x-Koordinate des ersten Punktes
 * @param y1 y-Koordinate des ersten Punktes
 * @param x2 Map-Koordinate des Zielpunktes
 * @param y2 Map-Koordinate des Zielpunktes
 * @return liefert Quadrat-Abstand zwischen den Map-Punkten 
 */
int16 get_dist(int16 x1, int16 y1, int16 x2, int16 y2) {
	int16 xt = x2 - x1;
	int16 yt = y2 - y1;

	/* Abstandsermittlung nach dem guten alten Pythagoras ohne Ziehen der Wurzel */
	return (xt * xt + yt * yt);
}
