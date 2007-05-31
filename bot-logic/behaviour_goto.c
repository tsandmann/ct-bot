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

/*! @file 	behaviour_goto.c
 * @brief 	Bot faehrt einen punkt an
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/



#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_GOTO_AVAILABLE
#include <stdlib.h>


#define MOT_GOTO_MAX  	 4 			/*!< Richtungsaenderungen, bis goto erreicht sein muss */
#define GOTO_REACHED	 2			/*!< Wenn Encoder-Distanz <= GOTO_REACHED dann stop */
#define GOTO_SLOW		 4			/*!< Wenn Encoder-Distanz < GOTO_SLOW dann langsame Fahrt */
#define GOTO_NORMAL	10			/*!< Wenn Encoder-Distanz < GOTO_NORMAL dann normale Fahrt */
#define GOTO_FAST		40			/*!< Wenn Encoder-Distanz < GOTO_FAST dann schnelle Fahrt, sonst maximale Fahrt */


int16 mot_l_goto=0;		/*!< Speichert wie weit der linke Motor drehen soll */
int16 mot_r_goto=0;		/*!< Speichert wie weit der rechte Motor drehen soll */


/*!
 * Kuemmert sich intern um die Ausfuehrung der goto-Kommandos,
 * @param *data der Verhaltensdatensatz
 * @see bot_goto()
 */
void bot_goto_behaviour(Behaviour_t *data){
	static int16 mot_goto_l=0;	/*!< Muss der linke Motor noch drehen?  */
	static int16 mot_goto_r=0;	/*!< Muss der rechte Motor noch drehen?  */

  	int16 diff_l;	/* Restdistanz links */
	int16 diff_r; /* Restdistanz rechts */

	/* Sind beide Zaehler Null und ist die Funktion active 
	 * -- sonst waeren wir nicht hier -- 
	 * so ist dies der erste Aufruf ==> initialisieren */	
	if (( mot_goto_l ==0) && ( mot_goto_r ==0)){
		/* Zaehler einstellen */
		if (mot_l_goto !=0) 
			mot_goto_l= MOT_GOTO_MAX; 
		if (mot_r_goto !=0) 
			mot_goto_r=MOT_GOTO_MAX;

		/* Encoder zuruecksetzen */
		sensEncL=0;
		sensEncR=0;			
	}		
	
	/* Motor L hat noch keine MOT_GOTO_MAX Nulldurchgaenge gehabt */
	if (mot_goto_l >0){
		diff_l = sensEncL - mot_l_goto;		/* Restdistanz links */

		if (abs(diff_l) <= GOTO_REACHED){				/* 2 Encoderstaende Genauigkeit reicht */
			speedWishLeft = BOT_SPEED_STOP;	/* Stop */
			mot_goto_l--;					/* wie Nulldurchgang behandeln */
		}else if (abs(diff_l) < GOTO_SLOW)
			speedWishLeft= BOT_SPEED_SLOW;
		else if (abs(diff_l) < GOTO_NORMAL)
			speedWishLeft= BOT_SPEED_NORMAL;
		else if (abs(diff_l) < GOTO_FAST)
			speedWishLeft= BOT_SPEED_FAST;
		else speedWishLeft= BOT_SPEED_MAX;

		// Richtung	
		if (diff_l>0) {		// Wenn uebersteuert,
			speedWishLeft= -speedWishLeft;	//Richtung umkehren
		}
		
		// Wenn neue Richtung ungleich alter Richtung
		if (((speedWishLeft<0)&& (direction.left == DIRECTION_FORWARD))|| ( (speedWishLeft>0) && (direction.left == DIRECTION_BACKWARD) ) ) 
			mot_goto_l--;		// Nulldurchgang merken
	} 
	
	// Motor R hat noch keine MOT_GOTO_MAX Nulldurchgaenge gehabt
	if (mot_goto_r >0){
		diff_r = sensEncR - mot_r_goto;	/* Restdistanz rechts */

		if (abs(diff_r) <= GOTO_REACHED){			// 2 Encoderstaende Genauigkeit reicht
			speedWishRight = BOT_SPEED_STOP;	//Stop
			mot_goto_r--;			// wie Nulldurchgang behandeln
		}else if (abs(diff_r) < GOTO_SLOW)
			speedWishRight= BOT_SPEED_SLOW;
		else if (abs(diff_r) < GOTO_NORMAL)
			speedWishRight= BOT_SPEED_NORMAL;
		else if (abs(diff_r) < GOTO_FAST)
			speedWishRight= BOT_SPEED_FAST;
		else speedWishRight= BOT_SPEED_MAX;

		// Richtung	
		if (diff_r>0) {		// Wenn uebersteurt,
			speedWishRight= -speedWishRight;	//Richtung umkehren
		}

		// Wenn neue Richtung ungleich alter Richtung
		if (((speedWishRight<0)&& (direction.right == DIRECTION_FORWARD))|| ( (speedWishRight>0) && (direction.right == DIRECTION_BACKWARD) ) ) 
			mot_goto_r--;		// Nulldurchgang merken
	} 
	
	/* Sind wir fertig, dann Regel deaktivieren */
	if ((mot_goto_l == 0) && (mot_goto_r == 0)){
		return_from_behaviour(data);
	}
}

/*!
 * Drehe die Raeder um die gegebene Zahl an Encoder-Schritten weiter
 * @param caller	Der Aufrufer
 * @param left		Schritte links
 * @param right		Schritte rechts
 */
void bot_goto(Behaviour_t * caller, int16 left, int16 right){
	// Zielwerte speichern
	mot_l_goto=left; 
	mot_r_goto=right;

	switch_to_behaviour(caller,bot_goto_behaviour,OVERRIDE);	
}
#endif
