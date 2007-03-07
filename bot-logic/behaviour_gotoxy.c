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

/*! @file 	behaviour_gotoxy.c
 * @brief 	Bot faehrt eine Position an
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
#include <math.h>




/* Parameter fuer bot_gotoxy_behaviour-Verhalten */
float target_x;				/*!< Zielkoordinate X */
float target_y;				/*!< Zielkoordinate Y */
float initialDiffX;			/*!< Anfangsdifferenz in X-Richtung */
float initialDiffY;			/*!< Anfangsdifferenz in Y-Richtung */


/*!
 * Auslagerung der Berechnung der benoetigten Drehung aus dem gotoxy_behaviour
 * @param xDiff
 * @param yDiff
 */
int16 bot_gotoxy_calc_turn(float xDiff, float yDiff){
	float newHeading;
	if(fabs(yDiff)>0.001 && fabs(xDiff)>0.001){
		newHeading=atan(yDiff/xDiff)*180/(M_PI);
		if (xDiff<0) 
			newHeading+=180;
	} else{
		if(fabs(xDiff)<=0.001) 
			newHeading=(yDiff>0)?90:(-90);
		else 
			newHeading=(xDiff>0)?0:(180);
	}

	int16 toTurn=(int16)newHeading-heading;		
	if (toTurn > 180) toTurn-=360;
	if (toTurn < -180) toTurn+=360;

	return toTurn;
}

/*!
 * Das Verhalten faehrt von der aktuellen Position zur angegebenen Position (x/y)
 * @param *data der Verhaltensdatensatz
 * Verbessert von Thomas Noll, Jens Schoemann, Ben Horst (Philipps-Universitaet Marburg)
 */
void bot_gotoxy_behaviour(Behaviour_t *data){
	#define INITIAL_TURN 	0
	#define GOTO_LOOP 	1
	#define CORRECT_HEAD	2
	#define REACHED_POS	3
	static int16 speedLeft=BOT_SPEED_FOLLOW;
	static int16 speedRight=BOT_SPEED_FOLLOW;
	static int8 gotoState=INITIAL_TURN;
	/* aus aktueller Position und Ziel neuen Zielwinkel berechnen */
	float xDiff=target_x-x_pos;
	float yDiff=target_y-y_pos;
	
	if(xDiff*initialDiffX <0 || yDiff*initialDiffY <0){	// Hier kann noch verbessert werden
		gotoState=REACHED_POS;			// z.B. Abfragen statt *-Operation
		speedWishLeft=BOT_SPEED_STOP;		// bzw. neue Drehung statt Stehenbleiben
		speedWishRight=BOT_SPEED_STOP;
	}

	
	switch(gotoState) {
		case INITIAL_TURN:
			gotoState=GOTO_LOOP;
			bot_turn(data,bot_gotoxy_calc_turn(xDiff,yDiff));
			break;

		case GOTO_LOOP:
			/* Position erreicht? */
			if ((xDiff)<10 || (yDiff)<10) {
				gotoState=CORRECT_HEAD;
				bot_turn(data,bot_gotoxy_calc_turn(xDiff,yDiff));
				break;
			}
			speedWishLeft=speedLeft;
			speedWishRight=speedRight;
			break;

		case CORRECT_HEAD:
			/* Position erreicht? */
			if ((xDiff)<3 && (yDiff)<3) {
				gotoState=REACHED_POS;
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				break;
			}
			speedWishLeft=BOT_SPEED_SLOW;
			speedWishRight=BOT_SPEED_SLOW;
			break;

		case REACHED_POS:
			gotoState=INITIAL_TURN;
			return_from_behaviour(data);
			break;
	}

}

/*!
 * Botenfunktion: Das Verhalten faehrt von der aktuellen Position zur angegebenen Position (x/y)
 * @param caller Aufrufendes Verhalten
 * @param x X-Ordinate an die der Bot fahren soll
 * @param y Y-Ordinate an die der Bot fahren soll
 */
void bot_gotoxy(Behaviour_t *caller, float x, float y){
	target_x=x;
	target_y=y;
	initialDiffX=x-x_pos;
	initialDiffY=y-y_pos;
	switch_to_behaviour(caller, bot_gotoxy_behaviour, NOOVERRIDE);
}
#endif
