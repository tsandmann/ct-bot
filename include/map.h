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
 * \file 	map.h
 * \brief 	Karte
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	19.09.2006
 */

#ifndef MAP_H_
#define MAP_H_

#ifdef MAP_AVAILABLE
#include "bot-logic/bot-logic.h"
#include "fifo.h"
#include "os_thread.h"
#include "botfs.h"

#define MAP_CLEAR_ON_INIT	/**< Leert die Karte, wenn der Bot gebootet wird */
#define MAP_USE_TRIG_CACHE	/**< Sollen sin(heading) und cos(heading) mit gecachet werden? */

/* Geomtrie der Karte - Achtung, nur aendern, wenn man die Konsequenzen genau kennt! */
#define MAP_SIZE_MM			12288L	/**< Kantenlaenge der Karte in mm. Zentrum ist der Startplatz des Bots. Achtung, MAP_SIZE_MM * MAP_RESOLUTION / 1000 muss ganzzahliges Vielfaches von MACRO_BLOCK_LENGTH sein! */
#define MAP_SIZE			(MAP_SIZE_MM / 1000.0)	/**< Kantenlaenge der Karte in m (also MAP_SIZE_MM / 1000). Zentrum ist der Startplatz des Bots. */
#define MAP_RESOLUTION 		125		/**< Aufloesung der Karte in Punkte / m */
#define MAP_SECTION_POINTS 	16		/**< Kantenlaenge einer Section in Punkten ==> eine Section braucht MAP_SECTION_POINTS * MAP_SECTION_POINTS Byte */

#define MAP_UPDATE_STACK_SIZE	180	/**< Groesse des Stacks, der das Map-Update ausfuehrt [Byte] */
#ifdef DEBUG_BOTFS
#undef MAP_UPDATE_STACK_SIZE
#define MAP_UPDATE_STACK_SIZE	220
#endif
#define MAP_UPDATE_CACHE_SIZE	16	/**< Groesse des Map-Caches [# Eintraege] */
#define MAP_2_SIM_STACK_SIZE	150	/**< Groesse des Map-2-Sim-Thread-Stacks [Byte]*/

#define MAP_2_SIM_BUFFER_SIZE	32	/**< Anzahl der Bloecke, die fuer Map-2-Sim gecached werden koennen */

#define MAP_OBSTACLE_THRESHOLD	-20	/**< Schwellwert, ab dem ein Feld als Hindernis gilt */
#define MAP_DRIVEN_THRESHOLD	1	/**< Schwellwert, ab dem ein Feld als befahren gilt */

#define MAP_RATIO_NONE	0		/**< Rueckgabe von map_get_ratio(), falls kein Feld den Kriterien entspricht */
#define MAP_RATIO_FULL	255		/**< Rueckgabe von map_get_tatio(), falls alle Felder den Kriterien entsprechen */

/* Die folgenden Variablen/konstanten NICHT direkt benutzen, sondern die zugehoerigen Makros: get_map_min_x() und Co!
 * Denn sonst erhaelt man Karten- und nicht Weltkoordinaten! */
extern int16_t map_min_x;		/**< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
extern int16_t map_max_x;		/**< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
extern int16_t map_min_y;		/**< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
extern int16_t map_max_y;		/**< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate */

/** Map-Cache-Eintrag */
typedef struct {
	int16_t x_pos;		/**< X-Komponente der Position [mm] */
	int16_t y_pos;		/**< Y-Komponente der Position [mm] */
#ifdef MAP_USE_TRIG_CACHE
	float sin
#if defined __arm__ && __ARM_PCS_VFP == 1
		__attribute__ ((aligned (4)))
#endif
		; /**< sin(heading) */
	float cos
#if defined __arm__ && __ARM_PCS_VFP == 1
		__attribute__ ((aligned (4)))
#endif
		; /**< cos(heading) */
#else
	int16_t heading;	/**< Blickrichtung [1/10 Grad] */
#endif // MAP_USE_TRIG_CACHE
	uint8_t dataL;		/**< Entfernung linker Distanzsensor [5 mm] aber auch BorderSensor [0/1] */
	uint8_t dataR;		/**< Entfernung rechter Distanzsensor [5 mm] aber auch BorderSensor [0/1] */
	scan_mode_t mode;	/**< Was soll aktualisiert werden */
#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
	uint8_t loc_prob;	/**< gibt an, wie sicher wir ueber die Position sind [0; 255] */
#endif
} PACKED map_cache_t;

#ifdef BOT_FS_AVAILABLE
/** Header der Map-Datei */
typedef union {
	struct {
		uint16_t alignment_offset;	/**< Offset des Kartenanfangs zur Ausrichtung auf Makroblockgroesse */
		int16_t map_min_x;			/**< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
		int16_t map_max_x;			/**< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
		int16_t map_min_y;			/**< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
		int16_t map_max_y;			/**< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate */
	} PACKED data;
	uint8_t raw[BOTFS_HEADER_DATA_SIZE];
} map_header_t;
#endif // BOT_FS_AVAILABLE

