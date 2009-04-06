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
 * @file 	behaviour_pathplaning.c
 * @brief   Wave-Pfadplanungsverhalten; eine niedrigaufloesende Map wird ueber die hochaufloesende gelegt und
 * auf dieser folgende Schritte ausgefuehrt:
 * -Innerhalb des wirklich benutzten Mappenbereiches wird jede Zelle durchlaufen und falls der Durchschnittswert
 * der Hochaufloesenden Map < 0 ist (Hinderniswert) hier eingetragen mit Wert 1
 * -anzufahrende Zielposition erhaelt Mapwert 2
 * -ausgehend von Zielposition laeuft eine Welle los bis zum Bot-Ausgangspunkt, d.h. von 2 beginnend erhaelt jede Nachbarzelle
 * den naechst hoeheren Zellenwert
 * -wird als ein Nachbar die Botposition erreicht, wird der Pfad zurueckverfolgt und immer die Zelle mit kleinerem Wert gewaehlt;
 * die Zellenkoordinaten werden als Weltkoordinaten auf den Stack gelegt und koennen nun abgefahren werden

 *
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	23.09.2008
 */

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_PATHPLANING_AVAILABLE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ct-Bot.h"
#include "bot-local.h"
#include "map.h"
#include "ui/available_screens.h"
#include "rc5-codes.h"
#include "display.h"
#include "log.h"
#include "pos_store.h"

#define DEBUG_PATHPLANING	// Schalter fuer Debugausgaben
//#define DEBUG_PATHPLANING_VERBOSE	// zeichnet Zellen in die Map-Anzeige des Sim ein, rot: Hindernis, gruen: frei

#ifdef MCU
#undef DEBUG_PATHPLANING
#endif
#ifndef LOG_AVAILABLE
#undef DEBUG_PATHPLANING
#endif
#ifndef DEBUG_PATHPLANING
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif


/****** fuer geringaufloesende Map zur Pfadplanung ******/

#ifdef MCU
#ifndef MCU_ATMEGA644X
#error "behaviour_pathplaning derzeit nur mit ATmega644(P) moeglich!"
#endif
#endif	// MCU
#ifdef PC
#define MAP_SIZE_LOWRES			12.288	/*!< Breite / Hoehe der Karte [m] */
#else
//TODO:	Groessere Karte auch fuer MCU waere wuenschenswert
#define MAP_SIZE_LOWRES			4.096	/*!< Breite / Hoehe der Karte [m] */
#endif	// PC

#define MAP_SECTION_POINTS_LOWRES 32 /*!< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS*MAP_SECTION_POINTS Bytes  */
#define MAP_RESOLUTION_LOWRES 	7.8125	 /*!< Aufloesung der Karte in Punkte pro Meter */
#define MAP_CELL_SIZE_LOWRES	((uint16_t)(1000 / MAP_RESOLUTION_LOWRES))	/*!< Breite eines Map-Feldes in mm */
#define MAP_LENGTH_LOWRES		((uint16_t)(MAP_SIZE_LOWRES * MAP_RESOLUTION_LOWRES))	/*!< Kantenlaenge der gesamten Karte in Map-Punkten */

/*! Anzahl der Sections in der Lowres-Map */
#define MAP_SECTIONS_LOWRES (((uint16_t)(MAP_SIZE_LOWRES * MAP_RESOLUTION_LOWRES) / MAP_SECTION_POINTS_LOWRES))

typedef struct {
	int8_t section[MAP_SECTION_POINTS_LOWRES][MAP_SECTION_POINTS_LOWRES]; /*!< Einzelne Punkte */
} map_section_lowres_t; /*!< Datentyp fuer die Elementarfelder einer Gruppe */

map_section_lowres_t * map_lowres[MAP_SECTIONS_LOWRES][MAP_SECTIONS_LOWRES]; /*!< Array mit den Zeigern auf die Elemente */

static pos_store_t * planning_pos_store = NULL;		/*!< Positionsspeicher */
static position_t pos_store_data[POS_STORE_SIZE];	/*!< Stack-Speicher fuer Positionsspeicher */

