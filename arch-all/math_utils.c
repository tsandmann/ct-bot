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

/**
 * \file 	math_utils.c
 * \brief 	Hilfsfunktionen fuer mathematische Dinge, architekturunabhaengig
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	17.10.2007
 */
#include "ct-Bot.h"
#include "math_utils.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "sensor.h"
#include "log.h"

/**
 * Berechnung einer Winkeldifferenz zwischen dem aktuellen Standpunkt und einem anderen Ort
 * \param xDiff	x-Differenz
 * \param yDiff	y-Differenz
 * \return 		Berechnete Winkeldifferenz [Bogenmass]
 */
float calc_angle_diff_rad(int16_t xDiff, int16_t yDiff) {
	const float newHeading = atan2((float) yDiff, (float) xDiff);

	float toTurn = newHeading - rad(heading);
	if (toTurn > M_PI) toTurn -= 2.0f * M_PI;
	if (toTurn < -M_PI) toTurn += 2.0f * M_PI;

	return toTurn;
}

/**
 * Berechnet die Differenz eines Winkels zur aktuellen Botausrichtung
 * \param angle		Winkel [Grad] zum Vergleich mit heading
 * \return			Winkeldifferenz [Grad] in Richtung der derzeitigen Botdrehung.
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
		diff = heading_int - angle;
	} else {
		/* Drehung im negativen Sinn */
		diff = angle - heading_int;
	}
	if (diff < 0) {
		/* Ueberlauf */
		diff += 360;
	}
	return diff;
}

/**
 * Ermittlung des Quadrat-Abstands zwischen zwei Punkten
 * \param x1	X-Koordinate des ersten Punktes
 * \param y1	y-Koordinate des ersten Punktes
 * \param x2	X-Koordinate des zweiten Punktes
 * \param y2	Y-Koordiante des zweiten Punktes
 * \return		liefert Quadrat-Abstand zwischen den zwei Punkten
 */
int32_t get_dist(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	int16_t xt = x2 - x1;
	int16_t yt = y2 - y1;

	/* Abstandsermittlung nach dem guten alten Pythagoras ohne Ziehen der Wurzel */
	return (int32_t) ((int32_t) xt * (int32_t) xt) + (int32_t) ((int32_t) yt * (int32_t) yt);
}

/**
 * Ermittelt die Koordinaten eines Punktes, der um dx mm in x- und
 * dy mm in y-Richtung gegenueber der aktuellen Bot-Position verschoben ist.
 * \param alpha	Winkel zum Punkt [Grad]
 * \param dx	x-Komponente des Verschiebungsvektors [mm]
 * \param dy	y-Komponente des Verschiebungsvektors [mm]
 * \return		Gesuchter Punkt
 */
position_t calc_point_in_distance(float alpha, int16_t dx, int16_t dy) {
	float h = rad(alpha);
	float cos_h = cosf(h);
	float sin_h = sinf(h);

	position_t dest;
	dest.x = x_pos + (int16_t)((dx * cos_h) - (dy * sin_h));
	dest.y = y_pos + (int16_t)((dy * cos_h) + (dx * sin_h));
	return dest;
}

#if defined BPS_AVAILABLE && ! defined BOT_2_RPI_AVAILABLE
/**
 * Berechnet den Standort via Rueckwaertseinschnitt nach Cassini, wenn drei angepeilte Positionen bekannt sind.
 * \param a			Koordinaten von Bake A [mm]
 * \param m			Koordinaten von Bake M [mm]
 * \param b			Koordinaten von Bake B [mm]
 * \param angle_am	Winkel zwischen A und M [Grad] aus Peilung, > 0
 * \param angle_mb	Winkel zwischen M und B [Grad] aus Peilung, > 0
 * \return			Koordinaten des berechneten Standorts [mm], oder {INT16_MAX, INT16_MAX} falls Fehler
 */
