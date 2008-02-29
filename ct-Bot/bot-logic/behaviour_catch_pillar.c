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


/*! @file 	behaviour_catch_pillar.c
 * @brief 	sucht nach einer Dose und fängt sie ein
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	08.12.06
*/


#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE

#include <math.h>

#define START 0
#define SEARCH_LEFT 1
#define LEFT_FOUND 2
#define TURN_MIDDLE 3
#define GO 4
#define END 99

// Zustände für das Ausladeverhalten START und END sind bereits weiter oben definiert  
#define GO_BACK 1
#define CLOSE_DOOR 2

static uint8 catch_pillar_state=START;		/*!< Statusvariable für das Einfang-Verhalten */
static uint8 unload_pillar_state=START;		/*!< Statusvariable für das Auslade-Verhalten */
static int16 startangle=0;      			/*!< gemerkter Anfangswinkel einfach als Integer */
/*!
 * Fange eine Dose ein
 * @param *data der Verhaltensdatensatz
 */
void bot_catch_pillar_behaviour(Behaviour_t *data){
	static uint8 cancelcheck=False; 
	static float angle;

	switch (catch_pillar_state){
		case START:
		       startangle=heading;
		       cancelcheck=False;
		       catch_pillar_state=SEARCH_LEFT;
		     break;
		
		case SEARCH_LEFT:
		    // Nach 1x Rundumsuche und nix gefunden ist Schluss; Dazu wird der Check zum Start
		    // nach Ueberschreiten der 5Grad-Toleranz eingeschaltet und die Drehung dann beendet wenn
		    // der Winkel wieder innerhalb dieser Toleranz liegt
	        if (cancelcheck) {
	          if (fabs(heading - startangle) < 5 )
		        // Damit nicht endlos rundum gedreht wird Abbruch, wenn kein Gegenstand gefunden
		        catch_pillar_state = END;	          
	        } else {
	        	if (fabs(heading - startangle) >= 5) 
	        	  cancelcheck=True;	        	
	        }
		
		    // linker Sensor muss was sehen, der rechte aber nicht
			if (sensDistL < MAX_PILLAR_DISTANCE && sensDistR > MAX_PILLAR_DISTANCE){	// sieht der linke Sensor schon was?
				angle=heading;
				catch_pillar_state=LEFT_FOUND;
			} else
				speedWishLeft=-BOT_SPEED_SLOW;
				speedWishRight=+BOT_SPEED_SLOW;
				//bot_turn(data,5);	// Ein Stueck drehen
			break;
			
		case LEFT_FOUND:
		    // links und rechts darf nicht gleichzeitig was gesehen werden, sonst weiter mit drehen
			if (sensDistL < MAX_PILLAR_DISTANCE && sensDistR < MAX_PILLAR_DISTANCE){
		    	catch_pillar_state=SEARCH_LEFT;
		    	break;
		    }	
		    
			if (sensDistR < MAX_PILLAR_DISTANCE){	// sieht der rechte Sensor schon was?
				angle= heading- angle;
				if (angle < 0)
					angle+=360;
				angle= heading - angle/2;
				catch_pillar_state=TURN_MIDDLE;
//				bot_turn(data,-angle/2);
			} else
				speedWishLeft=-BOT_SPEED_SLOW;
				speedWishRight=+BOT_SPEED_SLOW;
//				bot_turn(data,5);	// Eins Stueck drehen
			break;
			
		case TURN_MIDDLE:
				if (fabs(heading - angle) > 2){
					speedWishLeft=+BOT_SPEED_SLOW;
					speedWishRight=-BOT_SPEED_SLOW;
				} else {
					bot_servo(data,SERVO1,DOOR_OPEN); // Klappe auf
					catch_pillar_state=GO;
				}
			break;
			
		case GO:
			if (sensTrans ==0){
				speedWishLeft=+BOT_SPEED_SLOW;
				speedWishRight=+BOT_SPEED_SLOW;
				// nicht endlos vorwaertslaufen, weiter mit Suche bis Drehende 
				if (sensDistL < MAX_PILLAR_DISTANCE && sensDistR < MAX_PILLAR_DISTANCE)
		    		catch_pillar_state=SEARCH_LEFT;			  
			}
			else {
				  bot_servo(data,SERVO1,DOOR_CLOSE);
				  catch_pillar_state=END;
			 }

			break;

		default:
			catch_pillar_state=START;  
			return_from_behaviour(data);
			break;
	}
}

/*!
 * Fange ein Objekt ein
 * @param caller Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_catch_pillar(Behaviour_t * caller){
	catch_pillar_state=START;
	unload_pillar_state=END;		// Sicherheitshalber das Unloade-Verhalten auf Ende setzen
	// Zielwerte speichern
	switch_to_behaviour(caller,bot_catch_pillar_behaviour,OVERRIDE);
}

/*!
 * Gibt die Dose wieder aus, Entladevorgang
 * @param *data der Verhaltensdatensatz
 */
void bot_unload_pillar_behaviour(Behaviour_t *data){

	switch (unload_pillar_state){
		case START:
             if (sensTrans ==1){  // ist was im Bauch gehts los
             	unload_pillar_state=GO_BACK;
             	// ist was drin, dann Klappe auf und danach Rueckwaerts
             	bot_servo(data,SERVO1,DOOR_OPEN); // Klappe auf
             } else	// nix zu tun 
             	unload_pillar_state=END;
			break;
        case GO_BACK:
              bot_drive_distance(data,0,-BOT_SPEED_FOLLOW,10);// 10cm rueckwaerts nur bei Hindernis
              unload_pillar_state=CLOSE_DOOR;
            break;
		case CLOSE_DOOR:
		     bot_servo(data,SERVO1,DOOR_CLOSE);
		     unload_pillar_state=END;
		    break;
		default:
			unload_pillar_state=START;// bei Umschaltung via Verhaltensscreen ist dies notwendig
			return_from_behaviour(data);
			break;
	}
}





/*!
 * Entlaedt das Objekt wieder
 * @param caller Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_unload_pillar(Behaviour_t * caller){
	unload_pillar_state=START;
	catch_pillar_state=END;		// Sicherheitshalber das catch-Verhalten auf Ende setzen
	// Zielwerte speichern
	switch_to_behaviour(caller,bot_unload_pillar_behaviour,OVERRIDE);
}

#endif
