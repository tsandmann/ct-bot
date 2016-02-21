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
 * \file 	behaviour_calibrate_sharps.c
 * \brief 	Kalibriert die Distanzsensoren des Bots
 *
 * Einige Zeilen sind auskommentiert. Sie waren dazu gedacht, die Kalibrierung zu automatisieren,
 * das geht aber zurzeit (noch) nicht, weil die Positionsbestimmung fuer kleine Distanzen etwas
 * zu ungenau ist (=> Encoderdatenauswertung checken!)
 *
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	21.04.2007
 */

#include "bot-logic.h"

#ifdef BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sensor.h"
#include "display.h"
#include "gui.h"
#include "rc5-codes.h"
#include "log.h"
#include "eeprom.h"

#define STEP_COUNT 14 // Anzahl der Entfernungen, an denen gemessen wird

//static Behaviour_t* data = NULL;
//static float start_x = 0;
//static float start_head = 0;
static uint8_t last_toggle = 0;			/**< letztes Toggle-Bit der Distsensoren */
static uint8_t step = 0;				/**< Abstand zum naechsten Messpunkt [cm] */
static uint8_t count = 0;				/**< aktueller Messpunkt */
static uint16_t distL = 0;				/**< Rohdaten des linken Sharps */
static uint16_t distR = 0;				/**< Rohdaten des rechten Sharps */
static uint16_t distance = 0;			/**< Entfernung zum Hindernis [cm] */
static int8_t measure_count = 0;		/**< Counter fuer Sharp-Messungen */
static uint8_t userinput_done = 0;		/**< 1: User war schon fleissig, 0: warten */

static distSens_t buffer[2][STEP_COUNT]; /**< Puffer des Kalibrierungsergebnisses im RAM */

static void (* pNextJob)(void) = NULL;	/**< naechste Teilaufgabe */
static void (* pLastJob)(void) = NULL;	/**< letzte Teilaufgabe (vor Stopp) */

static void goto_next_pos(void);		/**< Stellt den Bot auf die naechste Position bzw. laesst den User das tun */

static const uint8_t max_steps = STEP_COUNT; /**< Anzahl der Entfernungen, an denen gemessen wird */

///**
// * Hilfsfunktion fuer wait_for_stop()
// * \see wait_for_stop()
// */
//static void wait_for_stop_helper(void) {
//	speedWishLeft = BOT_SPEED_STOP;
//	speedWishRight = BOT_SPEED_STOP;
//
//	/* Nachlauf abwarten */
//	if (fabs(v_enc_left) < 1.f && fabs(v_enc_right) < 1.f) {
//		/* zurueck zum Aufrufer */
//		pNextJob = pLastJob; // wurde zuvor von wait_for_stop() gerettet
//	}
//}
//
///**
// * Haelt den Bot an und wartet den Nachlauf ab
// * anschliessend geht's mit dem Aufrufer weiter
// */
//static inline void wait_for_stop(void) {
//	pLastJob = pNextJob;
//	pNextJob = wait_for_stop_helper;
//}

/**
 * Hilfsfunktion fuer wait_for_userinput()
 * \see	 wait_for_userinput()
 */
static void wait_for_userinput_helper(void) {
	/* Einagbe abwarten */
	if (userinput_done) {
		userinput_done = 0;
		/* zurueck zum Aufrufer */
		pNextJob = pLastJob; // wurde zuvor von wait_for_userinput() gerettet
	}
}

/**
 * Haelt den Bot an und wartet eine Useraktion ab
 * anschliessend geht's mit dem Aufrufer weiter
 */
static inline void wait_for_userinput(void) {
	pLastJob = pNextJob;
	pNextJob = wait_for_userinput_helper;
}

/**
 * Berechnet die aktuelle Entfernung eines Sensors zum Ausganspunkt / dem Hindernis
 * \param sensor	0: links, 1: rechts
 * \return			Entfernung [mm]
 */
static uint16_t calc_distance(uint8_t sensor) {
	(void) sensor;
//	float dHead = (start_head - heading) * 2.f * M_PI / 360.f;
//	float dX = start_x - x_enc;
//	float s_m = dX / cos(dHead);
//	float dS = tan(dHead) * DISTSENSOR_POS_SW;
//	float result = sensor == 0 ? s_m - dS : s_m + dS;
//	return result;
	return distance * 10; // cm in mm umrechnen
}

/**
 * Schreibt Spannung und Entfernung in den RAM-Puffer
 */
static void update_data(void) {
	uint16_t dist = calc_distance(0);
	LOG_INFO("%u: links: %u mm = %u", count, dist, distL);
	buffer[0][count].dist = dist;
	buffer[0][count].voltage = distL;
	dist = calc_distance(1);
	LOG_INFO("%u: rechts: %u mm = %u", count, dist, distR);
	buffer[1][count].dist = dist;
	buffer[1][count].voltage = distR;

	if (count != max_steps - 1) {
		/* neue Entfernung */
		pNextJob = goto_next_pos;
	} else {
		/* fertig */
		count++;
		pNextJob = NULL;
	}
}

/**
 * Misst die Entfernung mit den Sharps
 */
static void measure_distance(void) {
	if (measure_count == 0) {
		distL = 0;
		distR = 0;
	}
	/* auf neue Messung pruefen */
	if (last_toggle != sensDistLToggle) {
		last_toggle = sensDistLToggle;
		measure_count++;
	} else {
		return;
	}
	distL += (uint16_t) sensDistL;
	distR += (uint16_t) sensDistR;
	/* acht Messungen abwarten */
	if (measure_count == 8) {
		distL >>= 3;
		distR >>= 3;

		last_toggle = 1;
		pNextJob = update_data;
		measure_count = 0;
	}
}

