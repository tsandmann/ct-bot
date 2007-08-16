#ifndef MAP_H_
#define MAP_H_


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
#include <stdio.h>
#include "ct-Bot.h"

#ifdef MAP_AVAILABLE

/* Es lohnt nicht gigantische Karten auszugeben, wenn sie nichts enthalten, daher hier zwei Varianten, um die Karte auf die realen groesse zu reduzieren */
#define SHRINK_MAP_ONLINE		/*!< Wenn gesetzt, wird bei jedem update der belegte Bereich der Karte protokolliert. Pro: schnelle ausgabe Contra permanenter aufwand  */
//#define SHRINK_MAP_OFFLINE		/*!< Wenn gesetzt, wird erst beid er Ausgabe der belegte Bereich der Karte berechnet. Pro: kein permanenter aufwand Contra: ausgabe dauert lange */

#ifdef MCU
	#ifdef MMC_AVAILABLE
		#define MAP_SIZE			12	/*!< Kantenlaenge der Karte in Metern. Zentrum ist der Startplatz des Bots. Achtung! in kombination mit Macroblocks sind nur ganzzahlige Vielfache der Macroblock kantenlänge erlaubt */
		#define MAP_RESOLUTION 		128	/*!< Aufloesung der Karte in Punkte pro Meter */
		#define MAP_SECTION_POINTS 	16	/*!< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS*MAP_SECTION_POINTS Bytes  */
	#else
		#define MAP_SIZE			4	/*! Kantenlaenge der Karte in Metern. Zentrum ist der Startplatz des Bots */
		#define MAP_SECTION_POINTS 32	/*!< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS*MAP_SECTION_POINTS Bytes  */
		#define MAP_RESOLUTION 	(MAP_SECTION_POINTS/MAP_SIZE)	/*!< Aufloesung der Karte in Punkte pro Meter */
	#endif
#else
	#define MAP_SIZE			12	/*!< Kantenlaenge der Karte in Metern. Zentrum ist der Startplatz des Bots. Achtung! in kombination mit Macroblocks sind nur ganzzahlige Vielfache der Macroblock kantenlänge erlaubt */
	#define MAP_RESOLUTION 		128	/*!< Aufloesung der Karte in Punkte pro Meter */
	#define MAP_SECTION_POINTS 	16	/*!< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS*MAP_SECTION_POINTS Bytes  */
#endif

/*!  Suchkreis (Botdurchmesser) in Mapfelder je nach Aufloesung umgerechnet*/
#define MAP_RADIUS_FIELDS_GODEST	  (BOT_DIAMETER * MAP_RESOLUTION / 100)	/*!< Umkreisfelder fuer Pfadsuche */
/*! hier gleich Defines definieren, um kostspielige Berechnungen zu sparen */
#if MAP_RADIUS_FIELDS_GODEST / 2 < 1 
	#define MAP_RADIUS_FIELDS_GODEST_HALF 1  // sicherstellen dass nicht 0 auftritt
#else
	#define MAP_RADIUS_FIELDS_GODEST_HALF (MAP_RADIUS_FIELDS_GODEST / 2)
#endif
#define MAP_RADIUS_FIELDS_GODEST_HALF_QUAD (MAP_RADIUS_FIELDS_GODEST_HALF * MAP_RADIUS_FIELDS_GODEST_HALF) // halber Quadratradius

