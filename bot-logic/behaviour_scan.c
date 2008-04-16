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
 * @file 	behaviour_scan.c
 * @brief 	Scannt die Umgebung und traegt sie in die Karte ein
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */

#include "bot-logic/bot-logik.h"
#include <math.h>
#include <stdlib.h>
#include "map.h"
#include "timer.h"
#include "display.h"
#include "log.h"
#include "os_thread.h"
#include "delay.h"
#include "led.h"
#include "mmc.h"
#include "fifo.h"
#include "math_utils.h"

#ifdef BEHAVIOUR_SCAN_AVAILABLE

#ifndef MAP_AVAILABLE
	#error "MAP_AVAILABLE muss an sein, damit behaviour_scan.c etwas sinnvolles tun kann"
#endif
#ifndef OS_AVAILABLE
	#error "OS_AVAILABLE muss an sein fuer behaviour_scan"
#endif

//#define DEBUG_MAP
//TODO:	Status fuer Loecher eintragen
//TODO:	int16 statt float fuer Position
//TODO:	Stacksize nachrechnen / optimieren

#define MAP_UPDATE_STACK_SIZE	256	
#define MAP_UPDATE_CACHE_SIZE	30

scan_mode_t scan_otf_modes = {1, 1, 1, 1};	/*!< Modi des Verhaltens. Default: location, distance, border an, Kartographie-Modus */

/*! Map-Cache-Eintrag */
typedef struct {
	int16_t x_pos;		/*!< X-Komponente der Position [mm] */
	int16_t y_pos;		/*!< Y-Komponente der Position [mm] */
	int16_t heading;	/*!< Blickrichtung [1/10 Grad]*/
	uint8_t distL;		/*!< Entfernung linker Distanzsensor [5 mm] */
	uint8_t distR;		/*!< Entfernung rechter Distanzsensor [5 mm] */
} map_cache_t;

map_cache_t map_update_cache[MAP_UPDATE_CACHE_SIZE];	/*!< Cache */
fifo_t map_update_fifo;									/*!< Fifo fuer Cache */

uint8_t map_update_stack[MAP_UPDATE_STACK_SIZE];		/*!< Stack des Update-Threads */
static Tcb_t * map_update_thread;						/*!< Thread fuer Map-Update */

/*!
 * Main-Funktion des Map-Update-Threads
 */
void bot_scan_onthefly_do_map_update(void) {
	static map_cache_t cache_tmp;	// Stackspeicher sparen
	static int16_t last_x, last_y;
	LOG_INFO("MAP-thread started");
	while (1) {
		/* Cache-Eintrag holen 
		 * PC-Version blockiert hier, falls Fifo leer */
		if (fifo_get_data(&map_update_fifo, &cache_tmp, sizeof(map_cache_t)) > 0) {
			/* Strahlen updaten */
			if (scan_otf_modes.distance && cache_tmp.distL != 0) {
				map_update(cache_tmp.x_pos, cache_tmp.y_pos, cache_tmp.heading/10.0f, cache_tmp.distL*5, cache_tmp.distR*5);
			}
			/* Grundflaeche updaten */
			if (scan_otf_modes.location) {
				int16_t diff = get_dist(cache_tmp.x_pos, cache_tmp.y_pos, last_x, last_y);
				last_x = cache_tmp.x_pos;
				last_y = cache_tmp.y_pos;
				if (diff > SCAN_ONTHEFLY_DIST_RESOLUTION) {
					map_update_location(cache_tmp.x_pos, cache_tmp.y_pos);
				}
			}
		} else {
			/* Fifo leer => weiter mit Main-Thread */
			os_thread_wakeup(os_threads);	// main ist immer der Erste im Array
		}
	}
}

/*!
 * Initialisiert das Scan-Verhalten
 */
void bot_scan_onthefly_init(void) {
	fifo_init(&map_update_fifo, map_update_cache, sizeof(map_update_cache));
	map_update_thread = os_create_thread(&map_update_stack[MAP_UPDATE_STACK_SIZE-1], bot_scan_onthefly_do_map_update);
	if (map_update_thread != NULL) {
		LOG_INFO("MAP-thread created");
	}
}