static position_t startwave;	/*!< Ausgangspunkt der Welle (eigentlicher Zielpunkt), in Mapkoordinaten */
static position_t destination;	/*!< Zielpunkt in Weltkoordinaten */

/*! Map-Koordinate des Startpunktes (Botpos) zum Terminieren der fortlaufenden Welle, da diese vom Zielpunkt ausgeht */
static position_t endkoord;

/* das Rechteck der real benutzten Koordinaten der Welt */
static int16_t min_x;
static int16_t min_y;
static int16_t max_x;
static int16_t max_y;

/*! Wellenzaehler; uint8_t ausreichend da in dieser Aufloesung 8 Punkte je Meter -> bei 12 Meter noch unterhalb Wertebereich */
static uint8_t wavecounter = 0;

// Zustaende des Wellenverhaltens
#define START							0
#define SET_HAZARDS_TO_LOWRES			1
#define NEIGHBOURS_FROM_FIFO			2
#define SEARCH_STACKPATH_AND_QUEUE		3
#define START_BOT_GO_STACK_BEHAVIOUR	4
#define CORRECT_POSITION				5
#define END								99

/*! Begrenzung des Wellenzaehlers, d.h. obere Grenze als Abbruchbedingung */
#define MAX_WAVECOUNTER 120

static uint8_t wave_state = 0; /*!< Statusvariable */

static int8_t map_compare_haz = 0; /*!< Vergleichswert unterhalb dem Hinderniswert gesetzt wird */

/*!
 * Konvertiert eine Lowres-Kartenkoordinate in eine Weltkoordinate
 * @param map_koord	Kartenkoordinate
 * @return 			Weltkoordiante
 */
static int16_t map_to_world_lowres(uint16_t map_koord) {
	return (map_koord - MAP_LENGTH_LOWRES / 2) * MAP_CELL_SIZE_LOWRES;
}

/*!
 * Konvertiert eine Weltkoordinate in eine Lowres-Kartenkoordinate
 * @param koord	Weltkoordiante
 * @return		Kartenkoordinate
 */
static uint16_t world_to_map_lowres(int16_t koord) {
	return (koord + MAP_LENGTH_LOWRES * (MAP_CELL_SIZE_LOWRES / 2))
			/ MAP_CELL_SIZE_LOWRES;
}

/*!
 * Zugriff auf ein Feld der Lowres-Karte. Kann lesend oder schreibend sein.
 * @param field	X/Y-Koorrdinate der Karte
 * @param value	Neuer Wert des Feldes
 * @param set	0 zum Lesen, 1 zum Schreiben
 * @return      Mapwert
 */
static int8_t access_field_lowres(position_t field, int8_t value, uint8_t set) {
	uint16_t section_x, section_y, index_x, index_y;

//TODO: Sectiongroesse von 512 Byte waere besser, um auf MMC auslagern zu koennen.

	// Berechne in welcher Sektion sich der Punkt befindet
	section_x = field.x / MAP_SECTION_POINTS_LOWRES;
	section_y = field.y / MAP_SECTION_POINTS_LOWRES;

	if ((section_x >= MAP_SECTIONS_LOWRES)
			|| (section_y >= MAP_SECTIONS_LOWRES)) {
		LOG_ERROR("Versuch ein Feld ausserhalb der Karte zu lesen!! x=%d y=%d\n", field.x, field.y);
		return 0;
	}

	// Berechne den Index innerhalb der Section
	index_x = field.x % MAP_SECTION_POINTS_LOWRES;
	index_y = field.y % MAP_SECTION_POINTS_LOWRES;

	if (set) { // Schreibzugriff
		// Eventuell existiert die Section noch nicht
		if (map_lowres[section_x][section_y] == NULL) {
			// Dann anlegen
			map_lowres[section_x][section_y] = malloc(
					sizeof(map_section_lowres_t));
			if (map_lowres[section_x][section_y] == NULL) {
				deactivateBehaviour(bot_calc_wave_behaviour);
				return 0;
			}
			memset(map_lowres[section_x][section_y], 0,
					sizeof(map_section_lowres_t));
		}
		map_lowres[section_x][section_y]->section[index_x][index_y] = value;
		return value; // Schluss mit Werterueckgabe nach Schreibzugriff
	}

	// Eventuell existiert die Section noch nicht
	if (map_lowres[section_x][section_y] == NULL) {
		return 0;
	}

	return map_lowres[section_x][section_y]->section[index_x][index_y];
}

