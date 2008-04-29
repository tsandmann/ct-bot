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
 * @file 	map.c  
 * @brief 	Karte 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	19.09.06
 */	
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ct-Bot.h"
#include "bot-local.h"
#include "sensor_correction.h"
#include "map.h"
#include "mmc.h"
#include "mini-fat.h"
#include "ui/available_screens.h"
#include "rc5-codes.h"
#include "sensor.h"
#include "display.h"
#include "log.h"
#include "timer.h"
#include "fifo.h"
#include "os_thread.h"

#ifdef MAP_AVAILABLE

#ifndef MMC_AVAILABLE
  #ifdef MCU
  	#error Map geht auf dem MCU nicht ohne MMC
  #endif
#endif 
 

//#define DEBUG_MAP			// Schalter um recht viel Debug-Code anzumachen
//#define DEBUG_MAP_TIMES	// Schalter um Performance-Messungen fuer MMC anzumachen
//#define DEBUG_STORAGE		// Noch mehr Ausgaben zum Thema organisation der Kartenstruktur, Macroblocks, Sections
//#define DEBUG_SCAN_OTF	// Debug-Infos des Update-Threads an

#define MAP_INFO_AVAILABLE
#ifdef MCU
	// Soll auch der echte Bot Infos ausgeben, kommentiert man die folgende Zeile aus
	#undef MAP_INFO_AVAILABLE	// spart Flash
#endif
	
#ifndef LOG_AVAILABLE
	#undef DEBUG_MAP
	#undef DEBUG_STORAGE
#endif
#ifndef DEBUG_MAP
	#undef DEBUG_STORAGE
	#undef LOG_DEBUG
	#define LOG_DEBUG(a, ...) {}
#endif

/*
 * Eine Karte ist wie folgt organisiert:
 * Es gibt Sektionen zu je MAP_SECTION_POINTS * MAP_SECTION_POINTS. 
 * Diese Sektionen enthalten direkt die Pixel-Daten
 * Auf dem PC liegen die Daten genauso im RAM, wie sie auf der MMC
 * angeordnet sind.
 * 
 * Auf dem MCU passt eine Sektion in einen Flashblock, bzw immer 2 
 * Sektionen in einen Block Es stehen immer 2 Sections also 1 
 * Flash-Block im SRAM und werden bei Bedarf gewechselt.
 * 
 * Felder sind vom Typ int8 und haben einen Wertebereich von -128 
 * bis 127.
 * 0 bedeutet: wir wissen nichts über das Feld
 * negative Werte bedeuten: Hinderniss
 * postivie Werte bedeuten: freier Weg
 * je groesser der Betrag ist, desto sicherer die Aussage ueber das 
 * Feld.
 * Der Wert -128 ist Loechern vorbehalten und wird dann auch nicht 
 * durch die Abstandssensoren veraendert.
 *
 * Felder werden wie folgt aktualisiert:
 * Wird ein Punkt als frei betrachtet, erhoehen wir den Wert des 
 * Feldes um MAP_STEP_FREE.
 * Wird ein Punkt als belegt erkannt, ziehen wir um ihn einen 
 * Streukreis mit dem Radius MAP_RADIUS.  
 * Das Zentrum des Kreises wird um MAP_STEP_OCCUPIED dekrementiert, 
 * nach aussen hin immer weniger.
 * Wird ein Feld als Loch erkannt, setzen wir den Wert fest auf -128.
 */

#define MAP_SECTIONS (((uint16_t)(MAP_SIZE*MAP_RESOLUTION)/MAP_SECTION_POINTS))	/*!< Anzahl der Sections in der Map */

#define MAP_STEP_FREE_SENSOR		2	/*!< Um diesen Wert wird ein Feld inkrementiert, wenn es vom Sensor als frei erkannt wird */
#define MAP_STEP_FREE_LOCATION		20	/*!< Um diesen Wert wird ein Feld inkrementiert, wenn der Bot drüber fährt */

#define MAP_STEP_OCCUPIED	10	/*!< Um diesen Wert wird ein Feld dekrementiert, wenn es als belegt erkannt wird */

#define MAP_RADIUS			50	/*!< Umkreis eines Messpunktes, der als Besetzt aktualisiert wird (Streukreis) [mm]*/
#define MAP_RADIUS_FIELDS	(MAP_RESOLUTION*MAP_RADIUS/1000)	/*!< Umkreis einen Messpunkt, der als Besetzt aktualisiert wird (Streukreis) [Felder]*/

#define MAP_PRINT_SCALE					/*!< Soll das PGM eine Skala erhalten */
#define MAP_SCALE	(MAP_RESOLUTION/2)	/*!< Alle wieviel Punkte kommt ein Skalen-Strich */

#define MACRO_BLOCK_LENGTH	512L // Kantenlaenge eines Macroblocks in Punkten/Byte
#define MAP_LENGTH_IN_MACRO_BLOCKS ((uint16_t)(MAP_SIZE*MAP_RESOLUTION)/MACRO_BLOCK_LENGTH)

#ifdef SHRINK_MAP_ONLINE
	uint16_t map_min_x = MAP_SIZE * MAP_RESOLUTION / 2; /*!< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
	uint16_t map_max_x = MAP_SIZE * MAP_RESOLUTION / 2; /*!< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
	uint16_t map_min_y = MAP_SIZE * MAP_RESOLUTION / 2; /*!< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
	uint16_t map_max_y = MAP_SIZE * MAP_RESOLUTION / 2; /*!< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate */
#else
	const uint16_t map_min_x = 0;							/*!< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
	const uint16_t map_min_y = 0;							/*!< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
	const uint16_t map_max_x = MAP_SIZE * MAP_RESOLUTION;	/*!< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
	const uint16_t map_max_y = MAP_SIZE * MAP_RESOLUTION;	/*!< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate */
#endif

/*! Datentyp fuer die Elementarfelder einer Gruppe */
typedef struct {
	int8_t section[MAP_SECTION_POINTS][MAP_SECTION_POINTS]; /*!< Einzelne Punkte */
} map_section_t;

static uint32_t map_start_block = 0;	/*!< Block, bei dem die Karte auf der MMC-Karte beginnt */

map_cache_t map_update_cache[MAP_UPDATE_CACHE_SIZE];	/*!< Cache */
fifo_t map_update_fifo;									/*!< Fifo fuer Cache */

