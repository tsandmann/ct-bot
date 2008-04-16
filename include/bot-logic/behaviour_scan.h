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
 * @file 	behaviour_scan.h
 * @brief 	Scannt die Umgebung und traegt sie in die Karte ein
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */

#ifndef BEHAVIOUR_SCAN_H_
#define BEHAVIOUR_SCAN_H_

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_SCAN_AVAILABLE

#define SCAN_ONTHEFLY_DIST_RESOLUTION 30			/*!< Alle wieviel gefahrene Strecke [mm] soll die Karte aktualisiert werden */
#define SCAN_ONTHEFLY_DIST_RESOLUTION_DISTSENS 60	/*!< Alle wieviel gefahrene Strecke [mm] sollen die Distanzsensordaten in der Karte aktualisiert werden */
#define SCAN_ONTHEFLY_ANGLE_RESOLUTION 10			/*!< Alle wieviel Gerad Drehung [Grad] soll die Karte aktualisiert werden */

/*! Modi des Scan-Verhaltens */
typedef struct {
	uint8_t location:1;	/*!< Grundflaechen-Update an/aus */
	uint8_t distance:1;	/*!< Distanzsensor-Update an/aus */
	uint8_t border:1;	/*!< Abgrundsensor-Update an/aus */
	uint8_t map_mode:1;	/*!< Kartograhpie-Modus an/aus (Bot stoppt, falls Cache voll) */
} scan_mode_t;

extern scan_mode_t scan_otf_modes;	/*!< Modi des Verhaltens */

/*!
 * Schaltet Grundflaechen-Update an oder aus
 * @param value	1: an, 0: aus
 */
static inline void set_scan_otf_location(uint8_t value) {
	scan_otf_modes.location = value;
}

/*!
 * Schaltet Distanzsensor-Update an oder aus
 * @param value	1: an, 0: aus
 */
static inline void set_scan_otf_distance(uint8_t value) {
	scan_otf_modes.distance = value;
}

/*!
 * Schaltet Abgrundsensor-Update an oder aus
 * @param value	1: an, 0: aus
 */
static inline void set_scan_otf_border(uint8_t value) {
	scan_otf_modes.border = value;
}

/*!
 * Schaltet Kartographie-Modus an oder aus.
 * Im Kartographie-Modus haelt der Bot an, falls der Cache voll 
 * ist, anstatt Eintraege zu verwerfen.
 * @param value	1: an, 0: aus
 */
static inline void set_scan_otf_mapmode(uint8_t value) {
	scan_otf_modes.map_mode = value;
}

/*!
 * Initialisiert das Scan-Verhalten
 */
void bot_scan_onthefly_init(void);

/*!
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_onthefly_behaviour(Behaviour_t *data);

/*!
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_behaviour(Behaviour_t *data);


/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung; muss registriert werden um
 * den erkannten Abgrund in die Map einzutragen
 */
void border_in_map_handler(void); 

/*! 
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *caller	Der Aufrufer
 */
void bot_scan(Behaviour_t* caller);
#endif
#endif /*BEHAVIOUR_SCAN_H_*/
