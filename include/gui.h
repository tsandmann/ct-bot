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

/*! 
 * @brief 		Display-Screen Registrierung
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		12.02.2007	
 * @param fkt 	Zeiger auf eine Funktion, die den Display-Screen anzeigt 
 * Legt einen neuen Display-Screen an und haengt eine Anzeigefunktion ein.
 * Diese Funktion kann auch RC5-Kommandos behandeln. Wurde eine Taste ausgewertet, setzt man RC5_Code auf 0.
 */
int8 register_screen(void* fkt);

/*! 
 * @brief 			Display-Screen Anzeige
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			12.02.2007	
 * @param screen 	Nummer des Screens, der angezeigt werden soll
 * Zeigt einen Screen an und fuehrt die RC5-Kommandoauswertung aus, falls noch nicht geschehen.
 */
void gui_display(int8 screen);
#endif	// gui_H_