uint8_t map_update_stack[MAP_UPDATE_STACK_SIZE];		/*!< Stack des Update-Threads */
static Tcb_t * map_update_thread;						/*!< Thread fuer Map-Update */
static uint8_t lock_signal = 0;							/*!< Signal zur Synchronisation von Kartenzugriffen */

#ifdef MCU
//  avr-gcc >= 4.2.2 ?
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 2 && __GNUC_PATCHLEVEL__ >= 2
// kein Pro- und Epilog
void map_update_main(void) __attribute__((OS_task));
#define PROLOG()	// NOP
#else
// kein Pro- und Epilog
void map_update_main(void) __attribute__((naked));
// Frame-Pointer laden (bei naked macht der Compiler das nicht)
#define PROLOG()	asm volatile(							\
					"ldi r28, lo8(map_update_stack)	\n\t"	\
					"ldi r29, hi8(map_update_stack)		"	\
					:::	"memory")
#endif	// GCC-Version
#else
void map_update_main(void);
#define PROLOG()	// NOP
#endif	// MCU

// Es passen immer 2 Sektionen in den Puffer
uint8_t map_buffer[sizeof(map_section_t) * 2];	/*!< statischer Puffer */
map_section_t * map[2];							/*!< Array mit den Zeigern auf die Elemente */
uint16_t map_current_block = 0; 				/*!< Block, der aktuell im Puffer steht. Nur bis 32 MByte adressierbar */
uint8_t map_current_block_updated = False; 		/*!< markiert, ob der aktuelle Block gegenueber der MMC-Karte veraendert wurde */

#ifdef PC
typedef struct {
	map_section_t sections[2];
} mmc_container_t;

mmc_container_t map_storage[MAP_SECTIONS * MAP_SECTIONS / 2];	/*!< Statischer Speicherplatz fuer die Karte */

// MMC-Zugriffe emuliert der PC
#define mmc_read_sector(block, buffer)		memcpy(&buffer, &(map_storage[block]), sizeof(mmc_container_t));
#define mmc_write_sector(block, buffer)		memcpy(&(map_storage[block]), &buffer, sizeof(mmc_container_t));
#endif	// PC

/*!
 * initialisiere die Karte
 * @return	0 wenn alles ok ist
 */
int8_t map_init(void) {
	/* Update-Thread initialisieren */
#ifdef OS_DEBUG
	os_mask_stack(map_update_stack, MAP_UPDATE_STACK_SIZE);
#endif
	fifo_init(&map_update_fifo, map_update_cache, sizeof(map_update_cache));
	map_update_thread = os_create_thread(&map_update_stack[MAP_UPDATE_STACK_SIZE-1], map_update_main);

#ifdef BEHAVIOUR_SCAN_AVAILABLE
	/* Modi des Update-Verhaltens. Default: location, distance, border an, Kartographie-Modus */
	scan_otf_modes.location = 1;
	scan_otf_modes.distance = 1;
	scan_otf_modes.border = 0;
	scan_otf_modes.map_mode = 1;
#endif
	
	// Die Karte auf den Puffer biegen
	map[0] = (map_section_t *)map_buffer;
	map[1] = (map_section_t *)(map_buffer+sizeof(map_section_t));

#ifdef MCU
	map_current_block_updated = 0xFF; // Die MMC-Karte ist erstmal nicht verfuegbar
	if (mmc_get_init_state() != 0)
		return 1;
	map_start_block = mini_fat_find_block("MAP", map_buffer);
	if (map_start_block == 0xFFFFFFFF) {
		return 1;
	}
	// Makroblock-Alignment auf ihre Groesse
	uint32_t file_block = map_start_block;
	map_start_block += 2 * MACRO_BLOCK_LENGTH * (MACRO_BLOCK_LENGTH / 512) - 1;
	map_start_block &= 0xFFFFFC00;
	
	// Map-Offset im Datei-Header (0x120-0x123) speichern
	*((uint32_t *)&map_buffer[0x120]) = map_start_block - file_block;
	mmc_write_sector(file_block-1, map_buffer);
	
	map_current_block_updated = False;
	map_current_block = 0;
#endif	// MCU
	return 0;
}

/*!
 * liefert einen Zeiger auf die Section zurueck, in der der Punkt liegt.
 * Auf dem MCU kuemmert sie sich darum, die entsprechende Karte aus der MMC-Karte zu laden
 * @param x	X-Ordinate der Karte (nicht der Welt!!!)
 * @param y	Y-Ordinate der Karte (nicht der Welt!!!)
 * @return	Zeiger auf die Section
 */
