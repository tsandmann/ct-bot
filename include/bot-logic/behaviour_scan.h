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
 * @date 	03.11.2006
 */

#ifndef BEHAVIOUR_SCAN_H_
#define BEHAVIOUR_SCAN_H_

#define SCAN_OTF_RESOLUTION_DISTANCE_LOCATION	60	/*!< Nach welcher gefahrenen Strecke [mm] soll die Standflaeche aktualisiert werden */

#define SCAN_OTF_RESOLUTION_DISTANCE_BORDER		10	/*!< Nach welcher gefahrenen Strecke [mm] sollen die Abgrundsensoren fuer die Karte ausgewertet werden */
#define SCAN_OTF_RESOLUTION_ANGLE_BORDER		10	/*!< Alle wieviel Grad Drehung [Grad] sollen die Abgrundsensoren fuer die Karte ausgewertet werden */

#define SCAN_OTF_RESOLUTION_ANGLE_DISTSENS		10	/*!< Alle wieviel Grad Drehung [Grad] sollen die Distanzsensoren fuer die Karte ausgewertet werden */
#define SCAN_OTF_RESOLUTION_DISTANCE_DISTSENS	180	/*!< Nach welcher gefahrenen Strecke [mm] sollen die  Distanzsensoren fuer die Karte ausgewertet werden */

#define MAP_MAX_ERROR_RADIUS				200		/*!< maximaler Wert [mm] von pos_error_radius, der fuer die Berechnung der Positionssicherheit verwendet wird (alles darueber wird mit location_prob = 0 eingetragen) */

#define SCAN_OTF_SLEEP_TIME					2000	/*!< Wartezeit fuer andere Verhalten, falls Cache (sehr) voll ist [ms] */
/*! Schwellwert fuer freien Cache-Speicher, ab dem der Cache als sehr voll gilt */
#define SCAN_OTF_CACHE_LEVEL_THRESHOLD		(MAP_UPDATE_CACHE_SIZE / 3)

/*! Modi des Scan-Verhaltens */
typedef union {
	struct {
		unsigned location:1;	/*!< Grundflaechen-Update an/aus */
		unsigned distance:1;	/*!< Distanzsensor-Update an/aus */
		unsigned border:1;		/*!< Abgrundsensor-Update an/aus */
		unsigned map_mode:1;	/*!< Kartograhpie-Modus an/aus (Bot stoppt, falls Cache voll) */
	} PACKED_FORCE data;
	uint8_t raw;			/*!< Alle Modi als Raw-Daten */
} scan_mode_t;

#ifdef BEHAVIOUR_SCAN_AVAILABLE

extern scan_mode_t scan_otf_modes;	/*!< Modi des Verhaltens */

/*!
 * Schaltet Grundflaechen-Update an oder aus
 * @param value	1: an, 0: aus
 */
static inline void set_scan_otf_location(uint8_t value) {
	bit_t tmp = {value};
	scan_otf_modes.data.location = tmp.bit;
}

/*!
 * Schaltet Distanzsensor-Update an oder aus
 * @param value	1: an, 0: aus
 */
static inline void set_scan_otf_distance(uint8_t value) {
	bit_t tmp = {value};
	scan_otf_modes.data.distance = tmp.bit;
}

/*!
 * Schaltet Abgrundsensor-Update an oder aus
 * @param value	1: an, 0: aus
 */
static inline void set_scan_otf_border(uint8_t value) {
	bit_t tmp = {value};
	scan_otf_modes.data.border = tmp.bit;
}

/*!
 * Schaltet Kartographie-Modus an oder aus.
 * Im Kartographie-Modus haelt der Bot an, falls der Cache voll
 * ist, anstatt Eintraege zu verwerfen.
 * @param value	1: an, 0: aus
 */
static inline void set_scan_otf_mapmode(uint8_t value) {
	bit_t tmp = {value};
	scan_otf_modes.data.map_mode = tmp.bit;
}

/*!
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_onthefly_behaviour(Behaviour_t * data);

#endif // BEHAVIOUR_SCAN_AVAILABLE
#endif // BEHAVIOUR_SCAN_H_
