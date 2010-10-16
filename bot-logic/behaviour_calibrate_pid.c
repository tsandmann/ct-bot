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
 * @file 	behaviour_calibrate_pid.c
 * @brief 	Kalibriert die Motorregelung des Bots
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	14.04.2007
 */

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
#include <math.h>
#include <stdio.h>
#include <float.h>

#include "motor.h"
#include "timer.h"
#include "log.h"

extern uint8_t encoderRateInfo[2];		/*!< aktuelle Ist-Geschwindigkeit */
extern int8_t Kp, Ki, Kd;				/*!< PID-Parameter */
uint16_t cal_pid_ete = 0;				/*!< verbleibende Zeit bis zum Ende der Kalibrierung in Sekunden */
static int16_t cal_speed;				/*!< Bot-Speed fuer waehrend der Kalibrierung */
static uint32_t ticks = 0;				/*!< Timestamp-Speicher */
static uint32_t ticks_start = 0;		/*!< Timestamp zu Beginn der Bewertungsbrechnung */
static float xm_left;					/*!< gleitender Mittelwert links */
static float xm_right;					/*!< gleitender Mittelwert rechts */
static float vm_left;					/*!< gleitende Varianz links */
static float vm_right;					/*!< gleitende Varianz rechts */
static float best_weight;				/*!< beste Bewertung bisher */
static int8_t best_Kp = 0;				/*!< bester Kp-Wert bisher */
static int8_t best_Ki = 0;				/*!< bester Ki-Wert bisher */
static int8_t best_Kd = 0;				/*!< bester Kd-Wert bisher */
static int8_t Kp_region_end = 0;		/*!< obere Schranke des grob ermittelten Kp-Bereichs */

static void (* pNextJob)(void) = NULL;	/*!< naechste Teilaufgabe */
static void (* pLastJob)(void) = NULL;	/*!< letzte Teilaufgabe (vor Stopp) */

/*!
 * @brief	Ermittelt die beste Kp/Ki-Kombination (bei gegebener Kp-Region)
 * Ergebnis steht in Kp und Ki
 * Nachfolgende Teilfunktion ist @see find_best_Kp()
 * Laufzeit: 27 Minuten (bei max_Ki == 32)
 */
static void find_best_Kp_Ki(void);

/*!
 * @brief	Ermittelt den besten Kp-Wert bei gegebenem Ki
 * Ergebnis steht in Kp
 * Nachfolgende Teilfunktion ist @see find_best_Kd()
 * Laufzeit: 6 Minuten (bei max_Kp == 126)
 */
static void find_best_Kp(void);

/*!
 * @brief	Ermittelt den besten Kd-Wert bei gegebenem Kp und Ki
 * Ergebnis steht in Kd
 * Letzte Teilfunktion des Verhaltens
 * Laufzeit: 6 Minuten (bei max_Kd == 64)
 */
static void find_best_Kd(void);

/*	<einstellbare Parameter>	*/
static const float a = 0.01;			/*!< Gewichtungsfaktor */
static const uint8_t max_Kp = 126;		/*!< groesstes Kp, das behandelt wird (0; 127) */
static const uint8_t max_Ki = 32;		/*!< groesstes Ki, das behandelt wird (0; 127) */
static const uint8_t max_Kd = 64;		/*!< groesstes Kd, das behandelt wird [0; 127) */
/*	</einstellbare Parameter>	*/


/*!
 * @brief	Hilfsfunktion fuer wait_for_stop()
 * @see		wait_for_stop()
 */
static void wait_for_stop_helper(void) {
	speedWishLeft = BOT_SPEED_STOP;
	speedWishRight = BOT_SPEED_STOP;

	/* Nachlauf abwarten */
	if (fabs(v_enc_left) < 10.0f && fabs(v_enc_right) < 10.0f) {
		/* zurueck zum Aufrufer */
		pNextJob = pLastJob;	// wurde zuvor von wait_for_stop() gerettet
	}
}

/*!
 * @brief	Haelt den Bot an und wartet den Nachlauf ab
 * anschliessend geht's mit dem Aufrufer weiter
 */
static inline void wait_for_stop(void) {
	pLastJob = pNextJob;
	pNextJob = wait_for_stop_helper;
}

/*!
 * @brief	Bewertungsfunktion
 * @return	Die Bewertung
 * Je kleiner return, desto besser!
 */
