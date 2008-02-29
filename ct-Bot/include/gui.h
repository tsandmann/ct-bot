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

#ifndef gui_H_
#define gui_H_

#include "ui/available_screens.h"

#ifdef DISPLAY_AVAILABLE
extern int8 max_screens;	/*!< Anzahl der zurzeit registrierten Screens */
extern void (* screen_functions[DISPLAY_SCREENS])(void);	/*!< hier liegen die Zeiger auf die Display-Funktionen */
extern uint8_t EEPROM resetsEEPROM;	/*!< Reset-Counter-Wert im EEPROM */

/*! 
 * @brief 			Display-Screen Anzeige
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			12.02.2007	
 * @param screen 	Nummer des Screens, der angezeigt werden soll
 * Zeigt einen Screen an und fuehrt die RC5-Kommandoauswertung aus, falls noch nicht geschehen.
 */
void gui_display(int8 screen);

/*! 
 * @brief 	Display-Screen Initialisierung
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007	
 * Traegt die Anzeige-Funktionen in das Array ein.
 */
void gui_init(void);

#ifdef MISC_DISPLAY_AVAILABLE
	/*! 
	 * @brief	Zeigt ein paar Infos an, die man nicht naeher zuordnen kann
	 */
	void misc_display(void);
#endif

#ifdef RESET_INFO_DISPLAY_AVAILABLE
	extern uint8 reset_flag;	 /*!< Nimmt den Status von MCU(C)SR bevor dieses Register auf 0x00 gesetzt wird */
	/*! 
	 * @brief	Zeigt Informationen ueber den Reset an
	 */	
	void reset_info_display(void);
#endif

#ifdef RAM_DISPLAY_AVAILABLE
	/*!
	 * @brief	Zeigt die aktuelle Speicherbelegung an.
	 * 			Achtung, die Stackgroesse bezieht sich auf den Stack *dieser* Funktion!
	 * 			Die Heapgroesse stimmt nur, wenn es dort keine Luecken gibt (z.b. durch free())
	 */
	void ram_display(void);
#endif
#endif	// DISDISPLAY_AVAILABLE
#endif	// gui_H_