static map_section_t * get_section(uint16_t x, uint16_t y) {

	// Berechne in welcher Sektion sich der Punkt befindet
	uint16_t section_x = x / MAP_SECTION_POINTS;
	uint16_t section_y = y / MAP_SECTION_POINTS;

	// Da imemr 2 Sections in einem Block stehen: richtige der beiden Sections raussuchen
	uint8_t index = section_x & 0x01;

	// Sicherheitscheck 1
	if ((section_x >= MAP_SECTIONS) || (section_y >= MAP_SECTIONS)) {
		LOG_DEBUG("Versuch auf ein Feld ausserhalb der Karte zu zugreifen!! x=%u y=%u",x,y);
		return map[0];
	}

	// Sicherheitscheck 2
	// wenn die Karte nicht sauber initialisiert ist, mache nix!
	if (map_current_block_updated == 0xFF) {
		return map[0];
	}
	
	uint16_t macroblock = x / MACRO_BLOCK_LENGTH + (y / MACRO_BLOCK_LENGTH) * MAP_LENGTH_IN_MACRO_BLOCKS;

#ifdef DEBUG_STORAGE
	LOG_DEBUG("Macroblock= %u", macroblock);
#endif
	// Berechne den gesuchten Block
	uint16_t local_x = x % MACRO_BLOCK_LENGTH;
	// wenn MACRO_BLOCK_LENGTH eine 2er Potenz ist, optimiert der Compiler hier
	uint16_t local_y = y % MACRO_BLOCK_LENGTH;

#ifdef DEBUG_STORAGE
	LOG_DEBUG("\tlocal_x= %u, local_y= %u", local_x, local_y);
#endif

	uint16_t block = local_x / MAP_SECTION_POINTS + (local_y / MAP_SECTION_POINTS)
			* (MACRO_BLOCK_LENGTH / MAP_SECTION_POINTS);

	block = block >> 1; // es passen immer 2 Sections in einen Block

	block += macroblock * MACRO_BLOCK_LENGTH * (MACRO_BLOCK_LENGTH / 512); // noch in den richtigen Macroblock springen

	// Ist der Block schon geladen?
	if (map_current_block == block) {
#ifdef DEBUG_STORAGE
		LOG_DEBUG("ist noch im Puffer");
#endif
		return map[index];
	}

	// Block ist also nicht im Puffer
#ifdef DEBUG_STORAGE
	LOG_DEBUG("ist nicht im Puffer");
#endif

	// Wurde der Block im RAM veraendert?
	if (map_current_block_updated == True) {
		// Dann erstmal sichern
		uint32_t mmc_block = map_start_block + map_current_block;	// Offset fuer die Lage der Karte drauf
#ifdef DEBUG_MAP_TIMES
		LOG_INFO("writing block 0x%04x%04x", (uint16_t)(mmc_block>>16), (uint16_t)mmc_block);
		uint16_t start_ticks = TIMER_GET_TICKCOUNT_16;
#endif
		mmc_write_sector(mmc_block, map_buffer);
#ifdef DEBUG_MAP_TIMES
		uint16_t end_ticks = TIMER_GET_TICKCOUNT_16;
		LOG_INFO("swapout took %u ms", (end_ticks-start_ticks)*176/1000);
#endif
	}

	// Statusvariablen anpassen
	map_current_block = block;
	map_current_block_updated = False;
	
	// Lade den neuen Block
	// Auf der MMC beginnt die Karte nicht bei 0, sondern irgendwo, auf dem PC schadet es nix		
	uint32_t mmc_block = block + map_start_block;	// Offset fuer die Lage der Karte drauf
#ifdef DEBUG_MAP_TIMES
	LOG_INFO("reading block 0x%04x%04x", (uint16_t)(mmc_block>>16), (uint16_t)mmc_block);
	uint16_t start_ticks = TIMER_GET_TICKCOUNT_16;
#endif
	LOG_DEBUG("mmc_block=%lu", mmc_block);
	mmc_read_sector(mmc_block, map_buffer);
#ifdef DEBUG_MAP_TIMES
	uint16_t end_ticks = TIMER_GET_TICKCOUNT_16;
	LOG_INFO("swapin took %u ms", (end_ticks-start_ticks)*176/1000);
#endif

	return map[index];
}

/*!
 * Konvertiert eine Weltkoordinate in eine Kartenkoordinate
 * @param koord	Weltkoordiante
 * @return		Kartenkoordinate
 */
static uint16_t world_to_map(int16_t koord) {
#if (1000 / MAP_RESOLUTION) * MAP_RESOLUTION != 1000
	#warning "MAP_RESOLUTION ist kein Teiler von 1000, Code in world_to_map() anpassen!"
#endif
	uint32_t tmp = koord + (uint16_t)(MAP_SIZE * MAP_RESOLUTION * 4);
	return tmp / (1000 / MAP_RESOLUTION);
}

//#define OLD_GET_SET

#ifdef OLD_GET_SET
/*!
 * liefert den Wert eines Feldes 
 * @param x	X-Ordinate der Karte (nicht der Welt!!!)
 * @param y	Y-Ordinate der Karte (nicht der Welt!!!)
 * @return	Wert des Feldes (>0 heisst frei, <0 heisst belegt)
 */
static int8_t get_field(uint16_t x, uint16_t y) {
	uint16_t index_x, index_y;
	
	// Suche die Section heraus
	map_section_t * section = get_section(x, y);
	

	// Berechne den Index innerhalb der Section
	index_x = x % MAP_SECTION_POINTS;
	index_y = y % MAP_SECTION_POINTS;

	return section->section[index_x][index_y];	
}

/*!
 * Setzt den Wert eines Feldes auf den angegebenen Wert
 * @param x		X-Ordinate der Karte (nicht der Welt!!!)
 * @param y		Y-Ordinate der Karte (nicht der Welt!!!)
 * @param value	Neuer Wert des Feldes (> 0 heisst frei, <0 heisst belegt)
 */
static void set_field(uint16_t x, uint16_t y, int8_t value) {
	uint16_t index_x, index_y;

	// Suche die Section heraus
	map_section_t * section = get_section(x, y);

	// Berechne den Index innerhalb der Section
	index_x = x % MAP_SECTION_POINTS;
	index_y = y % MAP_SECTION_POINTS;

	section->section[index_x][index_y] = value;

#ifdef SHRINK_MAP_ONLINE
	// Belegte Kartengroesse anpassen
	if (x < map_min_x)
		map_min_x=x;
	if (x > map_max_x)
		map_max_x= x;
	if (y < map_min_y)
		map_min_y=y;
	if (y > map_max_y)
		map_max_y= y;
#endif

	if (map_current_block_updated != 0xFF) {
		map_current_block_updated = True;
	}
}

#else	// !OLD_GET_SET

/* get/set_field auf access_field umleiten */
#define get_field(x, y)		access_field(x, y, 0, 0)
#define set_field(x, y, v)	access_field(x, y, v, 1)

/*!
 * Zugriff auf ein Feld der Karte. Kann lesend oder schreibend sein.
 * @param x		X-Ordinate der Karte
 * @param y		Y-Ordinate der Karte
 * @param value	Neuer Wert des Feldes (> 0 heisst frei, <0 heisst belegt)
 * @param set	0 zum Lesen, 1 zum Schreiben
 */
static int8_t access_field(uint16_t x, uint16_t y, int8_t value, uint8_t set) {
	uint16_t index_x, index_y;

	// Suche die Section heraus
	map_section_t * section = get_section(x, y);

	// Berechne den Index innerhalb der Section
	index_x = x % MAP_SECTION_POINTS;
	index_y = y % MAP_SECTION_POINTS;
	
	int8_t * data = &section->section[index_x][index_y];
	
	if (set) {	
		*data = value;

		if (map_current_block_updated != 0xFF) {
			map_current_block_updated = True;
		}

#ifdef SHRINK_MAP_ONLINE
		// Belegte Kartengroesse anpassen
		if (x < map_min_x)
			map_min_x=x;
		if (x > map_max_x)
			map_max_x= x;
		if (y < map_min_y)
			map_min_y=y;
		if (y > map_max_y)
			map_max_y= y;
#endif
	}
	return *data;
}
#endif	// OLD_GET_SET

/*!
 * liefert den Durschnittswert um einen Punkt der Karte herum 
 * @param x 		x-Ordinate der Karte 
 * @param y 		y-Ordinate der Karte
 * @param radius	gewuenschter Radius
 * @return 			Wert des Durchschnitts um das Feld (>0 heisst frei, <0 heisst belegt)
 */