#define MAPFIELD_IGNORE          20  /*!< negativer Schwellwert, bei dem Laut Map Hindernis gemeldet wird */
#define HAZPOT                   5   /*! hohe Hinderniswahrscheinlichkeit und trotzdem weiter beruecksichtigt */
#define MAP_ART_HAZARD           30  /*!< kuenstliches Hindernis in MAP fuer Pathplaning in lokalem Minimum */

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
 * liefert den Durschnittswert um eine Ort herum 
 * @param x x-Ordinate der Welt 
 * @param y y-Ordinate der Welt
 * @param radius Radius der Umgebung, die Beruecksichtigt wird
 * @return Durchschnitsswert im Umkreis um den Ort (>0 heisst frei, <0 heisst belegt
 */
int8 map_get_average(float x, float y, float radius);

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
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * @param  from_x Startort x Kartenkoordinaten
 * @param  from_y Startort y Kartenkoordinaten
 * @param  to_x Zielort x Kartenkoordinaten
 * @param  to_y Zielort y Kartenkoordinaten
 * @return 1 wenn alles frei ist
 */
uint8 map_way_free_fields(uint16 from_x, uint16 from_y, uint16 to_x, uint16 to_y);
/*!
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * @param  from_x Startort x Weltkoordinaten
 * @param  from_y Startort y Weltkoordinaten
 * @param  to_x Zielort x Weltkoordinaten
 * @param  to_y Zielort y Weltkoordinaten
 * @return 1 wenn alles frei ist
 */
int8 map_way_free(float from_x, float from_y, float to_x, float to_y);

/*!
 * Setzt den Wert eines Feldes auf den angegebenen Wert
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param value neuer wert des Feldes (> 0 heisst frei, <0 heisst belegt
 */
void map_set_field(uint16 x, uint16 y, int8 value);

/*!
 * markiert ein Feld als belegt -- drueckt den Feldwert etwas mehr in Richtung "belegt"
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 */
void map_update_occupied (uint16 x, uint16 y);

/*!
 * setzt ein Map-Feld auf einen Wert mit Umfeldaktualisierung; Hindernis wird mit halben Botradius
 * eingetragen, da die Pfadpunkte im ganzen Umkreis liegen und nicht ueberschrieben werden duerfen;
 * Abgrund/ Loch wird aber auch im ganzen Botradius eingetragen; bei geringer MCU-Aufloesung ohne Umfeld mit
 * direktem Eintragen des Wertes auf xy; Umfeld wird dann in Richtung Hindernis gedrueckt 
 * @param x Map-Koordinate
 * @param y Map-Koordinate
 * @param val im Umkreis einzutragender Wert
 */
void map_set_value_occupied (uint16 x, uint16 y, int8 val);


/*! 
 * markiert die Mapkoordinaten als Loch zum entsprechenden Abgrundsensor
 * @param x	bereits berechnete Koordinaten nach links vom Mittelpunkt in Hoehe des Sensors
 * @param y	bereits berechnete Koordinaten nach rechts vom Mittelpunkt in Hoehe des Sensors
 * @param h	Blickrichtung bereits umgerechnet in Bogenmass
 */
void update_map_sensor_hole(float x, float y, float h);

/*!
 * gibt True zurueck wenn Map-Wert value innerhalb des Umkreises radius von xy liegt sonst False;
 * wird verwendet zum Check, ob sich ein Punkt (naechster Pfadpunkt, Loch) innerhalb eines bestimmten
 * Umkreises befindet; findet nur Verwendung bei hoeherer Aufloesung
 * @param x Map-Koordinate
 * @param y Map-Koordinate
 * @param radius Radius des Umfeldes
 * @param value Mapwert des Vergleiches
 * @return True wenn Wert value gefunden
 */
uint8 map_get_value_field_circle(uint16 x, uint16 y, uint8 radius, int8 value);

/*!
 * ermittelt ob der Wert val innerhalb des Umkreises mit Radius r von xy liegt; bei geringer MCU-Aufloesung direkter
 * Vergleich mit xy
 * @param x Map-Koordinate
 * @param y Map-Koordinate
 * @param radius Vergleichsradius (halber Suchkreis)
 * @param val Vergleichswert
 * @return True wenn Wert val gefunden sonst False
 */
uint8 value_in_circle (uint16 x, uint16 y, uint8 radius, int8 val);
 
/*! 
 * Routine ermittelt ab dem vom Mittelpunkt links/ rechts versetzten Punkt xp yp, in Blickrichtung
 * dist mm von den Abgrundsensoren voraus, die X-Mapkoordinate 
 * verwendet zur Map-Lochmarkierung; kann aber allgemeingueltig verwendet werden um
 * Mapkoordinaten zu erhalten in einem bestimmten Abstand voraus
 * @param xp 	Koord vom Mittelpunkt des Bots verschoben
 * @param yp 	Koord vom Mittelpunkt des Bots verschoben
 * @param h 	Blickrichtung
 * @param dist	Abstand voraus in mm
 * @return 		berechnete X-Mapkoordinate 
 */
uint16 get_mapposx_dist(float xp, float yp, float h, uint16 dist); 

/*! 
 * Routine ermittelt ab dem vom Mittelpunkt links/ rechts versetzten Punkt xp yp, in Blickrichtung
 * dist mm von den Abgrundsensoren voraus, die Y-Mapkoordinate 
 * verwendet zur Map-Lochmarkierung; kann aber allgemeingueltig verwendet werden um
 * Mapkoordinaten zu erhalten in einem bestimmten Abstand voraus
 * @param xp	Koord vom Mittelpunkt des Bots verschoben
 * @param yp	Koord vom Mittelpunkt des Bots verschoben
 * @param h		Blickrichtung
 * @param dist	Abstand voraus in mm
 * @return 		berechnete X-Mapkoordinate 
 */ 
uint16 get_mapposy_dist(float xp, float yp, float h, uint16 dist) ;

/*!
 * liefert den Durschnittswert um einen Punkt der Karte herum
 * @param x x-Ordinate der Karte
 * @param y y-Ordinate der Karte
 * @return Wert des Durchschnitts um das Feld (>0 heisst frei, <0 heisst belegt
 */
int8 map_get_average_fields (uint16 x, uint16 y, uint16 radius);
 
/*!
 * Check, ob die MAP-Koordinate xy mit destxy identisch ist (bei kleinen Aufloesungen) oder sich
 * innerhalb des halben Radius-Umkreises befindet (hoehere Aufloesungen); z.B. verwendet um das
 * Zielfahren mit gewisser Toleranz zu versehen
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param destx Ziel-x-Ordinate der Karte (nicht der Welt!!!)
 * @param desty Ziel-y-Ordinate der Karte (nicht der Welt!!!)
 * @return True wenn xy im Umkreis vorhanden oder identisch ist
 */
uint8 map_in_dest (uint16 x, uint16 y, uint16 destx, uint16 desty); 
 
/*!
 * Loescht die Mapfelder, die in einem bestimtmen Wertebereich min_val max_val liegen, d.h. diese
 * werden auf 0 gesetzt 
 * @param min_val minimaler Wert
 * @param max_val maximaler Wert
 */ 
void clear_map(int8 min_val, int8 max_val);

/*!
 * Zeigt die Karte an
 */
void print_map(void);

/*!
 * initialisiere die Karte
 * @return 0 wenn alles ok ist
 */
int8 map_init(void);

/*! 
 * Liest eine Map wieder ein 
 * @param filename Quelldatei
 */
void read_map(char * filename);

/*!
 * Schreibt einbe Karte in eine PGM-Datei
 * @param filename Zieldatei
 */
void map_to_pgm(char * filename);

// Makros, um die belegte kartenbereiche (in weltkoordinaten) zu ermitteln
#define map_get_min_x() map_to_world(map_min_x)		/*!< Minimum in X-Richtung */
#define map_get_min_y() map_to_world(map_min_y)		/*!< Minimum in Y-Richtung */
#define map_get_max_x() map_to_world(map_max_x)		/*!< Maximum in X-Richtung */
#define map_get_max_y() map_to_world(map_max_y)		/*!< Maximum in Y-Richtung */

/*!
 * zeichnet ein Testmuster in die Karte
 */
void map_draw_test_scheme(void);

/*!
 * Zeigt ein Paar Infos über dioe Karte an
 */
void map_info(void);

#endif


#endif /*MAP_H_*/
