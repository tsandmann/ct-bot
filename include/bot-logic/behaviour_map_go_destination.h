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
 * @file 	behaviour_map_go_destination.h
 * @brief 	Verhalten
 *          -zum Setzen eines Ziels mittel RC-Taste
 *          -um zu diesem Ziel zu gehen von einer beliebigen MAP-Position aus unter Umgehung
 *           aller zu diesem Zeitpunkt bekannten Hindernisse; Pfadplanung erfolgt mittels globaler
 *           Potenzialfeldmethode und der berechneten Potenziale laut des erstellten MAP-Grids mit den
 *           Hindernis-Wahrscheinlichkeiten
 *
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	30.04.07
 */
 
#include "bot-logic/bot-logik.h"

#ifndef BEHAVIOUR_MAP_GO_DESTINATION_H_
#define BEHAVIOUR_MAP_GO_DESTINATION_H_


#ifdef BEHAVIOUR_MAP_GO_DESTINATION_AVAILABLE

/*! 
 * True wenn Border_map_behaviour einen Abgrund erkannt und markiert hatte;
 * muss vom Anwenderprog auf False gesetzt werden und dient zur Erkennung ob Verhalten
 * zugeschlagen hat 
 */
extern uint8 border_behaviour_fired;
 
/*! Verhalten zur Pfadplanung */
void bot_path_bestfirst(Behaviour_t *data);
void bot_path_bestfirst_behaviour(Behaviour_t *data);

/*! Verhalten zum Setzen der Zielkoordinaten */
void bot_set_destination(uint16 x, uint16 y);

/*! Verhalten zur Abgrunderkennung und eintragen in die Map */ 
void bot_set_border_in_map_behaviour(Behaviour_t *data);


/*! Verhalten zum echten Abfahren des bots nach der Punkte-Wegeliste laut Pfadplanung zum global gesetzten Ziel*/
void bot_gotoxy_behaviour_map(Behaviour_t *data);

/*! Display zum Aufruf der Mapgo-Routinen */
void mapgo_display(void);

int8_t register_emergency_proc(void* fkt);
void border_mapgo_handler(void);

#endif /* BEHAVIOUR_MAP_GO_DESTINATION_H_ */
#endif /* BEHAVIOUR_MAP_GO_DESTINATION_AVAILABLE */
