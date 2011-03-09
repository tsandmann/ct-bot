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
 * @file 	gui.c
 * @brief 	Display-GUI des Bots
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007
 */

#include "bot-logic/available_behaviours.h"
#include "ui/available_screens.h"
#include "ct-Bot.h"
#include "global.h"
#include "rc5.h"
#include "sensor.h"
#include "motor.h"
#include "display.h"
#include "mmc.h"
#include "log.h"
#include "bot-logic/bot-logik.h"
#include "gui.h"
#include "led.h"
#include "mini-fat.h"
#include "map.h"
#include "pos_store.h"
#include <stdlib.h>
#include <string.h>
#include "rc5-codes.h"
#include "eeprom.h"
#include "os_scheduler.h"

/*! Keymap fuer Keypad-Eingaben */
EEPROM uint8_t gui_keypad_table[][5] = {
	{'.', ',', ':', ';', '0'},	// 0
	{' ', '1',   0,   0,   0},	// 1
	{'a', 'b', 'c', '2',   0},	// 2
	{'d', 'e', 'f', '3',   0},	// 3
	{'g', 'h', 'i', '4',   0},	// 4
	{'j', 'k', 'l', '5',   0},	// 5
	{'m', 'n', 'o', '6',   0},	// 6
	{'p', 'q', 'r', 's', '7'},	// 7
	{'t', 'u', 'v', '8',   0},	// 8
	{'w', 'x', 'y', 'z', '9'}	// 9
};

#ifdef DISPLAY_AVAILABLE

int8_t max_screens = 0;	/*!< Anzahl der zurzeit registrierten Screens */
void (* screen_functions[DISPLAY_SCREENS])(void) = {NULL};	/*!< hier liegen die Zeiger auf die Display-Funktionen */

#ifdef KEYPAD_AVAILABLE
static uint32_t keypad_last_pressed = 0;	/*!< Zeitpunkt der letzten Tasteneingabe */
static uint8_t keypad_row = 0;				/*!< Zeile fuer Ausgabe der Eingabe */
static uint8_t keypad_col = 0;				/*!< Spalte fuer Ausgabe der Eingabe */
static uint8_t keypad_mode = 0;				/*!< Modus, 0: alphanumerisch, 1: numerisch */
static char keypad_buffer[21];				/*!< Eingabepuffer */
static char * keypad_result = NULL;			/*!< aktuelle Position im Puffer */
static void (* keypad_callback)(char * result) = NULL;	/*!< Callback-Funktion nach Abschluss */


/*!
 * Startet eine neue Keypad-Eingabe. Abgeschlossen wird sie mit der
 * Taste "Play", abgebrochen mit "Stop".
 * Nach  Abschluss wird die uebergebene Callback-Funktion aufgerufen
 * mit dem Eingabe-String als Parameter.
 * @param *callback	Zeiger auf eine Funktion, die die Eingabe bekommt
 * @param row		Zeile der Cursorposition fuer die Anzeige der Eingabe
 * @param col		Spalte der Cursorposition fuer die Anzeige der Eingabe
 */
void gui_keypad_request(void (* callback)(char * result), uint8_t row, uint8_t col) {
	keypad_callback = callback;
	keypad_row = row;
	keypad_col = col;
	keypad_mode = 0;
	keypad_result = keypad_buffer - 1;
}

/*!
 * Ermoeglicht Eingabe von Text und Zahlen ueber eine Ziffern-Fernbedienung.
 * Diese Funktion prueft, ob derzeit eine Eingabe laeuft und wertet den
 * RC5-Code entsprechend aus.
 * Mit der Taste "Play" wird eine Eingabe abgeschlossen, mit "Stop" verworfen.
 * @param rc5	RC5-Code, der ausgewertet werden soll
 * @return		RC5-Code unveraendert, falls Keypad nicht aktiv, sonst 0
 */
static uint16_t gui_keypad_check(uint16_t rc5) {
	static uint8_t pressed = 0;
	static uint16_t last_rc5 = 0;

	if (keypad_callback == NULL) {
		/* keine Keypad-Eingabe aktiv */
		return rc5;
	}
	switch (rc5) {
	case RC5_CODE_I_II:
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
		display_cursor(keypad_row, keypad_col);
		display_printf("%s  ", keypad_buffer);
		memset(keypad_buffer, 0, 21);
		// no break;
	case 0:
		/* keine neue Eingabe */
		return 0;
	}

	if (!timer_ms_passed_32(&keypad_last_pressed, 1000) && rc5 == last_rc5) {
		/* dieselbe Taste wurde mehrfach gedrueckt */
		pressed++;
	} else {
		/* eins weiter schalten */
		keypad_result++;
		pressed = 0;
	}

//	LOG_DEBUG("pressed=%u", pressed);

	uint8_t key = (uint8_t) (rc5 - RC5_CODE_0);
	if (key > 9) {
		/* ungueltige Eingabe */
		return 0;
	}
	if (pressed > 4) {
		/* Umlauf */
		pressed = 0;
	}

//	LOG_DEBUG("key=%u", key);
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
//	LOG_DEBUG("data=%c", data);
	return 0;
}
#endif // KEYPAD_AVAILABLE