/*!
 * Der Roboter aktualisiert kontinuierlich seine Karte
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_onthefly_behaviour(Behaviour_t * data) {
	#ifdef DEBUG_MAP
		uint32_t start_ticks = TIMER_GET_TICKCOUNT_32;
	#endif
	static int16_t last_x, last_y, last_dist_x, last_dist_y, last_head;
	map_cache_t cache_tmp;

	/* Verhalten je nach Cache-Fuellstand */
	uint8_t cache_free = map_update_fifo.size - map_update_fifo.count;
	if (cache_free < MAP_UPDATE_CACHE_SIZE/3) {
		if (cache_free == 0) {
			/* Cache ganz voll */
			if (scan_otf_modes.map_mode) {
				/* Stopp */
				motor_set(BOT_SPEED_STOP, BOT_SPEED_STOP);
//				LOG_DEBUG("Map-Cache voll, halte Bot an");
				os_thread_sleep(2000);
			} else {
				/* Cache voll, neuen Eintrag verwerfen */
//				LOG_DEBUG("Map-Cache voll, verwerfe neuen Eintrag");
			}
			return;
		}
		/* Cache sehr voll */
		if ((int16_t)v_enc_left == 0 && (int16_t)v_enc_right == 0) {
			/* Standzeit als Pause nutzen */
			os_thread_sleep(2000);
			return;
		}
	}
	/* Cache updaten, falls sich der Bot weit genug bewegt hat. */
	int16_t diff = get_dist(x_pos, y_pos, last_x, last_y);
	int16_t turned = turned_angle(last_head);
	if (diff > (SCAN_ONTHEFLY_DIST_RESOLUTION*SCAN_ONTHEFLY_DIST_RESOLUTION) ||
			turned > SCAN_ONTHEFLY_ANGLE_RESOLUTION) {
		cache_tmp.x_pos = x_pos;
		cache_tmp.y_pos = y_pos;
		cache_tmp.heading = (int16_t)(heading*10.0f);
		int16_t diff_dist = get_dist(x_pos, y_pos, last_dist_x, last_dist_y);
		if (diff_dist > (SCAN_ONTHEFLY_DIST_RESOLUTION_DISTSENS*SCAN_ONTHEFLY_DIST_RESOLUTION_DISTSENS) ||
				turned > SCAN_ONTHEFLY_ANGLE_RESOLUTION) {
			cache_tmp.distL = sensDistL/5;
			cache_tmp.distR = sensDistR/5;
			last_dist_x = x_pos;
			last_dist_y = y_pos;
		} else {
			cache_tmp.distL = 0;		
		}
		fifo_put_data(&map_update_fifo, &cache_tmp, sizeof(map_cache_t));

		last_x = x_pos;
		last_y = y_pos;
		last_head = (int16_t) heading;

		#ifdef DEBUG_MAP
			uint32_t end_ticks= TIMER_GET_TICKCOUNT_32;
			uint16_t diff = (end_ticks-start_ticks)*176/1000;
			LOG_DEBUG("time: %u ms", TIMER_GET_TICKCOUNT_32*176/1000);
			LOG_DEBUG("map updated, took %u ms", diff);
		#endif
	}

//	LOG_INFO("MAIN is going to sleep for 100 ms");
//	os_thread_sleep(100L);
	
	/* Rest der Zeitscheibe (10 ms) schlafen legen */
	os_thread_yield();
	
//	LOG_INFO("MAIN is back! :-)");
}

#if 0	// inaktiv
#define BOT_SCAN_STATE_START 0
static uint8 bot_scan_state = BOT_SCAN_STATE_START;	/*!< Zustandsvariable fuer bot_scan_behaviour */
#endif

