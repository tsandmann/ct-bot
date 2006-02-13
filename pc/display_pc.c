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

/*! @file 	display_pc.c 
 * @brief 	Routinen zur Displaysteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include "display.h"
#include <stdio.h>      /* for printf() and fprintf() */

#ifdef WIN32
#include <windows.h>
#endif	/* WIN32 */

#ifdef DISPLAY_AVAILABLE

#define DISPLAY_LENGTH	20			/*!< Wieviele Zeichen passen in eine Zeile */

volatile char display_update=0;	/*!< Muss das Display aktualisiert werden? */

char display_buf[DISPLAY_BUFFER];		/*!< Pufferstring fuer Displayausgaben */


#ifdef WIN32
	#define ESC		0x1B
	#define POSITION(Ze, Sp)   gotoxy(Sp, Ze)
	#define CLEAR              clrscr()
#else 
	#define POSITION(Ze, Sp)   printf("\033[%d;%dH",Ze,Sp)		/*!< Befehl um eine Posion anzuspringen */
	#define CLEAR              printf("\033[2J")				/*!< Befehl um das display zu loeschen */
#endif 

#ifdef WIN32
static void clrscr(void);
static void gotoxy(int x, int y);
#endif	/* WIN32 */

/*!
 * Loescht das ganze Display
 */
void display_clear(void){
	CLEAR;
}

/*!
** LCD_Cursor: Positioniert den LCD-Cursor bei "row", "column".
*/
void display_cursor (int row, int column) {
	POSITION(row, column	);
}

/*! 
 * Init Display
 */
void display_init(void){
	CLEAR;
}

/*! 
 * Zeigt einen String an 
 * @return -1, falls String zu Ende; 0, falls Zeile (20 Zeichen) zu Ende
 */
int display_string(char data[20]){
	printf(data);
	return -1;
}

/*! 
 * Zeigt den String an, der in display_buffer steht. 
 * @return 0 falls 0x00-Zeichen erreicht; -1, falls DISPLAY_LENGTH oder DISPLAY_BUFFER Zeichen ausgegeben wurden
 */
int display_buffer(){
	printf(display_buf);
	return 0;
}

#ifdef WIN32

/*!
 * Loescht die Konsole.
 */
static void clrscr(void) {
	COORD coordScreen = { 0, 0 };
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(hConsole, &csbi);
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	FillConsoleOutputCharacter(	hConsole, 
								TEXT(' '),
								dwConSize,
								coordScreen,
								&cCharsWritten);
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	FillConsoleOutputAttribute(	hConsole,
                             	csbi.wAttributes,
                             	dwConSize,
                             	coordScreen,
                             	&cCharsWritten);
	SetConsoleCursorPosition(hConsole, coordScreen);
	return;
}

/*!
 * Springt an die angegebenen Koordinaten in der Konsole.
 * @param x Spalte
 * @param y Zeile
 */
static void gotoxy(int x, int y) {
	COORD point;
	point.X = x; point.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), point);
	return;
}

#endif	/* WIN32 */

#endif
#endif
