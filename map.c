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
 * \file 	map.c
 * \brief 	Karte
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	19.09.2006
 */

#include "ct-Bot.h"

#ifdef MAP_AVAILABLE
#include "ui/available_screens.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sensor_correction.h"
#include "map.h"
#include "mmc.h"
#include "rc5-codes.h"
#include "sensor.h"
#include "display.h"
#include "log.h"
#include "timer.h"
#include "fifo.h"
#include "os_thread.h"
#include "math_utils.h"
#include "command.h"
#include "motor.h"
#include "init.h"

#if !defined MMC_AVAILABLE && defined MCU
#error "Map geht auf dem MCU nicht ohne MMC"
#endif

#ifndef SDFAT_AVAILABLE
#error "Map braucht SDFAT_AVAILABLE"
#endif

#ifndef OS_AVAILABLE
#error "Map braucht OS_AVAILABLE"
#endif


//#define DEBUG_MAP			// Schalter um recht viel Debug-Code anzumachen
//#define DEBUG_MAP_TIMES	// Schalter um Performance-Messungen fuer MMC anzumachen
//#define DEBUG_STORAGE		// Noch mehr Ausgaben zum Thema Organisation der Kartenstruktur, Macroblocks, Sections
//#define DEBUG_SCAN_OTF	// Debug-Infos des Update-Threads an
//#define DEBUG_GET_RATIO	// zeichnet Debug-Infos in die Map-Anzeige des Sim, gruen: Bereich komplett innerhalb des gewuenschten Intervalls, rot: Bereich nicht (komplett) innerhalb des gewuenschten Intervalls
//#define DEBUG_GET_RATIO_VERBOSE	// zeichnet detaillierte Infos in die Map-Anzeige, gruen: Felder innerhalb des gewuenschten Intervalls, rot: Felder ausserhalb des gewuenschten Intervalls
//#define DEBUG_MAP_GET_AVERAGE		// zeichnet Debug-Infos in die Map-Anzeige des Sim fuer map_get_average(), gruen: Durchschnitt des Feldes >= MAP_OBSTACLE_THRESHOLD, rot: sonst
//#define DEBUG_MAP_GET_AVERAGE_VERBOSE	// zeichnet belegte Map-Felder, die map_get_average() auswertet rot und freie gruen

//#define MAP_TESTS_AVAILABLE	// Schalter um Test-Code zu aktivieren
#define MAP_INFO_AVAILABLE		// Schalter um Info-Code zu aktivieren
#ifdef MCU
// Soll auch der echte Bot Infos ausgeben, kommentiert man die folgende Zeile aus
#undef MAP_INFO_AVAILABLE // spart Flash
#endif

#ifndef MAP_2_SIM_AVAILABLE
#undef DEBUG_GET_RATIO
#undef DEBUG_GET_RATIO_VERBOSE
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
 * Felder sind vom Typ int8_t und haben einen Wertebereich von -128
 * bis 127.
 * 0 bedeutet: wir wissen nichts ueber das Feld
 * negative Werte bedeuten: Hindernis
 * positive Werte bedeuten: freier Weg
 * Je groesser der Betrag ist, desto sicherer die Aussage ueber das
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

/** Anzahl der Sections in der Map */
#define MAP_SECTIONS	(MAP_SIZE_MM * MAP_RESOLUTION / 1000 / MAP_SECTION_POINTS)

#define MAP_STEP_FREE_SENSOR		2	/**< Um diesen Wert wird ein Feld inkrementiert, wenn es vom Sensor als frei erkannt wird */
#define MAP_STEP_FREE_LOCATION		20	/**< Um diesen Wert wird ein Feld inkrementiert, wenn der Bot drueber faehrt */

#define MAP_STEP_OCCUPIED			5	/**< Um diesen Wert wird ein Feld dekrementiert, wenn es als belegt erkannt wird */

#define MAP_RADIUS					50	/**< Umkreis eines Messpunktes, der als besetzt aktualisiert wird (Streukreis) [mm] */
/** Umkreis einen Messpunkt, der als besetzt aktualisiert wird (Streukreis) [Felder] */
#define MAP_RADIUS_FIELDS			(MAP_RESOLUTION * MAP_RADIUS / 1000)

#define MAP_PRINT_SCALE					/**< Soll das PGM eine Skala erhalten? */
#define MAP_SCALE	(MAP_RESOLUTION / 2)	/**< Alle wieviel Punkte kommt ein Skalen-Strich */

#define MACRO_BLOCK_LENGTH	512U			/**< Kantenlaenge eines Makroblocks in Punkten/Byte */
#define MAP_LENGTH_IN_MACRO_BLOCKS ((uint8_t) (MAP_SIZE_MM * MAP_RESOLUTION / 1000 / MACRO_BLOCK_LENGTH)) /**< Kantenlaenge der Karte in Makrobloecken */
#define MAP_FILE_SIZE	((uint16_t) ((uint32_t) ((uint32_t) MAP_SECTION_POINTS * MAP_SECTION_POINTS) * MAP_SECTIONS * MAP_SECTIONS \
						/ MAP_BLOCK_SIZE)) /**< Dateigroesse der Map in Bloecken */
#define MAP_ALIGNMENT_MASK	(2UL * MACRO_BLOCK_LENGTH * MACRO_BLOCK_LENGTH / MAP_BLOCK_SIZE - 1) /**< fuer die Ausrichtung der Karte an einer Sektorgrenze zu Optimierungszwecken */

#define MAP_FILENAME			"ctbot.map" /**< Dateiname der Karte */
#define MAP_FILE_ALIGNMENT	(512UL * 1024UL / MAP_BLOCK_SIZE) /**< Alingment der Map-Datei (512 KB) */

int16_t map_min_x = MAP_SIZE * MAP_RESOLUTION / 2; /**< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
int16_t map_max_x = MAP_SIZE * MAP_RESOLUTION / 2; /**< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
int16_t map_min_y = MAP_SIZE * MAP_RESOLUTION / 2; /**< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
int16_t map_max_y = MAP_SIZE * MAP_RESOLUTION / 2; /**< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate */
static uint8_t min_max_updated = False; /**< wurden die Min- / Max-Werte veraendert? */
uint16_t alignment_offset = 0;

/** Datentyp fuer die Elementarfelder einer Gruppe */
typedef struct {
	int8_t section[MAP_SECTION_POINTS][MAP_SECTION_POINTS]; /**< Einzelne Punkte */
} PACKED_FORCE map_section_t;

static pFatFile map_file_desc; /**< Datei-Deskriptor der Map */

static uint8_t map_update_fifo_buffer[MAP_UPDATE_CACHE_SIZE];	/**< Puffer fuer Map-Cache-Indizes / FiFo */
fifo_t map_update_fifo;										/**< Fifo fuer Map-Cache */
map_cache_t map_update_cache[MAP_UPDATE_CACHE_SIZE];			/**< Map-Cache */

uint8_t map_update_stack[MAP_UPDATE_STACK_SIZE];	/**< Stack des Update-Threads */
static Tcb_t* map_update_thread;					/**< Thread fuer Map-Update */
static os_signal_t lock_signal = OS_SIGNAL_INITIALIZER; /**< Signal zur Synchronisation von Kartenzugriffen */

void map_update_main(void) OS_TASK_ATTR;

#define map_buffer GET_MMC_BUFFER(map_buffer)	/**< Map-Puffer */
static map_section_t* map[2];					/**< Array mit den Zeigern auf die Elemente, es passen immer 2 Sektionen in den Puffer */

static struct {
	uint16_t block;		/**< Block, der aktuell im Puffer steht. Nur bis 32 MByte adressierbar */
	uint8_t updated;		/**< markiert, ob der aktuelle Block gegenueber der MMC-Karte veraendert wurde */
	int16_t x;			/**< X-Koordinate des Blocks */
	int16_t y;			/**< Y-Koordinate des Blocks */
} map_current_block = { 0, False, 0, 0 }; /**< Daten des aktuellen Blocks */

static uint8_t init_state = 0; /**< Status der Initialisierung (0 (nicht initialisiert), 1 (alles OK)) */

#ifdef MAP_2_SIM_AVAILABLE
void map_2_sim_main(void) OS_TASK_ATTR;

static fifo_t map_2_sim_fifo; /**< Fifo fuer Map-2-Sim-Daten */
static uint16_t map_2_sim_cache[MAP_2_SIM_BUFFER_SIZE]; /**< Speicher fuer Map-2-Sim-Daten (Adressen der geaenderten Bloecke) */
static struct {
	position_t pos; /**< letzte Bot-Position */
	int16_t heading; /**< letzte Bot-Ausrichtung */
#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
	int16_t error; /**< letzter Fehlerradius */
#endif
} map_2_sim_data = {
	{MAP_SIZE * MAP_RESOLUTION / 2, MAP_SIZE * MAP_RESOLUTION / 2}, 0
#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
	, 0
#endif
};
static Tcb_t* map_2_sim_worker; /**< Worker-Thread fuer die Map-2-Sim-Anzeige */
uint8_t map_2_sim_worker_stack[MAP_2_SIM_STACK_SIZE]; /**< Stack des Map-2-Sim-Threads */
static pFatFile map_2_sim_file_desc; /**< File-Deskriptor fuer Map-2-Sim */
static uint8_t map_2_sim_buffer[512]; /**< Puffer fuer zur Map-2-Sim-Kommunikation */
static os_signal_t map_2_sim_signal = OS_SIGNAL_INITIALIZER; /**< Signal, um gleichzeitiges Senden von Map-Daten zu verhindern */
#endif // MAP_2_SIM_AVAILABLE

#ifdef PC
char* map_file = "sim.map"; /**< Dateiname fuer Ex- / Import */
#endif // PC


static uint16_t get_block(int16_t x, int16_t y) {
	/* Sicherheitscheck */
	if (((uint16_t) x >= (uint16_t) (MAP_SIZE * MAP_RESOLUTION)) || ((uint16_t) y >= (uint16_t) (MAP_SIZE * MAP_RESOLUTION))) {
		LOG_ERROR("Zugriff ausserhalb der Karte: x=%u y=%u", x, y);
		return 0xffff;
	}

	/* Berechne den gesuchten Block */
	uint16_t local_x = (uint16_t) x % MACRO_BLOCK_LENGTH;
	// wenn MACRO_BLOCK_LENGTH eine 2er Potenz ist, optimiert der Compiler hier
	uint16_t local_y = (uint16_t) y % MACRO_BLOCK_LENGTH;

#ifdef DEBUG_STORAGE
	LOG_DEBUG("local_x=%d, local_y=%d", local_x, local_y);
#endif

	uint16_t block = local_x / MAP_SECTION_POINTS + (local_y / MAP_SECTION_POINTS) * (MACRO_BLOCK_LENGTH / MAP_SECTION_POINTS);

	block = block >> 1; // es passen immer 2 Sections in einen Block

	/* Makroblock berechnen */
	uint16_t macroblock = (uint16_t) x / MACRO_BLOCK_LENGTH + ((uint16_t) y / MACRO_BLOCK_LENGTH) * MAP_LENGTH_IN_MACRO_BLOCKS;

#ifdef DEBUG_STORAGE
	LOG_DEBUG("Macroblock=%u", macroblock);
#endif

	block += (uint16_t) (macroblock * MACRO_BLOCK_LENGTH * (MACRO_BLOCK_LENGTH / MAP_BLOCK_SIZE)); // noch in den richtigen Makroblock springen

#ifdef DEBUG_STORAGE
	LOG_DEBUG("block=%u", block);
#endif

	return block;
}

