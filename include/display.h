/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	display.h 
 * @brief 	Routinen zur Displaysteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#ifndef display_H_
#define display_H_

#define DISPLAY_LENGTH	20	/*!< Wieviele Zeichen passen in eine Zeile */

extern uint8 display_update;		/*!< Muss das Display aktualisiert werden? */
//#ifdef DISPLAY_SCREENS_AVAILABLE
//	#define DISPLAY_SCREENS	5				/*!< Anzahl der Screens */
	#define DISPLAY_SCREEN_TOGGLE	42		/*!< Screen-Nummer, die zum wechseln verwendet wird */
	extern uint8 display_screen;	/*!< Welcher Screen soll gezeigt werden? */
//#endif

/*! 
 * Init Display
 */
void display_init(void);

/*! 
 * Zeigt einen String an 
 * @return -1 falls String zu Ende, 0 falls Zeile (20 zeichen) zu Ende
 */
//int display_string(char data[20]);

/*!
 * Loescht das ganze Display
 */
void display_clear(void);

/*!
 * Positioniert den Cursor
 * @param row Zeile
 * @param column Spalte
 */
void display_cursor (uint8 row, uint8 column) ;

/*!
 * Schreibt einen String auf das Display.
 * @param format Format, wie beim printf
 * @param ... Variable Argumentenliste, wie beim printf
 */
void display_printf(char *format, ...);

//void display_test();
#endif
