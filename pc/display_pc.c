/*! @file 	display_pc.c 
 * @brief 	Routinen zur Displaysteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include "display.h"
#include <stdio.h>      /* for printf() and fprintf() */


#ifdef DISPLAY_AVAILABLE

#define DISPLAY_LENGTH	20

volatile char display_update=0;	///< Muss das Display aktualisiert werden?

char display_buf[DISPLAY_BUFFER];		///< Pufferstring für Displayausgaben


#ifdef WIN32
	#define ESC		0x1B
	#define POSITION(Ze, Sp)   //printf("%c[%d;%dH",ESC,Ze,Sp)
	#define CLEAR              //printf("%c[2J",ESC)
#else 
	#define POSITION(Ze, Sp)   printf("\033[%d;%dH",Ze,Sp)
	#define CLEAR              printf("\033[2J")
#endif 

/*!
 * Löscht das ganze Display
 */
void display_clear(void){
	CLEAR;
}

/*
** LCD_Cursor: Position the LCD cursor at "row", "column".
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
 * @return -1 falls string zuende 0 falls Zeile (20 zeichen) zuende
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


#endif
#endif
