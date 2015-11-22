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
 * \file 	gui.h
 * \brief 	Display-GUI des Bots
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	12.02.2007
 */

#ifndef GUI_H_
#define GUI_H_

#ifdef DISPLAY_AVAILABLE
#include "ui/available_screens.h"

extern int8_t max_screens;	/**< Anzahl der zurzeit registrierten Screens */
extern void (* screen_functions[DISPLAY_SCREENS])(void);	/**< hier liegen die Zeiger auf die Display-Funktionen */

/**
 * Display-Screen Anzeige
 *
 * Zeigt einen Screen an und fuehrt die RC5-Kommandoauswertung aus, falls noch nicht geschehen.
 * \param screen Nummer des Screens, der angezeigt werden soll
 */
void gui_display(uint8_t screen);

/**
 * Display-Screen Initialisierung
 * Traegt die Anzeige-Funktionen in das Array ein.
 */
void gui_init(void);

#ifdef KEYPAD_AVAILABLE
/**
 * Startet eine neue Keypad-Eingabe.
 *
 * Abgeschlossen wird sie mit der Taste "Play", abgebrochen mit "Stop".
 * Nach  Abschluss wird die uebergebene Callback-Funktion aufgerufen
 * mit dem Eingabe-String als Parameter.
 * \param *callback	Zeiger auf eine Funktion, die die Eingabe bekommt
 * \param mode		voreingestellter Modus (0: alpha-num, 1: numerisch)
 * \param row		Zeile der Cursorposition fuer die Anzeige der Eingabe
 * \param col		Spalte der Cursorposition fuer die Anzeige der Eingabe
 */
void gui_keypad_request(void (* callback)(char * result), uint8_t mode, uint8_t row, uint8_t col);
#endif // KEYPAD_AVAILABLE

#ifdef DISPLAY_MISC_AVAILABLE
/**
 * Zeigt ein paar Infos an, die man nicht naeher zuordnen kann
 */
void misc_display(void);
#endif // DISPLAY_MISC_AVAILABLE

#ifdef DISPLAY_RESET_INFO_AVAILABLE
/**
 * Zeigt Informationen ueber den Reset an
 */
void reset_info_display(void);
#endif // DISPLAY_RESET_INFO_AVAILABLE

#ifdef DISPLAY_RAM_AVAILABLE
/**
 * Zeigt die aktuelle Speicherbelegung an.
 * Achtung, die Stackgroesse bezieht sich auf den Stack *dieser* Funktion!
 * Die Heapgroesse stimmt nur, wenn es dort keine Luecken gibt (z.b. durch free())
 */
void ram_display(void);
#endif // DISPLAY_RAM_AVAILABLE
#endif // DISPLAY_AVAILABLE
#endif // GUI_H_