static int8_t get_average_fields(uint16_t x, uint16_t y, int16_t radius) {
	int16_t avg = 0;
	int16_t avg_line = 0;
	
	int16_t dX, dY;
	int16_t h = radius * radius;
	
	/* warten bis Karte frei ist (nur MCU) */
	os_signal_set(&lock_signal);
	
	/* Daten auslesen */
	for (dX = -radius; dX <= radius; dX++) {
		for (dY = -radius; dY <= radius; dY++) {
			if (dX*dX + dY*dY <= h) {	 
				avg_line += get_field(x + dX, y + dY);
			}
		}
		avg += avg_line / (radius * 2);
	}
	
	/* Signal zur Kartensperre wieder freigeben (nur MCU) */
	os_signal_release();

	return (int8_t)(avg / (radius * 2));
}

/*!
 * liefert den Durschnittswert um eine Ort herum 
 * @param x			x-Ordinate der Welt
 * @param y			y-Ordinate der Welt
 * @param radius	Radius der Umgebung, die beruecksichtigt wird [mm]
 * @return 			Durchschnitsswert im Umkreis um den Ort (>0 heisst frei, <0 heisst belegt)
 */
int8_t map_get_average(int16_t x, int16_t y, int16_t radius) {
	//Ort in Kartenkoordinaten
	uint16_t X = world_to_map(x);
	uint16_t Y = world_to_map(y);
	int16_t R = radius * MAP_RESOLUTION / 1000;
	if (R == 0) {
		/* nur ein Feld gewuenscht, kleiner geht auch nicht */
		R = 1;
	}
	
	return get_average_fields(X, Y, R);
}

/*!
 * Aendert den Wert eines Feldes um den angegebenen Betrag 
 * @param x		x-Ordinate der Karte (nicht der Welt!!!)
 * @param y		y-Ordinate der Karte (nicht der Welt!!!)
 * @param value	Betrag um den das Feld veraendert wird (>= heisst freier, <0 heisst belegter)
 */
static void update_field(uint16_t x, uint16_t y, int8_t value) {

	int16_t tmp = get_field(x, y);
	if (tmp == -128) // Nicht aktualiseren, wenn es sich um ein Loch handelt
		return;

	tmp += value;

	// pruefen, ob kein Ueberlauf stattgefunden hat
	if ((tmp < 128) && (tmp > -128))
		set_field(x, y, (int8_t)tmp);
}

/*!
 * Aendert den Wert eines Kreises um den angegebenen Betrag 
 * @param x			x-Ordinate der Karte (nicht der Welt!!!)
 * @param y			y-Ordinate der Karte (nicht der Welt!!!)
 * @param radius	Radius in Kartenpunkten
 * @param value		Betrag um den das Feld veraendert wird (>= heisst freier, <0 heisst belegter)
 */
static void update_field_circle(uint16_t x, uint16_t y, int16_t radius,
		int8_t value) {
	const int16_t square = radius * radius;

	int16_t sec_x_max = ((x + radius) / MAP_SECTION_POINTS);
	int16_t sec_x_min = ((x - radius) / MAP_SECTION_POINTS);
	int16_t sec_y_max = ((y + radius) / MAP_SECTION_POINTS);
	int16_t sec_y_min = ((y - radius) / MAP_SECTION_POINTS);
	//	LOG_DEBUG("Betroffene Sektionen X: %d-%d, Y:%d-%d\n",sec_x_min,sec_x_max,sec_y_min,sec_y_max);

	int16_t sec_x, sec_y, X, Y, dX, dY;
	int16_t starty, startx, stopx, stopy;
	// Gehe über alle betroffenen Sektionen 
	for (sec_y = sec_y_min; sec_y <= sec_y_max; sec_y++) {
		// Bereich weiter eingrenzen
		if (sec_y*MAP_SECTION_POINTS > (y-radius))
			starty=sec_y*MAP_SECTION_POINTS;
		else
			starty=y-radius;
		if ((sec_y+1)*MAP_SECTION_POINTS < (y+radius))
			stopy=(sec_y+1)*MAP_SECTION_POINTS;
		else
			stopy=y+radius;

		for (sec_x= sec_x_min; sec_x <= sec_x_max; sec_x++) {
			if (sec_x*MAP_SECTION_POINTS > (x-radius))
				startx=sec_x*MAP_SECTION_POINTS;
			else
				startx=x-radius;
			if ((sec_x+1)*MAP_SECTION_POINTS < (x+radius))
				stopx=(sec_x+1)*MAP_SECTION_POINTS;
			else
				stopx=x+radius;

			for (Y = starty; Y < stopy; Y++) {
				dY= y-Y; // Distanz berechnen
				dY*=dY; // Quadrat vorberechnen
				for (X = startx; X < stopx; X++) {
					dX= x-X; // Distanz berechnen
					dX*=dX; // Quadrat vorberechnen
					if (dX + dY <= square) // wenn Punkt unter Bot	 
						update_field(X, Y, value); // dann aktualisieren
				}

			}
		}
	}
}

/*!
 * Markiert ein Feld als belegt -- drueckt den Feldwert etwas mehr in Richtung "belegt"
 * @param x	x-Ordinate der Karte (nicht der Welt!!!)
 * @param y	y-Ordinate der Karte (nicht der Welt!!!)
 */
static void update_occupied(uint16_t x, uint16_t y) {
	// Nur wenn ein Umkreis gewuenscht ist, auch einen malen
#if MAP_RADIUS_FIELDS > 0
	uint8_t r;
	for (r=1; r<=MAP_RADIUS_FIELDS; r++) {
		update_field_circle(x, y, r, -MAP_STEP_OCCUPIED/MAP_RADIUS_FIELDS);
	}
#else 
	update_field(x, y, -MAP_STEP_OCCUPIED);
#endif
}

/*!
 * Map-Umfeldaktualisierung mit einem bestimmten Wert ab einer Position xy mit Radius r bei
 * @param x			Map-Koordinate
 * @param y			Map-Koordinate
 * @param radius	Radius des Umfeldes
 * @param value 	Mapwert; nur eingetragen wenn aktueller Mapwert groesser value ist
 */