/*!
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_behaviour(Behaviour_t * data) {
#if 0	// inaktiv
	#define BOT_SCAN_STATE_SCAN 1	

	#define ANGLE_RESOLUTION 5	/*!< Aufloesung fuer den Scan in Grad */

	//	static uint16 bot_scan_start_angle; /*!< Winkel, bei dem mit dem Scan begonnen wurde */
	static float turned; /*!< Winkel um den bereits gedreht wurde */

	static float last_scan_angle; /*!< Winkel bei dem zuletzt gescannt wurde */

	float diff;

	switch (bot_scan_state) {
	case BOT_SCAN_STATE_START:
		turned=0;
		last_scan_angle=heading-ANGLE_RESOLUTION;
		bot_scan_state=BOT_SCAN_STATE_SCAN;
		break;
	case BOT_SCAN_STATE_SCAN:
		diff = heading - last_scan_angle;
		if (diff < -180)
			diff+=360;
		if (diff*1.15>= ANGLE_RESOLUTION) {
			turned+= diff;
			last_scan_angle=heading;

			#ifdef MAP_AVAILABLE
				// Eigentlicher Scan hier
				map_update(x_pos, y_pos, heading, sensDistL, sensDistR);
				////////////
			#endif
		}

		if (turned >= 360-ANGLE_RESOLUTION) // Ende erreicht
			bot_scan_state++;
		break;
	default:
		bot_scan_state = BOT_SCAN_STATE_START;
		#ifdef MAP_AVAILABLE
			map_print();
		#endif
		return_from_behaviour(data);
		break;
	}
#endif
}

/*! 
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *caller	Der Aufrufer
 */
void bot_scan(Behaviour_t * caller) {	
	bot_turn(caller, 360);
#if 0	// inaktiv
	bot_scan_state = BOT_SCAN_STATE_START;
	switch_to_behaviour(0, bot_scan_behaviour, OVERRIDE);
#endif
}

#if 0	// inaktiv
/*! 
 * eigentliche Aufrufroutine zum Eintragen des Abgrundes in den Mapkoordinaten, wenn
 * die Abgrundsensoren zugeschlagen haben  
 * @param x aktuelle Bot-Koordinate als Welt- (nicht Map-) Koordinaten
 * @param y aktuelle Bot-Koordinate als Welt- (nicht Map-) Koordinaten
 * @param head Blickrichtung 
 */
void update_map_hole(float x, float y, float head) {
	float h= head * (M_PI/180.0f); // Umrechnung in Bogenmass 
	// uint8 border_behaviour_fired=False;

	if (sensBorderL > BORDER_DANGEROUS) {
		//Ort des linken Sensors in Weltkoordinaten (Mittelpunktentfernung)
		float Pl_x = x - (DISTSENSOR_POS_B_SW * sin(h));
		float Pl_y = y + (DISTSENSOR_POS_B_SW * cos(h));
		map_update_sensor_hole(Pl_x, Pl_y, h); // Eintragen des Loches in die Map	   
	}

	if (sensBorderR > BORDER_DANGEROUS) {
		//Ort des rechten Sensors in Weltkoordinaten (Mittelpunktentfernung)
		float Pr_x = x + (DISTSENSOR_POS_B_SW * sin(h));
		float Pr_y = y - (DISTSENSOR_POS_B_SW * cos(h));
		map_update_sensor_hole(Pr_x, Pr_y, h); // Eintragen des Loches in die Map
	}
}
#endif

/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung; muss registriert werden um
 * den erkannten Abgrund in die Map einzutragen
 */
void border_in_map_handler(void) {
#if 0	// inaktiv
	// Routine muss zuerst checken, ob on_the_fly auch gerade aktiv ist, da nur in diesem
	// Fall etwas gemacht werden muss
	if (!behaviour_is_activated(bot_scan_onthefly_behaviour))
		return;

	/* bei Abgrunderkennung Position des Abgrundes in Map mit -128 eintragen */
	update_map_hole(x_pos, y_pos, heading);
#endif
}
#endif	// BEHAVIOUR_SCAN_AVAILABLE
