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

/*! @file 	behaviour_scan.c
 * @brief 	Scannt die Umgebung und traegt sie in die Karte ein
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#include "bot-logic/bot-logik.h"
#include "map.h"
#include <math.h>

#ifdef BEHAVIOUR_SCAN_AVAILABLE
/*!
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_onthefly_behaviour(Behaviour_t *data){	
	#define ONTHEFLY_DIST_RESOLUTION 0.02		/*!< Alle wieviel gefahrene Strecke [m] soll die Karte aktualisiert werden. Achtung er prueft x und y getrennt, daher ist die tatsaechlich zurueckgelegte Strecke im worst case sqrt(2)*ONTHEFLY_DIST_RESOLUTION  */
	#define ONTHEFLY_ANGLE_RESOLUTION 10		/*!< Alle wieviel Gerad Drehung [m] soll die Karte aktualisiert werden */
	
	static float last_x, last_y, last_head;
	
	float diff_x = x_pos-last_x;
	float diff_y = y_pos-last_y;
	
	#ifdef MAP_AVAILABLE
		// Wenn der Bot faehrt, aktualisieren wir alles
		if ((diff_x > ONTHEFLY_DIST_RESOLUTION) ||( diff_y > ONTHEFLY_DIST_RESOLUTION)){
			update_map_location(x_pos,y_pos);
			update_map(x_pos,y_pos,heading,sensDistL,sensDistR);

			last_x=x_pos;
			last_y=y_pos;
			last_head=heading;
			return;
		} 
		
		// Wenn der bot nur dreht, aktualisieren wir nur die Blickstrahlen
		if ( fabs(last_head-heading) > ONTHEFLY_ANGLE_RESOLUTION ){
			update_map(x_pos,y_pos,heading,sensDistL,sensDistR);
			last_head=heading;
		}
		/*	if ((diff_x*diff_x + diff_y*diff_y > ONTHEFLY_DIST_RESOLUTION)||fabs(last_head-heading) > ONTHEFLY_ANGLE_RESOLUTION ){
				last_x=x_pos;
				last_y=y_pos;
				last_head=heading;
				print_map();
			}
		*/	
	#endif
}


#define BOT_SCAN_STATE_START 0
uint8 bot_scan_state = BOT_SCAN_STATE_START;	/*!< Zustandsvariable fuer bot_scan_behaviour */

/*!
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_behaviour(Behaviour_t *data){	
	#define BOT_SCAN_STATE_SCAN 1	

	#define ANGLE_RESOLUTION 5	/*!< Aufloesung fuer den Scan in Grad */

//	static uint16 bot_scan_start_angle; /*!< Winkel, bei dem mit dem Scan begonnen wurde */
	static float turned;		/*!< Winkel um den bereits gedreht wurde */
	
	static float last_scan_angle;		/*!< Winkel bei dem zuletzt gescannt wurde */
	
	float diff;
	
	switch (bot_scan_state){
		case BOT_SCAN_STATE_START:
		
			turned=0;
			last_scan_angle=heading-ANGLE_RESOLUTION;
			bot_scan_state=BOT_SCAN_STATE_SCAN;
			break;
		case BOT_SCAN_STATE_SCAN:
			diff = heading - last_scan_angle;
			if (diff < -180)
				diff+=360;
			if (diff*1.15 >= ANGLE_RESOLUTION){
				turned+= diff;
				last_scan_angle=heading;
				
				#ifdef MAP_AVAILBALE
					// Eigentlicher Scan hier
					update_map(x_pos,y_pos,heading,sensDistL,sensDistR);
					////////////
				#endif
				
			}
		
			if (turned >= 360-ANGLE_RESOLUTION)	// Ende erreicht
				bot_scan_state++;
			break;			
		default:
			bot_scan_state = BOT_SCAN_STATE_START;
			#ifdef MAP_AVAILBALE
				print_map();
			#endif
			return_from_behaviour(data);
			break;
	}
}

/*! 
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param Der aufrufer
 */
void bot_scan(Behaviour_t* caller){	

	bot_scan_state = BOT_SCAN_STATE_START;
	bot_turn(caller,360);
	switch_to_behaviour(0, bot_scan_behaviour,OVERRIDE);

//	update_map(x_pos,y_pos,heading,sensDistL,sensDistR);
//	print_map();	
}
#endif