static void set_value_field_circle(uint16_t x, uint16_t y, int8_t radius, int8_t value) {
	int16_t dX, dY;
    int16_t h = radius*radius;
	for (dX = -radius; dX <= radius; dX++) {
		for (dY = -radius; dY <= radius; dY++) {
			if (dX*dX + dY*dY <= h) {
				// nur innerhalb des Umkreises
				if (get_field(x + dX, y + dY) > value) {
					// Mapwert hoeher Richtung frei
					set_field (x + dX, y + dY,value);	// dann Wert eintragen
				}
			}
		}
	}
}

/*!
 * setzt ein Map-Feld auf einen Wert mit Umfeldaktualisierung; 
 * Hindernis wird mit MAP_RADIUS_FIELDS eingetragen
 * @param x		Map-Koordinate
 * @param y		Map-Koordinate
 * @param val	im Umkreis einzutragender Wert
 */
static void set_value_occupied(uint16_t x, uint16_t y, int8_t val) {
	uint8_t r;
	// in Map mit dem Radius um x/y eintragen
	for (r=1; r<=MAP_RADIUS_FIELDS; r++) {
		set_value_field_circle(x, y, r, val);
	}
}

/*! 
 * Aktualisiert die Karte mit den Daten eines Distanz-Sensors
 * @param x		X-Achse der Position des Sensors
 * @param y 	Y-Achse der Position des Sensors
 * @param h 	Blickrichtung im Bogenmass
 * @param dist 	Sensorwert 
 */ 
static void update_sensor_distance(int16_t x, int16_t y, float h, int16_t dist) {
	// Ort des Sensors in Kartenkoordinaten
	uint16_t X = world_to_map(x);
	uint16_t Y = world_to_map(y);

	uint16_t d;
	if (dist == SENS_IR_INFINITE) {
		d = SENS_IR_MAX_DIST;
	} else {
		d = dist;
	}
	
	// liefert die Mapkoordinaten des Hindernisses / Ende des Frei-Strahls 
	uint16_t PH_X = world_to_map(x + (int16_t)(d * cos(h)));
	uint16_t PH_Y = world_to_map(y + (int16_t)(d * sin(h)));

	if ((dist > 80 ) && (dist < SENS_IR_INFINITE)) {
		update_occupied(PH_X, PH_Y);
	}

	// Nun markiere alle Felder vor dem Hinderniss als frei
	uint16_t i;

	uint16_t lX = X; //	Beginne mit dem Feld, in dem der Sensor steht
	uint16_t lY = Y;

	int8_t sX = (PH_X < X ? -1 : 1);
	uint16_t dX = abs(PH_X - X); // Laenge der Linie in X-Richtung

	int8_t sY = (PH_Y < Y ? -1 : 1);
	uint16_t dY =abs(PH_Y - Y); // Laenge der Linie in Y-Richtung

	if (dX >= dY) { // Hangle Dich an der laengeren Achse entlang
		dY--; // stoppe ein Feld vor dem Hinderniss
		uint16_t lh = dX / 2;
		for (i=0; i<dX; ++i) {
			update_field(lX+i*sX, lY, MAP_STEP_FREE_SENSOR);
			lh += dY;
			if (lh >= dX) {
				lh -= dX;
				lY += sY;
			}
		}
	} else {
		dX--; // stoppe ein Feld vor dem Hinderniss
		uint16_t lh = dY / 2;
		for (i=0; i<dY; ++i) {
			update_field(lX, lY+i*sY, MAP_STEP_FREE_SENSOR);
			lh += dX;
			if (lh >= dY) {
				lh -= dY;
				lX += sX;
			}
		}
	}
}

/*!
 * Aktualisiert die interne Karte anhand der Sensordaten
 * @param x		X-Achse der Position in Weltkoordinaten
 * @param y		Y-Achse der Position in Weltkoordinaten
 * @param head	Blickrichtung in Grad
 * @param distL	Sensorwert links
 * @param distR	Sensorwert rechts
 */
static void update_distance(int16_t x, int16_t y, float head, int16_t distL,
		int16_t distR) {
//	LOG_DEBUG("update_map: x=%f, y=%f, head=%f, distL=%d, distR=%d\n",x,y,head,distL,distR);

//TODO:	Berechnungen optimieren, 
//		vielleicht Start und Endpunkt an map_update_sensor_distance() uebergeben?
	float h = head * (M_PI/180.0f);
	float cos_h = cos(h);
	float sin_h = sin(h);

	// Ort des rechten Sensors in Weltkoordinaten
	int16_t Pr_x = x + (DISTSENSOR_POS_SW * sin_h)
			+ (DISTSENSOR_POS_FW * cos_h);
	int16_t Pr_y = y - (DISTSENSOR_POS_SW * cos_h)
			+ (DISTSENSOR_POS_FW * sin_h);

	// Ort des linken Sensors in Weltkoordinaten
	int16_t Pl_x = x - (DISTSENSOR_POS_SW * sin_h)
			+ (DISTSENSOR_POS_FW * cos_h);
	int16_t Pl_y = y + (DISTSENSOR_POS_SW * cos_h)
			+ (DISTSENSOR_POS_FW * sin_h);

	update_sensor_distance(Pl_x, Pl_y, h, distL);
	update_sensor_distance(Pr_x, Pr_y, h, distR);
}

/*!
 * Aktualisiert den Standkreis der internen Karte
 * @param x X-Achse der Position in Weltkoordinaten
 * @param y Y-Achse der Position in Weltkoordinaten
 */
static void update_location(int16_t x, int16_t y) {
	uint16_t x_map = world_to_map(x);
	uint16_t y_map = world_to_map(y);

	// Aktualisiere zuerst die vom Bot selbst belegte Flaeche
	update_field_circle(x_map, y_map, BOT_DIAMETER/20*MAP_RESOLUTION/100,
			MAP_STEP_FREE_LOCATION);
}

/*!
 * Aktualisiert die interne Karte anhand der Abgrund-Sensordaten
 * @param x			X-Achse der Position in Weltkoordinaten
 * @param y			Y-Achse der Position in Weltkoordinaten
 * @param head		Blickrichtung in Grad
 * @param borderL	Sensor links 1= abgrund 0 = frei
 * @param borderR	Sensor rechts 1= abgrund 0 = frei
 */