extern fifo_t map_update_fifo;			/**< Fifo fuer Cache */
extern map_cache_t map_update_cache[];	/**< Map-Cache */
extern uint8_t map_update_stack[];		/**< Stack des Update-Threads */

#ifdef MAP_2_SIM_AVAILABLE
extern uint8_t map_2_sim_worker_stack[];	/**< Stack des Map-2-Sim-Threads */
#endif // MAP_2_SIM_AVAILABLE

/**
 * Prueft, ob die Karte zurzeit gesperrt ist.
 * \return	1, falls Karte gesperrt, 0 sonst
 */
uint8_t map_locked(void);

/**
 * Liefert den Durschnittswert um einen Ort herum
 * \param x			X-Ordinate der Welt
 * \param y			Y-Ordinate der Welt
 * \param radius	Radius der Umgebung, die beruecksichtigt wird [mm]; 0 fuer ein Map-Feld (Punkt)
 * \return			Durchschnitsswert im Umkreis um den Ort (>0 heisst frei, <0 heisst belegt)
 */
int8_t map_get_average(int16_t x, int16_t y, int16_t radius);

/**
 * Liefert den Wert eines Feldes
 * \param x	X-Ordinate der Welt
 * \param y	Y-Ordinate der Welt
 * \return	Wert des Feldes (>0 heisst frei, <0 heisst belegt)
 */
static inline int8_t map_get_point(int16_t x, int16_t y) {
	return map_get_average(x, y, 0);
}

/**
 * Berechnet das Verhaeltnis der Felder einer Region R die ausschliesslich mit Werten zwischen
 * min und max belegt sind und allen Feldern von R.
 * Die Region R wird als Gerade von (x1|y1) bis (x2|y2) und eine Breite width angegeben. Die Gerade
 * verlaeuft in der Mitte von R.
 * Beispiel: Steht der Bot an (0|0) und man moechte den Weg 50 cm voraus pruefen, gibt man x1 = y1 = y2 = 0,
 * x2 = 500 und width = BOT_DIAMETER an.
 * Am besten bewertet man das Ergebnis mit Hilfe der defines MAP_RATIO_FULL und MAP_RATIO_NONE (s.u.)
 * \param x1		Startpunkt der Region R, X-Anteil; Weltkoordinaten [mm]
 * \param y1		Startpunkt der Region R, Y-Anteil; Weltkoordinaten [mm]
 * \param x2		Endpunkt der Region R, X-Anteil; Weltkoordinaten [mm]
 * \param y2		Endpunkt der Region R, Y-Anteil; Weltkoordinaten [mm]
 * \param width		Breite der Region R (jeweils width/2 links und rechts der Geraden) [mm]
 * \param min_val	minimaler Feldwert, der vorkommen darf
 * \param max_val	maximaler Feldwert, der vorkommen darf
 * \return			Verhaeltnis von Anzahl der Felder, die zwischen min_val und max_val liegen, zu
 * 					Anzahl aller Felder der Region * MAP_RATIO_FULL;
 * 					MAP_RATIO_NONE 	-> kein Feld liegt im gewuenschten Bereich;
 * 					MAP_RATIO_FULL	-> alle Felder liegen im gewuenschten Bereich
 */
uint8_t map_get_ratio(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
		int16_t width, int8_t min_val, int8_t max_val);

/**
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * \param from_x	Startort x Weltkoordinaten [mm]
 * \param from_y	Startort y Weltkoordinaten [mm]
 * \param to_x		Zielort x Weltkoordinaten [mm]
 * \param to_y		Zielort y Weltkoordinaten [mm]
 * \param margin	Breite eines Toleranzbereichs links und rechts der Fahrspur, der ebenfalls frei sein muss [mm]
 * \return			1, wenn alles frei ist
 */
uint8_t map_way_free(int16_t from_x, int16_t from_y, int16_t to_x, int16_t to_y, uint8_t margin);

/**
 * Haelt den Bot an und schreibt den Map-Update-Cache komplett zurueck
 */
void map_flush_cache(void);

/**
 * Zeigt die Karte an
 */
void map_print(void);

/**
 * Entfernt alle frei-Informationen aus der Karte, so dass nur die
 * Hindernisse uebrig bleiben.
 */
void map_clean(void);

/**
 * Initialisiert die Karte
 * \return	0 wenn alles ok ist
 */
int8_t map_init(void);

/**
 * Konvertiert eine Weltkoordinate in eine Kartenkoordinate
 * \param koord	Weltkoordiante
 * \return		Kartenkoordinate
 */
