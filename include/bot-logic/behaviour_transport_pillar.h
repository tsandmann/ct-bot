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


/**
 * \file 	behaviour_transport_pillar.h
 * \brief 	Bot startet von einem Farb-Startpad und entdeckt die Welt, bis er auf ein anderes Farbpad stoesst.
 *
 * Er faehrt nun zwischen den beiden Farbpads hin und her, sammelt bei
 * Ankunft auf einem Farbpad ein in der Naehe befindliches Hindernis in sein Transportfach ein
 * und bringt dieses zum anderen Farbpad. Auf seinem Weg zur Abladestelle oder waehrend des
 * Entdeckens der Welt zur Farbpadsuche weicht er Hindernissen geschickt aus.
 * Es kann mittels des Wand-Explore Verhaltens nur mittels der Farbpads gefahren werden ohne sich
 * Koordinaten zu merken, womit er nicht unbedingt zielgerichtet von einem zum anderen Farbpad
 * fahren kann. Mittels Zuschalten der Verwendung der MAP-Koordinaten werden die Koords der Pads
 * gemerkt und sich dorthin ausgerichtet. Es kann nun mit einem Fahrverhalten zwischen beiden hin- und
 * hergefahren werden, wobei entweder der Wandfolger dient oder auch andere Fahrverhalten
 * Anwendung finden (Auswahl jeweils per Define).
 * Der Verhaltensstart erfolgt entweder via Remotecall oder Taste 9. Befindet sich der Bot auf einem Farbpad, so kann
 * via Taste 7 dieses als Zielpad definiert werden (Simfarben werden automatisch erkannt; Real ist dies schwierig, daher
 * manuell definierbar)
 * Zur Steuerung mit Tasten und der Positionsanzeigen wurde ein eigener Screen definiert
 *
 * \author 	Frank Menzel (menzelfr@gmx.net)
 * \date 	23.10.2007
 */

#ifndef BEHAVIOUR_TRANSPORT_PILLAR_H_
#define BEHAVIOUR_TRANSPORT_PILLAR_H_

#ifdef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
/**
 * Transport-Pillarverhalten  
 * \param *data	Der Verhaltensdatensatz
 */
void bot_transport_pillar_behaviour(Behaviour_t * data);

/**
 * Ruft das Pillarverhalten auf 
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_transport_pillar(Behaviour_t * caller);

/**
 * Routine zum Setzen der Zielkoordinaten auf der Zielposition/ Zielpad 
 * \param x X-World-Zielkoordinate
 * \param y Y-World-Zielkoordinate
 */
void bot_set_destkoords(float x, float y);

/**
 * Display zum Start der Transport_Pillar-Routinen
 */
void transportpillar_display(void);

#endif // BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
#endif // BEHAVIOUR_TRANSPORT_PILLAR_H_