static inline float calc_weighting(void) {
	if (encoderRateInfo[0] <= 6 || encoderRateInfo[1] <= 6) {
		/* mindestens ein Rad steht still => ganz schlecht */
		vm_left = FLT_MAX/2;
		vm_right = FLT_MAX/2;
		return FLT_MAX/2;
	}

	/* Mittelwert und Varianz links und rechts aktualisieren */
	xm_left = xm_left * (1.0f - a) + (float)encoderRateInfo[0] * a;
	xm_right = xm_right * (1.0f - a) + (float)encoderRateInfo[1] * a;
	vm_left = vm_left * (1.0f - a) + pow((float)encoderRateInfo[0] - xm_left, 2) * a;
	vm_right = vm_right * (1.0f - a) + pow((float)encoderRateInfo[1] - xm_right, 2) * a;

	/* Durchschnitt von Varianz links und rechts ergibt neue Bewertung */
	return vm_left * 0.5 + vm_right * 0.5;
}

/*!
 * @brief	Initialisiert die Daten der Bewertungsfunktion neu
 * Mittelwerte werden mit Sollspeed initialisiert
 */
static void clear_weighting(void) {
	xm_left = cal_speed;
	xm_right = cal_speed;
	vm_left = 0.0f;
	vm_right = 0.0f;
	ticks_start = TIMER_GET_TICKCOUNT_32;
}

/*!
 * @brief			Berechnet aktuelle Mittelwerte und Varianzen und prueft nach dt ms auf bessere Bewertung
 * @param dt		Zeit in ms bevor eine neue Bewertung erstellt wird
 * @param pid_param	Zeiger auf den veraenderlichen PID-Parameter
 * @param step		Wert, um den *pid_param inkrementiert wird
 * Neue beste Parameter werden gespeichert und vor jeder Parameteraenderung wird der Bot kurz angehalten.
 * Bei neuem besten Wert werden die bisher besten Parameter per LOG ausgegeben
 */
static void compare_weightings(const uint16_t dt, int8_t * pid_param, const int8_t step) {
	/* Mittelwerte und Varianzen aktualisieren */
	float weight = calc_weighting();

	/* nach dt ms aktuelle Bewertung mit bisher bester vergleichen */
	if (timer_ms_passed_32(&ticks, dt)) {
		uint32_t dt_real = TIMER_GET_TICKCOUNT_32 - ticks_start;
		weight = weight / (float)dt_real * (dt*1000.0f/176.0f);		// Bewertung normieren
		if (best_weight >= weight) {
			/* Verbesserung => die aktuellen Parameter merken */
			best_weight = weight;
			best_Kp = Kp;
			best_Ki = Ki;
			best_Kd = Kd;

			/* User per LOG informieren */
			LOG_INFO("Kp=%d\tKi=%d\tKd=%d\tnach %u s", best_Kp, best_Ki, best_Kd, TICKS_TO_MS(TIMER_GET_TICKCOUNT_32)/1000);
//			uint32_t weight_int = weight * 1000000.0f;
			LOG_DEBUG("weight=%lu", (uint32_t)(weight * 1000000.0f));
		}

		/* neuen Parameter einstellen und Nachlauf abwarten */
		*pid_param = (int8_t) (*pid_param + step);

//		uint32_t weight_int = weight * 1000000.0f;
//		LOG_DEBUG("weight=%lu", weight_int);

		wait_for_stop();

		/* alten Daten verwerfen */
		clear_weighting();

		/* verbleibende Zeit aktualisieren */
		cal_pid_ete -= dt / 1000;
	}
}

/*!
 * @brief	Sucht die beste "Kp-Region" (sehr grob)
 * Ergebnis steht in Kp + 10
 * Nachfolgende Teilfunktion ist @see find_best_Kp_Ki()
 * Laufzeit: 3 Minuten (bei max_Kp == 126)
 */
static void find_Kp_region(void) {
	/* Go! */
	speedWishLeft = -cal_speed;
	speedWishRight = cal_speed;

	/* Kp alle 15 s um 10 erhoehen und besten Parameter speichern */
	compare_weightings(15000, &Kp, 10);

	if (Kp < 0 || Kp >= max_Kp) {
		/* alle Kp ueberprueft => nun Ki suchen */
		pNextJob = find_best_Kp_Ki;

		/* wir suchen Ki in der Umgebung (+/- 10) von best_Kp */
		Kp = (int8_t) (best_Kp > 10 ? best_Kp - 10 : 0);
		Kp_region_end = (int8_t) (best_Kp < 116 ? best_Kp + 10 : 120);
		Ki = 1;
		best_weight = FLT_MAX;
	}
}

/*
 * Ermittelt die beste Kp/Ki-Kombination (bei gegebener Kp-Region)
 * Ergebnis steht in Kp und Ki
 * Nachfolgende Teilfunktion ist @see find_best_Kp()
 * Laufzeit: 27 Minuten (bei max_Ki == 32)
 */