int16_t world_to_map(int16_t koord);

/**
 * Konvertiert eine Kartenkoordinate in eine Weltkoordinate
 * \param map_koord	Kartenkoordinate
 * \return 			Weltkoordiante
 */
static inline int16_t map_to_world(int16_t map_koord) {
#if (1000 / MAP_RESOLUTION) * MAP_RESOLUTION != 1000
#error "MAP_RESOLUTION ist kein Teiler von 1000, Code in map_to_world() anpassen!"
#endif
	int32_t tmp = map_koord * (1000 / MAP_RESOLUTION);
	return (int16_t) (tmp - (uint16_t) (MAP_SIZE * MAP_RESOLUTION * 4));
}

// Makros, um die belegten Kartenbereiche (in Weltkoordinaten) zu ermitteln
#define map_get_min_x() map_to_world(map_min_x)		/**< Minimum in X-Richtung */
#define map_get_min_y() map_to_world(map_min_y)		/**< Minimum in Y-Richtung */
#define map_get_max_x() map_to_world(map_max_x)		/**< Maximum in X-Richtung */
#define map_get_max_y() map_to_world(map_max_y)		/**< Maximum in Y-Richtung */

#if defined BOT_FS_AVAILABLE && defined BOTFS_COPY_AVAILABLE
/**
 * Kopiert die aktuelle Karte in eine BotFS-Datei
 * \param *file Name der Zieldatei (wird geloescht, falls sie schon existiert)
 * \return 0 falls kein Fehler, sonst Fehlercode
 */
int8_t map_save_to_file(const char * file);

/**
 * Laedt die Karte aus einer BotFS-Datei, aktuelle Karte wird dadurch geloescht
 * \param *file Name der zu ladenden BotFS-Datei
 * \return 0 falls kein Fehler, sonst Fehlercode
 */
int8_t map_load_from_file(const char * file);
#endif // BOT_FS_AVAILABLE && BOTFS_COPY_AVAILABLE

#ifdef MAP_2_SIM_AVAILABLE
/**
 * Uebertraegt die komplette Karte an den Sim
 */
void map_2_sim_send(void);

/**
 * Zeichnet eine Linie in die Map-Anzeige des Sim
 * \param from	Startpunkt der Linie (Map-Koordinate)
 * \param to	Endpunkt der Linie (Map-Koordinate)
 * \param color	Farbe der Linie: 0=gruen, 1=rot, sonst schwarz
 */
void map_draw_line(position_t from, position_t to, uint8_t color);

/**
 * Zeichnet eine Linie von Koordinate from nach to in der Farbe color in die Map ein;
 * dient zur Visualisierung der Arbeitsweise des Verhaltens
 * \param from	Koordinaten des ersten Punktes der Linie (Welt)
 * \param to	Koordinaten des zweiten Punktes der Linie (Welt)
 * \param color Farbe der Linie: 0=gruen, 1=rot, sonst schwarz
 */
void map_draw_line_world(position_t from, position_t to, uint8_t color);

/**
 * Zeichnet ein Rechteck in die Map-Anzeige des Sim
 * \param from	Startpunkt der Geraden mittig durch das Rechteck (Map-Koordinate)
 * \param to	Endpunkt der Geraden mittig durch das Rechteck (Map-Koordinate)
 * \param width	Breite des Rechtecks (jeweils width/2 links und rechts der Gerade; in Map-Aufloesung)
 * \param color	Farbe der Linien: 0=gruen, 1=rot, sonst schwarz
 */
void map_draw_rect(position_t from, position_t to, uint8_t width, uint8_t color);

/**
 * Zeichnet einen Kreis in die Map-Anzeige des Sim
 * \param center Korrdinaten des Kreismittelpunkts (Map-Koordinaten)
 * \param radius Radius des Kreies (in Map-Aufloesung)
 * \param color	Farbe der Linien: 0=gruen, 1=rot, sonst schwarz
 */
void map_draw_circle(position_t center, int16_t radius, uint8_t color);
#endif // MAP_2_SIM_AVAILABLE

#ifdef PC
char * map_file; /**< Dateiname fuer Ex- / Import */

/**
 * Liest eine Karte aus einer Map-Datei (MiniFAT-Format) ein
 * \param *filename Quelldatei
 * \return Fehlercode, 0 falls alles ok
 */
int map_read(const char * filename);

/**
 * Schreibt einbe Karte in eine PGM-Datei
 * \param filename	Zieldatei
 */
void map_to_pgm(const char * filename);
#endif // PC

#ifdef DISPLAY_MAP_AVAILABLE
/**
 * Handler fuer Map-Display
 */
void map_display(void);
#endif // DISPLAY_MAP_AVAILABLE

#endif // MAP_AVAILABLE
#endif // MAP_H_
