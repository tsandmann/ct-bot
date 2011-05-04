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
 * \file 	gui.c
 * \brief 	Display-GUI des Bots
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	12.02.2007
 */

#include "ct-Bot.h"
#include "eeprom.h"

/*! Keymap fuer Keypad-Eingaben */
EEPROM uint8_t gui_keypad_table[][5] = {
	{'.', ',', ':', ';', '0'}, // 0
	{'-', ' ', '1',   0,   0}, // 1
	{'a', 'b', 'c', '2',   0}, // 2
	{'d', 'e', 'f', '3',   0}, // 3
	{'g', 'h', 'i', '4',   0}, // 4
	{'j', 'k', 'l', '5',   0}, // 5
	{'m', 'n', 'o', '6',   0}, // 6
	{'p', 'q', 'r', 's', '7'}, // 7
	{'t', 'u', 'v', '8',   0}, // 8
	{'w', 'x', 'y', 'z', '9'}, // 9
	{'-',   0,   0,   0,   0}  // 11 (bietet Minus auch im Numerik-Modus direkt an)
};

#ifdef DISPLAY_AVAILABLE
#include "gui.h"
#include "bot-logic/bot-logic.h"
#include "rc5.h"
#include "sensor.h"
#include "motor.h"
#include "display.h"
#include "mmc.h"
#include "log.h"
#include "led.h"
#include "mini-fat.h"
#include "map.h"
#include "command.h"
#include "pos_store.h"
#include "rc5-codes.h"
#include "os_scheduler.h"
#include "timer.h"
#include <stdlib.h>
#include <string.h>

/* Abhaengigkeiten */
#ifndef SPEED_CONTROL_AVAILABLE
#undef DISPLAY_REGELUNG_AVAILABLE
#endif

#ifndef MCU
#undef DISPLAY_RESET_INFO_AVAILABLE
#undef DISPLAY_RAM_AVAILABLE
#undef DISPLAY_OS_AVAILABLE
#endif // !MCU

#if ! defined KEYPAD_AVAILABLE || ! defined BEHAVIOUR_REMOTECALL_AVAILABLE
#undef DISPLAY_REMOTECALL_AVAILABLE
#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
#warning "RemoteCall-Display geht nur mit aktivem Keypad, Display wurde daher deaktiviert"
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE
#endif // ! KEYPAD_AVAILABLE || ! BEHAVIOUR_REMOTECALL_AVAILABLE

#ifndef MMC_AVAILABLE
#undef DISPLAY_MMC_INFO
#undef DISPLAY_MINIFAT_INFO
#ifdef PC
#ifndef MMC_VM_AVAILABLE
#undef DISPLAY_MINIFAT_INFO
#endif
#endif // PC
#endif // !MMC_AVAILABLE

#ifdef BOT_FS_AVAILABLE
#undef DISPLAY_MINIFAT_INFO
#endif

#ifndef MAP_AVAILABLE
#undef DISPLAY_MAP_AVAILABLE
#else
// MMC-Zugriff geht nur, wenn gerade kein Map-Update laueft
#undef DISPLAY_MMC_INFO
#endif // !MAP_AVAILABLE

#ifndef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
#undef DISPLAY_TRANSPORT_PILLAR
#endif

#ifndef BEHAVIOUR_DRIVE_STACK_AVAILABLE
#undef DISPLAY_DRIVE_STACK_AVAILABLE
#endif

#ifndef BEHAVIOUR_PATHPLANING_AVAILABLE
#undef DISPLAY_PATHPLANING_AVAILABLE
#endif

#ifndef BEHAVIOUR_DRIVE_CHESS_AVAILABLE
#undef DISPLAY_DRIVE_CHESS_AVAILABLE
#endif

#ifndef OS_AVAILABLE
#undef DISPLAY_OS_AVAILABLE
#endif

#ifndef BEHAVIOUR_UBASIC_AVAILABLE
#undef DISPLAY_UBASIC_AVAILABLE
#endif


int8_t max_screens = 0;	/*!< Anzahl der zurzeit registrierten Screens */
void (* screen_functions[DISPLAY_SCREENS])(void) = {NULL}; /*!< hier liegen die Zeiger auf die Display-Funktionen */

#ifdef KEYPAD_AVAILABLE
//#define KEYPAD_DEBUG

#ifndef KEYPAD_DEBUG
#undef LOG_DEBUG
#define LOG_DEBUG(...)
#endif

static uint32_t keypad_last_pressed = 0;	/*!< Zeitpunkt der letzten Tasteneingabe */
static uint8_t keypad_row = 0;				/*!< Zeile fuer Ausgabe der Eingabe */
static uint8_t keypad_col = 0;				/*!< Spalte fuer Ausgabe der Eingabe */
static uint8_t keypad_mode = 0;				/*!< Modus, 0: alphanumerisch, 1: numerisch */
static char keypad_buffer[21];				/*!< Eingabepuffer */
static char * keypad_result = NULL;			/*!< aktuelle Position im Puffer */
static void (* keypad_callback)(char * result) = NULL; /*!< Callback-Funktion nach Abschluss */