position_t calc_resection(position_t a, position_t m, position_t b, float angle_am, float angle_mb) {
	// Bezeichner wie im pdf
	float alpha = rad(angle_am); // ab hier Bogenmass
	float beta = rad(angle_mb);

	/* Definitionsbereich von tan() und cot() pruefen */
	if (fmodf(alpha, M_PI_2) == 0.0f) {
		alpha += 0.001f;
	}
	if (fmodf(beta, M_PI_2) == 0.0f) {
		beta += 0.001f;
	}

	float cot_alpha = 1.0f / tanf(alpha); // cot(alpha)
	float cot_beta = 1.0f / tanf(beta);

	float Yc = a.y + (a.x - m.x) * cot_alpha;
	float Xc = a.x - (a.y - m.y) * cot_alpha;

	float Yd = b.y + (m.x - b.x) * cot_beta;
	float Xd = b.x - (m.y - b.y) * cot_beta;

	/* gefaehrlichen Kreis pruefen */
	if (Xc == Xd && Yc == Yd) {
		// C und D liegen auf einem Kreis -> Verfahren geht nicht
		return (position_t){INT16_MAX, INT16_MAX};
	}

	float m1 = (Yd - Yc) / (Xd - Xc); // m1 != 0
	float m1_rec = 1.0f / m1; // m1_rec != 0
	float m1_sq = m1 * m1;
	float b1 = Yc - m1 * Xc;
	float b2 = m.y + m1_rec * m.x;

	float Xn = (b2 - b1) / (m1 + m1_rec);
	float Yn = (b1 + b2 * m1_sq) / (m1_sq + 1.0f);


//	if (a.x != Xn && m.x != Xn && b.x != Xn) {
//
//		//float n_a = atan2(a.y - Yn, a.x - Xn);
//		float n_a = atan((a.y - Yn) / (a.x - Xn));
//	//	printf("n_a=%f\n", n_a);
//		//float n_m = atan2(m.y - Yn, m.x - Xn);
//		float n_m = atan((m.y - Yn) / (m.x - Xn));
//	//	printf("n_m=%f\n", n_m);
//		//float n_b = atan2(b.y - Yn, b.x - Xn);
//		float n_b = atan((b.y - Yn) / (b.x - Xn));
//	//	printf("n_b=%f\n", n_b);
//
//		float alpha2 = n_a - n_m;
//		float beta2 = n_m - n_b;
//
//	//	printf("alpha2=%f\tbeta2=%f\n", alpha2, beta2);
//	//	printf("N = (%f|%f)\n", Xn, Yn);
//
//		if (fabs(alpha - alpha2) > 0.1f && fabs(fabs(alpha - alpha2) - M_PI) > 0.1f) {
//			LOG_ERROR("A=(%d|%d) M=(%d|%d) B=(%d|%d)", a.x, a.y, m.x, m.y, b.x, b.y);
//			LOG_ERROR("alpha=%f beta=%f", alpha / DEG2RAD, beta / DEG2RAD);
//			LOG_ERROR(" Abweichung bei alpha von %f Grad", fabs(alpha - alpha2) / DEG2RAD);
//			LOG_ERROR("  alpha2=%f alpha=%f", alpha2 / DEG2RAD, alpha / DEG2RAD);
//			LOG_ERROR("");
//		}
//		if (fabs(beta - beta2) > 0.1f && fabs(fabs(beta - beta2) - M_PI) > 0.1f) {
//			LOG_ERROR("A=(%d|%d) M=(%d|%d) B=(%d|%d)", a.x, a.y, m.x, m.y, b.x, b.y);
//			LOG_ERROR("alpha=%f beta=%f", alpha / DEG2RAD, beta / DEG2RAD);
//			LOG_ERROR(" Abweichung bei beta von %f Grad", fabs(beta - beta2) / DEG2RAD);
//			LOG_ERROR("  beta2=%f beta=%f", beta2 / DEG2RAD, beta / DEG2RAD);
//			LOG_ERROR("");
//		}
//	}

	position_t n;
	n.x = iroundf(Xn);
	n.y = iroundf(Yn);
	return n;
}

#ifdef PC
void test_calc_resection(void);

/**
 * Testet calc_resection()
 */