/**
 * Initialisiert die Karte
 * \param clean_map True: Karte wird geloescht, False: Karte bleibt erhalten
 * \return 0 wenn alles ok
 */
static int8_t init(uint8_t clean_map) {
	if (init_state == 1) {
		return 0;
	}

#ifdef BEHAVIOUR_SCAN_AVAILABLE
	deactivateBehaviour(bot_scan_onthefly_behaviour);
#endif

	display_cursor(1, 1);
	display_printf("Bitte warten...");
	display_cursor(2, 1);
	display_printf("MAP wird");
	display_cursor(3, 1);
	display_printf("initialisiert.");

	// Die Karte auf den Puffer biegen
	map[0] = (map_section_t*) map_buffer;
	map[1] = (map_section_t*) (map_buffer + sizeof(map_section_t));

	map_current_block.updated = 0xff; // Die MMC-Karte ist erstmal nicht verfuegbar

	LOG_DEBUG("map::init(): sdfat_open(\"%s\")...", MAP_FILENAME);

	const uint8_t mode = SDFAT_O_RDWR | SDFAT_O_CREAT;
	uint8_t res = sdfat_open(MAP_FILENAME, &map_file_desc, mode);
	LOG_DEBUG("map::init(): sdfat_open()=%d", res);
	if (res != 0) {
		LOG_DEBUG("map::init(): Dateioeffnen lieferte %d", res);
		LOG_ERROR("map::init(): Fehler beim Oeffnen / Anlegen der Datei");
		return 2;
	}

	const uint32_t data_offset = sdfat_get_first_block(map_file_desc) + (sizeof(map_header_t) / MAP_BLOCK_SIZE); // Blockadresse in Bloecken
	const uint32_t alignment_mask = (512UL * 1024UL) / MAP_BLOCK_SIZE - 1; // in Bloecken
	alignment_offset = (uint16_t) (((data_offset + alignment_mask) & ~alignment_mask) - data_offset); // in Bloecken
	const uint32_t map_offset = (data_offset + alignment_offset) * MAP_BLOCK_SIZE; // in Byte
	(void) map_offset;
	LOG_INFO("map::init(): data_offset=0x%x%04x blocks", (uint16_t) (data_offset >> 16), (uint16_t) data_offset);
	LOG_INFO("map::init(): alignment_mask=0x%x%04x blocks", (uint16_t) (alignment_mask >> 16), (uint16_t) alignment_mask);
	LOG_INFO("map::init(): alignment_offset=0x%04x blocks", alignment_offset);
	LOG_INFO("map::init(): map_offset=0x%x%04x byte", (uint16_t) (map_offset >> 16), (uint16_t) map_offset);

	map_header_t* p_head_data = (map_header_t*) map_buffer;
	/* Min- / Max-Werte initialisieren */
	p_head_data->map_min_x = 0;
	p_head_data->map_max_x = MAP_SIZE * MAP_RESOLUTION - 1;
	p_head_data->map_min_y = 0;
	p_head_data->map_max_y = MAP_SIZE * MAP_RESOLUTION - 1;
	p_head_data->alignment_offset = alignment_offset;

	if (sdfat_get_filesize(map_file_desc) < ((uint32_t) (MAP_FILE_SIZE + alignment_offset) * MAP_BLOCK_SIZE + sizeof(map_header_t))) {
		LOG_DEBUG("map::init(): Datei zu klein, neu initialisieren");
		clean_map = True;
	} else {
		LOG_DEBUG("map::init(): Dateigroesse passt, lese Header ein");
		sdfat_rewind(map_file_desc);
		if (sdfat_read(map_file_desc, p_head_data, sizeof(map_header_t)) != sizeof(map_header_t)) {
			LOG_DEBUG("map::init(): sdfat_read(head) failed");
			return 5;
		}
	}

	if (clean_map) {
		LOG_DEBUG("map::init(): Map wird geloescht/neu initialisiert");

		map_min_x = p_head_data->map_min_x;
		map_max_x = p_head_data->map_max_x;
		map_min_y = p_head_data->map_min_y;
		map_max_y = p_head_data->map_max_y;

		LOG_DEBUG("map::init(): map_min_x=%d map_min_y=%d", map_min_x, map_min_y);
		LOG_DEBUG("map::init(): map_max_x=%d map_max_y=%d", map_max_x, map_max_y);
		LOG_DEBUG("map::init(): alignment_offset=0x%04x blocks", p_head_data->alignment_offset);

		/* Min/Max Werte auf default setzen */
		p_head_data->map_min_x = MAP_SIZE * MAP_RESOLUTION / 2;
		p_head_data->map_max_x = MAP_SIZE * MAP_RESOLUTION / 2;
		p_head_data->map_min_y = MAP_SIZE * MAP_RESOLUTION / 2;
		p_head_data->map_max_y = MAP_SIZE * MAP_RESOLUTION / 2;
		p_head_data->alignment_offset = alignment_offset;

		sdfat_rewind(map_file_desc);
		if (sdfat_write(map_file_desc, map_buffer, sizeof(map_header_t)) != sizeof(map_header_t)) {
			LOG_DEBUG("map::init(): sdfat_write(head) failed");
			return 6;
		}

		memset(map_buffer, 0, MAP_BLOCK_SIZE);
		const uint16_t min_block = sdfat_get_filesize(map_file_desc) / MAP_BLOCK_SIZE < alignment_offset ? 0 : get_block(map_min_x, map_min_y) + alignment_offset;
		const uint16_t max_block = get_block(map_max_x, map_max_y) + alignment_offset;
		LOG_DEBUG("map::init(): min_block=%u max_block=%u", min_block, max_block);

		if (sdfat_seek(map_file_desc, (int32_t) (min_block * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
			LOG_DEBUG("map::init(): sdfat_seek(0x%ld) failed", min_block * MAP_BLOCK_SIZE);
			return 7;
		}

		uint16_t i;
		for (i = min_block; i <= max_block; ++i) {
			if (sdfat_write(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
				LOG_DEBUG("map::init(): sdfat_write(0x%x) failed", i);
				return 8;
			}
		}

		p_head_data->map_min_x = MAP_SIZE * MAP_RESOLUTION / 2;
		p_head_data->map_max_x = MAP_SIZE * MAP_RESOLUTION / 2;
		p_head_data->map_min_y = MAP_SIZE * MAP_RESOLUTION / 2;
		p_head_data->map_max_y = MAP_SIZE * MAP_RESOLUTION / 2;
		p_head_data->alignment_offset = alignment_offset;
	}

	/* Min- / Max-Werte laden */
	map_min_x = p_head_data->map_min_x;
	map_max_x = p_head_data->map_max_x;
	map_min_y = p_head_data->map_min_y;
	map_max_y = p_head_data->map_max_y;
#ifdef DEBUG_STORAGE
	LOG_DEBUG("map::init(): map_min_x=0x%x, map_max_x=0x%x, map_min_y=0x%x, map_max_y=0x%x\n", map_min_x, map_max_x, map_min_y, map_max_y);
#endif

#ifdef MAP_2_SIM_AVAILABLE
	if (sdfat_open(MAP_FILENAME, &map_2_sim_file_desc, SDFAT_O_READ)) {
		LOG_DEBUG("map::init(): Mapdatei konnte nicht fuer Map-2-Sim geoeffnet werden");
		return 9;
	}
#endif // MAP_2_SIM_AVAILABLE

	map_current_block.updated = False;
	map_current_block.block = 0;

	if (clean_map) {
		memset(map_buffer, 0, sizeof(map_buffer));
	} else {
		/* Block 0 laden */
		if (sdfat_seek(map_file_desc, (int32_t) ((map_current_block.block + alignment_offset) * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
			LOG_DEBUG("map::init(): sdfat_seek(0x%x) failed", map_current_block.block + alignment_offset);
			return 10;
		}
		if (sdfat_read(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
			LOG_DEBUG("map::init(): sdfat_read(0x%x) failed", map_current_block.block + alignment_offset);
			return 11;
		}
	}

	/* Thread-Setup */
	if (init_state == 0) {
		/* Update-Thread initialisieren */
#ifdef OS_DEBUG
		os_mask_stack(map_update_stack, MAP_UPDATE_STACK_SIZE);
#ifdef MAP_2_SIM_AVAILABLE
		os_mask_stack(map_2_sim_worker_stack, MAP_2_SIM_STACK_SIZE);
#endif
#endif // OS_DEBUG
		fifo_init(&map_update_fifo, map_update_fifo_buffer, (uint8_t) sizeof(map_update_fifo_buffer));
#ifdef MAP_2_SIM_AVAILABLE
		fifo_init(&map_2_sim_fifo, map_2_sim_cache, sizeof(map_2_sim_cache));
#endif

		map_update_thread = os_create_thread(&map_update_stack[MAP_UPDATE_STACK_SIZE - 1], map_update_main);

#ifdef MAP_2_SIM_AVAILABLE
		map_2_sim_worker = os_create_thread(&map_2_sim_worker_stack[MAP_2_SIM_STACK_SIZE - 1], map_2_sim_main);
#endif // MAP_2_SIM_AVAILABLE
	}

	init_state = 1;
#ifdef BEHAVIOUR_SCAN_AVAILABLE
	activateBehaviour(NULL, bot_scan_onthefly_behaviour);
#endif
	return 0;
}

/**
 * Initialisiert die Karte
 * \return	0 wenn alles ok ist
 */
int8_t map_init(void) {
#ifdef MAP_CLEAR_ON_INIT
	return init(True);
#else
	return init(False);
#endif
}

/**
 * Haelt den Bot an und schreibt den Map-Update-Cache komplett zurueck
 */
void map_flush_cache(void) {
#ifdef MCU
	if (map_locked() == 1) {
		motor_set(BOT_SPEED_STOP, BOT_SPEED_STOP);
	}
#endif // MCU
	/* Warten, bis Update fertig */
	os_signal_set(&lock_signal);
	/* Sperre sofort wieder freigeben */
	os_signal_release(&lock_signal);

	sdfat_rewind(map_file_desc);
	if (sdfat_read(map_file_desc, map_buffer, sizeof(map_header_t)) != sizeof(map_header_t)) {
		LOG_ERROR("map_flush_cache(): sdfat_read(head) failed");
		return;
	}

	map_header_t* p_head_data = (map_header_t*) map_buffer;
	p_head_data->map_min_x = map_min_x;
	p_head_data->map_max_x = map_max_x;
	p_head_data->map_min_y = map_min_y;
	p_head_data->map_max_y = map_max_y;

	sdfat_rewind(map_file_desc);
	if (sdfat_write(map_file_desc, p_head_data, sizeof(map_header_t)) != sizeof(map_header_t)) {
		LOG_ERROR("map_flush_cache(): sdfat_write(head) failed");
	}

	if (sdfat_seek(map_file_desc, (int32_t) ((map_current_block.block + alignment_offset) * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
		LOG_ERROR("map_flush_cache(): sdfat_seek(0x%x) failed", map_current_block.block + alignment_offset);
		return;
	}
	if (sdfat_read(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
		LOG_ERROR("map_flush_cache(): sdfat_read(0x%x) failed", map_current_block.block + alignment_offset);
	}

	sdfat_flush(map_file_desc);
}

/**
 * Konvertiert eine Weltkoordinate in eine Kartenkoordinate
 * \param koord	Weltkoordiante
 * \return		Kartenkoordinate
 */
int16_t world_to_map(int16_t koord) {
#if (1000 / MAP_RESOLUTION) * MAP_RESOLUTION != 1000
#error "MAP_RESOLUTION ist kein Teiler von 1000, Code in world_to_map() anpassen!"
#endif
#if defined MCU && MAP_RESOLUTION == 125
	__asm__ __volatile__(
		"lsr %B0				\n\t"
		"ror %A0				\n\t"
		"lsr %B0				\n\t"
		"ror %A0				\n\t"
		"lsr %B0				\n\t"
		"ror %A0				\n\t"
		"sbrc %B0,4				\n\t"
		"ori %B0,224			\n\t"
		"subi %A0,lo8(-(768))	\n\t"
		"sbci %B0,hi8(-(768))		"
		:	"+d" (koord)
	);
	return koord;
#else
	int32_t tmp = koord + (int16_t) (MAP_SIZE * MAP_RESOLUTION * 4);
	return (int16_t) (tmp / (1000 / MAP_RESOLUTION));
#endif
}

/**
 * Liefert einen Zeiger auf die Section zurueck, in der der Punkt liegt.
 * Auf dem MCU kuemmert sie sich darum, die entsprechende Karte aus der MMC-Karte zu laden
 * \param x	X-Ordinate der Karte (nicht der Welt!!!)
 * \param y	Y-Ordinate der Karte (nicht der Welt!!!)
 * \return	Zeiger auf die Section
 */
static map_section_t* get_section(int16_t x, int16_t y) {
	const uint16_t block = get_block(x, y);
	if (block == 0xffff) {
		return NULL;
	}

	// Da immer 2 Sections in einem Block stehen: richtige der beiden Sections raussuchen
	const uint8_t index = (uint8_t) ((x / MAP_SECTION_POINTS) & 0x1);

	/* Ist der Block schon geladen? */
	if (map_current_block.block == block) {
#ifdef DEBUG_STORAGE
		LOG_DEBUG("ist noch im Puffer");
#endif
		return map[index];
	}

	/* Block ist also nicht im Puffer */
#ifdef DEBUG_STORAGE
	LOG_DEBUG("ist nicht im Puffer");
#endif

	/* Wurde der Block im RAM veraendert? */
	if (map_current_block.updated == True) {
		/* Shrinking */
		if (map_current_block.x < map_min_x) {
			map_min_x = map_current_block.x;
			min_max_updated = True;
		} else if (map_current_block.x > map_max_x) {
			map_max_x = map_current_block.x + ((MAP_SECTION_POINTS * 2) - 1);
			min_max_updated = True;
		}
		if (map_current_block.y < map_min_y) {
			map_min_y = map_current_block.y;
			min_max_updated = True;
		} else if (map_current_block.y > map_max_y) {
			map_max_y = map_current_block.y + (MAP_SECTION_POINTS - 1);
			min_max_updated = True;
		}

		/* Dann erstmal sichern */
#ifdef DEBUG_MAP_TIMES
		LOG_INFO("writing block 0%x", map_current_block.block + alignment_offset);
		uint16_t start_ticks = TIMER_GET_TICKCOUNT_16;
#endif
		if (sdfat_seek(map_file_desc, (int32_t) ((map_current_block.block + alignment_offset) * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
			LOG_DEBUG("map::get_section(): sdfat_seek(0x%x) failed", map_current_block.block + alignment_offset);
			map_current_block.updated = False;
			return NULL;
		}
		if (sdfat_write(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
			LOG_DEBUG("map::get_section(): sdfat_write(0x%x) failed", map_current_block.block + alignment_offset);
			map_current_block.updated = False;
			return NULL;
		}

#ifdef MAP_2_SIM_AVAILABLE
		map_2_sim_data.pos.x = world_to_map(x_pos);
		map_2_sim_data.pos.y = world_to_map(y_pos);
		map_2_sim_data.heading = heading_int;
#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
		map_2_sim_data.error = pos_error_radius / (1000 / MAP_RESOLUTION) + (BOT_DIAMETER / 2 / (1000 / MAP_RESOLUTION));
#endif
		fifo_put_data(&map_2_sim_fifo, &map_current_block.block, sizeof(map_current_block.block), True);
#endif // MAP_2_SIM_AVAILABLE

#ifdef DEBUG_MAP_TIMES
		uint16_t end_ticks = TIMER_GET_TICKCOUNT_16;
		LOG_INFO("swapout took %u ms", (end_ticks - start_ticks) * 176 / 1000);
		(void) start_ticks;
		(void) end_ticks;
#endif
	}

	/* Statusvariablen anpassen */
	map_current_block.block = block;
	map_current_block.x = x & ~((MAP_SECTION_POINTS * 2) - 1); // 32 Einheiten in X-Richtung und
	map_current_block.y = y & ~(MAP_SECTION_POINTS - 1); // 16 Einheiten in Y-Richtung pro Block
	map_current_block.updated = False;

	/* Lade den neuen Block */
#ifdef DEBUG_MAP_TIMES
	LOG_INFO("reading block 0x%x", block + alignment_offset);
	uint16_t start_ticks = TIMER_GET_TICKCOUNT_16;
#endif
	if (sdfat_seek(map_file_desc, (int32_t) ((block + alignment_offset) * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
		LOG_DEBUG("map::get_section(): sdfat_seek(0x%x) failed", block + alignment_offset);
		return NULL;
	}
	if (sdfat_read(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
		LOG_DEBUG("map::get_section(): sdfat_read(0x%x) failed", block + alignment_offset);
		return NULL;
	}
#ifdef DEBUG_MAP_TIMES
	uint16_t end_ticks = TIMER_GET_TICKCOUNT_16;
	LOG_INFO("swapin took %u ms", (end_ticks - start_ticks) * 176 / 1000);
	(void) start_ticks;
	(void) end_ticks;
#endif

	return map[index];
}

/**
 * Prueft, ob die Karte zurzeit gesperrt ist.
 * \return	1, falls Karte gesperrt, 0 sonst
 */
uint8_t map_locked(void) {
	return lock_signal.value;
}

/**
 * Zugriff auf ein Feld der Karte. Kann lesend oder schreibend sein.
 * \param x		X-Ordinate der Karte
 * \param y		Y-Ordinate der Karte
 * \param value	Neuer Wert des Feldes (> 0 heisst frei, <0 heisst belegt)
 * \param set	0 zum Lesen, 1 zum Schreiben
 * \return		Wert, der jetzt an (x|y) steht
 */
static int8_t access_field(int16_t x, int16_t y, int8_t value, uint8_t set) {
	// Suche die Section heraus
	map_section_t* p_section = get_section(x, y);
	if (! p_section) {
		LOG_DEBUG("map::access_field(%d,%d,%d,%u): get_section failed", x, y, value, set);
		return 0;
	}

	// Berechne den Index innerhalb der Section
	const uint16_t index_x = (uint16_t) x % MAP_SECTION_POINTS;
	const uint16_t index_y = (uint16_t) y % MAP_SECTION_POINTS;

	int8_t* data = &p_section->section[index_x][index_y];

	if (set) {
		*data = value;
		map_current_block.updated = True;
	}
	return *data;
}

/**
 * liefert den Durschnittswert um einen Punkt der Karte herum
 * \param x 		x-Ordinate der Karte
 * \param y 		y-Ordinate der Karte
 * \param radius	gewuenschter Radius
 * \return 			Wert des Durchschnitts um das Feld (>0 heisst frei, <0 heisst belegt)
 */
static int8_t get_average_fields(int16_t x, int16_t y, int8_t radius) {
	int32_t avg = 0;
	int8_t dX, dY;
	int16_t count = 0;
	const int16_t h = muls8(radius, radius);

	/* Daten auslesen */
	for (dX = (int8_t) -radius; dX <= radius; dX++) {
		for (dY = (int8_t) -radius; dY <= radius; dY++) {
			if (muls8(dX, dX) + muls8(dY, dY) <= h) {
				int8_t tmp = access_field(x + dX, y + dY, 0, 0);
				avg += tmp;
				count++;
#ifdef DEBUG_MAP_GET_AVERAGE_VERBOSE
				uint8_t color = tmp < MAP_OBSTACLE_THRESHOLD ? 1 : 0;
				position_t pos;
				pos.x = x + dX;
				pos.y = y + dY;
				map_draw_line(pos, pos, color);
#endif // DEBUG_MAP_GET_AVERAGE
			}
		}
	}

	int8_t result = (int8_t)(count > 0 ? avg / count : 0);
#if defined DEBUG_MAP_GET_AVERAGE && !defined DEBUG_MAP_GET_AVERAGE_VERBOSE
	uint8_t color = result < MAP_OBSTACLE_THRESHOLD ? 1 : 0;
	for (dX =- radius; dX <= radius; dX++) {
		for (dY =- radius; dY <= radius; dY++) {
			if (muls8(dX, dX) + muls8(dY, dY) <= h) {
				position_t pos;
				pos.x = x + dX;
				pos.y = y + dY;
				map_draw_line(pos, pos, color);
			}
		}
	}
#endif // DEBUG_MAP_GET_AVERAGE
	return result;
}

/**
 * liefert den Durschnittswert um einen Ort herum
 * \param x			x-Ordinate der Welt
 * \param y			y-Ordinate der Welt
 * \param radius	Radius der Umgebung, die beruecksichtigt wird [mm]; 0 fuer ein Map-Feld (Punkt)
 * \return 			Durchschnitsswert im Umkreis um den Ort (>0 heisst frei, <0 heisst belegt)
 */
int8_t map_get_average(int16_t x, int16_t y, int16_t radius) {
	// Ort in Kartenkoordinaten
	int16_t X = world_to_map(x);
	int16_t Y = world_to_map(y);
	int8_t R = (int8_t) (radius / (1000 / MAP_RESOLUTION));

	/* warten bis Karte frei ist */
	map_flush_cache();

	int8_t result = get_average_fields(X, Y, R);

	return result;
}

/**
 * Aendert den Wert eines Feldes um den angegebenen Betrag
 * \param x		x-Ordinate der Karte (nicht der Welt!!!)
 * \param y		y-Ordinate der Karte (nicht der Welt!!!)
 * \param value	Betrag um den das Feld veraendert wird (>0 heisst freier, <0 heisst belegter)
 */
static void update_field(int16_t x, int16_t y, int8_t value) {
	int8_t tmp = access_field(x, y, 0, 0);
	if (tmp == -128) {
		// Nicht aktualiseren, wenn es sich um ein Loch handelt
		return;
	}

	int8_t new_value = (int8_t) (tmp + value);
	/* Saturation */
	if (value > 0) {
		if (new_value < tmp) {
			new_value = 127;
		}
	} else {
		if (new_value > tmp || new_value == -128) {
			new_value = -127;
		}
	}

	access_field(x, y, new_value, 1);
}

/**
 * Aendert den Wert eines Kreises um den angegebenen Betrag
 * \param x			x-Ordinate der Karte (nicht der Welt!!!)
 * \param y			y-Ordinate der Karte (nicht der Welt!!!)
 * \param radius	Radius in Kartenpunkten
 * \param value		Betrag um den das Feld veraendert wird (>0 heisst freier, <0 heisst belegter)
 */
static void update_field_circle(int16_t x, int16_t y, int8_t radius, int8_t value) {
	const int16_t square = muls8(radius, radius);

	int16_t sec_x_max = (x + radius) / MAP_SECTION_POINTS;
	int16_t sec_x_min = (x - radius) / MAP_SECTION_POINTS;
	int16_t sec_y_max = (y + radius) / MAP_SECTION_POINTS;
	int16_t sec_y_min = (y - radius) / MAP_SECTION_POINTS;
//	LOG_DEBUG("Betroffene Sektionen X: %d-%d, Y:%d-%d", sec_x_min, sec_x_max, sec_y_min, sec_y_max);

	int16_t sec_x, sec_y, X, Y, dX, dY;
	int16_t starty, startx, stopx, stopy;
	// Gehe ueber alle betroffenen Sektionen
	for (sec_y = sec_y_min; sec_y <= sec_y_max; sec_y++) {
		// Bereich weiter eingrenzen
		if (sec_y * MAP_SECTION_POINTS > (y - radius)) {
			starty = sec_y * MAP_SECTION_POINTS;
		} else {
			starty = y - radius;
		}
		if ((sec_y + 1) * MAP_SECTION_POINTS < (y + radius)) {
			stopy = (sec_y + 1) * MAP_SECTION_POINTS;
		} else {
			stopy = y + radius;
		}

		for (sec_x = sec_x_min; sec_x <= sec_x_max; sec_x++) {
			if (sec_x * MAP_SECTION_POINTS > (x - radius)) {
				startx = sec_x * MAP_SECTION_POINTS;
			} else {
				startx = x - radius;
			}
			if ((sec_x + 1) * MAP_SECTION_POINTS < (x + radius)) {
				stopx = (sec_x + 1) * MAP_SECTION_POINTS;
			} else {
				stopx = x + radius;
			}

			for (Y = starty; Y < stopy; Y++) {
				int8_t tmp = (int8_t) (y - Y); // Distanz berechnen
				dY = muls8(tmp, tmp); // Quadrat vorberechnen
				for (X = startx; X < stopx; X++) {
					tmp = (int8_t) (x - X); // Distanz berechnen
					dX = muls8(tmp, tmp); // Quadrat vorberechnen
					if (dX + dY <= square) { // wenn Punkt unter Bot
						update_field(X, Y, value); // dann aktualisieren
					}
				}
			}
		}
	}
}

/**
 * Markiert ein Feld als belegt -- drueckt den Feldwert etwas mehr in Richtung "belegt"
 * \param x	x-Ordinate der Karte (nicht der Welt!!!)
 * \param y	y-Ordinate der Karte (nicht der Welt!!!)
 * \param location_prob Gibt an, wie sicher wir ueber die Position sind [0; 255]
 */
static void update_occupied(int16_t x, int16_t y, uint8_t location_prob) {
#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
	const uint8_t prob = location_prob;
#else
	(void) location_prob;
	const uint8_t prob = 255;
#endif
	const int8_t r = MAP_RADIUS_FIELDS;
	const int8_t r_2 = r * r;
	int16_t X, Y;
	int8_t dX = r;
	for (X = x - r; X <= x + r; ++X, --dX) {
		const int8_t dX_2 = (int8_t) (dX * dX);
		int8_t dY = r;
		for (Y = y - r; Y <= y + r; ++Y, --dY) {
			int8_t h = (int8_t) (dY * dY + dX_2);
			if (h <= r_2) {
				h /= 2;
				if (h < MAP_STEP_OCCUPIED) {
					h = MAP_STEP_OCCUPIED;
				}
				update_field(X, Y, (int8_t) ((((- MAP_STEP_OCCUPIED * MAP_STEP_OCCUPIED) / h) + 1) * prob / 255 - 1));
			}
		}
	}
}

/**
 * Map-Umfeldaktualisierung mit einem bestimmten Wert ab einer Position xy mit Radius r bei
 * \param x			Map-Koordinate
 * \param y			Map-Koordinate
 * \param radius	Radius des Umfeldes
 * \param value 	Mapwert; nur eingetragen wenn aktueller Mapwert groesser value ist
 */
static void set_value_field_circle(int16_t x, int16_t y, int8_t radius, int8_t value) {
	int8_t dX, dY;
	int16_t h = muls8(radius, radius);
	for (dX = (int8_t) -radius; dX <= radius; dX++) {
		int16_t dX2 = muls8(dX, dX);
		for (dY = (int8_t) -radius; dY <= radius; dY++) {
			if (dX2 + muls8(dY, dY) <= h) {
				// nur innerhalb des Umkreises
				const int16_t x_tmp = x + dX;
				const int16_t y_tmp = y + dY;
				if (access_field(x_tmp, y_tmp, 0, 0) > value) {
					// Mapwert hoeher Richtung frei
					access_field (x_tmp, y_tmp, value, 1); // dann Wert eintragen
				}
			}
		}
	}
}

/**
 * setzt ein Map-Feld auf einen Wert mit Umfeldaktualisierung;
 * Hindernis wird mit MAP_RADIUS_FIELDS eingetragen
 * \param x		Map-Koordinate
 * \param y		Map-Koordinate
 * \param val	im Umkreis einzutragender Wert
 */
static void set_value_occupied(int16_t x, int16_t y, int8_t val) {
	int8_t r;
	// in Map mit dem Radius um x/y eintragen
	for (r = 1; r <= MAP_RADIUS_FIELDS; r++) {
		set_value_field_circle(x, y, r, val);
	}
}

/**
 * Aktualisiert die Karte mit den Daten eines Distanz-Sensors
 * \param x		X-Achse der Position des Sensors
 * \param y 	Y-Achse der Position des Sensors
 * \param h_sin sin(Blickrichtung)
 * \param h_cos	cos(Blickrichtung)
 * \param dist 	Sensorwert
 * \param location_prob Gibt an, wie sicher wir ueber die Position sind [0; 255]
 */
static void update_sensor_distance(int16_t x, int16_t y, float h_sin, float h_cos, int16_t dist, uint8_t location_prob) {
	// Ort des Sensors in Kartenkoordinaten
	int16_t X = world_to_map(x);
	int16_t Y = world_to_map(y);

	int16_t d;
	if (dist == SENS_IR_INFINITE) {
		d = SENS_IR_MAX_DIST;
	} else {
		d = dist;
	}

	// liefert die Mapkoordinaten des Hindernisses / Ende des Frei-Strahls
	int16_t PH_X = world_to_map(x + (int16_t) (d * h_cos));
	int16_t PH_Y = world_to_map(y + (int16_t) (d * h_sin));

	// Nun markiere alle Felder vor dem Hindernis als frei
	int8_t i;

	int16_t lX = X; // Beginne mit dem Feld, in dem der Sensor steht
	int16_t lY = Y;

	const int8_t sX = (int8_t) (PH_X < X ? -1 : 1);
	int8_t dX = (int8_t) abs(PH_X - X); // Laenge der Linie in X-Richtung

	const int8_t sY =  (int8_t) (PH_Y < Y ? -1 : 1);
	int8_t dY = (int8_t) abs(PH_Y - Y); // Laenge der Linie in Y-Richtung

#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
	const int8_t step_value = (int8_t) ((MAP_STEP_FREE_SENSOR - 1) * location_prob / 255 + 1);
#else
	const int8_t step_value = MAP_STEP_FREE_SENSOR;
#endif

	if (dX >= dY) { // Hangle Dich an der laengeren Achse entlang
		if (dY > 0) dY--; // stoppe ein Feld vor dem Hindernis
		int8_t lh = dX / 2;
		for (i = 0; i < dX; ++i) {
			update_field(lX, lY, step_value);
			lX += sX;
			lh = (int8_t) (lh + dY);
			if (lh >= dX) {
				lh = (int8_t) (lh - dX);
				lY += sY;
			}
		}
	} else {
		if (dX > 0) dX--; // stoppe ein Feld vor dem Hindernis
		int8_t lh = dY / 2;
		for (i = 0; i < dY; ++i) {
			update_field(lX, lY, step_value);
			lY += sY;
			lh = (int8_t) (lh + dX);
			if (lh >= dY) {
				lh = (int8_t) (lh - dY);
				lX += sX;
			}
		}
	}

	/* Hindernis eintragen */
	if (dist <= SENS_IR_MAX_DIST) {
		update_occupied(PH_X, PH_Y, location_prob);
	}
}

/**
 * Aktualisiert die interne Karte anhand der Sensordaten
 * \param x			X-Achse der Position in Weltkoordinaten
 * \param y			Y-Achse der Position in Weltkoordinaten
 * \param sin_head	sin(Blickrichtung)
 * \param cos_head	cos(Blickrichtung)
 * \param distL		Sensorwert links
 * \param distR		Sensorwert rechts
 * \param location_prob Gibt an, wie sicher wir ueber die Position sind [0; 255]
 */
static void update_distance(int16_t x, int16_t y, float sin_head, float cos_head, int16_t distL, int16_t distR, uint8_t location_prob) {
	// Ort des rechten Sensors in Weltkoordinaten
	int16_t Pr_x = x + (int16_t)(DISTSENSOR_POS_SW * sin_head + DISTSENSOR_POS_FW * cos_head);
	int16_t Pr_y = y - (int16_t)(DISTSENSOR_POS_SW * cos_head - DISTSENSOR_POS_FW * sin_head);

	// Ort des linken Sensors in Weltkoordinaten
	int16_t Pl_x = x - (int16_t)(DISTSENSOR_POS_SW * sin_head - DISTSENSOR_POS_FW * cos_head);
	int16_t Pl_y = y + (int16_t)(DISTSENSOR_POS_SW * cos_head + DISTSENSOR_POS_FW * sin_head);

	update_sensor_distance(Pl_x, Pl_y, sin_head, cos_head, distL, location_prob);
	update_sensor_distance(Pr_x, Pr_y, sin_head, cos_head, distR, location_prob);
}

/**
 * Aktualisiert den Standkreis der internen Karte
 * \param x X-Achse der Position in Weltkoordinaten
 * \param y Y-Achse der Position in Weltkoordinaten
 * \param location_prob Gibt an, wie sicher wir ueber die Position sind [0; 255]
 */
static void update_location(int16_t x, int16_t y, uint8_t location_prob) {
	int16_t x_map = world_to_map(x);
	int16_t y_map = world_to_map(y);

	// Aktualisiere die vom Bot selbst belegte Flaeche
	update_field_circle(x_map, y_map, BOT_DIAMETER / 20 * MAP_RESOLUTION / 100,
			(int8_t) ((MAP_STEP_FREE_LOCATION - 1) * location_prob / 255 + 1));
}

/**
 * Aktualisiert die interne Karte anhand der Abgrund-Sensordaten
 * \param x			X-Achse der Position in Weltkoordinaten
 * \param y			Y-Achse der Position in Weltkoordinaten
 * \param sin_head	sin(Blickrichtung [Grad])
 * \param cos_head	cos(Blickrichtung [Grad])
 * \param borderL	Sensor links 1= abgrund 0 = frei
 * \param borderR	Sensor rechts 1= abgrund 0 = frei
 */
static void update_border(int16_t x, int16_t y, float sin_head, float cos_head, uint8_t borderL, uint8_t borderR) {
	if (borderR > 0) {
		// Ort des rechten Sensors in Mapkoordinaten
		int16_t x_map = world_to_map(x + (int16_t) (BORDERSENSOR_POS_SW * sin_head + BORDERSENSOR_POS_FW * cos_head));
		int16_t y_map = world_to_map(y - (int16_t) (BORDERSENSOR_POS_SW * cos_head - BORDERSENSOR_POS_FW * sin_head));
		set_value_occupied(x_map, y_map, -128);
	}

	if (borderL > 0) {
		int16_t x_map = world_to_map(x - (int16_t)(BORDERSENSOR_POS_SW * sin_head - BORDERSENSOR_POS_FW * cos_head));

		int16_t y_map = world_to_map(y + (int16_t)(BORDERSENSOR_POS_SW * cos_head + BORDERSENSOR_POS_FW * sin_head));
		set_value_occupied(x_map, y_map, -128);
	}
}

/**
 * Berechnet das Verhaeltnis der Felder einer Region R die ausschliesslich mit Werten zwischen
 * min und max belegt sind und allen Feldern von R.
 * Die Region R wird als Gerade von (x1|y1) bis (x2|y2) und eine Breite width angegeben. Die Gerade
 * verlaeuft in der Mitte von R.
 * \param x1		Startpunkt der Region R, X-Anteil; Kartenkoordinaten
 * \param y1		Startpunkt der Region R, Y-Anteil; Kartenkoordinaten
 * \param x2		Endpunkt der Region R, X-Anteil; Kartenkoordinaten
 * \param y2		Endpunkt der Region R, Y-Anteil; Kartenkoordinaten
 * \param width		Breite der Region R (jeweils width/2 links und rechts der Gerade)
 * \param min_val	minimaler Feldwert, der vorkommen darf
 * \param max_val	maximaler Feldwert, der vorkommen darf
 * \return			Verhaeltnis von Anzahl der Felder, die zwischen min_val und max_val liegen, zu
 * 					Anzahl aller Felder der Region * MAP_RATIO_FULL;
 * 					MAP_RATIO_NONE 	-> kein Feld liegt im gewuenschten Bereich;
 * 					MAP_RATIO_FULL	-> alle Felder liegen im gewuenschten Bereich
 */
static uint8_t get_ratio(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t width, int8_t min_val, int8_t max_val) {
	uint16_t count = 0;
	int16_t i;

	/* Gehe alle Felder der Reihe nach durch */
	int16_t lX = x1;
	int16_t lY = y1;

	int8_t sX = (int8_t) (x2 < x1 ? -1 : 1);
	int16_t dX = abs(x2 - x1);	// Laenge der Linie in X-Richtung

	int8_t sY = (int8_t) (y2 < y1 ? -1 : 1);
	int16_t dY = abs(y2 - y1);	// Laenge der Linie in Y-Richtung

	int16_t w = 0;
	uint8_t corr = (uint8_t) (width & 1); // LSB von width, falls width ungerade ist, muss die Schleife eins weiter laufen
	width /= 2;
	if (width == 0) {
		width = 1;
	}

#ifdef DEBUG_GET_RATIO_VERBOSE
	command_write(CMD_MAP, SUB_MAP_CLEAR_LINES, 4, 0, 0);
#endif // DEBUG_GET_RATIO_VERBOSE

	/* Hangle Dich an der laengeren Achse entlang */
	if (dX >= dY) {
		int16_t lh = dX / 2;
		for (i = 0; i < dX; i++) {
			for (w = -width; w < width + corr; w++) {
				int8_t field = access_field(lX + i * sX, lY + w, 0, 0);
				if (field >= min_val && field <= max_val) {
					count++;
#ifdef DEBUG_GET_RATIO_VERBOSE
					position_t tmp;
					tmp.x = lX + i * sX;
					tmp.y = lY + w;
					map_draw_line(tmp, tmp, 0);
				} else {
					position_t tmp;
					tmp.x = lX + i * sX;
					tmp.y = lY + w;
					map_draw_line(tmp, tmp, 1);
#endif // DEBUG_GET_RATIO_VERBOSE
				}
			}

			lh += dY;
			if (lh >= dX) {
				lh -= dX;
				lY += sY;
			}
		}
	} else {
		int16_t lh = dY / 2;
		for (i = 0; i < dY; i++) {
			for (w = -width; w < width + corr; w++) {
				int8_t field = access_field(lX + w, lY + i * sY, 0, 0);
				if (field >= min_val && field <= max_val) {
					count++;
#ifdef DEBUG_GET_RATIO_VERBOSE
					position_t tmp;
					tmp.x = lX + w;
					tmp.y = lY + i * sY;
					map_draw_line(tmp, tmp, 0);
				} else {
					position_t tmp;
					tmp.x = lX + w;
					tmp.y = lY + i * sY;
					map_draw_line(tmp, tmp, 1);
#endif // DEBUG_GET_RATIO_VERBOSE
				}
			}

			lh += dX;
			if (lh >= dY) {
				lh -= dY;
				lX += sX;
			}
		}
	}

	/* Verhaeltnis zu allen Feldern berechnen */
	uint16_t fields = (uint16_t) i * (uint16_t) (width * 2 + corr);
	if (fields == 0) {
		return 255;
	}
	uint8_t result = (uint8_t) ((uint32_t) count * 255 / fields);

#ifdef DEBUG_GET_RATIO
#ifndef DEBUG_GET_RATIO_VERBOSE
	command_write(CMD_MAP, SUB_MAP_CLEAR_LINES, 12, 0, 0);
#endif
	position_t from, to;
	from.x = x1;
	from.y = y1;
	to.x = x2;
	to.y = y2;
	map_draw_rect(from, to, (uint8_t) (width * 2), (uint8_t) (result == MAP_RATIO_FULL ? 0 : 1));
#endif // DEBUG_GET_RATIO

	return result;
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
uint8_t map_get_ratio(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t width, int8_t min_val, int8_t max_val) {
	/* warten bis Karte frei ist */
	map_flush_cache();

	/* Ergebnis berechnen */
	uint8_t result = get_ratio(world_to_map(x1), world_to_map(y1), world_to_map(x2), world_to_map(y2), width / (1000 / MAP_RESOLUTION), min_val, max_val);

	return result;
}

/**
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * \param from_x	Startort x Weltkoordinaten [mm]
 * \param from_y	Startort y Weltkoordinaten [mm]
 * \param to_x		Zielort x Weltkoordinaten [mm]
 * \param to_y		Zielort y Weltkoordinaten [mm]
 * \param margin	Breite eines Toleranzbereichs links und rechts der Fahrspur, der ebenfalls frei sein muss [mm]
 * \return			1, wenn alles frei ist
 */
uint8_t map_way_free(int16_t from_x, int16_t from_y, int16_t to_x, int16_t to_y, uint8_t margin) {
	uint8_t result = map_get_ratio(from_x, from_y, to_x, to_y, BOT_DIAMETER + 2 * margin, MAP_OBSTACLE_THRESHOLD, 127);
	return (uint8_t) (result == MAP_RATIO_FULL);
}

#ifdef MAP_2_SIM_AVAILABLE
/**
 * Zeichnet eine Linie in die Map-Anzeige des Sim
 * \param from	Startpunkt der Linie (Map-Koordinate)
 * \param to	Endpunkt der Linie (Map-Koordinate)
 * \param color	Farbe der Linie: 0=gruen, 1=rot, sonst schwarz
 */
void map_draw_line(position_t from, position_t to, uint8_t color) {
	// Datenformat: {from.x, from.y, to.x, to.y} als payload, color in data_l, 0 in data_r
	uint8_t data[8];
	int16_t* ptr = (int16_t*) &data[0];
	*ptr = from.x;
	ptr++;
	*ptr = from.y;
	ptr++;
	*ptr = to.x;
	ptr++;
	*ptr = to.y;
	int16_t c = color;
	command_write_rawdata(CMD_MAP, SUB_MAP_LINE, c, 0, sizeof(data), data);
}

/**
 * Zeichnet eine Linie von Koordinate from nach to in der Farbe color in die Map ein;
 * dient zur Visualisierung der Arbeitsweise des Verhaltens
 * \param from	Koordinaten des ersten Punktes der Linie (Welt)
 * \param to	Koordinaten des zweiten Punktes der Linie (Welt)
 * \param color Farbe der Linie: 0=gruen, 1=rot, sonst schwarz
 */
void map_draw_line_world(position_t from, position_t to, uint8_t color) {
	from.x = world_to_map(from.x);
	from.y = world_to_map(from.y);
	to.x = world_to_map(to.x);
	to.y = world_to_map(to.y);
	map_draw_line(from, to, color);
}

/**
 * Zeichnet ein Rechteck in die Map-Anzeige des Sim
 * \param from	Startpunkt der Geraden mittig durch das Rechteck (Map-Koordinate)
 * \param to	Endpunkt der Geraden mittig durch das Rechteck (Map-Koordinate)
 * \param width	Breite des Rechtecks (jeweils width/2 links und rechts der Gerade; in Map-Aufloesung)
 * \param color	Farbe der Linien: 0=gruen, 1=rot, sonst schwarz
 */
void map_draw_rect(position_t from, position_t to, uint8_t width, uint8_t color) {
	/* Eckpunkte des Rechtecks berechnen */
	float alpha = atan2(to.y - from.y, to.x - from.x);
	float w_2 = width / 2.0f;
	int16_t dx = (int16_t) (w_2 * sinf(alpha));
	int16_t dy = (int16_t) (w_2 * cosf(alpha));

	position_t from1;
	from1.x = from.x - dx;
	from1.y = from.y + dy;
	position_t to1;
	to1.x = to.x - dx;
	to1.y = to.y + dy;
	position_t from2;
	from2.x = from.x + dx;
	from2.y = from.y - dy;
	position_t to2;
	to2.x = to.x + dx;
	to2.y = to.y - dy;

	/* Linien zeichnen */
	map_draw_line(from1, to1, color);
	map_draw_line(from2, to2, color);
	map_draw_line(from1, from2, color);
	map_draw_line(to1, to2, color);
}

/**
 * Zeichnet einen Kreis in die Map-Anzeige des Sim
 * \param center Korrdinaten des Kreismittelpunkts (Map-Koordinaten)
 * \param radius Radius des Kreies (in Map-Aufloesung)
 * \param color	Farbe der Linien: 0=gruen, 1=rot, sonst schwarz
 */
void map_draw_circle(position_t center, int16_t radius, uint8_t color) {
	// Datenformat: {center.x, center.y} als payload, color in data_l, radius in data_r
	uint8_t data[4];
	int16_t* ptr = (int16_t*) &data[0];
	*ptr = center.x;
	ptr++;
	*ptr = center.y;
	const int16_t c = color;
	command_write_rawdata(CMD_MAP, SUB_MAP_CIRCLE, c, radius, sizeof(data), data);
}
#endif // MAP_2_SIM_AVAILABLE


/**
 * Main-Funktion des Map-Update-Threads
 */
void map_update_main(void) {
	/* Endlosschleife -> Thread wird vom OS blockiert / gibt die Kontrolle ab,
	 * wenn der Puffer leer ist */
	while (1) {
		/* Cache-Eintrag holen
		 * Thread blockiert hier, falls Fifo leer */
		uint8_t index = _inline_fifo_get(&map_update_fifo, False);
		map_cache_t* cache_tmp = &map_update_cache[index];

		os_signal_lock(&lock_signal); // Zugriff auf die Map sperren
#ifdef DEBUG_SCAN_OTF
		LOG_DEBUG("lese Cache: x= %d y= %d distance= %d loaction=%d border=%d", cache_tmp->x_pos, cache_tmp->y_pos, cache_tmp->mode.data.distance,
			cache_tmp->mode.data.location, cache_tmp->mode.data.border);

		if ((cache_tmp->mode.data.distance || cache_tmp->mode.data.location || cache_tmp->mode.data.border) == 0)
		LOG_DEBUG("Achtung: Dieser Eintrag ergibt keinen Sinn, kein einziges mode-bit gesetzt");
#endif

#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
		const uint8_t location_prob = cache_tmp->loc_prob;
#else
		const uint8_t location_prob = 255;
#endif

		/* Grundflaeche updaten, falls location-mode */
		if (cache_tmp->mode.data.location) {
			update_location(cache_tmp->x_pos, cache_tmp->y_pos, location_prob);
		}

#ifdef MAP_USE_TRIG_CACHE
		float* sin_head = &cache_tmp->sin;
		float* cos_head = &cache_tmp->cos;
#else
		float* sin_head = NULL;
		float* cos_head = NULL;
		if (cache_tmp->mode.data.border || cache_tmp->mode.data.distance) {
			const float head = rad(cache_tmp->heading / 10.f);
			float sin_tmp = sinf(head);
			float cos_tmp = cosf(head);
			sin_head = &sin_tmp;
			cos_head = &cos_tmp;
		}
#endif // MAP_USE_TRIG_CACHE

		/* Abgrundsensoren updaten, falls border-mode */
		if (cache_tmp->mode.data.border) {
			update_border(cache_tmp->x_pos, cache_tmp->y_pos, *sin_head, *cos_head, cache_tmp->dataL, cache_tmp->dataR);
		}

		else // border-mode schliesst distance-mode aus, weil Felder der Struktur gemeinsam verwendet werden

		/* Strahlen updaten, falls distance-mode und der aktuelle Eintrag Daten dazu hat */
		if (cache_tmp->mode.data.distance) {
			update_distance(cache_tmp->x_pos, cache_tmp->y_pos, *sin_head, *cos_head, cache_tmp->dataL * 5, cache_tmp->dataR * 5, location_prob);
		}

		/* Falls Fifo leer, used-blocks zurueckschreiben und Sperre aufheben */
		if (map_update_fifo.count == 0) {
			if (min_max_updated == True) {
				if (map_current_block.updated == True) {
					/* letzten Block sichern */
					if (sdfat_seek(map_file_desc, (int32_t) ((map_current_block.block + alignment_offset) * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
						LOG_DEBUG("map_update_main(): sdfat_seek(0x%x) failed", map_current_block.block + alignment_offset);
					}
					if (sdfat_write(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
						LOG_DEBUG("map_update_main(): sdfat_write(0x%x) failed", map_current_block.block + alignment_offset);
					}
					map_current_block.updated = False;
				}

				min_max_updated = False;
				map_header_t* p_head_data = (map_header_t*) map_buffer;
				sdfat_rewind(map_file_desc);
				if (sdfat_read(map_file_desc, p_head_data, sizeof(map_header_t)) != sizeof(map_header_t)) {
					LOG_DEBUG("map_update_main(): sdfat_read(header) failed");
				}
				/* Min- / Max-Werte speichern */
				p_head_data->map_min_x = map_min_x;
				p_head_data->map_max_x = map_max_x;
				p_head_data->map_min_y = map_min_y;
				p_head_data->map_max_y = map_max_y;
				sdfat_rewind(map_file_desc);
				if (sdfat_write(map_file_desc, p_head_data, sizeof(map_header_t)) != sizeof(map_header_t)) {
					LOG_DEBUG("map_update_main(): sdfat_write(header) failed");
				}

				/* letzen Block wieder laden */
				if (sdfat_seek(map_file_desc, (int32_t) ((map_current_block.block + alignment_offset) * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
					LOG_DEBUG("map_update_main(): sdfat_seek(0x%x) failed", map_current_block.block + alignment_offset);
				}
				if (sdfat_read(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
					LOG_DEBUG("map_update_main(): sdfat_read(0x%x) failed", map_current_block.block + alignment_offset);
				}
			}
		}
		os_signal_unlock(&lock_signal); // Zugriff auf Map wieder freigeben
	}
}

//#define MAP_2_SIM_DEBUG
#ifdef MAP_2_SIM_AVAILABLE
/**
 * Main-Funktion des Map-2-Sim-Threads
 */
void map_2_sim_main(void) {
	static uint16_t cache_copy[MAP_2_SIM_BUFFER_SIZE] = {0};
#ifdef MAP_2_SIM_DEBUG
	static int8_t max_entries = 0;
#endif

	/* Endlosschleife -> Thread wird vom OS blockiert / gibt die Kontrolle ab,
	 * wenn der Puffer leer ist */
	while (1) {
		/* Daten aus Fifo holen
		 * Thread blockiert hier, falls Fifo leer */
		uint8_t size = (uint8_t) fifo_get_data(&map_2_sim_fifo, &cache_copy, MAP_2_SIM_BUFFER_SIZE * sizeof(cache_copy[0]));
		os_signal_set(&map_2_sim_signal);
		os_signal_release(&map_2_sim_signal);
		const int8_t count = (int8_t) (size / sizeof(cache_copy[0])); // Anzahl der Eintraege
#ifdef MAP_2_SIM_DEBUG
		if (count > max_entries) {
			max_entries = count;
			LOG_INFO("max_entries=%d", max_entries);
		}
#endif // MAP_2_SIM_DEBUG
		uint8_t i;
		for (i = 0; i < count; ++i) {
			/* eingetragenen Block in der Liste der bereits Gesendeten suchen */
			int8_t j;
			for (j = (int8_t) (count - 1); j > i; --j) {
				if (cache_copy[i] == cache_copy[j]) {
//					printf("ueberspringe Block %u\n", cache_copy[i]);
					cache_copy[i] = 0;
					break; // Treffer, block kommt spaeter noch mal, also verwerfen
				}
			}
			if (j == i) {
				const int16_t max_block = MAP_SECTIONS * MAP_SECTIONS / 2;
				const int16_t block = (int16_t) cache_copy[i];
				cache_copy[i] = 0;
				if (block > max_block) {
					LOG_ERROR("map_2_sim_main(): Block %u ausserhalb der Karte!", block);
					continue;
				}
				/* Block nicht gefunden -> wurde noch nicht gesendet, also jetzt senden */
//				printf("sende Block %u\n", block);
				if (sdfat_seek(map_2_sim_file_desc, ((int32_t) block + (int32_t) alignment_offset) * MAP_BLOCK_SIZE + sizeof(map_header_t), SEEK_SET)) {
					LOG_DEBUG("map_2_sim_main(): sdfat_seek(0x%x) failed", block + alignment_offset);
					continue;
				}
				if (sdfat_read(map_2_sim_file_desc, map_2_sim_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
					LOG_DEBUG("map_2_sim_main(): sdfat_read(0x%x) failed", block + alignment_offset);
					continue;
				}

				os_thread_sleep(20);
				command_write_rawdata(CMD_MAP, SUB_MAP_DATA_1, block, map_2_sim_data.pos.x, 128, map_2_sim_buffer);
				os_thread_sleep(20);
				command_write_rawdata(CMD_MAP, SUB_MAP_DATA_2, block, map_2_sim_data.pos.y, 128, &map_2_sim_buffer[128]);
				os_thread_sleep(20);
				command_write_rawdata(CMD_MAP, SUB_MAP_DATA_3, block, map_2_sim_data.heading, 128, &map_2_sim_buffer[256]);
				os_thread_sleep(20);
				command_write_rawdata(CMD_MAP, SUB_MAP_DATA_4, block, 0, 128, &map_2_sim_buffer[384]);
			}
		}
#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
		command_write(CMD_MAP, SUB_MAP_CLEAR_CIRCLES, 0, 0, 0);
		map_draw_circle(map_2_sim_data.pos, map_2_sim_data.error, 0);
#endif
//		printf("\n");
	}
}

/**
 * Uebertraegt die komplette Karte an den Sim
 */
void map_2_sim_send(void) {
	/* Warten, bis Map-Update fertig */
	map_flush_cache();
	os_signal_lock(&lock_signal);
	os_signal_lock(&map_2_sim_signal);

	/* Belegte Bloecke uebertragen */
	int16_t x, y;
	int16_t max_x = map_max_x + MAP_SECTION_POINTS;
	if (max_x > MAP_SIZE * MAP_RESOLUTION) {
		max_x = MAP_SIZE * MAP_RESOLUTION - 1;
	}
	for (x = map_min_x; x < max_x; x += MAP_SECTION_POINTS * 2) { // in einem Block liegen 2 Sections in x-Richtung aneinander
		for (y = map_min_y; y <= map_max_y; y += MAP_SECTION_POINTS) {
			access_field(x, y, 0, 0); // Block in Puffer laden
			const int16_t block = (int16_t) map_current_block.block;
			command_write_rawdata(CMD_MAP, SUB_MAP_DATA_1, block, map_2_sim_data.pos.x, 128, map_buffer);
			command_write_rawdata(CMD_MAP, SUB_MAP_DATA_2, block, map_2_sim_data.pos.y, 128, &map_buffer[128]);
			command_write_rawdata(CMD_MAP, SUB_MAP_DATA_3, block, map_2_sim_data.heading, 128, &map_buffer[256]);
			command_write_rawdata(CMD_MAP, SUB_MAP_DATA_4, block, 0, 128, &map_buffer[384]);
		}
	}

	/* Sperre wieder freigeben */
	os_signal_unlock(&map_2_sim_signal);
	os_signal_unlock(&lock_signal);
}
#endif // MAP_2_SIM_AVAILABLE

/**
 * Zeigt die Karte an
 */
void map_print(void) {
#ifdef PC
	map_to_pgm("map.pgm");
#endif
}

/**
 * Loescht die komplette Karte
 */
static void delete(void) {
	/* warten bis Karte frei ist */
	map_flush_cache();
	os_signal_lock(&lock_signal);

	/* alle Felder zuruecksetzen */
	int16_t x, y;
	for (x = map_min_x; x < map_max_x; ++x) {
		for (y = map_min_y; y < map_max_y; ++y) {
			access_field(x, y, 0, 1);
		}
	}

	/* Groesse neu initialisieren */
	map_min_x = (int16_t) (MAP_SIZE * MAP_RESOLUTION / 2);
	map_max_x = (int16_t) (MAP_SIZE * MAP_RESOLUTION / 2);
	map_min_y = (int16_t) (MAP_SIZE * MAP_RESOLUTION / 2);
	map_max_y = (int16_t) (MAP_SIZE * MAP_RESOLUTION / 2);
	min_max_updated = True;

	os_signal_unlock(&lock_signal);

#if defined PC && defined MAP_2_SIM_AVAILABLE
	map_2_sim_send();
#endif
}

/**
 * Entfernt alle frei-Informationen aus der Karte, so dass nur die
 * Hindernisse uebrig bleiben.
 */
void map_clean(void) {
	/* warten bis Karte frei ist */
	map_flush_cache();
	os_signal_lock(&lock_signal);

	/* Alle positiven Werte auf 0 setzen */
	int16_t x, y;
	for (x = map_min_x; x < map_max_x; ++x) {
		for (y = map_min_y; y < map_max_y; ++y) {
			int8_t tmp = access_field(x, y, 0, 0);
			if (tmp > 0) {
				access_field(x, y, 0, 1);
			}
		}
	}

	os_signal_unlock(&lock_signal);

#if defined PC && defined MAP_2_SIM_AVAILABLE
	map_2_sim_send();
#endif
}

/**
 * Exportiert die aktuelle Karte in eine Datei
 * \param *file Name der Zieldatei (wird geloescht, falls sie schon existiert)
 * \return 0 falls kein Fehler, sonst Fehlercode
 */
int8_t map_save_to_file(const char* file) {
	LOG_INFO("map_save_to_file(\"%s\")", file);

	if (strcmp(file, MAP_FILENAME) == 0) {
		return 0;
	}

	/* warten bis Karte frei ist */
	map_flush_cache();

	LOG_INFO("map_save_to_file(): map_min_x=%u, map_max_x=%u, map_min_y=%u, map_max_y=%u", map_min_x, map_max_x, map_min_y, map_max_y);

	pFatFile dest;
	if (sdfat_open(file, &dest, SDFAT_O_RDWR | SDFAT_O_TRUNC | SDFAT_O_CREAT)) {
		LOG_ERROR("map_save_to_file(): file create failed");
		return 1;
	}

	const uint32_t size = sdfat_get_filesize(map_file_desc) / MAP_BLOCK_SIZE;
	LOG_INFO("map_save_to_file(): size=0x%lx blocks", size - alignment_offset - sizeof(map_header_t) / MAP_BLOCK_SIZE);
	sdfat_rewind(map_file_desc);
	uint32_t i;
	for (i = 0; i < size; ++i) {
		if (sdfat_read(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
			LOG_ERROR("map_save_to_file(): sdfat_read() failed, i=0x%x", i);
			return 2;
		}
		if (sdfat_write(dest, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
			LOG_ERROR("map_save_to_file(): sdfat_write() failed, i=0x%x", i);
			return 3;
		}
	}

	/* aktuellen Block wieder laden */
	if (sdfat_seek(map_file_desc, (int32_t) ((map_current_block.block + alignment_offset) * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
		LOG_DEBUG("map_load_from_file(): sdfat_seek(0x%x) failed", map_current_block.block + alignment_offset);
	}
	if (sdfat_read(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
		LOG_DEBUG("map_load_from_file(): sdfat_read(0x%x) failed", map_current_block.block + alignment_offset);
		return 4;
	}

	return 0;
}

/**
 * Laedt die Karte aus einer Datei, die aktuelle Karte wird dadurch geloescht
 * \param *file Name der zu ladenden Datei
 * \return 0 falls kein Fehler, sonst Fehlercode
 */
int8_t map_load_from_file(const char* file) {
	LOG_INFO("map_load_from_file(): Lese Karte aus Datei \"%s\" ein...", file);

	if (strcmp(file, MAP_FILENAME) == 0) {
		return 0;
	}

	/* warten bis Karte frei ist */
	LOG_DEBUG("map_load_from_file(): waiting for lock...");
	map_flush_cache();
	LOG_DEBUG("map_load_from_file(): map_flush_cache() done.");

	/* Quelldatei oeffnen */
	pFatFile src_file;
	uint8_t res = sdfat_open(file, &src_file, SDFAT_O_READ);
	if (res) {
		LOG_DEBUG("map_load_from_file(): sdfat_open(\"%s\")=%u", file, res);
		LOG_ERROR("map_load_from_file(): sdfat_open() failed");
		return 1;
	}

	/* Map loeschen */
	delete();

	os_signal_lock(&lock_signal);
	map_header_t* p_head_buffer = (map_header_t*) map_buffer;
	if (sdfat_read(src_file, p_head_buffer, sizeof(map_header_t)) != sizeof(map_header_t)) {
		LOG_ERROR("map_load_from_file(): sdfat_read(head) failed");
		sdfat_close(src_file);
		return 3;
	}

	uint16_t src_alignment_offset = p_head_buffer->alignment_offset;
	LOG_INFO("map_load_from_file(): src_alignment_offset=0x%x", src_alignment_offset);

	/* Groesse aus Quelle initialisieren */
	map_min_x = p_head_buffer->map_min_x;
	map_max_x = p_head_buffer->map_max_x;
	map_min_y = p_head_buffer->map_min_y;
	map_max_y = p_head_buffer->map_max_y;
	LOG_INFO("map_load_from_file(): min_x=%u, max_x=%u, min_y=%u, max_y=%u", map_min_x, map_max_x, map_min_y, map_max_y);

	p_head_buffer->alignment_offset = alignment_offset;
	sdfat_rewind(map_file_desc);
	if (sdfat_write(map_file_desc, p_head_buffer, sizeof(map_header_t)) != sizeof(map_header_t)) {
		LOG_ERROR("map_load_from_file(): sdfat_write(head) failed");
		sdfat_close(src_file);
		os_signal_unlock(&lock_signal);
		return 4;
	}
	min_max_updated = False;

	/* Quelldatei nach Map-Datei kopieren */
	const uint32_t size = sdfat_get_filesize(src_file) / MAP_BLOCK_SIZE - src_alignment_offset - sizeof(map_header_t) / MAP_BLOCK_SIZE;

	if (sdfat_seek(src_file, src_alignment_offset * MAP_BLOCK_SIZE + sizeof(map_header_t), SEEK_SET)) {
		LOG_ERROR("map_load_from_file(): sdfat_seek(0x%lx) failed", src_alignment_offset * MAP_BLOCK_SIZE + sizeof(map_header_t));
		sdfat_close(src_file);
		os_signal_unlock(&lock_signal);
		return 5;
	}

	if (sdfat_seek(map_file_desc, alignment_offset * MAP_BLOCK_SIZE + sizeof(map_header_t), SEEK_SET)) {
		LOG_ERROR("map_load_from_file(): sdfat_seek(0x%lx) failed", alignment_offset * MAP_BLOCK_SIZE + sizeof(map_header_t));
		sdfat_close(src_file);
		os_signal_unlock(&lock_signal);
		return 6;
	}

	uint32_t i;
	for (i = 0; i < size; ++i) {
		if (sdfat_read(src_file, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
			LOG_ERROR("map_load_from_file(): sdfat_read() failed, i=0x%x", i);
			sdfat_close(src_file);
			os_signal_unlock(&lock_signal);
			return 7;
		}
		if (sdfat_write(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
			LOG_ERROR("map_load_from_file(): sdfat_write() failed, i=0x%x", i);
			sdfat_close(src_file);
			os_signal_unlock(&lock_signal);
			return 8;
		}
	}

	LOG_INFO("map_load_from_file(): filesize=0x%x blocks", size);
	sdfat_close(src_file);

	map_current_block.updated = False;
	map_current_block.block = 0;

	/* Block 0 laden */
	if (sdfat_seek(map_file_desc, (int32_t) ((map_current_block.block + alignment_offset) * MAP_BLOCK_SIZE) + sizeof(map_header_t), SEEK_SET)) {
		LOG_DEBUG("map_load_from_file(): sdfat_seek(0x%x) failed", map_current_block.block + alignment_offset);
		os_signal_unlock(&lock_signal);
		return 9;
	}
	if (sdfat_read(map_file_desc, map_buffer, MAP_BLOCK_SIZE) != MAP_BLOCK_SIZE) {
		LOG_ERROR("map_load_from_file(): sdfat_read(0x%x) failed", map_current_block.block + alignment_offset);
		os_signal_unlock(&lock_signal);
		return 10;
	}

	os_signal_unlock(&lock_signal);

#if defined PC && defined MAP_2_SIM_AVAILABLE
	map_2_sim_send();
#endif

	return 0;
}


// *** PC-only Code ***

#if defined PC || 0
/**
 *
 * Zeichnet ein Testmuster in die Karte
 */
static void draw_test_scheme(void) {
	/* warten bis Karte frei ist */
	map_flush_cache();
	os_signal_lock(&lock_signal);

	int16_t x, y;
	LOG_DEBUG("Linie");

	// Erstmal eine ganz simple Linie
	for (x = 0; x < MAP_SECTION_POINTS * MAP_SECTIONS; ++x) {
		access_field(x, x, -120, 1);
		access_field((int16_t) (MAP_SECTION_POINTS * MAP_SECTIONS) - x - 1, x, -120, 1);
	}

	LOG_DEBUG("Section Test")

	int8_t tmp = -128;
	for (x = world_to_map(-128); x < world_to_map(128) /*+ MAP_SECTION_POINTS * 2*/; ++x) {
		for (y = world_to_map(-128); y < world_to_map(128) /*+ MAP_SECTION_POINTS*/; ++y) {
			//printf("x=%d y=%d\n", x, y);
			access_field(x, y, tmp++, 1);
		}
	}

	LOG_DEBUG("Section Grenzen");

	// Grenzen der Sections Zeichnen
	for (x = 0; x < MAP_SECTION_POINTS * MAP_SECTIONS; ++x) {
		for (y = 0; y < MAP_SECTIONS; ++y) {
			access_field(x, y * MAP_SECTION_POINTS, -10, 1);
			access_field(y * MAP_SECTION_POINTS, x, -10, 1);
		}
	}

	LOG_DEBUG("Makroblock Grenzen");

	// Grenzen der Macroblocks einzeichnen
	for (x = 0; x < MAP_SECTION_POINTS * MAP_SECTIONS; ++x) {
		for (y = 0; y < MAP_LENGTH_IN_MACRO_BLOCKS; ++y) {
			access_field(x, y * (int16_t) MACRO_BLOCK_LENGTH, -60, 1);
			access_field(y * (int16_t) MACRO_BLOCK_LENGTH, x, -60, 1);
		}
	}

	LOG_DEBUG("fertig.");
	os_signal_unlock(&lock_signal);
}

/**
 * Verkleinert die Karte vom uebergebenen auf den benutzten Bereich. Achtung,
 * unter Umstaenden muss man vorher die Puffervariablen sinnvoll initialisieren!!!
 * \param min_x Zeiger auf einen uint16_t, der den minimalen X-Wert puffert
 * \param max_x Zeiger auf einen uint16_t, der den maximalen X-Wert puffert
 * \param min_y Zeiger auf einen uint16_t, der den minimalen Y-Wert puffert
 * \param max_y Zeiger auf einen uint16_t, der den maximalen Y-Wert puffert
 */
void shrink(int16_t* min_x, int16_t* max_x, int16_t* min_y, int16_t* max_y) {
	int16_t x, y;

	// lokale Variablen mit den defaults befuellen
	*min_x = map_min_x;
	*max_x = map_max_x;
	*min_y = map_min_y;
	*max_y = map_max_y;

	/* warten bis Karte frei ist */
	map_flush_cache();
	os_signal_lock(&lock_signal);

	// Kartengroesse reduzieren
	int8_t free = 1;
	while ((*min_y < *max_y) && (free == 1)) {
		for (x = *min_x; x < *max_x; x++) {
			if (access_field(x, *min_y, 0, 0) != 0) {
				free = 0;
				break;
			}
		}
		*min_y += 1;
	}

	free = 1;
	while ((*min_y < *max_y) && (free == 1)) {
		for (x = *min_x; x < *max_x; x++) {
			if (access_field(x, *max_y-1, 0, 0) != 0) {
				free = 0;
				break;
			}
		}
		*max_y -= 1;
	}

	free = 1;
	while ((*min_x < *max_x) && (free == 1)) {
		for (y = *min_y; y < *max_y; y++) {
			if (access_field(*min_x, y, 0, 0) != 0) {
				free = 0;
				break;
			}
		}
		*min_x += 1;
	}
	free = 1;
	while ((*min_x < *max_x) && (free == 1)) {
		for (y = *min_y; y < *max_y; y++) {
			if (access_field(*max_x - 1, y, 0, 0) != 0) {
				free = 0;
				break;
			}
		}
		*max_x -= 1;
	}
	os_signal_unlock(&lock_signal);
}

/**
 * Schreibt eine Karte in eine PGM-Datei
 * \param *filename Zieldatei
 */
void map_to_pgm(const char* filename) {
	LOG_INFO("map_to_pgm(): Speichere Karte nach %s", filename);
	FILE* fp = fopen(filename, "wb");
	if (fp == NULL) {
		LOG_ERROR("map_to_pgm(): Konnte Datei nicht oeffnen, Abbruch");
		return;
	}

	// lokale Variablen mit den Defaults befuellen
	int16_t min_x = map_min_x;
	int16_t max_x = map_max_x;
	int16_t min_y = map_min_y;
	int16_t max_y = map_max_y;

	int16_t map_size_x = max_x - min_x;
	int16_t map_size_y = max_y - min_y;
#ifdef MAP_PRINT_SCALE
	fprintf(fp, "P5 %d %d 255 ", map_size_x + 10, map_size_y + 10);
#else
	fprintf(fp,"P5 %d %d 255 ", map_size_x, map_size_y);
#endif // MAP_PRINT_SCALE
	LOG_INFO("map_to_pgm(): Karte beginnt bei X=%d,Y=%d und geht bis X=%d,Y=%d (%d * %d Punkte)", min_x, min_y, max_x, max_y, map_size_x, map_size_y);

	/* warten bis Karte frei ist */
	map_flush_cache();
	os_signal_lock(&lock_signal);

	uint8_t tmp;
	int16_t x, y;
	for (y = max_y; y > min_y; y--) {
		for (x = min_x; x < max_x; ++x) {
			tmp = (uint8_t) (access_field(x, y - 1, 0, 0) + 128);
			fwrite(&tmp, 1, 1, fp);
		}

#ifdef MAP_PRINT_SCALE
		// und noch schnell ne Skala basteln
		for (x = 0; x < 10; ++x) {
			if (y % MAP_SCALE == 0) {
				tmp = 0;
			} else {
				tmp = 255;
			}
			fwrite(&tmp, 1, 1, fp);
		}
#endif // MAP_PRINT_SCALE
	}
	os_signal_unlock(&lock_signal);

#ifdef MAP_PRINT_SCALE
	for (y = 0; y < 10; ++y) {
		for (x = min_x; x < max_x + 10; ++x) {
			if (x % MAP_SCALE == 0) {
				tmp = 0;
			} else {
				tmp = 255;
			}
			fwrite(&tmp, 1, 1, fp);
		}
	}
#endif // MAP_PRINT_SCALE
	fclose(fp);
}

#ifdef MAP_TESTS_AVAILABLE
/**
 * Testet die Funktion map_get_ratio()
 * \return 0 falls alles OK, 1 falls Fehler
 */
static int16_t map_test_get_ratio(void) {
	delete();
	printf("map deleted\n");
	int16_t all_ok = 1;
	uint8_t result = map_get_ratio(-100, -100, 100, 100, 100, 0, 0);
	if (result != 255) {
		all_ok = 0;
	}
	printf("map_get_ratio(-100, -100, 100, 100, 100, 0, 0) = %u\n", result);
	result = map_get_ratio(-100, -100, 100, 100, 100, 100, 100);
	if (result != 0) {
		all_ok = 0;
	}
	printf("map_get_ratio(-100, -100, 100, 100, 100, 100, 100) = %u\n", result);
	int16_t i, j, k;
	int8_t value = -120;
	for (k=0; k<13; k++) {
		for (i=-800; i<800; i++) {
			for (j=-800; j<800; j++) {
				access_field(world_to_map(i), world_to_map(j), value, 1);
			}
		}
		printf("\nmap filled with %d\n", value);
		result = map_get_ratio(-200, -100, 100, 200, 100, 0, 0);
		if (result != 0) {
			if (value != 0) {
				all_ok = 0;
				printf("map_get_ratio(-200, -100, 100, 200, 100, 0, 0) = %u\n", result);
			}
		} else {
			if (value == 0) {
				all_ok = 0;
				printf("map_get_ratio(-200, -100, 100, 200, 100, 0, 0) = %u\n", result);
			}
		}
		result = map_get_ratio(-200, -100, 100, 200, 100, value, value);
		if (result != 255) {
			all_ok = 0;
			printf("map_get_ratio(-200, -100, 100, 200, 100, %d, %d) = %u\n", value, value, result);
		}
		int16_t width = 0;
		int8_t ok = 1;
		for (i=-150; i<=150; i+=50) {
			for (j=-150; j<=150; j+=50) {
				uint8_t last_result = map_get_ratio(i, j, 155, 155, 0, value, value);
				if (last_result != 255) {
					printf("\tmap_get_ratio(%d, %d, 155, 155, %u, %d, %d) = %u\n", i, j, width, value, value, last_result);
					printf("test(%4d,%4d)\tFAILED\n", i, j);
					continue;
				}
				for (width=0; width<=6400; width+=8) {
					result = map_get_ratio(i, j, 155, 155, width, value, value);
					int16_t diff = abs((int16_t)result - (int16_t)last_result);
					if (diff > 2) {
						ok = 0;
						printf("\tmap_get_ratio(%d, %d, 155, 155, %u, %d, %d) = %u\n", i, j, width, value, value, result);
						printf("\tlast_result=%d\n", last_result);
						break;
					}
					last_result = result;
				}
				if (result >= 255 && value != 0) {
					ok = 0;
				}
				if (value == 0 && result != 255) {
					ok = 0;
				}
				if (ok == 1) {
					printf("test(%4d,%4d)\tPASSED\n", i, j);
				} else {
					printf("test(%4d,%4d)\tFAILED\n", i, j);
				}
				all_ok += ok;
				ok = 1;
			}
		}
		value += 20;
	}
	if (all_ok == 7 * 7 * 13 + 1) {
		printf("all tests PASSED\n");
		return 0;
	} else {
		printf(">=1 test FAILED\n");
		printf("all_ok=%d\n", all_ok);
		return 1;
	}
}
#endif // MAP_TESTS_AVAILABLE
#endif // PC


// Gui-Code

#ifdef MAP_INFO_AVAILABLE
/**
 * Zeigt ein paar Infos ueber die Karte an
 */
static void info(void) {
	LOG_INFO("MAP:");
	LOG_INFO("%zu\t Punkte pro Section (MAP_SECTIONS)", (size_t) MAP_SECTIONS);
	LOG_INFO("%u\t Sections (MAP_SECTION_POINTS)", MAP_SECTION_POINTS);
	LOG_INFO("%zu\t Punkte Kantenlaenge (MAP_SECTION_POINTS*MAP_SECTIONS)", (size_t) (MAP_SECTION_POINTS * MAP_SECTIONS));
	uint32_t points_in_map = (uint32_t) MAP_SECTION_POINTS * (uint32_t)MAP_SECTION_POINTS * (uint32_t) MAP_SECTIONS * (uint32_t)MAP_SECTIONS;
	LOG_INFO("%u%u\t Punkte gesamt", (uint16_t) (points_in_map / 10000), (uint16_t) (points_in_map % 10000) );
	points_in_map /= 1024; // Umrechnen in KByte
	LOG_INFO("%u%u\t KByte", (uint16_t) (points_in_map / 10000), (uint16_t) (points_in_map % 10000));
	LOG_INFO("%u\t Punkte pro Meter (MAP_RESOLUTION)", MAP_RESOLUTION);
	LOG_INFO("%u\t Meter Kantenlaenge (MAP_SIZE)", (uint16_t) MAP_SIZE);

	LOG_INFO("Die Karte verwendet Macroblocks");
	LOG_INFO("%u\t Laenge eine Macroblocks in Punkten (MACRO_BLOCK_LENGTH)", MACRO_BLOCK_LENGTH);
	LOG_INFO("%u\t Anzahl der Macroblocks in einer Zeile (MAP_LENGTH_IN_MACRO_BLOCKS)", MAP_LENGTH_IN_MACRO_BLOCKS);
	LOG_INFO("alignment_offset=0x%x", alignment_offset);
}
#endif // MAP_INFO_AVAILABLE

#ifdef DISPLAY_MAP_AVAILABLE
/**
 * Handler fuer Map-Display
 */
void map_display(void) {
	display_cursor(1, 1);
	display_puts("1: print  2: delete");
	display_cursor(2, 1);
	display_puts("3: draw_scheme");
	display_cursor(3, 1);
#ifdef MAP_INFO_AVAILABLE
	display_puts("4: info   7: clean");
#else
	display_puts("7: clean");
#endif
	display_cursor(4, 1);
	display_puts("8/9: export/import");

#ifdef RC5_AVAILABLE
	/* Keyhandler */
	switch (RC5_Code) {
	case RC5_CODE_1:
		map_print();
		RC5_Code = 0;
		break;

	case RC5_CODE_2:
		delete();
		RC5_Code = 0;
		break;

#if defined PC || 0
	case RC5_CODE_3:
		draw_test_scheme();
		RC5_Code = 0;
		break;
#endif // PC

#ifdef MAP_INFO_AVAILABLE
	case RC5_CODE_4:
		info();
		RC5_Code = 0;
		break;
#endif // MAP_INFO_AVAILABLE

#ifdef PC
#ifdef MAP_TESTS_AVAILABLE
	case RC5_CODE_5:
		map_test_get_ratio();
		RC5_Code = 0;
		break;
#endif // MAP_TESTS_AVAILABLE
#endif // PC

	case RC5_CODE_7:
		map_clean();
		RC5_Code = 0;
		break;

	case RC5_CODE_8: {
		int8_t res = map_save_to_file("exported.map");
		if (res != 0) {
			LOG_ERROR("map_save_to_file() schlug fehl: %d", res);
		}
		RC5_Code = 0;
		break;
	}

	case RC5_CODE_9: {
		int8_t res = map_load_from_file("exported.map");
		if (res != 0) {
			LOG_ERROR("map_load_from_file() schlug fehl: %d", res);
		}
		RC5_Code = 0;
		break;
	}
	}
#endif // RC5_AVAILABLE
}
#endif // DISPLAY_MAP_AVAILABLE

#endif // MAP_AVAILABLE