/*!
 * \brief Startet eine neue Keypad-Eingabe.
 *
 * Abgeschlossen wird sie mit der Taste "Play", abgebrochen mit "Stop".
 * Nach  Abschluss wird die uebergebene Callback-Funktion aufgerufen
 * mit dem Eingabe-String als Parameter.
 * \param *callback	Zeiger auf eine Funktion, die die Eingabe bekommt
 * \param mode		voreingestellter Modus (0: alpha-num, 1: numerisch)
 * \param row		Zeile der Cursorposition fuer die Anzeige der Eingabe
 * \param col		Spalte der Cursorposition fuer die Anzeige der Eingabe
 */
void gui_keypad_request(void (* callback)(char * result), uint8_t mode, uint8_t row, uint8_t col) {
	keypad_callback = callback;
	keypad_row = row;
	keypad_col = col;
	keypad_mode = mode;
	keypad_result = keypad_buffer - 1;
}

/*!
 * \brief Ermoeglicht Eingabe von Text und Zahlen ueber eine Ziffern-Fernbedienung.
 *
 * Diese Funktion prueft, ob derzeit eine Eingabe laeuft und wertet den
 * RC5-Code entsprechend aus.
 * Mit der Taste "Play" wird eine Eingabe abgeschlossen, mit "Stop" verworfen.
 * Punkt-Taste (RC5_CODE_DOT) schaltet zwischen aplha-numerischem und numerischem Modus um.
 * \param rc5	RC5-Code, der ausgewertet werden soll
 * \return		RC5-Code unveraendert, falls Keypad nicht aktiv, sonst 0
 */
static uint16_t gui_keypad_check(uint16_t rc5) {
	static uint8_t pressed = 0;
	static uint16_t last_rc5 = 0;

	if (keypad_callback == NULL) {
		/* keine Keypad-Eingabe aktiv */
		return rc5;
	}

	switch (rc5) {
	case RC5_CODE_DOT:
		keypad_mode = (uint8_t) ((keypad_mode + 1) & 1);
		return 0;
	case RC5_CODE_STOP:
		/* Abbruch */
		*keypad_buffer = 0;
		// no break
	case RC5_CODE_PLAY:
		/* fertig */
		keypad_callback(keypad_buffer);
		keypad_callback = NULL;
		memset(keypad_buffer, 0, 21);
		// no break;
	case 0:
		/* keine neue Eingabe */
		return 0;
	}

	const uint8_t key = (uint8_t) (rc5 - RC5_CODE_0);
	if (key > 10) {
		/* ungueltige Eingabe */
		return 0;
	}

	if (!timer_ms_passed_32(&keypad_last_pressed, 1000) && rc5 == last_rc5) {
		/* dieselbe Taste wurde mehrfach gedrueckt */
		pressed++;
	} else {
		/* eins weiter schalten */
		keypad_result++;
		pressed = 0;
		LOG_DEBUG("neue Spalte:%u", keypad_result - keypad_buffer);
	}

	LOG_DEBUG("pressed=%u", pressed);

	if (pressed > 4) {
		/* Umlauf */
		pressed = 0;
	}

	LOG_DEBUG("key=%u", key);
	char data;
	while ((data = (char) ctbot_eeprom_read_byte(&gui_keypad_table[key][pressed])) == 0) {
		/* bei einigen Tasten ist nicht alles belegt */
		pressed = 0;
	}

	/* im numerischen Fall nur Ziffern auswaehlen */
	if (keypad_mode == 1) {
		uint8_t i = 4;
		while ((data = (char) ctbot_eeprom_read_byte(&gui_keypad_table[key][i])) == 0) {
			i--;
		}
		rc5 = 0; // naechsten Tastendruck als neue Taste registrieren
	}

	last_rc5 = rc5;
	keypad_last_pressed = TIMER_GET_TICKCOUNT_32;
	*keypad_result = data;
	LOG_DEBUG("data=%c", data);
	LOG_DEBUG("result=%s", keypad_buffer);
	return 0;
}
#endif // KEYPAD_AVAILABLE

/*!
 * \brief Display-Screen Registrierung
 * \param fkt 	Zeiger auf eine Funktion, die den Display-Screen anzeigt
 *
 * Legt einen neuen Display-Screen an und haengt eine Anzeigefunktion ein.
 * Diese Funktion kann auch RC5-Kommandos behandeln. Wurde eine Taste ausgewertet, setzt man RC5_Code auf 0.
 */
static int8_t register_screen(void (* fkt)(void)) {
	if (max_screens == DISPLAY_SCREENS) {
		return -1; // sorry, aber fuer dich ist kein Platz mehr da :(
	}
	int8_t screen_nr = max_screens++; // neuen Screen hinten anfuegen
	screen_functions[screen_nr] = fkt; // Pointer im Array speichern
	return screen_nr;
}

/*!
 * Display-Screen Anzeige
 *
 * Zeigt einen Screen an und fuehrt die RC5-Kommandoauswertung aus, falls noch nicht geschehen.
 * \param screen Nummer des Screens, der angezeigt werden soll
 */