void test_calc_resection(void) {
	position_t a = {1147, 1131};
	position_t m = {764, 750};
	position_t b = {865, 224};
	position_t n;
	float am = 29;
	float mb = 35;

	for (a.x=5000; a.x<=7000; a.x+=500) {
		for (a.y=5000; a.y<=7000; a.y+=500) {
			for (m.x=2000; m.x<=4000; m.x+=500) {
				for (m.y=2000; m.y<=4000; m.y+=500) {
					if (a.x == m.x && a.y == m.y) {
						continue;
					}
					if (a.y < m.y) {
						break;
					}
					for (b.x=0; b.x<=2000; b.x+=500) {
						for (b.y=0; b.y<=2000; b.y+=500) {
							if ((m.x == b.x && m.y == b.y) || (a.x == b.x && a.y == b.y)) {
								continue;
							}
							if (m.y < b.y) {
								break;
							}

							for (n.x=8000; n.x<=16000; n.x++) {
								for (n.y=8000; n.y<=16000; n.y++) {
									if ((n.x == a.x && n.y == a.y) || (n.x == m.x && n.y == m.y) || (n.x == b.x && n.y == b.y)) {
										continue;
									}

									double n_a = deg(atan2(a.x - n.x, a.y - n.y));
									double n_m = deg(atan2(m.x - n.x, m.y - n.y));
									double n_b = deg(atan2(b.x - n.x, b.y - n.y));

									am = n_m - n_a;
									mb = n_b - n_m;

									position_t res = calc_resection(a, m, b, am, mb);

									if (abs(res.x - n.x) > 1 || abs(res.y - n.y) > 1) {
										LOG_ERROR("A=(%d|%d) M=(%d|%d) B=(%d|%d)", a.x, a.y, m.x, m.y, b.x, b.y);
										LOG_ERROR("alpha=%f beta=%f", am, mb);
										LOG_ERROR(" Abweichung vom gesuchten Standort:");
										LOG_ERROR(" erwartet:(%d|%d) erhalten:(%d|%d)", n.x, n.y, res.x, res.y);
									}
								}
							}


//							for (am=10; am<90; am++) {
//								for (mb=10; mb<90; mb++) {

//									calc_resection(a, m, b, am, mb);

//									double n_a = atan2(a.x - n.x, a.y - n.y) / DEG2RAD;
//		//							printf("n_a=%f\n", n_a);
//									double n_m = atan2(m.x - n.x, m.y - n.y) / DEG2RAD;
//		//							printf("n_m=%f\n", n_m);
//									double n_b = atan2(b.x - n.x, b.y - n.y) / DEG2RAD;
//		//							printf("n_b=%f\n", n_b);
//
//									double alpha = n_m - n_a;
//									double beta = n_b - n_m;
//
//									if (alpha < 0.0f) {
//										alpha = 360.0f + alpha;
//									}
//									if (beta < 0.0f) {
//										beta = 360.0f + beta;
//									}
//
//									count1++;
//									float diff_a = fabs(alpha - am);
//									while (diff_a > 173.0f) {
//										diff_a = fabs(diff_a - 180.0f);
//									}
//									float diff_b = fabs(beta - mb);
//									while (diff_b > 173.0f) {
//										diff_b = fabs(diff_b - 180.0f);
//									}
//									if (diff_a > 7.0f || diff_b > 7.0f) {
//										count2++;
//										LOG_ERROR("alpha=%f != am=%d || beta=%f != mb=%d", alpha, am, beta, mb);
//										LOG_ERROR("i1=%d j1=%d i2=%d j2=%d i3=%d j3=%d", i1, j1, i2, j2, i3, j3);
//									}
//								}
//							}
						}
					}
				}
			}
		}
	}

//	position_t n = calc_resection(a, m, b, am, mb);
//
//	printf("N = (%d|%d)\n", n.x, n.y);
//
//	double n_a = atan2(a.y - n.y, a.x - n.x) / DEG2RAD;
//	printf("n_a=%f\n", n_a);
//	double n_m = atan2(m.y - n.y, m.x - n.x) / DEG2RAD;
//	printf("n_m=%f\n", n_m);
//	double n_b = atan2(b.y - n.y, b.x - n.x) / DEG2RAD;
//	printf("n_b=%f\n", n_b);
//
//	double alpha = fabs(n_m - n_a);
//	double beta = fabs(n_b - n_m);
//
//	printf("alpha=%f\tbeta=%f\n", alpha, beta);
}
#endif // PC
#endif // BPS_AVAILABLE
