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
#include <stdarg.h>
#include <stdio.h>

/* definiere DISPLAY_REMOTE_AVAILABLE, wenn die Display-Daten per TCP an das 
 * Simulationsprogramm gesendet werden sollen.  Wenn es nicht gesetzt ist, 
 * dann erscheint die LCD Ausgabe auf der Startkonsole */


#ifdef DISPLAY_REMOTE_AVAILABLE
 #include "command.h"
 #include "bot-2-sim.h"  
#else
 #ifdef WIN32
  #include <windows.h>
 #endif	/* WIN32 */
#endif

#ifdef DISPLAY_AVAILABLE

/*! Puffergroesse fuer eine Zeile in bytes */
#define DISPLAY_BUFFER_SIZE	(DISPLAY_LENGTH + 1)

//uint8 display_update=0;	/*!< Muss das Display aktualisiert werden? */
uint8 display_screen=0;	/*!< zurzeit aktiver Displayscreen */

char display_buf[DISPLAY_BUFFER_SIZE];	/*!< Pufferstring fuer Displayausgaben */

#ifdef DISPLAY_REMOTE_AVAILABLE
    #define CLEAR              command_write(CMD_AKT_LCD, SUB_LCD_CLEAR, NULL, NULL,0)
    #define POSITION(Ze, Sp)   {Ze--; Sp--; command_write(CMD_AKT_LCD, SUB_LCD_CURSOR, (int16*)&(Sp),(int16*) &(Ze),0);}
    #define printf(data)       {command_write_data(CMD_AKT_LCD, SUB_LCD_DATA, NULL, NULL, (data));}
#else
 #ifdef WIN32
    static void clrscr(void);
    static void gotoxy(int x, int y);
	#define POSITION(Ze, Sp)   gotoxy(Sp, Ze)
	#define CLEAR              clrscr()
 #else 
	#define POSITION(Ze, Sp)   printf("\033[%d;%dH",Ze,Sp)		/*!< Befehl um eine Posion anzuspringen */
	#define CLEAR              printf("\033[2J")				/*!< Befehl um das display zu loeschen */
 #endif 
#endif


/*!
 * Loescht das ganze Display
 */
void display_clear(void){
	CLEAR;
}

/*!
** LCD_Cursor: Positioniert den LCD-Cursor bei "row", "column".
*/
void display_cursor (uint8 row, uint8 column) {
	int16 r=row, c=column;	// Cast auf int16
	POSITION(r, c);
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
int display_string(char data[DISPLAY_LENGTH]){
	printf(data);
	return -1;
}

/*!
 * Schreibt einen String auf das Display.
 * @param format Format, wie beim printf
 * @param ... Variable Argumentenliste, wie beim printf
 */
void display_printf(char *format, ...) {
	
	va_list	args;
	
	/* Sicher gehen, das der zur Verfuegung stehende Puffer nicht
	 * ueberschrieben wird.
	 */
	va_start(args, format);
	vsnprintf(display_buf, DISPLAY_BUFFER_SIZE, format, args);
	va_end(args);

	printf(display_buf);	
	
	return;
}

#ifndef DISPLAY_REMOTE_AVAILABLE
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
	point.X = x-1; point.Y = y-1;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), point);
	return;
}

#endif	/* WIN32 */
#endif  /* !DISPLAY_REMOTE_AVAILABLE */

#endif
#endif