static void update_border(int16_t x, int16_t y, float head, uint8_t borderL,
		uint8_t borderR) {
	float h = head * (M_PI/180.0); // Bogenmass
	float sin_head = sin(h);
	float cos_head = cos(h);

	if (borderR > 0) {
		// Ort des rechten Sensors in Mapkoordinaten
		uint16_t x_map = world_to_map(x
				+ BORDERSENSOR_POS_SW * sin_head +
				BORDERSENSOR_POS_FW * cos_head);
		uint16_t y_map = world_to_map(y
				- BORDERSENSOR_POS_SW * cos_head +
				BORDERSENSOR_POS_FW * sin_head);
		set_value_occupied(x_map, y_map, -128);
	}

	if (borderL > 0) {
		uint16_t x_map = world_to_map(x
				- BORDERSENSOR_POS_SW * sin_head +
				BORDERSENSOR_POS_FW * cos_head);

		uint16_t y_map = world_to_map(y
				+ BORDERSENSOR_POS_SW * cos_head +
				BORDERSENSOR_POS_FW * sin_head);
		set_value_occupied(x_map, y_map, -128);
	}
}

/*!
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * @param  from_x	Startort x Kartenkoordinaten
 * @param  from_y	Startort y Kartenkoordinaten
 * @param  to_x		Zielort x Kartenkoordinaten
 * @param  to_y		Zielort y Kartenkoordinaten
 * @return			1 wenn alles frei ist
 */
static uint8_t way_free_fields(uint16_t from_x, uint16_t from_y,
		uint16_t to_x, uint16_t to_y) {
//TODO:	Aehnlichkeiten mit map_update_sensor_distance() in Fkt auslagern?
//		Idee: process_line(from_x, from_y, to_x, to_y, line_width, worker_function);
	
	// gehe alle Felder der Reihe nach durch
	uint16_t i;

	uint16_t lX = from_x; //	Beginne mit dem Feld, in dem der Bot steht
	uint16_t lY = from_y;

	int8_t sX = (to_x < from_x ? -1 : 1);
	uint16_t dX = abs(to_x - from_x); // Laenge der Linie in X-Richtung

	int8_t sY = (to_y < from_y ? -1 : 1);
	uint16_t dY = abs(to_y - from_y); // Laenge der Linie in Y-Richtung

	int16_t width = (BOT_DIAMETER/10*MAP_RESOLUTION)/100;
	int16_t w = 0;

	if (dX >= dY) { // Hangle Dich an der laengeren Achse entlang
		uint16_t lh = dX / 2;
		for (i=0; i<dX; ++i) {
			for (w=-width; w<= width; w++) {
				// wir muessen die ganze Breite des absuchen
				if (get_field(lX+i*sX, lY+w) < -MAPFIELD_IGNORE) {
					// ein Hinderniss reicht fuer den Abbruch
					return 0;
				}
			}

			lh += dY;
			if (lh >= dX) {
				lh -= dX;
				lY += sY;
			}
		}
	} else {
		uint16_t lh = dY / 2;
		for (i=0; i<dY; ++i) {
			for (w=-width; w<= width; w++) {
				// wir muessen die ganze Breite des absuchen
				if (get_field(lX+w, lY+i*sY) <-MAPFIELD_IGNORE) { 
					// ein Hinderniss reicht fuer den Abbruch
					return 0;
				}
			}
			
			lh += dX;
			if (lh >= dY) {
				lh -= dY;
				lX += sX;
			}
		}
	}

	return 1;
}

/*!
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * @param  from_x	Startort x Weltkoordinaten
 * @param  from_y	Startort y Weltkoordinaten
 * @param  to_x		Zielort x Weltkoordinaten
 * @param  to_y		Zielort y Weltkoordinaten
 * @return			1 wenn alles frei ist
 */
int8_t map_way_free(int16_t from_x, int16_t from_y, int16_t to_x, int16_t to_y) {
	return way_free_fields(world_to_map(from_x), world_to_map(from_y),
			world_to_map(to_x), world_to_map(to_y));
}


/*!
 * Main-Funktion des Map-Update-Threads
 */
void map_update_main(void) {
	PROLOG();	// bei aelteren Compilern Frame-Pointer manuell laden
	
	map_cache_t cache_tmp;

	/* Endlosschleife -> Thread wird vom OS blockiert / gibt die Kontrolle ab, 
	 * wenn der Puffer leer ist */
	while (1) {
		/* Cache-Eintrag holen 
		 * PC-Version blockiert hier, falls Fifo leer */
		if (fifo_get_data(&map_update_fifo, &cache_tmp, sizeof(map_cache_t))
				> 0) {

			os_signal_lock(&lock_signal);	// Zugriff auf die Map sperren
#ifdef DEBUG_SCAN_OTF
			LOG_DEBUG("lese Cache: x= %d y= %d head= %f distance= %d loaction=%d border=%d",cache_tmp.x_pos, cache_tmp.y_pos, cache_tmp.heading/10.0f,cache_tmp.mode.distance, cache_tmp.mode.location, cache_tmp.mode.border);

			if ((cache_tmp.mode.distance || cache_tmp.mode.location || cache_tmp.mode.border) == 0)
			LOG_DEBUG("Achtung: Dieser Eintrag ergibt keinen Sinn, kein einziges mode-bit gesetzt");
#endif

			/* Strahlen updaten, falls distance-mode und der aktuelle Eintrag Daten dazu hat*/
			if (cache_tmp.mode.distance) {
				update_distance(cache_tmp.x_pos, cache_tmp.y_pos,
						cache_tmp.heading/10.0f, cache_tmp.dataL*5,
						cache_tmp.dataR*5);
			}

			/* Grundflaeche updaten, falls location-mode */
			if (cache_tmp.mode.location) {
				update_location(cache_tmp.x_pos, cache_tmp.y_pos);
			}

			/* Abgrundsensoren updaten, falls border-mode */
			if (cache_tmp.mode.border) {
				update_border(cache_tmp.x_pos, cache_tmp.y_pos,
						cache_tmp.heading/10.0f, cache_tmp.dataL,
						cache_tmp.dataR);
			}

		} else {
			/* Fifo leer => weiter mit Main-Thread */
			os_signal_unlock(&lock_signal);	// Zugriff auf Map wieder freigeben
			os_thread_wakeup(os_threads);	// Main ist immer der Erste im Array
		}
	}
}


/*!
 * Zeigt die Karte an
 */
void map_print(void) {
#ifdef PC
	map_to_pgm("map.pgm");
#endif	// PC
}

/*!
 * Loescht die komplette Karte
 */
