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

#ifndef OS_AVAILABLE
  	#error behaviour_scan_on_the_fly geht nicht ohne OS_AVAILABLE
#endif

//#define DEBUG_MAP
//#define DEBUG_SCAN_OTF	// Debug-Infos an

//TODO:	int16 statt float fuer Position
//TODO:	Stacksize nachrechnen / optimieren

#define MAP_UPDATE_STACK_SIZE	256	
#define MAP_UPDATE_CACHE_SIZE	26

#define SCAN_OTF_RESOLUTION_DISTANCE_LOCATION (BOT_DIAMETER/2)		/*!< Nach welcher gefahrenen Strecke [mm] soll die Standfläche aktualisiert werden*/

#define SCAN_OTF_RESOLUTION_DISTANCE_BORDER  10		/*!< Nach welcher gefahrenen Strecke [mm] sollen die Abgrundsensoren für die Karte ausgewertet werden */
#define SCAN_OTF_RESOLUTION_ANGLE_BORDER 	  10	/*!< Alle wieviel Grad Drehung [Grad] sollen die Abgrundsensoren für die Karte ausgewertet werden */

#define SCAN_OTF_RESOLUTION_ANGLE_DISTSENS 	  10	/*!< Alle wieviel Grad Drehung [Grad] sollen die Distanzsensoren für die Karte ausgewertet werden */
#define SCAN_OTF_RESOLUTION_DISTANCE_DISTSENS (BOT_DIAMETER*2)	/*!< Nach welcher gefahrenen Strecke [mm] sollen die  Distanzsensoren für die Karte ausgewertet werden */

scan_mode_t scan_otf_modes = {1, 1, 1, 1};	/*!< Modi des Verhaltens. Default: location, distance, border an, Kartographie-Modus */

/*! Map-Cache-Eintrag */
typedef struct {
	int16_t x_pos;		/*!< X-Komponente der Position [mm] */
	int16_t y_pos;		/*!< Y-Komponente der Position [mm] */
	int16_t heading;	/*!< Blickrichtung [1/10 Grad]*/
	uint8_t dataL;		/*!< Entfernung linker Distanzsensor [5 mm] aber auch BorderSensor [0/1] */
	uint8_t dataR;		/*!< Entfernung rechter Distanzsensor [5 mm] aber auch BorderSensor [0/1] */
	scan_mode_t mode;	/*!< Was soll aktualisiert werden */
#ifndef DOXYGEN
	} __attribute__ ((packed)) map_cache_t;// Keine Luecken in der Struktur lassen
#else
	} map_cache_t;
#endif

map_cache_t map_update_cache[MAP_UPDATE_CACHE_SIZE];	/*!< Cache */
fifo_t map_update_fifo;									/*!< Fifo fuer Cache */

uint8_t map_update_stack[MAP_UPDATE_STACK_SIZE];		/*!< Stack des Update-Threads */
static Tcb_t * map_update_thread;						/*!< Thread fuer Map-Update */

/*!
 * Main-Funktion des Map-Update-Threads
 */
