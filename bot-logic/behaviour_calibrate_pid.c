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
 * 
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

extern uint8 encoderRateInfo[2];		/*!< aktuelle Ist-Geschwindigkeit */
extern int8 Kp, Ki, Kd;					/*!< PID-Parameter */
static int16 cal_speed;					/*!< Bot-Speed fuer waehrend der Kalibrierung */
static uint32 ticks = 0;				/*!< Timestamp-Speicher */
static float xm_left;					/*!< gleitender Mittelwert links */
static float xm_right;					/*!< gleitender Mittelwert rechts */
static float vm_left;					/*!< gleitende Varianz links */
static float vm_right;					/*!< gleitende Varianz rechts */
static float best_weight;				/*!< beste Bewertung bisher */
static int8 best_Kp = 0;				/*!< bester Kp-Wert bisher */
static int8 best_Ki = 0;				/*!< bester Ki-Wert bisher */
static int8 Kp_region_end = 0;			/*!< obere Schranke des grob ermittelten Kp-Bereichs */

static void (* next_job)(void) = NULL;	/*!< naechste Teilaufgabe */
static void (* last_job)(void) = NULL;	/*!< letzte Teilaufgabe (vor Stopp) */

static const float a = 0.02;			/*!< Gewichtungsfaktor */

static void find_best_Kp_Ki(void);
static void find_best_Kp(void);

/*!
 * @brief	Bewertungsfunktion
 * @return	Bewertung
 * Je kleiner return, desto besser!
 */
static float calc_weighting(void) {
	if (encoderRateInfo[0] <= 6 && encoderRateInfo[1] <= 6) {
		vm_left = FLT_MAX;
		vm_right = FLT_MAX;
		return FLT_MAX;	
	}
	xm_left = xm_left * (1.0f-a) + (float)encoderRateInfo[0] * 20.0f * a;
	xm_right = xm_right * (1.0f-a) + (float)encoderRateInfo[1] * 20.0f * a;
	vm_left = vm_left * (1.0f-a) + pow((float)encoderRateInfo[0] * 20.0f - xm_left, 2) * a;
	vm_right = vm_right * (1.0f-a) + pow((float)encoderRateInfo[1] * 20.0f - xm_right, 2) * a;
	return vm_left * 0.5 + vm_right * 0.5;	
}

/*!
 * @brief	Initialisiert die Bewertungsfunktion neu
 * Mittelwerte werden mit Sollspeed initialisiert
 */
static void clear_weighting(void) {
	xm_left = cal_speed * 10.0f;
	xm_right = cal_speed * 10.0f;
	vm_left = 0.0f;
	vm_right = 0.0f;
}

//static void adjust_params(uint8 direction, float weight) {
//	static uint8 Kp_tmp = 0;
//	int8 old_Kp = Kp;
//	if (direction) {
//		if (Kp < 122) {
//			if (Kp_tmp) Kp += 5;
//			else Kp_tmp = 1;
//		}
//	} else {
//		if (Kp > 5) {
//			if (!Kp_tmp) Kp -= 5;
//			else Kp_tmp = 0;
//		}
//	}
//	if (old_Kp == Kp && best_weight >= fabs(weight - last_weight))
//		Ki++;
//		
//}

/*!
 * @brief	Haelt den Bot an und wartet ein wenig den Nachlauf ab
 * anschliessend geht's mit dem Aufrufer weiter
 */
static void wait_for_stop(void) {
	static uint8 count = 1;
	if (count != 0) {
		count++;
		speedWishLeft = BOT_SPEED_STOP;
		speedWishRight = BOT_SPEED_STOP;
	} else {
		count = 1;
		next_job = last_job;
	}
}

/*!
 * @brief	Sucht die beste "Kp-Region" (grob) 
 * Ergebnis steht in Kp + 10
 * Laufzeit: 3 Minuten
 */