/*!
 * Loescht die komplette Lowres-Karte
 */
static inline void delete_lowres(void) {
	uint8_t i, j;
	LOG_DEBUG("Delete von 0-%1d", MAP_SECTION_POINTS_LOWRES);
	for (j = 0; j < MAP_SECTIONS_LOWRES*MAP_SECTION_POINTS_LOWRES; j++) {
		//LOG_DEBUG("Zeile %1d", j);
		for (i = 0; i < MAP_SECTIONS_LOWRES*MAP_SECTION_POINTS_LOWRES; i++) {
			access_field_lowres((position_t) {i, j}, 0, 1);
			//LOG_DEBUG("Spalte %1d",i);
		}
	}
}

/*!
 * Eintragen der Hindernisse in Lowres-Karte aus der Map-Highres-Karte
 */
static void set_hazards(void) {
	int16_t x, y;

	// Umrechnen des die Welt umschliessenden Rechtecks in Lowres-Koordinaten
	min_x = world_to_map_lowres(map_get_min_x()) - 1;
	max_x = world_to_map_lowres(map_get_max_x()) + 1;
	min_y = world_to_map_lowres(map_get_min_y()) - 1;
	max_y = world_to_map_lowres(map_get_max_y()) + 1;
	min_y = (min_y > 0) ? min_y : 0; // sicherstellen dass Grenzen positiv sind
	min_x = (min_x > 0) ? min_x : 0;

	int16_t yw;

	// Zellen durchlaufen und aus der hochaufloesenden Weltkarte Hindernisse hier eintragen
	for (y = max_y; y >= min_y; y--) {
		yw = map_to_world_lowres(y) + MAP_CELL_SIZE_LOWRES / 2; // Zeilenwert als Weltkoord umrechnen, Mittelpunkt der Zelle
		for (x = min_x; x <= max_x; x++) {
			int16_t xw = map_to_world_lowres(x) + MAP_CELL_SIZE_LOWRES / 2; // Mittelpunkt der Zelle, also Haelfte addieren
			int16_t x1 = xw - MAP_CELL_SIZE_LOWRES / 2; // Punkt "unten", darum Haelfte der Zellengroesse abziehen
			int16_t x2 = xw + MAP_CELL_SIZE_LOWRES / 2; // Punkt "oben", darum Haelfte der Zellegroesse addieren
#ifdef DEBUG_PATHPLANING_VERBOSE
			position_t from, to;
			from.x = world_to_map(x1);
			from.y = world_to_map(yw);
			to.x = world_to_map(x2);
			to.y = from.y;
#endif	// DEBUG_PATHPLANING_VERBOSE
			uint8_t ratio = map_get_ratio(x1, yw, x2, yw, MAP_CELL_SIZE_LOWRES,
					map_compare_haz, 127);
			if (ratio < MAP_RATIO_FULL) {
				access_field_lowres((position_t) {x, y}, 1, 1);
#ifdef DEBUG_PATHPLANING_VERBOSE
//				LOG_DEBUG("Trage Hindernis in (%d|%d) ein, ratio=%u", x, y, ratio);
				map_draw_rect(from, to, MAP_CELL_SIZE_LOWRES / (1000 / MAP_RESOLUTION), 1);
#endif	// DEBUG_PATHPLANING_VERBOSE
			} else {
#ifdef DEBUG_PATHPLANING_VERBOSE
//				LOG_DEBUG("KEIN Hindernis in (%d|%d), ratio=%u", x, y, ratio);
				map_draw_rect(from, to, MAP_CELL_SIZE_LOWRES / (1000 / MAP_RESOLUTION), 0);
#endif	// DEBUG_PATHPLANING_VERBOSE
			}
		}
	}
}