void bot_scan_onthefly_do_map_update(void) {
	static map_cache_t cache_tmp;	// Stackspeicher sparen
	LOG_INFO("MAP-thread started");
	
	while (1) {
		/* Cache-Eintrag holen 
		 * PC-Version blockiert hier, falls Fifo leer */
		if (fifo_get_data(&map_update_fifo, &cache_tmp, sizeof(map_cache_t)) > 0) {
			
			#ifdef DEBUG_SCAN_OTF
				LOG_DEBUG("lese Cache: x= %d y= %d head= %f distance= %d loaction=%d border=%d",cache_tmp.x_pos, cache_tmp.y_pos, cache_tmp.heading/10.0f,cache_tmp.mode.distance, cache_tmp.mode.location, cache_tmp.mode.border);

				if ((cache_tmp.mode.distance || cache_tmp.mode.location || cache_tmp.mode.border)==0)
					LOG_DEBUG("Achtung: Dieser Eintrag ergibt keinen Sinn, kein einziges mode-bit gesetzt");
			#endif
			
			/* Strahlen updaten, falls distance-mode und der aktuelle Eintrag Daten dazu hat*/
			if (scan_otf_modes.distance && cache_tmp.mode.distance) {
				map_update_distance(cache_tmp.x_pos, cache_tmp.y_pos, cache_tmp.heading/10.0f, cache_tmp.dataL*5, cache_tmp.dataR*5);
			}
			
			/* Grundflaeche updaten, falls location-mode */
			if (scan_otf_modes.location && cache_tmp.mode.location) {
					map_update_location(cache_tmp.x_pos, cache_tmp.y_pos);
			}
			
			/* Abgrundsensoren updaten, falls border-mode */
			if (scan_otf_modes.border && cache_tmp.mode.border) {
				map_update_border(cache_tmp.x_pos, cache_tmp.y_pos,cache_tmp.heading/10.0f, cache_tmp.dataL,cache_tmp.dataR);
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
	static int16_t last_location_x, last_location_y;
	static int16_t last_dist_x, last_dist_y, last_dist_head;
	static int16_t last_border_x, last_border_y, last_border_head;
	map_cache_t cache_tmp;

	/* Verhalten je nach Cache-Fuellstand */
	uint8_t cache_free = map_update_fifo.size - map_update_fifo.count;
	if (cache_free < MAP_UPDATE_CACHE_SIZE/3) {
		if (cache_free == 0) {
			/* Cache ganz voll */
			if (scan_otf_modes.map_mode) {
				/* Stoppe den Bot, damit wir zeit haben die Karte einzutragen */
				motor_set(BOT_SPEED_STOP, BOT_SPEED_STOP);
//				LOG_DEBUG("Map-Cache voll, halte Bot an");
				/* HAlte alle Verhalten eine Weile an, weil sie ja sonst weiterfahren würden */
				os_thread_sleep(2000);
			} else {
				/* Cache voll, neuen Eintrag verwerfen */
//				LOG_DEBUG("Map-Cache voll, verwerfe neuen Eintrag");
			}
			return;
		}
		/* Cache sehr voll */
		if ((int16_t)v_enc_left == 0 && (int16_t)v_enc_right == 0) {
			/* Falls Bot gerade steht, dann kleine Pause */
			os_thread_sleep(2000);
			return;
		}
	}
	
	/* Cache updaten, falls sich der Bot weit genug bewegt hat. */
	cache_tmp.mode.location=0;
	cache_tmp.mode.distance=0;
	cache_tmp.mode.border=0;
	cache_tmp.dataL=0;
	cache_tmp.dataR=0;
	
	
	/*
	 * STANDFLÄCHE
	 * Die Standfläche tragen wir nur ein, wenn der Bot auch ein Stück gefahren ist
	 */ 
	// ermitteln, wie weit der Bot seit dem letzten location-update gefahren ist
	uint16_t diff = get_dist(x_pos, y_pos, last_location_x, last_location_y);
	if (diff > (SCAN_OTF_RESOLUTION_DISTANCE_LOCATION*SCAN_OTF_RESOLUTION_DISTANCE_LOCATION)){
		// ist er weiter als SCAN_ONTHEFLY_DIST_RESOLUTION gefahren ==> standfläche aktualisieren
		cache_tmp.mode.location = 1;
		// Letzte Location-Update-position sichern
		last_location_x = x_pos;
		last_location_y = y_pos;
	}

	
	/* 
	 * DISTANZSENSOREN
	 * Die Distanzsensoren tragen wir beim geradeausfahren selten ein, 
	 * da sie viele Map-zellen überstreichen und das Eintragen teuer ist
	 * und sie auf der anderen Seite (beim vorwärtsfahren wenig neue Info liefern
	*/
	// ermitteln, wie weit der Bot gedreht hat
	int16_t turned = turned_angle(last_dist_head);
	// ermitteln, wie weit der Bot seit dem letzten distance-update gefahren ist
	diff = get_dist(x_pos, y_pos, last_dist_x, last_dist_y);
	if ((turned > SCAN_OTF_RESOLUTION_ANGLE_DISTSENS) ||
		(diff > (SCAN_OTF_RESOLUTION_DISTANCE_DISTSENS*SCAN_OTF_RESOLUTION_DISTANCE_DISTSENS))) {
		// Hat sich der Bot mehr als SCAN_ONTHEFLY_ANGLE_RESOLUTION gedreht ==> Blickstrahlen aktualisieren
		cache_tmp.mode.distance = 1;

		cache_tmp.dataL = sensDistL/5;
		cache_tmp.dataR = sensDistR/5;
		// Letzte Distance-Update-position sichern
		last_dist_x = x_pos;
		last_dist_y = y_pos;
		last_dist_head = (int16_t) heading;
	}
	

	/*
	 * ABGRUNDSENSOREN
	 * Wir werten diese nur aus, wenn der Bot entweder 
	 * SCAN_OTF_RESOLUTION_DISTANCE_BORDER mm gefahren ist oder
	 * SCAN_OTF_RESOLUTION_ANGLE_BORDER Grad gedreht hat 
	 */
	// ermitteln, wie weit der Bot seit dem letzten border-update gefahren ist
	diff = get_dist(x_pos, y_pos, last_border_x, last_border_y);
	// ermitteln, wie weit der Bot gedreht hat
	turned = turned_angle(last_border_head);
	if ((
		  (diff > (SCAN_OTF_RESOLUTION_DISTANCE_BORDER*SCAN_OTF_RESOLUTION_DISTANCE_BORDER)) ||
		  (turned > SCAN_OTF_RESOLUTION_ANGLE_BORDER)		
		) && 
		((sensBorderL > BORDER_DANGEROUS) || (sensBorderR > BORDER_DANGEROUS))) {
			cache_tmp.mode.border = 1;
			cache_tmp.mode.distance = 0;
			cache_tmp.dataL = (sensBorderL > BORDER_DANGEROUS);
			cache_tmp.dataR = (sensBorderR > BORDER_DANGEROUS);
			
			last_border_x = x_pos;
			last_border_y = y_pos;
			last_border_head = (int16_t) heading;
	}
	
	// ist ein Update angesagt?
	if (cache_tmp.mode.distance || cache_tmp.mode.location || cache_tmp.mode.border){			
		cache_tmp.x_pos = x_pos;
		cache_tmp.y_pos = y_pos;
		cache_tmp.heading = (int16_t)(heading*10.0f);		


		fifo_put_data(&map_update_fifo, &cache_tmp, sizeof(map_cache_t));

		#ifdef DEBUG_SCAN_OTF
			LOG_DEBUG("neuer Eintrag: x= %d y= %d head= %f distance= %d loaction=%d border=%d",cache_tmp.x_pos, cache_tmp.y_pos, cache_tmp.heading/10.0f,cache_tmp.mode.distance, cache_tmp.mode.location, cache_tmp.mode.border);
		#endif
	}
	
			
		#ifdef DEBUG_MAP
			uint32_t end_ticks= TIMER_GET_TICKCOUNT_32;
			uint16_t diff = (end_ticks-start_ticks)*176/1000;
			LOG_DEBUG("time: %u ms", TIMER_GET_TICKCOUNT_32*176/1000);
			LOG_DEBUG("map updated, took %u ms", diff);
		#endif
	

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
				map_update_distance(x_pos, y_pos, heading, sensDistL, sensDistR);
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

#endif	// BEHAVIOUR_SCAN_AVAILABLE