static void find_Kp_region(void) {
	/* go! */
	speedWishLeft = -cal_speed;
	speedWishRight = cal_speed;
	
	float weight = calc_weighting();
	if (timer_ms_passed(&ticks, 15000)) {
		if (best_weight >= weight) {
			best_weight = weight;
			best_Kp = Kp;
			clear_weighting();	
			LOG_INFO("best_Kp=%d\tnach %u s", best_Kp, TICKS_TO_MS(TIMER_GET_TICKCOUNT_32)/1000);				
		}
		LOG_DEBUG("weight=%lu", (uint32)weight);
		Kp += 10;
		/* stopp! */
		last_job = find_Kp_region;
		next_job = wait_for_stop;
		
		if (Kp < 0) {
			next_job = find_best_Kp_Ki;
			Kp = best_Kp > 10 ? best_Kp - 10 : 0;
			Kp_region_end = best_Kp < 116 ? best_Kp + 10 : 120;
			Ki = 1;
			best_weight = FLT_MAX;
		}
	}
}

/*!
 * @brief	Ermittelt die beste Kp/Ki-Kombination (bei gegebener Kp-Region)
 * Ergebnis steht in Kp und Ki
 * Laufzeit: 27 Minuten
 */
static void find_best_Kp_Ki(void) {
	/* Brute-Force (das dauert => Kaffeepause) */
	if (Ki <= 20) {	// 32
		if (Kp <= Kp_region_end) {
			/* go! */
			speedWishLeft = -cal_speed;
			speedWishRight = cal_speed;
			
			float weight = calc_weighting();
			if (timer_ms_passed(&ticks, 5000)) {
				if (best_weight >= weight) {
					best_weight = weight;
					best_Kp = Kp;
					best_Ki = Ki;
					clear_weighting();
					LOG_INFO("best_Kp=%d\tbest_Ki=%d\tnach %u s", best_Kp, best_Ki, TICKS_TO_MS(TIMER_GET_TICKCOUNT_32)/1000);
				}
				LOG_DEBUG("weight=%lu", (uint32)weight);
				Kp += 2;
				/* stopp! */
				last_job = find_best_Kp_Ki;
				next_job = wait_for_stop;
			}			
		} else {
			Kp -= 20;
			Ki++;
			/* stopp! */
			last_job = find_best_Kp_Ki;
			next_job = wait_for_stop;		
		}
	} else {
		next_job = find_best_Kp;
		//Kp = best_Kp;
		Kp = 10;
		Ki = best_Ki;
		clear_weighting();
		best_weight = FLT_MAX;
	}
}

/*!
 * @brief	Ermittelt den besten Kp-Wert bei gegebenem Ki
 * Ergebnis steht in Kp
 * Laufzeit: 5 Minuten 
 */
static void find_best_Kp(void) {
	if (Kp < 121) {
		/* go! */
		speedWishLeft = -cal_speed;
		speedWishRight = cal_speed;
		
		float weight = calc_weighting();
		if (timer_ms_passed(&ticks, 5000)) {
			if (best_weight >= weight) {
				best_weight = weight;
				best_Kp = Kp;
				clear_weighting();
				LOG_INFO("best_Kp=%d\tnach %u s", best_Kp, TICKS_TO_MS(TIMER_GET_TICKCOUNT_32)/1000);			
			}
			LOG_DEBUG("weight=%lu", (uint32)weight);
			Kp += 2;
			/* stopp! */
			last_job = find_best_Kp;
			next_job = wait_for_stop;
		}			
	} else {
		next_job = NULL;
		Kp = best_Kp;	
	}	
}

/*!
 * @brief	Das Verhalten
 */
void bot_calibrate_pid_behaviour(Behaviour_t *data) {
	if (next_job != NULL) next_job();
	else return_from_behaviour(data);
}

/*!
 * @brief	Kalibriert die Motorregelung des Bots
 * Die ermittelten Parameter werden eingestellt, aber nicht dauerhaft gespeichert!
 */
void bot_calibrate_pid(Behaviour_t *caller, int16 speed) {
	/* Inits */
	ticks = TIMER_GET_TICKCOUNT_32;
	cal_speed = speed;
	clear_weighting();
	best_Kp = 0;
	best_Ki = 0;
	Kp = 10;
	Ki = 5;
	Kd = 0;	
	best_weight = FLT_MAX;
	
	next_job = find_Kp_region;
	
	/* Verhalten an */
	switch_to_behaviour(caller, bot_calibrate_pid_behaviour, OVERRIDE);
}

#endif	// BEHAVIOUR_CALIBRATE_PID_AVAILABLE