static void delete(void) {
#ifdef MCU
	uint32_t map_filestart = mini_fat_find_block("MAP", map_buffer);
	mini_fat_clear_file(map_filestart, map_buffer);
	map_current_block_updated = False;
	map_current_block = 0;
#else
	memset(map_storage, 0, sizeof(map_storage));
#endif	// MCU

#ifdef SHRINK_MAP_ONLINE
	// Groesse neu initialisieren
	map_min_x = (uint16_t)(MAP_SIZE * MAP_RESOLUTION / 2);
	map_max_x = (uint16_t)(MAP_SIZE * MAP_RESOLUTION / 2);
	map_min_y = (uint16_t)(MAP_SIZE * MAP_RESOLUTION / 2);
	map_max_y = (uint16_t)(MAP_SIZE * MAP_RESOLUTION / 2);
#endif	// SHRINK_MAP_ONLINE
}


// PC-only Code

#ifdef PC
/*!
 * zeichnet ein Testmuster in die Karte
 */
static void draw_test_scheme(void) {
	int16_t x, y;

	// Erstmal eine ganz simple Linie
	for (x=0; x< MAP_SECTION_POINTS*MAP_SECTIONS; x++) {
		set_field(x, x, -120);
		set_field(MAP_SECTION_POINTS*MAP_SECTIONS-x-1,x,-120);
	}

	// Grenzen der Sections Zeichnen
	for (x=0; x< MAP_SECTION_POINTS*MAP_SECTIONS; x++) {
		for (y=0; y< MAP_SECTIONS; y++) {
			set_field(x, y*MAP_SECTION_POINTS, -10);
			set_field(y*MAP_SECTION_POINTS, x, -10);
		}
	}

	// Grenzen der Macroblocks einzeichnen
	for (x=0; x< MAP_SECTION_POINTS*MAP_SECTIONS; x++) {
		for (y=0; y< MAP_LENGTH_IN_MACRO_BLOCKS; y++) {
			set_field(x, y*MACRO_BLOCK_LENGTH, -60);
			set_field(y*MACRO_BLOCK_LENGTH, x, -60);
		}
	}
}

/*!
 * Verkleinert die Karte vom uebergebenen auf den benutzten Bereich. Achtung, 
 * unter Umstaenden muss man vorher die Puffervariablen sinnvoll initialisieren!!!
 * @param min_x Zeiger auf einen uint16, der den miniamlen X-Wert puffert
 * @param max_x Zeiger auf einen uint16, der den maxinmalen X-Wert puffert
 * @param min_y Zeiger auf einen uint16, der den miniamlen Y-Wert puffert
 * @param max_y Zeiger auf einen uint16, der den maxinmalen Y-Wert puffert
 */
static void shrink(uint16_t * min_x, uint16_t * max_x, uint16_t * min_y,
		uint16_t * max_y) {
	uint16_t x, y;

	// lokale Variablen mit den defaults befuellen
	*min_x = map_min_x;
	*max_x = map_max_x;
	*min_y = map_min_y;
	*max_y = map_max_y;

	// Kartengroesse reduzieren
	int8_t free=1;
	while ((*min_y < *max_y) && (free ==1)) {
		for (x=*min_x; x<*max_x; x++) {
			if (get_field(x, *min_y) != 0) {
				free=0;
				break;
			}
		}
		*min_y+=1;
	}

	free=1;
	while ((*min_y < *max_y) && (free ==1)) {
		for (x=*min_x; x<*max_x; x++) {
			if (get_field(x, *max_y-1) != 0) {
				free=0;
				break;
			}
		}
		*max_y-=1;
	}

	free=1;
	while ((*min_x < *max_x) && (free ==1)) {
		for (y=*min_y; y<*max_y; y++) {
			if (get_field(*min_x, y) != 0) {
				free=0;
				break;
			}
		}
		*min_x+=1;
	}
	free=1;
	while ((*min_x < *max_x) && (free ==1)) {
		for (y=*min_y; y<*max_y; y++) {
			if (get_field(*max_x-1, y) != 0) {
				free=0;
				break;
			}
		}
		*max_x-=1;
	}
}

/*!
 * Schreibt eine Karte in eine PGM-Datei
 * @param filename	Zieldatei
 */
void map_to_pgm(char * filename) {
	printf("Speichere Karte nach %s\n", filename);
	FILE * fp = fopen(filename, "wb");

	uint16_t x, y;

	// lokale Variablen mit den Defaults befuellen
	uint16_t min_x = map_min_x;
	uint16_t max_x = map_max_x;
	uint16_t min_y = map_min_y;
	uint16_t max_y = map_max_y;

#ifdef SHRINK_MAP_OFFLINE	// nun muessen wir die Grenezn ermitteln
	shrink(&min_x, &max_x, &min_y, &max_y);
#endif
	uint16_t map_size_x = max_x - min_x;
	uint16_t map_size_y = max_y - min_y;
#ifdef MAP_PRINT_SCALE
	fprintf(fp, "P5 %d %d 255 ", map_size_x+10, map_size_y+10);
#else
	fprintf(fp,"P5 %d %d 255 ", map_size_x, map_size_y);
#endif	// MAP_PRINT_SCALE
	printf(
			"Karte beginnt bei X=%d,Y=%d und geht bis X=%d,Y=%d (%d * %d Punkte)\n",
			min_x, min_y, max_x, max_y, map_size_x, map_size_y);

	uint8_t tmp;
	for (y=max_y; y>min_y; y--) {
		for (x=min_x; x<max_x; x++) {
			tmp=get_field(x, y-1)+128;
			fwrite(&tmp, 1, 1, fp);
		}

#ifdef MAP_PRINT_SCALE
		// und noch schnell ne Skala basteln
		for (x=0; x<10; x++) {
			if (y % MAP_SCALE == 0) {
				tmp=0;
			} else {
				tmp=255;
			}
			fwrite(&tmp, 1, 1, fp);
		}
#endif	// MAP_PRINT_SCALE
	}

#ifdef MAP_PRINT_SCALE
	for (y=0; y<10; y++) {
		for (x=min_x; x<max_x+10; x++) {
			if (x % MAP_SCALE == 0) {
				tmp=0;
			} else {
				tmp=255;
			}
			fwrite(&tmp, 1, 1, fp);
		}
	}
#endif	// MAP_PRINT_SCALE
	fclose(fp);
}

/*! 
 * Liest eine Map wieder ein 
 * @param filename	Quelldatei
 */