/*!
 * Wellenwert wird auf die uebergebene Koordinate eingetragen und in die FIFO-Queu uebernommen auf nicht-Hinderniswert
 * @param map 				X/Y-Lowres-Map Koordinate
 * @param actual_wave		einzutragender Wellenwert
 * @param end_reached		Kennung ob schon Zielpunkt erreicht wurde
 * @param *neighbour_found	Kennung ob Nachbarzelle gueltig war (kein Hindernis und initial), d.h.Kennung ob Welle gueltigen Nachbarn gefunden hatte
 * @return 					True wenn Zielpunkt erreicht sonst Wert end_reached selbst
 */
static uint8_t get_neighbour(position_t map, int8_t actual_wave,
		uint8_t end_reached, uint8_t * neighbour_found) {
	// nur im gueltigen Bereich
	if (map.x < 0 || map.y < 0 || map.x < min_x || map.x > max_x || map.y
			< min_y || map.y > max_y)
		return end_reached;

	// Mapwert auslesen aus uebergebener Koordinate des Nachbarn
	int8_t mapval = access_field_lowres(map, 0, 0);

	// Wellenwert wird nur auf noch initiale Felder eingetragen
	if (mapval == 0) { // nicht fuer Hindernis
		// in Lowres-Map Wellenwert auf die Koordinaten vermerken
		access_field_lowres(map, actual_wave, 1);

		// Nachbarpunkt als gueltig kennzeichnen
		*neighbour_found = True;

		// in FIFO-Queue aufnehmen zur weiteren Nachbarsuche
		if (!pos_store_queue(planning_pos_store, map)) {
			LOG_DEBUG(">> -Queue voll- %1d %1d bei Punkt: %1d %1d zu Welle: %1d", map.x, map.y, actual_wave);
		}
	} // nur initiale Felder

	// Ziel gefunden
	if (map.x == endkoord.x && map.y == endkoord.y) {
		LOG_DEBUG("Startkoord erreicht auf %1d %1d", map.x, map.y);
		return True;
	}

	// vorherigen Wert zurueckgeben
	return end_reached;
} // Ende get_neighbour

#ifdef DEBUG_PATHPLANING
/*!
 * Zeigt einen Ausschnitt der Planungs-Map auf Konsole an; gut zum Pruefen wo Hindernisse gesehen werden und die Welle verlaeuft
 */
static void show_labmap(void) {
	int16_t x, y;
	int16_t xw;
	int16_t yw;

	LOG_DEBUG("Wellenstart vom Zielpunkt %1d %1d", startwave.x, startwave.y);
	access_field_lowres(startwave, 2, 1); // vermerken des Wellen-Startpunktes
	LOG_DEBUG("Botpunkt %1d %1d", endkoord.x, endkoord.y);
	access_field_lowres(endkoord, 88, 1); // Botpos vermerken

	/* Grenzen finden */
	int16_t start_y, end_y;
	if (startwave.y > endkoord.y) {
		start_y = endkoord.y;
		end_y = startwave.y;
	} else {
		start_y = startwave.y;
		end_y = endkoord.y;
	}
	int16_t start_x, end_x;
	if (startwave.x > endkoord.x) {
		start_x = endkoord.x;
		end_x = startwave.x;
	} else {
		start_x = startwave.x;
		end_x = endkoord.x;
	}
	start_x -= 5;
	end_x += 5;
	start_y -= 5;
	end_y += 5;

	printf("y:    ");
	for (y=end_y; y>=start_y; y--) {
		printf("%2d|", y);
	}
	printf("\n");

	int8_t tmp = 0;
	for (x=end_x; x>=start_x; x--) {
		printf("x: %2d:", x);

		for (y=end_y; y>=start_y; y--) {
			xw = map_to_world_lowres(x);
			yw = map_to_world_lowres(y);
			tmp = access_field_lowres((position_t) {x, y}, tmp, 0);

			printf("%2d|", tmp);
		}
		printf("\n");
	}
}
#endif	// DEBUG_PATHPLANING