void gui_display(uint8_t screen) {
#ifdef KEYPAD_AVAILABLE
	/* Keypad-Eingabe checken */
	RC5_Code = gui_keypad_check(RC5_Code);
#endif

	/* weisse LED zeigt Tastendruck an */
#if defined LED_AVAILABLE && ! defined TEST_AVAILABLE && defined RC5_AVAILABLE
	if (RC5_Code != 0) {
		LED_on(LED_WEISS);
	}
#endif // LED_AVAILABLE && ! TEST_AVAILABLE && RC5_AVAILABLE

	/* Gueltigkeit der Screen-Nr. pruefen und Anzeigefunktion aufrufen, falls Screen belegt ist */
	if (screen < max_screens && screen_functions[screen] != NULL) {
		screen_functions[screen]();
	}

#ifdef KEYPAD_AVAILABLE
	/* Keypad-Eingabe */
	if (keypad_callback != NULL) {
		display_cursor(keypad_row, keypad_col);
		display_printf("%s ", keypad_buffer);

		uint8_t col = (uint8_t) (keypad_col + strlen(keypad_buffer));
		if ((uint32_t) (TIMER_GET_TICKCOUNT_32 - keypad_last_pressed) > MS_TO_TICKS(1000UL)) {
			++col;
		}

		if (keypad_mode == 1) {
			display_cursor(keypad_row, col);
			display_puts("#");
		}
		display_cursor(keypad_row, (uint8_t) (col - 1));
	}
#endif // KEYPAD_AVAILABLE

#ifdef RC5_AVAILABLE
	if (RC5_Code != 0) {
		/* falls rc5-Code noch nicht abgearbeitet, Standardbehandlung ausfuehren */
		default_key_handler();
		RC5_Code = 0; // fertig, RC5-Puffer loeschen
	}
#endif // RC5_AVAILABLE

#if defined LED_AVAILABLE && ! defined TEST_AVAILABLE && defined RC5_AVAILABLE
	LED_off(LED_WEISS);
#endif // LED_AVAILABLE && ! TEST_AVAILABLE && RC5_AVAILABLE
}

/*!
 * Display-Screen Initialisierung
 * Traegt die Anzeige-Funktionen in das Array ein.
 */
void gui_init(void) {
//	register_screen(NULL); // erzeugt einen leeren Display-Screen an erster Position
#ifdef DISPLAY_MINIFAT_INFO
	/* MiniFAT wird vor GUI initialisiert und schreibt deshalb einfach auf's leere Display, der Dummy hier verhindert nur das Ueberschreiben in den anschliessenden Bot-Zyklen, damit man die Daten noch lesen kann */
	register_screen(&mini_fat_display);
#endif
#ifdef DISPLAY_RESET_INFO_AVAILABLE
	register_screen(&reset_info_display);
#endif
#ifdef DISPLAY_SENSOR_AVAILABLE
	register_screen(&sensor_display);
#endif
#ifdef DISPLAY_REMOTECALL_AVAILABLE
	register_screen(&remotecall_display);
#endif
#ifdef BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
	register_screen(&bot_calibrate_sharps_display);
#endif
#ifdef DISPLAY_REGELUNG_AVAILABLE
	register_screen(&speedcontrol_display);
#endif
#ifdef DISPLAY_ODOMETRIC_INFO
	register_screen(&odometric_display);
#endif
#ifdef DISPLAY_MISC_AVAILABLE
	register_screen(&misc_display);
#endif
#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
	register_screen(&behaviour_display);
#endif
#ifdef LOG_DISPLAY_AVAILABLE
	register_screen(&log_display);
#endif
#if defined LOG_MMC_AVAILABLE && defined USE_MINILOG
	register_screen(&log_mmc_display);
#endif
#ifdef DISPLAY_MMC_INFO
	register_screen(&mmc_display);
#endif
#ifdef DISPLAY_OS_AVAILABLE
	register_screen(&os_display);
#endif
#ifdef DISPLAY_RAM_AVAILABLE
	register_screen(&ram_display);
#endif
#ifdef DISPLAY_MAP_GO_DESTINATION
	register_screen(&mapgo_display);
#endif
#ifdef DISPLAY_TRANSPORT_PILLAR
	register_screen(&transportpillar_display);
#endif
#ifdef DISPLAY_DRIVE_STACK_AVAILABLE
	register_screen(&drive_stack_display);
#endif
#ifdef DISPLAY_PATHPLANING_AVAILABLE
	register_screen(&pathplaning_display);
#endif
#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
	register_screen(&bot_line_shortest_way_display);
#endif
#ifdef DISPLAY_DRIVE_CHESS_AVAILABLE
	register_screen(&drive_chess_display);
#endif
#ifdef DISPLAY_UBASIC_AVAILABLE
	register_screen(&ubasic_display);
#endif
#ifdef DISPLAY_MAP_AVAILABLE
	register_screen(&map_display);
#endif
}

#endif // DISPLAY_AVAILABLE