void map_read(char * filename) {
	map_init();

	printf("Lese Karte (%s) von MMC/SD (Bot-Format)\n", filename);
	FILE * fp = fopen(filename, "rb");

	uint8_t buffer[512];
	fread(buffer, 512, 1, fp);

	if (buffer[0] != 'M' || buffer[1] != 'A' || buffer[2] != 'P') {
		printf("Datei %s enthaelt keine Karte\n", filename);
		return;
	}
	
	/* um Makroblock-Offset vorspulen */
	uint32_t offset = buffer[0x120] | buffer[0x121]<<8;
	printf("Makroblock-Offset=0x%04x\n", offset);
	fseek(fp, offset*512, SEEK_CUR);

	// Karte liegt auf der MMC genau wie im PC-RAM
	fread(&map_storage, sizeof(map_storage), 1, fp);

	fclose(fp);

#ifdef SHRINK_MAP_ONLINE
	// Groesse neu initialisieren
	map_min_x = 0;
	map_max_x = MAP_SIZE * MAP_RESOLUTION;
	map_min_y = 0;
	map_max_y = MAP_SIZE * MAP_RESOLUTION;

	// und Karte verkleinern
	shrink(&map_min_x, &map_max_x, &map_min_y, &map_max_y);
#endif
}
#endif	// PC


// Gui-Code

#ifdef MAP_INFO_AVAILABLE	
/*!
 * Zeigt ein paar Infos ueber die Karte an
 */
static void info(void) {
	LOG_INFO("MAP:");
	LOG_INFO("%u\t Punkte pro Section (MAP_SECTIONS)", MAP_SECTIONS);
	LOG_INFO("%u\t Sections (MAP_SECTION_POINTS)", MAP_SECTION_POINTS);
	LOG_INFO("%u\t Punkte Kantenlaenge (MAP_SECTION_POINTS*MAP_SECTIONS)",
			MAP_SECTION_POINTS*MAP_SECTIONS);
	uint32_t points_in_map = (uint32_t)MAP_SECTION_POINTS
			*(uint32_t)MAP_SECTION_POINTS*(uint32_t)MAP_SECTIONS
			*(uint32_t)MAP_SECTIONS;
	LOG_INFO("%u%u\t Punkte gesamt", (uint16)(points_in_map/10000),
			(uint16_t) (points_in_map % 10000) );
	points_in_map /= 1024; // Umrechnen in KByte
	LOG_INFO("%u%u\t KByte", (uint16)(points_in_map/10000),
			(uint16_t) (points_in_map % 10000));
	LOG_INFO("%u\t Punkte pro Meter (MAP_RESOLUTION)", MAP_RESOLUTION);
	LOG_INFO("%u\t Meter Kantenlaenge (MAP_SIZE)", (uint16_t)MAP_SIZE);

	LOG_INFO("Die Karte verwendet Macroblocks");
	LOG_INFO("%u\t Laenge eine Macroblocks in Punkten (MACRO_BLOCK_LENGTH)",
			MACRO_BLOCK_LENGTH);
	LOG_INFO(
			"%u\t Anzahl der Macroblocks in einer Zeile(MAP_LENGTH_IN_MACRO_BLOCKS)",
			MAP_LENGTH_IN_MACRO_BLOCKS);
}
#endif	// MAP_INFO_AVAILABLE

#ifdef DISPLAY_MAP_AVAILABLE
/*!
 * Handler fuer Map-Display
 */
void map_display(void) {
	display_cursor(1, 1);
	display_printf("1: map_print");
	display_cursor(2, 1);
	display_printf("2: map_delete");
#ifdef PC
	display_cursor(3, 1);
	display_printf	("3: draw_scheme");
#endif
#ifdef MAP_INFO_AVAILABLE
	display_cursor(4,1);
	display_printf("4: map_info");
#endif

	/* Keyhandler */
	switch (RC5_Code) {
		case RC5_CODE_1:
		map_print(); RC5_Code = 0; break;
		case RC5_CODE_2:
		delete(); RC5_Code = 0; break;
#ifdef PC		
		case RC5_CODE_3:
		draw_test_scheme(); RC5_Code = 0; break;
#endif
#ifdef MAP_INFO_AVAILABLE
		case RC5_CODE_4:
		info(); RC5_Code = 0; break;
#endif	
	}
}
#endif	// DISPLAY_MAP_AVAILABLE



// Alter Code

#if 0	// inaktiv, nicht thread-safe
/*! 
 * Loescht die Mapfelder, die in einem bestimtmen Wertebereich min_val max_val liegen, d.h. diese
 * werden auf 0 gesetzt 
 * @param min_val minimaler Wert
 * @param max_val maximaler Wert
 */ 
void map_clear(int8_t min_val, int8_t max_val) {
	int16_t x,y;
	int8_t tmp;
	// Mapfelder durchlaufen
	for (y=map_min_y; y<= map_max_y; y++) {
		for (x=map_max_x; x>= map_min_x; x--) {
			tmp = get_field(x, y);				
			if (tmp >= min_val && tmp <= max_val) // alles zwischen Intervall auf 0 setzen
				set_field(x, y, 0); 
		}		   
	}
} 
#endif

#if 0	// wozu?
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
static uint8_t get_value_field_circle(uint16_t x, uint16_t y, uint8_t radius, int8_t value) {
	int16_t dX, dY;
	uint16_t h = radius*radius;
	for (dX = -radius; dX <= radius; dX++) {
		for (dY = -radius; dY <= radius; dY++) {
			if (dX*dX + dY*dY <= h) {	// Vergleich nur innerhalb des Umkreises
				if (get_field(x + dX, y + dY)==value) {	// Mapwert hat Vergleichswert ?
					return True;	// dann Schluss mit True
				}
			}
		}
	}
	return False;                                       // kein Feldwert ist identisch im Umkreis
}
	
/*!
 * ermittelt ob der Wert val innerhalb des Umkreises mit Radius r von xy liegt; bei geringer MCU-Aufloesung direkter
 * Vergleich mit xy
 * @param x	Map-Koordinate
 * @param y Map-Koordinate
 * @param radius Vergleichsradius (halber Suchkreis)
 * @param val Vergleichswert
 * @return True wenn Wert val gefunden
 */
static uint8 value_in_circle(uint16_t x, uint16_t y, uint8_t radius, int8_t val) {
	uint8_t r;
	for (r=1; r<=radius; r++) {	// Botradius
		if (get_value_field_circle(x,y,r,val)) {
			// Schluss sobald Wert erkannt wird
			return True;
		}
	}
	return False;	// Wert nicht vorhanden
}
#endif

#endif	// MAP_AVAILABLE
