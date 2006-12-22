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

/*! @file 	remote_calls.h
 * @brief 	Liste mit Botenfkts, die man aus der Ferne aufrufen kann
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	19.12.06
*/

#ifndef REMOTE_CALLS_H_
#define REMOTE_CALLS_H_

#include "bot-logik.h"
#include <avr/pgmspace.h>

#define TEXT_LEN 15

// Die Kommandostruktur
typedef struct {
   void* (*func)(void *);      /*!< Zeiger auf die auszufuehrende Funktion*/
   uint8 len;					/*!< Anzahl der Bytes, die als Parameter kommen Und zwar ohne den obligatorischen caller-parameter*/
   char name[TEXT_LEN+1]; 	   /*!< Text, maximal TEXT_LEN Zeichen lang +  1 Zeichen terminierung*/
} call_t;

/*! Dieses Makro bereitet eine Botenfunktion als Remote-Call-Funktion vor. 
 * Der erste parameter ist der Funktionsname selbst
 * Der zweite Parameter ist die Anzahl an Bytes, die die Fkt erwartet.
 * Und zwar unabhaengig vom Datentyp. will man also einen uin16 uebergeben steht da 2
 * Will man einen Float uebergeben eine 4. Fuer zwei Floats eine 8, usw.
 */
#define PREPARE_REMOTE_CALL(func,number_of_bytes)  {(void*)func, number_of_bytes, #func }

/*! Hier muessen alle Funktionen rein, die Remote aufgerufen werden sollen
 * Ein eintrag erfolgt so:
 * PREPARE_REMOTE_CALL(BOTENFUNKTION,NUMBER_OF_BYTES)
 * Der letzte Eintrag brauch natuerlich Kein komma mehr
 * Alle Botenfunktionen muessen folgendem Schema entsprechen
 * void bot_xxx(Behaviour_t * caller, ...);
 * wieviele Parameter nach dem caller kommen ist voellig unerheblich. 
 * Allerdings muss man ihre gesamtlaeng in Byte kennen
 */
const call_t calls[] PROGMEM = {
   PREPARE_REMOTE_CALL(bot_turn,2),
   PREPARE_REMOTE_CALL(bot_gotoxy,8),
   PREPARE_REMOTE_CALL(bot_solve_maze,0) 
};

#endif /*REMOTE_CALLS_H_*/