/**
 * Stellt den Bot auf die naechste Position bzw. laesst den User das tun
 */
static void goto_next_pos(void) {
	//bot_drive_distance(data, 0, -50, step); // step cm zurueck
	distance = distance + step;
	count++;
	pNextJob = measure_distance;
	wait_for_userinput();
}

/**
 * Das eigentliche Verhalten
 * \param data	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * \see			bot_calibrate_sharps()
 * Die Funktionalitaet des Verhaltens ist aufgeteilt in:
 * \see goto_next_pos(), @see measure_distance(), \see update_data()
 */
void bot_calibrate_sharps_behaviour(Behaviour_t* data) {
	if (pNextJob) {
		pNextJob();
	} else {
		/* fertig! */
		display_clear();
		sensor_update_distance = sensor_dist_lookup; // Sensorauswertung wieder aktivieren
		/* Puffer ins EEPROM schreiben */
		ctbot_eeprom_write_block(sensDistDataL, buffer[0], max_steps * sizeof(distSens_t));
		ctbot_eeprom_write_block(sensDistDataR, buffer[1], max_steps * sizeof(distSens_t));
		return_from_behaviour(data);
		/* Fuer sensor_correction.h formatierte Logausgabe, erleichtert das Speichern der Init-EEPROM- / Sim-Werte */
		char tmp_s[12 * STEP_COUNT / 2 + 1]; // 12 Zeichen pro Durchlauf + '\0'
		uint8_t i, j, k;
		LOG_INFO("SENSDIST_DATA_LEFT:");
		for (k = 0; k < 2; ++k) {
			for (j = 0; j < 2; ++j) {
				tmp_s[0] = '\0';
				for (i = 0; i < max_steps / 2; ++i) {
					sprintf(tmp_s, "%s{%4u,%4u},", tmp_s, buffer[k][i + j * max_steps / 2].voltage, buffer[k][i + j * max_steps / 2].dist);
				}
				if (j == 1) {
					tmp_s[strlen(tmp_s) - 1] = '\0'; // kein Komma ausgeben, falls letzter Wert
				}
				/* Log-Ausgabe auf 40 Zeichen begrenzen */
				char tmp = tmp_s[36];
				tmp_s[36] = 0;
				LOG_INFO("%s \\", tmp_s);
				tmp_s[36] = tmp;
				tmp = tmp_s[72];
				tmp_s[72] = 0;
				LOG_INFO("%s \\", &tmp_s[36]);
				tmp_s[72] = tmp;
				LOG_INFO("%s \\", &tmp_s[72]);
			}
			if (k == 0) {
				LOG_INFO("SENSDIST_DATA_RIGHT:");
			}
		}
	}
}

/**
 * Kalibriert die Distanzsensoren des ct-Bots
 * \param caller Zeiger auf den Verhaltensdatensatz des Aufrufers
 */
void bot_calibrate_sharps(Behaviour_t* caller) {
	/* Inits */
//	data = caller;
//	start_head = heading;
//	start_x = x_enc + 100;
	last_toggle = 1;
	measure_count = -4;
	step = 5;
	distance = SENS_IR_MIN_DIST / 10;
	distL = 0;
	distR = 0;
	count = 0;
	userinput_done = 0;

	sensor_update_distance = sensor_dist_straight; // Sensorauswertung deaktivieren

	uint8_t i;
	for (i = 0; i < sizeof(screen_functions) / sizeof(screen_functions[0]); ++i) {
		if (screen_functions[i] == bot_calibrate_sharps_display) {
			break;
		}
	}
	display_screen = i;
	display_clear();

	/* Als erstes Entfernung / Spannung messen */
	pNextJob = measure_distance;

	wait_for_userinput();

	/* Verhalten an */
	switch_to_behaviour(caller, bot_calibrate_sharps_behaviour, BEHAVIOUR_OVERRIDE);
}

/**
 * Displayhandler fuer bot_calibrate_sharps-Verhalten
 */
void bot_calibrate_sharps_display(void) {
	/* Displayausgabe */
	display_cursor(1, 1);
	if (count != max_steps && pNextJob == wait_for_userinput_helper) {
		display_printf("Sharp-Kalibr. %2u/%2u", count + 1 , max_steps);
		LOG_INFO("Sharp-Kalibr. %2u/%2u", count + 1 , max_steps);
		display_cursor(2, 1);
		display_printf("Bot bitte auf %2u cm", distance);
		LOG_INFO("Bot bitte auf %2u cm", distance);
		display_cursor(3, 1);
		display_puts("stellen und mit");
		LOG_INFO("stellen und mit");
		display_cursor(4, 1);
		display_puts("\"Mute\" bestaetigen");
		LOG_INFO("\"Mute\" bestaetigen L=%d R=%d", sensDistL, sensDistR);

		/* Keyhandler */
		if (RC5_Code == RC5_CODE_MUTE) {
			userinput_done = 1;
			RC5_Code = 0;
		}
	} else if (pNextJob != NULL) {
		display_cursor(2, 1);
		display_puts("thinking...         ");
		LOG_INFO("thinking...");
	} else if (count == max_steps) {
		display_puts("fertig :-)");
	} else {
		display_puts("run calibrate_sharps");
		LOG_INFO("run calibrate_sharps");
	}
}

#endif // BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