/*!
 * Display-Screen Registrierung
 * @param fkt 	Zeiger auf eine Funktion, die den Display-Screen anzeigt
 * Legt einen neuen Display-Screen an und haengt eine Anzeigefunktion ein.
 * Diese Funktion kann auch RC5-Kommandos behandeln. Wurde eine Taste ausgewertet, setzt man RC5_Code auf 0.
 */
static int8_t register_screen(void (* fkt)(void)) {
	if (max_screens == DISPLAY_SCREENS) return -1; // sorry, aber fuer dich ist kein Platz mehr da :(
	int8_t screen_nr = max_screens++; // neuen Screen hinten anfuegen
	screen_functions[screen_nr] = fkt; // Pointer im Array speichern
	return screen_nr;
}

/*!
 * Display-Screen Anzeige
 * @param screen 	Nummer des Screens, der angezeigt werden soll
 * Zeigt einen Screen an und fuehrt die RC5-Kommandoauswertung aus, falls noch nicht geschehen.
 */
void gui_display(uint8_t screen) {
#ifdef KEYPAD_AVAILABLE
	/* Keypad-Eingabe checken */
	RC5_Code = gui_keypad_check(RC5_Code);
#endif
	/* Gueltigkeit der Screen-Nr. pruefen und Anzeigefunktion aufrufen, falls Screen belegt ist */
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
#ifdef RC5_AVAILABLE
	if (RC5_Code != 0) LED_on(LED_WEISS);
#endif // RC5_AVAILABLE
#endif // TEST_AVAILABLE
#endif // LED_AVAILABLE
	if (screen < max_screens && screen_functions[screen] != NULL) screen_functions[screen]();

#ifdef KEYPAD_AVAILABLE
	/* Keypad-Eingabe */
	if (keypad_callback != NULL) {
		display_cursor(keypad_row, keypad_col);
		display_printf("%s ", keypad_buffer);

		uint8_t col = (uint8_t) (keypad_col + strlen(keypad_buffer));
		if ((uint32_t)(TIMER_GET_TICKCOUNT_32 - keypad_last_pressed) > MS_TO_TICKS(1000UL)) {
			col++;
		}
		display_cursor(keypad_row, col);
		char c = (char) (keypad_mode == 1 ? '#' : '_');
		display_printf("%c", c);
		display_cursor(keypad_row, col);
	}
#endif // KEYPAD_AVAILABLE

#ifdef RC5_AVAILABLE
	if (RC5_Code != 0) default_key_handler(); // falls rc5-Code noch nicht abgearbeitet, Standardbehandlung ausfuehren
	RC5_Code = 0; // fertig, RC5-Puffer loeschen
#endif // RC5_AVAILABLE
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
#ifdef RC5_AVAILABLE
	LED_off(LED_WEISS);
#endif // RC5_AVAILABLE
#endif // TEST_AVAILABLE
#endif // LED_AVAILABLE
}

/*!
 * Display-Screen Initialisierung
 * Traegt die Anzeige-Funktionen in das Array ein.
 */
void gui_init(void) {
//	register_screen(NULL);
#ifdef DISPLAY_MINIFAT_INFO
	/* MiniFAT wird vor GUI initialisiert und schreibt deshalb einfach auf's leere Display, der Dummy hier verhindert nur das Ueberschreiben in den anschliessenden Bot-Zyklen, damit man die Daten noch lesen kann */
	register_screen(&mini_fat_display);
#endif
#ifdef RESET_INFO_DISPLAY_AVAILABLE
	register_screen(&reset_info_display);
#endif
#ifdef SENSOR_DISPLAY_AVAILABLE
	register_screen(&sensor_display);
#endif
#ifdef BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
	register_screen(&bot_calibrate_sharps_display);
#endif
#ifdef DISPLAY_REGELUNG_AVAILABLE
	register_screen(&speedcontrol_display);
#endif
#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
	register_screen(&behaviour_display);
#endif
#ifdef LOG_DISPLAY_AVAILABLE
	register_screen(&log_display);
#endif
#ifdef MISC_DISPLAY_AVAILABLE
	register_screen(&misc_display);
#endif
#ifdef DISPLAY_ODOMETRIC_INFO
	register_screen(&odometric_display);
#endif
#ifdef DISPLAY_MMC_INFO
	register_screen(&mmc_display);
#endif
#ifdef DISPLAY_OS_AVAILABLE
	register_screen(&os_display);
#endif
#ifdef RAM_DISPLAY_AVAILABLE
	register_screen(&ram_display);
#endif
#ifdef DISPLAY_MAP_AVAILABLE
	register_screen(&map_display);
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
#ifdef PATHPLANING_DISPLAY
#ifdef BEHAVIOUR_PATHPLANING_AVAILABLE
	register_screen(&pathplaning_display);
#endif
#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
	register_screen(&bot_line_shortest_way_display);
#endif
#ifdef DISPLAY_DRIVE_CHESS_AVAILABLE
	register_screen(&drive_chess_display);
#endif
#endif
}

#endif // DISPLAY_AVAILABLE
