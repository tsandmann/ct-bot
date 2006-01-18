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

/*! @file 	delay_pc.c
 * @brief 	Hilfsroutinen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#include "ct-Bot.h"

#ifdef PC


#include <stdio.h>      /* for printf() and fprintf() */

/*!
 * Warte 100 ms
 */
void delay_100ms(void){
	printf("delay_100ms() NOT Implemented Yet");
}


/*!
 * Verzoegert um ms Millisekunden
 * Wenn RTC_AVAILABLE, dann ueber rtc, sonst ueber delay_100ms.
 * ==> Aufloesung ohne rtc: 100-ms-Schritte, mit rtc: 5-ms-Schritte
 * @param ms Anzahl der Millisekunden
 */
void delay(int ms){
	printf("delay() NOT Implemented Yet");
}
#endif
