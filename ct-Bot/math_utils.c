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
