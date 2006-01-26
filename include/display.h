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

#define DISPLAY_BUFFER	30			/*!< Groesse des Display-Strings */

extern volatile char display_update;	/*!< Muss das Display aktualisiert werden? */
extern char display_buf[DISPLAY_BUFFER];		/*!< Pufferstring fuer Displayausgaben */
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
void display_cursor (int row, int column) ;

/*! 
 * Zeigt den String an, der in display_buffer steht. 
 * @return 0 falls 0x00-Zeichen erreicht; -1, falls DISPLAY_LENGTH oder DISPLAY_BUFFER Zeichen ausgegeben wurden
 */
int display_buffer();
//void display_test();
#endif
