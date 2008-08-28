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
 * @file 	map.h
 * @brief 	Karte
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	19.09.06
 */

#ifndef MAP_H_
#define MAP_H_

#include "ct-Bot.h"
#include "bot-logic/available_behaviours.h"

#ifdef MAP_AVAILABLE

/* Es lohnt nicht gigantische Karten auszugeben, wenn sie nichts enthalten, daher hier zwei Varianten, um die Karte auf die realen Groesse zu reduzieren */
//#define SHRINK_MAP_ONLINE		/*!< Wenn gesetzt, wird bei jedem Update der belegte Bereich der Karte protokolliert. Pro: schnelle ausgabe Contra permanenter aufwand  */
#define SHRINK_MAP_OFFLINE		/*!< Wenn gesetzt, wird erst bei der Ausgabe der belegte Bereich der Karte berechnet. Pro: kein permanenter aufwand Contra: ausgabe dauert lange */

/* Geomtrie der Karte - Achtung, nur aendern, wenn man die Konsequenzen genau kennt! */
#define MAP_SIZE			12.288	/*!< Kantenlaenge der Karte in Metern. Zentrum ist der Startplatz des Bots. Achtung, MAP_SIZE*MAP_RESOLUTION muss ganzzahliges Vielfaches von MACRO_BLOCK_LENGTH sein */
#define MAP_RESOLUTION 		125		/*!< Aufloesung der Karte in Punkte pro Meter */
#define MAP_SECTION_POINTS 	16		/*!< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS*MAP_SECTION_POINTS Bytes  */

#define MAP_UPDATE_STACK_SIZE	240	/*!< Groesse des Stacks, der das Map-Update ausfuehrt */
#define MAP_UPDATE_CACHE_SIZE	26	/*!< Groesse des Map-Caches */

#define MAP_OBSTACLE_THRESHOLD	-20	/*!< Schwellwert, ab dem ein Feld als Hindernis gilt */

// Die folgenden Variablen/konstanten NICHT direkt benutzen, sondern die zugehoerigen Makros: get_map_min_x() und Co!
// Denn sonst erhaelt man Karten und nicht Weltkoordinaten!
#ifdef SHRINK_MAP_ONLINE
extern uint16_t map_min_x;			/*!< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
extern uint16_t map_max_x;			/*!< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
extern uint16_t map_min_y;			/*!< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
extern uint16_t map_max_y;			/*!< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate  */
#else
extern const uint16_t map_min_x;	/*!< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
extern const uint16_t map_min_y;	/*!< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
extern const uint16_t map_max_x;	/*!< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
extern const uint16_t map_max_y;	/*!< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate */
#endif	// SHRINK_MAP_ONLINE

/*! Map-Cache-Eintrag */
typedef struct {
	int16_t x_pos;		/*!< X-Komponente der Position [mm] */
	int16_t y_pos;		/*!< Y-Komponente der Position [mm] */
	int16_t heading;	/*!< Blickrichtung [1/10 Grad] */
	uint8_t dataL;		/*!< Entfernung linker Distanzsensor [5 mm] aber auch BorderSensor [0/1] */
	uint8_t dataR;		/*!< Entfernung rechter Distanzsensor [5 mm] aber auch BorderSensor [0/1] */
	scan_mode_t mode;	/*!< Was soll aktualisiert werden */
#ifndef DOXYGEN
} __attribute__ ((packed)) map_cache_t;	// Keine Luecken in der Struktur lassen
#else
} map_cache_t;
#endif

extern map_cache_t map_update_cache[];	/*!< Cache */
extern fifo_t map_update_fifo;			/*!< Fifo fuer Cache */

/*!
 * Prueft, ob die Karte zurzeit gesperrt ist.
 * @return	1, falls Karte gesperrt, 0 sonst
 */
uint8_t map_locked(void);

/*!
 * liefert den Durschnittswert um eine Ort herum
 * @param x			X-Ordinate der Welt
 * @param y			Y-Ordinate der Welt
 * @param radius	Radius der Umgebung, die beruecksichtigt wird [mm]
 * @return			Durchschnitsswert im Umkreis um den Ort (>0 heisst frei, <0 heisst belegt)
 */
int8_t map_get_average(int16_t x, int16_t y, int16_t radius);

/*!
 * liefert den Wert eines Feldes
 * @param x	X-Ordinate der Welt
 * @param y	Y-Ordinate der Welt
 * @return	Wert des Feldes (>0 heisst frei, <0 heisst belegt)
 */
static inline int8_t map_get_point(int16_t x, int16_t y) {
	return map_get_average(x, y, 0);
}

/*!
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * @param from_x	Startort x Weltkoordinaten
 * @param from_y	Startort y Weltkoordinaten
 * @param to_x		Zielort x Weltkoordinaten
 * @param to_y		Zielort y Weltkoordinaten
 * @return 			1 wenn alles frei ist
 */
uint8_t map_way_free(int16_t from_x, int16_t from_y, int16_t to_x, int16_t to_y);

/*!
 * Haelt den Bot an und schreibt den Map-Update-Cache komplett zurueck
 */
void map_flush_cache(void);

/*!
 * Zeigt die Karte an
 */
void map_print(void);

/*!
 * initialisiere die Karte
 * @return	0 wenn alles ok ist
 */
int8_t map_init(void);

/*!
 * Konvertiert eine Kartenkoordinate in eine Weltkoordinate
 * @param map_koord	Kartenkoordinate
 * @return 			Weltkoordiante
 */
static inline int16_t map_to_world(uint16_t map_koord) {
#if (1000 / MAP_RESOLUTION) * MAP_RESOLUTION != 1000
	#warning "MAP_RESOLUTION ist kein Teiler von 1000, Code in map_to_world() anpassen!"
#endif
	int32_t tmp = map_koord * (1000 / MAP_RESOLUTION);
	return tmp - (uint16_t)(MAP_SIZE * MAP_RESOLUTION * 4);
}

// Makros, um die belegten Kartenbereiche (in Weltkoordinaten) zu ermitteln
#define map_get_min_x() map_to_world(map_min_x)		/*!< Minimum in X-Richtung */
#define map_get_min_y() map_to_world(map_min_y)		/*!< Minimum in Y-Richtung */
#define map_get_max_x() map_to_world(map_max_x)		/*!< Maximum in X-Richtung */
#define map_get_max_y() map_to_world(map_max_y)		/*!< Maximum in Y-Richtung */

#ifdef PC
/*!
 * Liest eine Map wieder ein
 * @param filename	Quelldatei
 */
void map_read(char * filename);

/*!
 * Schreibt einbe Karte in eine PGM-Datei
 * @param filename	Zieldatei
 */
void map_to_pgm(char * filename);
#endif	// PC

#ifdef DISPLAY_MAP_AVAILABLE
/*!
 * Handler fuer Map-Display
 */
void map_display(void);
#endif	// DISPLAY_MAP_AVAILABLE

#endif	// MAP_AVAILABLE
#endif	/*MAP_H_*/
