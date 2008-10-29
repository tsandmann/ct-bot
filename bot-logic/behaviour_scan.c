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

//#define DEBUG_SCAN_OTF	// Debug-Infos an

scan_mode_t scan_otf_modes;	/*!< Modi des Verhaltens */

/*!
 * Der Roboter aktualisiert kontinuierlich seine Karte
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_onthefly_behaviour(Behaviour_t * data) {
	static int16_t last_location_x, last_location_y;
	static int16_t last_dist_x, last_dist_y, last_dist_head;
	static int16_t last_border_x, last_border_y, last_border_head;
	map_cache_t cache_tmp;

	/* Verhalten je nach Cache-Fuellstand */
	uint8_t cache_free = map_update_fifo.size - map_update_fifo.count;
	if (cache_free < SCAN_OTF_CACHE_LEVEL_THRESHOLD) {
		if (cache_free == 0) {
			/* Cache ganz voll */
			if (scan_otf_modes.map_mode && 
					sensBorderL < BORDER_DANGEROUS && sensBorderR < BORDER_DANGEROUS) {
				/* Stoppe den Bot, damit wir Zeit haben die Karte einzutragen 
				 * aber nur, wenn kein Abgrund erkannt wurde */
				motor_set(BOT_SPEED_STOP, BOT_SPEED_STOP);
//				LOG_DEBUG("Map-Cache voll, halte Bot an");
				/* Halte alle Verhalten eine Weile an, weil sie ja sonst weiterfahren wuerden */
				os_thread_sleep(SCAN_OTF_SLEEP_TIME);
			} else {
				/* Cache voll, neuen Eintrag verwerfen */
//				LOG_DEBUG("Map-Cache voll, verwerfe neuen Eintrag");
			}
			return;
		}
		/* Cache sehr voll */
		if (v_enc_left == 0 && v_enc_right == 0) {
			/* Falls Bot gerade steht, dann kleine Pause */
			os_thread_sleep(SCAN_OTF_SLEEP_TIME);
			return;
		}
	}
	
	/* Cache updaten, falls sich der Bot weit genug bewegt hat. */
	cache_tmp.mode.raw = 0;
	cache_tmp.dataL = 0;
	cache_tmp.dataR = 0;
	
	/*
	 * STANDFLAECHE
	 * Die Standflaeche tragen wir nur ein, wenn der Bot auch ein Stueck gefahren ist
	 */ 
	if (scan_otf_modes.location) {
		// ermitteln, wie weit der Bot seit dem letzten location-update gefahren ist
		uint16_t diff = get_dist(x_pos, y_pos, last_location_x, last_location_y);
		if (diff > (SCAN_OTF_RESOLUTION_DISTANCE_LOCATION*SCAN_OTF_RESOLUTION_DISTANCE_LOCATION)){
			// ist er weiter als SCAN_ONTHEFLY_DIST_RESOLUTION gefahren ==> standfläche aktualisieren
			cache_tmp.mode.location = 1;
			// Letzte Location-Update-Position sichern
			last_location_x = x_pos;
			last_location_y = y_pos;
		}
	}
	
	/* 
	 * DISTANZSENSOREN
	 * Die Distanzsensoren tragen wir beim Geradeausfahren selten ein, 
	 * da sie viele Map-zellen ueberstreichen und das Eintragen teuer ist
	 * und sie auf der anderen Seite (beim Vorwaertsfahren) wenig neue Infos liefern
	*/
	if (scan_otf_modes.distance) {
		// ermitteln, wie weit der Bot gedreht hat
		int16_t turned = turned_angle(last_dist_head);
		// ermitteln, wie weit der Bot seit dem letzten distance-update gefahren ist
		uint16_t diff = get_dist(x_pos, y_pos, last_dist_x, last_dist_y);
		if ((turned > SCAN_OTF_RESOLUTION_ANGLE_DISTSENS) ||
			(diff > (SCAN_OTF_RESOLUTION_DISTANCE_DISTSENS*SCAN_OTF_RESOLUTION_DISTANCE_DISTSENS))) {
			// Hat sich der Bot mehr als SCAN_ONTHEFLY_ANGLE_RESOLUTION gedreht ==> Blickstrahlen aktualisieren
			cache_tmp.mode.distance = 1;
	
			cache_tmp.dataL = sensDistL/5;
			cache_tmp.dataR = sensDistR/5;
			// Letzte Distance-Update-Position sichern
			last_dist_x = x_pos;
			last_dist_y = y_pos;
			last_dist_head = (int16_t) heading;
		}
	}

	/*
	 * ABGRUNDSENSOREN
	 * Wir werten diese nur aus, wenn der Bot entweder 
	 * SCAN_OTF_RESOLUTION_DISTANCE_BORDER mm gefahren ist oder
	 * SCAN_OTF_RESOLUTION_ANGLE_BORDER Grad gedreht hat 
	 */
	if (scan_otf_modes.border) {
		// ermitteln, wie weit der Bot seit dem letzten border-update gefahren ist
		int16_t diff = get_dist(x_pos, y_pos, last_border_x, last_border_y);
		// ermitteln, wie weit der Bot gedreht hat
		uint16_t turned = turned_angle(last_border_head);
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
	}
	
	// ist ein Update angesagt?
	if (cache_tmp.mode.distance || cache_tmp.mode.location || cache_tmp.mode.border) {			
		cache_tmp.x_pos = x_pos;
		cache_tmp.y_pos = y_pos;
		cache_tmp.heading = (int16_t)(heading*10.0f);		


		fifo_put_data(&map_update_fifo, &cache_tmp, sizeof(map_cache_t));

		#ifdef DEBUG_SCAN_OTF
			LOG_DEBUG("neuer Eintrag: x= %d y= %d head= %f distance= %d loaction=%d border=%d",cache_tmp.x_pos, cache_tmp.y_pos, cache_tmp.heading/10.0f,cache_tmp.mode.distance, cache_tmp.mode.location, cache_tmp.mode.border);
		#endif
	}
	
	/* Rest der Zeitscheibe (10 ms) schlafen legen */
	os_thread_yield();
}

#endif	// BEHAVIOUR_SCAN_AVAILABLE
