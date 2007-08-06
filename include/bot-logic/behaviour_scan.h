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
 * @file 	behaviour_scan.h
 * @brief 	Scannt die Umgebung und traegt sie in die Karte ein
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */

#ifndef BEHAVIOUR_SCAN_H_
#define BEHAVIOUR_SCAN_H_

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_SCAN_AVAILABLE

#define SENSOR_LOCATION		1		/*!< Quelle die das Verhalten bot_scan_onthefly nutzt, um die Karte zu aktualisieren: Ort des Bots */
#define SENSOR_DISTANCE		2		/*!< Quelle die das Verhalten bot_scan_onthefly nutzt, um die Karte zu aktualisieren: Distanzsensoren des Bots */

#define SCAN_ONTHEFLY_DIST_RESOLUTION 0.02		/*!< Alle wieviel gefahrene Strecke [m] soll die Karte aktualisiert werden. Achtung er prueft x und y getrennt, daher ist die tatsaechlich zurueckgelegte Strecke im worst case sqrt(2)*ONTHEFLY_DIST_RESOLUTION  */
#define SCAN_ONTHEFLY_ANGLE_RESOLUTION 10		/*!< Alle wieviel Gerad Drehung [m] soll die Karte aktualisiert werden */

extern uint8 scan_on_the_fly_source; 

#define bot_scan_onthefly( sensor) {scan_on_the_fly_source = sensor;}

/*!
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_onthefly_behaviour(Behaviour_t *data);

/*!
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param *data der Verhaltensdatensatz
 */
void bot_scan_behaviour(Behaviour_t *data);


/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung; muss registriert werden um
 * den erkannten Abgrund in die Map einzutragen
 */
void border_in_map_handler(void); 

/*! 
 * Der Roboter faehrt einen Vollkreis und scannt dabei die Umgebung
 * @param Der aufrufer
 */
void bot_scan(Behaviour_t* caller);
#endif
#endif /*BEHAVIOUR_SCAN_H_*/
