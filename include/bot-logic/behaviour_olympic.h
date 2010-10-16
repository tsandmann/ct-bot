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
 * @file 	behaviour_olympic.h
 * @brief 	Bot sucht Saeulen und faehrt dann Slalom
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
 */

#ifndef BEHAVIOUR_OLYMPIC_H_
#define BEHAVIOUR_OLYMPIC_H_

#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE

/*!
 * Das Verhalten setzt sich aus 3 Teilverhalten zusammen:
 * Nach Licht suchen, auf das Licht zufahren, im Licht Slalom fahren.
 * @param *data	Verhaltensdatensatz
 */
void bot_olympic_behaviour(Behaviour_t * data);

/*!
 * Das Verhalten laesst den Roboter den Raum durchsuchen.
 * Das Verhalten hat mehrere unterschiedlich Zustaende:
 * 1. Zu einer Wand oder einem anderen Hindernis fahren.
 * 2. Zu einer Seite drehen, bis der Bot parallel zur Wand ist.
 * Es macht vielleicht Sinn, den Maussensor auszulesen, um eine Drehung um
 * einen bestimmten Winkel zu realisieren. Allerdings muesste dafuer auch der
 * Winkel des Bots zur Wand bekannt sein.
 * 3. Eine feste Strecke parallel zur Wand vorwaerts fahren.
 * Da bot_glance_behaviour abwechselnd zu beiden Seiten schaut, ist es fuer die Aufgabe,
 * einer Wand auf einer Seite des Bots zu folgen, nur bedingt gewachsen und muss
 * evtl. erweitert werden.
 * 4. Senkrecht zur Wand drehen.
 * Siehe 2.
 * 5. Einen Bogen fahren, bis der Bot wieder auf ein Hindernis stoesst.
 * Dann das Ganze von vorne beginnen, nur in die andere Richtung und mit einem
 * weiteren Bogen. So erforscht der Bot einigermassen systematisch den Raum.
 *
 * Da das Verhalten jeweils nach 10ms neu aufgerufen wird, muss der Bot sich
 * 'merken', in welchem Zustand er sich gerade befindet.
 *
 * @param *data	Verhaltensdatensatz
 */
void bot_explore_behaviour(Behaviour_t * data);

/*!
 * Das Verhalten laesst den Bot einen Slalom fahren.
 * @see bot_do_slalom()
 * @param *data	Verhaltensdatensatz
 */
void bot_do_slalom_behaviour(Behaviour_t * data);


/*!
 * Das Verhalten laesst den Bot zwischen einer Reihe beleuchteter Saeulen Slalom fahren.
 * Das Verhalten ist wie bot_explore() in eine Anzahl von Teilschritten unterteilt.
 * 1. Vor die aktuelle Saeule stellen, so dass sie zentral vor dem Bot und ungefaehr
 * COL_CLOSEST (100 mm) entfernt ist.
 * 2. 90 Grad nach rechts drehen.
 * 3. In einem relativ engen Bogen 20 cm weit fahren.
 * 4. Auf der rechten Seite des Bot nach einem Objekt suchen, dass
 * 	a) im rechten Sektor des Bot liegt, also zwischen -45 Grad und -135 Grad zur Fahrtrichtung liegt,
 * 	b) beleuchtet und
 * 	c) nicht zu weit entfernt ist.
 * Wenn es dieses Objekt gibt, wird es zur aktuellen Saeule und der Bot faehrt jetzt Slalom links.
 * 5. Sonst zurueck drehen, 90 Grad drehen und Slalom rechts fahren.
 * In diesem Schritt kann der Bot das Verhalten auch abbrechen, falls er gar kein Objekt mehr findet.
 *
 * @param *caller	Verhaltensdatensatz des Aufrufers
 */
void bot_do_slalom(Behaviour_t * caller);

#endif	// BEHAVIOUR_OLYMPIC_AVAILABLE
#endif	/*BEHAVIOUR_OLYMPIC_H_*/
