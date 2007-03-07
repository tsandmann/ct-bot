#ifndef MAP_H_
#define MAP_H_


/*
 * c't-Bot - Robotersimulator fuer den c't-Bot
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

/*! @file 	map.h  
 * @brief 	Karte 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	19.09.06
*/
#include <stdio.h>
#include "ct-Bot.h"

#ifdef MAP_AVAILABLE

/* Es lohnt nicht gigantische Karten auszugeben, wenn sie nichts enthalten, daher hier zwei Varianten, um die Karte auf die realen groesse zu reduzieren */
#define SHRINK_MAP_ONLINE		/*!< Wenn gesetzt, wird bei jedem update der belegte Bereich der Karte protokolliert. Pro: schnelle ausgabe Contra permanenter aufwand  */
//#define SHRINK_MAP_OFFLINE		/*!< Wenn gesetzt, wird erst beid er Ausgabe der belegte Bereich der Karte berechnet. Pro: kein permanenter aufwand Contra: ausgabe dauert lange */

#ifdef MCU
	#ifdef MMC_AVAILABLE
		#define MAP_SIZE			4	/*! Kantenlaenge der Karte in Metern. Ursprung ist der Startplatz des Bots */
		#define MAP_RESOLUTION 	128	/*!< Aufloesung der Karte in Punkte pro Meter */
		#define MAP_SECTION_POINTS 16	/*!< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS*MAP_SECTION_POINTS Bytes  */
	#else
		#define MAP_SIZE			4	/*! Kantenlaenge der Karte in Metern. Ursprung ist der Startplatz des Bots */
		#define MAP_SECTION_POINTS 32	/*!< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS*MAP_SECTION_POINTS Bytes  */
		#define MAP_RESOLUTION 	(MAP_SECTION_POINTS/MAP_SIZE)	/*!< Aufloesung der Karte in Punkte pro Meter */
	#endif
#else
	#define MAP_SIZE			4	/*! Kantenlaenge der Karte in Metern. Ursprung ist der Startplatz des Bots */
	#define MAP_RESOLUTION 	128	/*!< Aufloesung der Karte in Punkte pro Meter */
	#define MAP_SECTION_POINTS 16	/*!< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS*MAP_SECTION_POINTS Bytes  */
#endif

// Die folgenden Variablen/konstanten NICHT direkt benutzen, sondern die zugehoerigen Makros: get_map_min_x() und Co!
// Denn sonst erhaelt man Karten und nicht Weltkoordinaten!
#ifdef SHRINK_MAP_ONLINE
	extern uint16 map_min_x; /*!< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
	extern uint16 map_max_x; /*!< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
	extern uint16 map_min_y; /*!< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
	extern uint16 map_max_y; /*!< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate  */
#else
	#define map_min_x 0
	#define map_min_y 0
	#define map_max_x (MAP_SIZE*MAP_RESOLUTION)
	#define map_max_y (MAP_SIZE*MAP_RESOLUTION)
#endif



/*!
 * Aktualisiert die interne Karte
 * @param x X-Achse der Position
 * @param y Y-Achse der Position
 * @param head Blickrichtung in Grad
 * @param distL Sensorwert links
 * @param distR Sensorwert rechts
 */
void update_map(float x, float y, float head, int16 distL, int16 distR);

/*!
 * Aktualisiert den Standkreis der internen Karte
 * @param x X-Achse der Position
 * @param y Y-Achse der Position
 */
void update_map_location(float x, float y);

/*!
 * liefert den Wert eines Feldes 
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @return Wert des Feldes (>0 heisst frei, <0 heisst belegt
 */
int8 map_get_field (uint16 x, uint16 y);

/*!
 * liefert den Wert eines Feldes 
 * @param x x-Ordinate der Welt 
 * @param y y-Ordinate der Welt
 * @return Wert des Feldes (>0 heisst frei, <0 heisst belegt
 */
int8 map_get_point (float x, float y);

/*!
 * Konvertiert eine Weltkoordinate in eine Kartenkoordinate
 * @param koord Weltkordiante
 * @return kartenkoordinate
 */
uint16 world_to_map(float koord);

/*!
 * Konvertiert eine Kartenkoordinate in eine Weltkoordinate
 * @param map_koord kartenkoordinate
 * @return Weltkordiante
 */
float map_to_world(uint16 map_koord);

/*!
 * Zeigt die Karte an
 */
void print_map(void);

/*!
 *  initialisiere die Karte
 * @return 0 wenn alles ok ist
 */
int8 map_init(void);

/*! Liest eine Map wieder ein 
 * @param filename Quelldatei
 */
void read_map(char * filename);

/*!
 * Schreibt einbe Karte in eine PGM-Datei
 * @param filename Zieldatei
 */
void map_to_pgm(char * filename);

// Makros, um die belegte kartenbereiche (in weltkoordinaten) zu ermitteln
#define map_get_min_x() map_to_world(map_min_x)
#define map_get_min_y() map_to_world(map_min_y)
#define map_get_max_x() map_to_world(map_max_x)
#define map_get_max_y() map_to_world(map_max_y)


#endif


#endif /*MAP_H_*/
