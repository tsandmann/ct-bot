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

/*! 
 * @file 	behaviour_scan.c
 * @brief 	Scannt die Umgebung und traegt sie in die Karte ein
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */

#include "bot-logic/bot-logik.h"
#include "map.h"
#include <math.h>

#ifdef BEHAVIOUR_SCAN_AVAILABLE

#ifndef MAP_AVAILABLE
	#error MAP_AVAILABLE muss an sein, damit behaviour_scan.c etwas sinnvolles tun kann
#endif

uint8 scan_on_the_fly_source = SENSOR_LOCATION || SENSOR_DISTANCE; 

/*!
 * Der Roboter aktualisiert kontinuierlich seine Karte
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_onthefly_behaviour(Behaviour_t *data){		
		static float last_x, last_y, last_head;
		
		float diff_x = fabs(x_pos-last_x);
		float diff_y = fabs(y_pos-last_y);

		// Wenn der Bot faehrt, aktualisieren wir alles
		if ((diff_x > SCAN_ONTHEFLY_DIST_RESOLUTION) ||( diff_y > SCAN_ONTHEFLY_DIST_RESOLUTION)){
			if ((scan_on_the_fly_source & SENSOR_LOCATION) != 0)
				update_map_location(x_pos,y_pos);
			if ((scan_on_the_fly_source & SENSOR_DISTANCE) != 0)
				update_map(x_pos,y_pos,heading,sensDistL,sensDistR);

			last_x=x_pos;
			last_y=y_pos;
			last_head=heading;
			return;
		} 
	
		float diff_head = fabs(last_head-heading);		
		// Wenn der bot nur dreht, aktualisieren wir nur die Blickstrahlen
		if (( diff_head > SCAN_ONTHEFLY_ANGLE_RESOLUTION) && ((scan_on_the_fly_source & SENSOR_DISTANCE) != 0)  ){
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
}


#define BOT_SCAN_STATE_START 0
static uint8 bot_scan_state = BOT_SCAN_STATE_START;	/*!< Zustandsvariable fuer bot_scan_behaviour */

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
				
				#ifdef MAP_AVAILABLE
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
			#ifdef MAP_AVAILABLE
				print_map();
			#endif
			return_from_behaviour(data);
			break;
	}
}

/*! 
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param caller der Aufrufer
 */
void bot_scan(Behaviour_t* caller){	

	bot_scan_state = BOT_SCAN_STATE_START;
	bot_turn(caller,360);
	switch_to_behaviour(0, bot_scan_behaviour,OVERRIDE);

//	update_map(x_pos,y_pos,heading,sensDistL,sensDistR);
//	print_map();	
}

/*! 
 * eigentliche Aufrufroutine zum Eintragen des Abgrundes in den Mapkoordinaten, wenn
 * die Abgrundsensoren zugeschlagen haben  
 * @param x aktuelle Bot-Koordinate als Welt- (nicht Map-) Koordinaten
 * @param y aktuelle Bot-Koordinate als Welt- (nicht Map-) Koordinaten
 * @param head Blickrichtung 
 */ 
void update_map_hole(float x, float y, float head) {
	float h= head * M_PI /180;  // Umrechnung in Bogenmass 
	// uint8 border_behaviour_fired=False;

	if (sensBorderL > BORDER_DANGEROUS) {
		//Ort des linken Sensors in Weltkoordinaten (Mittelpunktentfernung)
		float Pl_x = x - (DISTSENSOR_POS_B_SW * sin(h));
		float Pl_y = y + (DISTSENSOR_POS_B_SW * cos(h));
		update_map_sensor_hole(Pl_x,Pl_y,h); // Eintragen des Loches in die Map	   
	}
	
	if (sensBorderR > BORDER_DANGEROUS) {        
		//Ort des rechten Sensors in Weltkoordinaten (Mittelpunktentfernung)
		float Pr_x = x + (DISTSENSOR_POS_B_SW * sin(h));
		float Pr_y = y - (DISTSENSOR_POS_B_SW * cos(h));
		update_map_sensor_hole(Pr_x,Pr_y,h); // Eintragen des Loches in die Map
	}
}

/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung; muss registriert werden um
 * den erkannten Abgrund in die Map einzutragen
 */
void border_in_map_handler(void) {
	// Routine muss zuerst checken, ob on_the_fly auch gerade aktiv ist, da nur in diesem
	// Fall etwas gemacht werden muss
	if (!behaviour_is_activated(bot_scan_onthefly_behaviour)) 
		return;

	/* bei Abgrunderkennung Position des Abgrundes in Map mit -128 eintragen */
	update_map_hole(x_pos,y_pos,heading);
}

#endif	// BEHAVIOUR_SCAN_AVAILABLE