/*!
 * Wave-Verhalten; berechnet die Welle ausgehend vom Zielpunkt bis zur Botposition; dann wird diese zurueckverfolgt und sich der Pfad
 * auf dem Stack gemerkt und anschliessend das Stack-Fahrverhalten aufgerufen
 * @param *data	Zeiger auf Verhaltensdatensatz
 */
void bot_calc_wave_behaviour(Behaviour_t * data) {

	/*! Mapvariable zum Durchlaufen der Lowres-Planungs-Map */
	static position_t pos = { 0, 0 };

	// Zaehlervariablen zum Bestimmen der Nachbarn
	static int8_t i = 0;
	static int8_t j = 0;

	// Kennung ob eine gueltige Nachbarzelle gefunden werden konnte
	static uint8_t neighbour_found = 0;

	// Endekennung
	static uint8_t endreached = False; // Kennung gesetzt fuer Ziel gefunden; Terminierung der Schleife


	switch (wave_state) {
	// zuerst loeschen der Planungs-LowRes-Karte
	case START:
		LOG_DEBUG("Loeschen der Lowres-Karte");
		delete_lowres();
		planning_pos_store = pos_store_create(data, pos_store_data); // Stack / Queue anlegen / und leeren
		wavecounter = 2; // geht ab Wert 2 los; d.h. Wert Wellenzentrum - Zielpunkt bekommt diesen Wert
		access_field_lowres(startwave, 2, 1);

		wave_state = SET_HAZARDS_TO_LOWRES;
		break;

	// Uebertragen der in der Highres-Karte vermerkten Hindernisse in die Lowres-Planungs-Karte
	case SET_HAZARDS_TO_LOWRES:
		LOG_DEBUG("Hindernisse eintragen");
		set_hazards(); // Hindernisse aus der Highres-Karte in die Planungs-Lowres-Karte eintragen
		wave_state = NEIGHBOURS_FROM_FIFO;
		break;

	// hier abarbeiten aller in der FIFO-Queue eingetragenen Koordinaten, d.h. fuer jede wird wieder der gueltige Nachbar in die Queue hinten eingefuegt bis Ende erreicht
	case NEIGHBOURS_FROM_FIFO:
		wavecounter++; // die ersten Nachbarn haben den Startwellenwert (2) + 1
		endreached = False; // Abbruchbedingung der Schleife init.

		pos_store_queue(planning_pos_store, startwave); // Startpunkt der Welle in die Queue einfuegen, damit gehts los

		LOG_DEBUG("Wellenstart bei %1d %1d Welle %1d", startwave.x, startwave.y, wavecounter);

		pos_store_queue(planning_pos_store, (position_t) {999, 999}); // nach Abarbeiten des Wellenstartpunktes muss Wellenzaehler erhoeht werden

		// Solange FIFO-Queue durchgehen und Nachbarn wieder anfuegen bis Queue leer ist oder Ziel (Botstartpunkt) gefunden (1 Nachbar ist Zielfeld)
		while (!endreached && pos_store_dequeue(planning_pos_store, &pos)) {
			// bei Kennung 999 ist Welle abgearbeitet, d.h. zu allen Punkten der Welle die Nachbarn wieder hinten angefuegt und Wellenzaehler wird erhoeht
			// wenn die Nachbarn der neuen Welle beginnen wird Wellenzaehler erhoeht
			if (pos.x == 999 && pos.y == 999) {
				// die letzte in Queue eingefuegte Koord wird sich gemerkt; wenn zu dieser bei naechster Welle alle Nachbarn
				// bearbeitet sind, ist naechste Welle vorbei
				wavecounter++;

				// Erhoehung fuer Wellenzaehler vermerken wenn Endepunkt noch nicht gefunden ist und gueltiger Nachbarpunkt vorhanden war
				if (!endreached && neighbour_found) {
					pos_store_queue(planning_pos_store, (position_t) {999, 999});
				}

				// Kennung fuer gueltigen Nachbarn wieder ruecksetzen
				neighbour_found = False;
			} else {
				// abarbeiten aller 4 Nachbarn zu einem Punkt der Queue, auch wenn schon gefunden wurde zwecks guter Pfadfindung; ein True wird nicht ueberschrieben
				for (j=-1; j<=1; j++) {
					for (i=-1; i<=1; i++) {
						if ((j == 0 && i != 0) || (i == 0 && j != 0)) {
//						if (!(i == 0 && j == 0)) { // Diagonalversion
							position_t tmp = pos;
							tmp.x += i;
							tmp.y += j;
							endreached = get_neighbour(tmp, wavecounter, endreached, &neighbour_found);
						}
					}
				}

			}
		}

		// Loeschen der FIFO-Queue; Stack- bzw. Queue-Array wird ja im folgenden
		// verwendet zur Speicherung der Abfahrpositionen bei der Wellenrueckverfolgung vom Bot ausgehend zum Wellenstartpunkt
		pos_store_clear(planning_pos_store);

		// Wellenpfad zurueckverfolgen oder gleich Ende wenn nichts gefunden
		wave_state = (endreached) ? SEARCH_STACKPATH_AND_QUEUE : END;

		if (endreached) {
			LOG_DEBUG("Welle hat Botpos erreicht")
		} else {
			LOG_DEBUG("Welle konnte Botpos nicht erreichen->Abbruch");
		}

		break;

	// in der Lowres-Karte sind nun die Hindernisse und die Welle eingetragen; vom Botpunkt zum Wellenstartpunkt
	// wird die Welle nun zurueckverfolgt und die anzufahrenden Koordinaten in den Stack gespeichert
	case SEARCH_STACKPATH_AND_QUEUE:
		LOG_DEBUG("--Welle hat Botposition erreicht bei Wert %1d", wavecounter);

		// ein paar notwendige Initialisierungen und Variablendeklarationen
		int8_t minval = 1;
		position_t nextdest = {0, 0};

		// Ausgehend vom Wellen-Ausgangspunkt, dem Zielpunkt, wird jeweils der Nachbar genommen mit dem kleinsten Wert
		pos = endkoord;

		int8_t mapval_min = wavecounter + 1; // auf erhoehten Wellenwert setzen, weil in Schleife der Wellenwert immer kleiner dem letzten Wellewert ist
		endreached = False; // Schleifenabbruchvar init.
		wavecounter = 0; // Wellenzaehler init.

#ifdef MAP_2_SIM_AVAILABLE
        position_t lastpos ;
        position_t aktpos ;
        lastpos.x = 0; // initialisieren
        lastpos.y = 0;
#endif	// MAP_2_SIM_AVAILABLE

		// Map durchlaufen und den Nachbarn mit immer niedrigerem Mapwert verfolgen bis Wellenstartpunkt erreicht wurde
		while (!endreached) {
			neighbour_found = False;

			// fuer jeden Nachbarn von Botposition bis Wellenstartpunkt den immer kleineren Wellenwert verfolgen
			for (j=-1; (j<=1) && !endreached; j++) {
				for (i=-1; (i<=1) && !endreached; i++) {

					// Vergleich nur fuer die richtigen 8 Nachbarn und wenn Wellenstartpunkt noch nicht erreicht
//					if (((j == 0 && i != 0) || (i == 0 && j != 0)) && !(pos.x + i == endkoord.x && pos.y + j == endkoord.y)) { // nur orthogonal
					if ((!(i == 0 && j == 0)) && !(pos.x + i == endkoord.x && pos.y + j == endkoord.y)) { // Diagonalversion
						// nur gueltige Koords gecheckt

						position_t tmp = pos;
						tmp.x += i;
						tmp.y += j;
						minval = access_field_lowres(tmp, 0, 0); // Mapwert auslesen
						// die Koordinate mit niedrigstem Wellenwert merken
						if (minval> 1 && minval < mapval_min) { // naechster genommener Wellenwert muss kleiner aus letztem Lauf sein
							nextdest.x = pos.x + i;
							nextdest.y = pos.y + j;

							LOG_DEBUG("kleinster Wellenwert %1d Koord %1d %1d", minval, nextdest.x, nextdest.y);

							// Wellenwert und Kennung fuer Nachbar gefunden setzen
							mapval_min = minval;
							neighbour_found = True;

							// wenn ein Nachbar Zielpunkt ist Endekennung setzen
							if (nextdest.x == startwave.x && nextdest.y == startwave.y) {
								endreached = True;
								LOG_DEBUG("Ende gefunden %1d %1d", nextdest.x, nextdest.y);
							}
						}
					}
				}
			}

			// hier die Punkte jeweils in die Queue (Stack) einfuegen; das hier zuerst eingefuegte naechste Ziel ist ja bei der Wellenrueckverfolgung die naechste Position vom Bot ausgehend;
			// die Positionen werden aber hinten angefuegt, d.h. die naechste Botpos zuerst bis zum Ziel (Start der Welle); das Botfahrverhalten muss dann aber auch die hier zuerst eingefuegten
			// Positionen auch zuerst abfahren
			if (neighbour_found || endreached) {
				LOG_DEBUG("Pfadpunkt in Queue %1d %1d Ende %1d", nextdest.x, nextdest.y, endreached);
				// zum spaeteren Stack-Abfahren die Zwischenziel-Koordinaten als Weltkoordinaten auf den Stack legen
				position_t pos;
				pos.x = map_to_world_lowres(nextdest.x) + MAP_CELL_SIZE_LOWRES / 2;
				pos.y = map_to_world_lowres(nextdest.y) + MAP_CELL_SIZE_LOWRES / 2;
				if (!pos_store_queue(planning_pos_store, pos)) {
					LOG_DEBUG("Queue ging schief - voll?");
					endreached = True;
				}

#ifdef MAP_2_SIM_AVAILABLE
				aktpos.x = world_to_map(map_to_world_lowres(nextdest.x) + MAP_CELL_SIZE_LOWRES / 2);
				aktpos.y = world_to_map(map_to_world_lowres(nextdest.y) + MAP_CELL_SIZE_LOWRES / 2);
				if (lastpos.x != 0 && lastpos.y != 0) {
					map_draw_line(lastpos, aktpos, 2); // Pfadpunkte schwarz darstellen
				}
				lastpos = aktpos;
#endif	// MAP_2_SIM_AVAILABLE
			}
			wavecounter++; // Wellencounter erhoehen; dient hier nur fuer Abbruchbedingung nach erreichen eines bestimmten Zaehlerstandes

			if (wavecounter >= MAX_WAVECOUNTER) { // spaetestens jetzt Abbruchbedingung zur Sicherheit
				endreached = True;
				LOG_DEBUG("Endecounter erreicht %1d", wavecounter);
			}

			// Ausgangspunkt fuer naechsten Durchlauf setzen, also ab Nachbarpunkt mit geringstem Wellenwert weiter zurueckverfolgen
			pos = nextdest;
		}

		// Wenn erfolgreich Pfad gefunden wurde, dann abfahren sonst Ende
		wave_state = (endreached) ? START_BOT_GO_STACK_BEHAVIOUR : END;
		if (endreached) {
			LOG_DEBUG("Pfad rueckwaerts gefunden->Stackgo")
		} else {
			LOG_DEBUG("Pfad not found->Ende");
		}
		break;

	case START_BOT_GO_STACK_BEHAVIOUR:
		LOG_DEBUG("--Pfad gefunden und Abfahren -STACKGO--");
#ifdef DEBUG_PATHPLANING
		show_labmap();
#endif
		bot_drive_stack_x(data, pos_store_get_index(planning_pos_store), 1);
		wave_state = CORRECT_POSITION;
		break;

	case CORRECT_POSITION:
		bot_goto_pos(data, destination.x, destination.y, 999);
		wave_state = END;
		break;

	default:
		LOG_DEBUG("Waveverhalten beendet. Wavecounter %1d", wavecounter);
		pos_store_release(planning_pos_store);
		planning_pos_store = NULL;
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Routine zum Setzen der Zielkoordinaten
 * @param x X-Welt-Zielkoordinate
 * @param y Y-Welt-Zielkoordinate
 */
static void bot_set_destination(int16_t x, int16_t y) {
	destination.x = x;
	destination.y = y;

	LOG_DEBUG("Wellen-Start bei %1d %1d", x, y);

	startwave.x = world_to_map_lowres(x);
	startwave.y = world_to_map_lowres(y);
}

/*!
 * Rufe das Wave-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param map_compare Map-Vergleichswert; Mapwerte kleiner dem Wert werden als Hindernisse eingetragen
 */
void bot_do_calc_wave(Behaviour_t * caller, int8_t map_compare) {
	switch_to_behaviour(caller, bot_calc_wave_behaviour, OVERRIDE);
	wave_state = 0;
    map_compare_haz = map_compare > -128 ? map_compare : -127; // Wert setzen, unterhalb dessen Hindernis gesetzt wird

	LOG_DEBUG("Start Welle vom Zielpunkt %1d %1d", startwave.x, startwave.y);

	// Botkoordinate setzen zum Terminieren des Wellenverhaltens
	endkoord.x = world_to_map_lowres(x_pos);
	endkoord.y = world_to_map_lowres(y_pos);
	LOG_DEBUG("Botpos auf %1d %1d", endkoord.x, endkoord.y);

	/* Kollisions-Verhalten ausschalten  */
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
	deactivateBehaviour(bot_avoid_col_behaviour);
#endif
}

/*!
 * Rufe das Wave-Verhalten auf mit Uebergabe des zu erreichenden Zielpunktes
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param dest_x    X-Welt-Zielkoordinate
 * @param dest_y    Y-Welt-Zielkoordinate
 * @param map_compare Map-Vergleichswert; Mapwerte kleiner dem Wert werden als Hindernisse eingetragen
 */
void bot_calc_wave(Behaviour_t * caller, int16_t dest_x, int16_t dest_y, int8_t map_compare) {
	// Zielpunkt setzen, ab dem die Welle losgeht
	bot_set_destination(dest_x, dest_y); // Weltkoords werden in Lowres-Mapkoords umgerechnet
 	bot_do_calc_wave(caller, map_compare);
}

#ifdef PATHPLANING_DISPLAY
/*!
 * Key-Handler fuer Display
 */
static void pathplaning_disp_key_handler(void) {
	/* Keyhandling fuer Pathplaning-Verhalten */
	switch (RC5_Code) {

	case RC5_CODE_4:
		RC5_Code = 0;
		//delete_lowres();
		bot_do_calc_wave(NULL, 30);  // nur auf bereits befahrenem Gebiet Weg zum Ziel planen
		break;

	case RC5_CODE_5:
		RC5_Code = 0;
		bot_do_calc_wave(NULL, 0);  // Aufruf mit Map-Vergleichswert 0; Werte darunter gelten als Hindernisse
		break;

#ifdef DEBUG_PATHPLANING
	case RC5_CODE_6:
		RC5_Code = 0;
		show_labmap();
		break;
#endif
	case RC5_CODE_8:
		RC5_Code = 0;
		bot_set_destination(x_pos, y_pos);
		break;

	}
} // Ende Keyhandler

/*!
 * Display der Pfadplanung-Routinen
 */
void pathplaning_display(void) {
	display_cursor(1, 1);
	display_printf("-Pathplaning-");
	display_cursor(2, 1);
	display_printf("4:PlanOnDrivenArea"); // nur auf befahrenem Gebiet planen
	display_cursor(3, 1);
	display_printf("5:GoPlaning");
	display_cursor(4, 1);
#ifdef DEBUG_PATHPLANING
	display_printf("6/8:ShowMap/SetDest");
#else
	display_printf("8:SetDest");
#endif

	pathplaning_disp_key_handler();
}
#endif	// PATHPLANING_DISPLAY
#endif	// BEHAVIOUR_PATHPLANING_AVAILABLE