static void find_best_Kp_Ki(void) {
	/* Brute-Force (das dauert => Kaffeepause) */

	/* Go! */
	speedWishLeft = -cal_speed;
	speedWishRight = cal_speed;

	/* Kp alle 5 s um 2 erhoehen und besten Parameter speichern */
	compare_weightings(5000, &Kp, 2);

	if (Kp > Kp_region_end) {
		/* alle Kp fuer aktuelles Ki ueberprueft => mit naechstem Ki weiter */
		Kp = (int8_t) (Kp - 22);
		Ki++;
	}
	if (Ki < 0 || Ki > max_Ki) {
		/* Ki-Suche beendet, jetzt bestes Kp zu diesem Ki suchen */
		pNextJob = find_best_Kp;
		Kp = 10;
		Ki = best_Ki;
		clear_weighting();
		best_weight = FLT_MAX;
	}
}

/*
 * Ermittelt den besten Kp-Wert bei gegebenem Ki
 * Ergebnis steht in Kp
 * Nachfolgende Teilfunktion ist @see find_best_Kd()
 * Laufzeit: 6 Minuten (bei max_Kp == 126)
 */
static void find_best_Kp(void) {
	/* Go! */
	speedWishLeft = -cal_speed;
	speedWishRight = cal_speed;

	/* Kp alle 5 s um 2 erhoehen und besten Parameter speichern */
	compare_weightings(5000, &Kp, 2);

	if (Kp < 0 || Kp > max_Kp) {
		/* Kp-Suche beendet, jetzt bestes Kd zu dieser Kp/Ki-Kombination suchen */
		pNextJob = find_best_Kd;
		Kp = best_Kp;
		Kd = 0;
		clear_weighting();
		best_weight = FLT_MAX;
	}
}

/*
 * Ermittelt den besten Kd-Wert bei gegebenem Kp und Ki
 * Ergebnis steht in Kd
 * Letzte Teilfunktion des Verhaltens
 * Laufzeit: 6 Minuten (bei max_Kd == 64)
 */
static void find_best_Kd(void) {
	/* Go! */
	speedWishLeft = -cal_speed;
	speedWishRight = cal_speed;

	/* Kd alle 5 s um 1 erhoehen und besten Parameter speichern */
	compare_weightings(5000, &Kd, 1);

	if (Kd < 0 || Kd > max_Kd) {
		/* Kd-Suche beendet, also auch Ende der Kalibrierung erreicht :-) */
		pNextJob = NULL;
		Kd = best_Kd;
		LOG_INFO("Job done! Kp=%d\tKi=%d\tKd=%d", Kp, Ki, Kd);
	}
}

/*!
 * @brief		Das eigentliche Verhalten
 * @param data	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @see			bot_calibrate_pid()
 * Die Funktionalitaet des Verhaltens ist aufgeteilt in:
 * @see find_Kp_region(), @see find_best_Kp_Ki(), @see find_best_Kp() und @see find_best_Kd()
 */
void bot_calibrate_pid_behaviour(Behaviour_t *data) {
	if (pNextJob) pNextJob();
	else return_from_behaviour(data);
}

/*!
 * @brief			Kalibriert die Motorregelung des ct-Bots
 * @param *caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @param speed		Geschwindigkeit, mit der Kalibriert werden soll (normalerweise BOT_SPEED_SLOW)
 * Die ermittelten Parameter werden eingestellt, aber nicht dauerhaft gespeichert!
 */
void bot_calibrate_pid(Behaviour_t * caller, int16_t speed) {
	/* Inits */
	ticks = TIMER_GET_TICKCOUNT_32;
	cal_speed = speed;
	clear_weighting();
	best_Kp = 0;
	best_Ki = 0;
	best_Kd = 0;
	Kp = 10;
	Ki = 5;
	Kd = 0;
	best_weight = FLT_MAX;
	/* Zeit fuer 		finde Kp-Region 	+	finde Kp/Ki-Kombi	+	finde Kp	+		finde Kd */
	cal_pid_ete = ((max_Kp - 9) / 10 + 1) * 15 + max_Ki * 11 * 5 + (max_Kp - 9) * 5 / 2 + (max_Kd + 1) * 5;

	/* Als erstes beste "Kp-Region" suchen */
	pNextJob = find_Kp_region;

	/* Verhalten an */
	switch_to_behaviour(caller, bot_calibrate_pid_behaviour, OVERRIDE);
}

#endif	// BEHAVIOUR_CALIBRATE_PID_AVAILABLE
