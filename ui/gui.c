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
 * @file 	gui.h
 * @brief 	Display-GUI des Bots
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007
 */
 
#include "ct-Bot.h"
#include "global.h"
#include "ui/available_screens.h"
#include "rc5.h"
#include "sensor.h"
#include "motor.h"
#include "display.h"
#include "mmc.h"
#include "log.h"
#include "bot-logic/bot-logik.h"
#include "gui.h"
#include <stdlib.h>

#ifdef DISPLAY_AVAILABLE

int8 max_screens = 0;	/*!< Anzahl der zurzeit registrierten Screens */
static void (* screen_functions[DISPLAY_SCREENS])(void) = {NULL};	/*!< hier liegen die Zeiger auf die Display-Funktionen */

/*! 
 * @brief 		Display-Screen Registrierung
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		12.02.2007	
 * @param fkt 	Zeiger auf eine Funktion, die den Display-Screen anzeigt 
 * Legt einen neuen Display-Screen an und haengt eine Anzeigefunktion ein.
 * Diese Funktion kann auch RC5-Kommandos behandeln. Wurde eine Taste ausgewertet, setzt man RC5_Code auf 0.
 */
int8 register_screen(void* fkt){
	if (max_screens == DISPLAY_SCREENS) return -1;	// sorry, aber fuer dich ist kein Platz mehr da :(
	int8 screen_nr = max_screens++;		// neuen Screen hinten anfuegen
	screen_functions[screen_nr] = fkt;	// Pointer im Array speichern
	return screen_nr;
}

/*! 
 * @brief 			Display-Screen Anzeige
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			12.02.2007	
 * @param screen 	Nummer des Screens, der angezeigt werden soll
 * Zeigt einen Screen an und fuehrt die RC5-Kommandoauswertung aus, falls noch nicht geschehen.
 */
void gui_display(int8 screen){
//	rc5_control();	// Vielleicht waere der Aufruf hier uebersichtlicher?
	/* Gueltigkeit der Screen-Nr. pruefen und Anzeigefunktion aufrufen, falls Screen belegt ist */
	if (screen < max_screens && screen_functions[screen] != NULL) screen_functions[screen]();
	if (RC5_Code != 0) default_key_handler();	// falls rc5-Code noch nicht abgearbeitet, Standardbehandlung ausfuehren
	RC5_Code = 0;	// fertig, RC5-Puffer loeschen
}

/*! 
 * @brief 	Display-Screen Initialisierung
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007	
 * Traegt die Anzeige-Funktionen in das Array ein.
 */
void gui_init(void){
	#ifdef SENSOR_DISPLAY_AVAILABLE 	
		register_screen(&sensor_display);
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
	#ifdef RESET_INFO_DISPLAY_AVAILABLE
		register_screen(&reset_info_display);
	#endif 
	#ifdef RAM_DISPLAY_AVAILABLE
		register_screen(&ram_display);
	#endif
}

#endif	// DISPLAY_AVAILABLE
