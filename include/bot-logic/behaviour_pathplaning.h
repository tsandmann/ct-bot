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
 * @file 	behaviour_pathplaning.h
 * @brief   Wave-Pfadplanungsverhalten; eine niedrigaufloesende Map wird ueber die hochaufloesende gelegt und
 * auf dieser folgende Schritte ausgefuehrt:
 * -Innerhalb des wirklich benutzten Mappenbereiches wird jede Zelle durchlaufen und falls der Durchschnittswert
 * der Hochaufloesenden Map < 0 ist (Hinderniswert) hier eingetragen mit Wert 1
 * -anzufahrende Zielposition erhaelt Mapwert 2
 * -ausgehend von Zielposition laeuft eine Welle los bis zum Bot-Ausgangspunkt, d.h. von 2 beginnend erhaelt jede Nachbarzelle
 * den naechst hoeheren Zellenwert
 * -wird als ein Nachbar die Botposition erreicht, wird der Pfad zurueckverfolgt und immer die Zelle mit kleinerem Wert gewaehlt;
 * die Zellenkoordinaten werden als Weltkoordinaten auf den Stack gelegt und kann nun abgefahren werden

 *
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	23.09.2008
*/


#ifndef BEHAVIOUR_PATHPLANING_H_
#define BEHAVIOUR_PATHPLANING_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_PATHPLANING_AVAILABLE

/*!
 * Display der Pfadplanung-Routinen
 */
void pathplaning_display(void);

/*!
 * Rufe das Wave-Verhalten auf mit Uebergabe des zu erreichenden Zielpunktes
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param dest_x    X-Welt-Zielkoordinate
 * @param dest_y    Y-Welt-Zielkoordinate
 * @param map_compare Map-Vergleichswert; Mapwerte kleiner dem Wert werden als Hindernisse eingetragen
 */
void bot_calc_wave(Behaviour_t * caller, int16_t dest_x, int16_t dest_y, int8_t map_compare);

/*!
 * Rufe das Wave-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param map_compare Map-Vergleichswert; Mapwerte kleiner dem Wert werden als Hindernisse eingetragen
 */
void bot_do_calc_wave(Behaviour_t * caller, int8_t map_compare);

/*!
 * Wave-Verhalten; berechnet die Welle ausgehend vom Zielpunkt bis zur Botposition; dann wird diese zurueckverfolgt und sich der Pfad
 * auf dem Stack gemerkt und anschliessend das Stack-Fahrverhalten aufgerufen
 * @param *data	Zeiger auf Verhaltensdatensatz
 */
void bot_calc_wave_behaviour(Behaviour_t *data);


#endif	// BEHAVIOUR_PATHPLANING_AVAILABLE
#endif	/* BEHAVIOUR_PATHPLANING_H_ */
