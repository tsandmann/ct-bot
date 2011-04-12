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
 * \file 	behaviour_line_shortest_way.h
 * \brief 	Linienverfolger, der an Kreuzungen eine bestimmte Vorzugsrichtung einschlaegt (links) und diesen Weg weiterverfolgt,
 * 			bis das Ziel (gruenes Feld an Wand) gefunden ist.
 *
 * Linien muessen immer an einem gruenen Feld ohne Hindernis enden, damit der Bot ein Ende erkennt und umdrehen kann.
 * Die Kreuzungen und der eingeschlagene Weg werden auf dem Stack vermerkt, Wege die nicht zum Ziel fuehren vergessen; Am Ziel angekommen
 * steht im Stack der kuerzeste Weg; Es kann nun via Display auf dem kuezesten Weg zum Ausgangspunkt zurueckgefahren werden oder der Bot wieder
 * manuell an den Start gestellt werden und das Ziel auf kuerzestem Weg angefahren werden.
 *
 * \author 	Frank Menzel (Menzelfr@gmx.de)
 * \date 	21.12.2008
 *
 * \todo	Unterstuetzung fuer Linienlabyrinthe mit Zyklen
 */

#ifndef BEHAVIOUR_LINE_SHORTEST_WAY_H_
#define BEHAVIOUR_LINE_SHORTEST_WAY_H_

#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
/*!
 * Das eigentliche Verhalten, welches den bot einer Linie folgen laesst, X-Kreuzungen erkennt und
 * dann in bestimmter Reihenfolge die Abzweigungen entlangfaehrt bis zu seinem Ziel (gruenes Feld an Hindernis); die
 * Kreuzungen werden enweder neu im Stack angelegt oder von dort geholt und die Wegeinfos dort wieder vermerkt; eine Kreuzung
 * wird vergessen, wenn kein Weg von dort zum Ziel gefuehrt hatte; Verhalten laesst den bot ebenefalls den bereits gemerkten Weg
 * zum Ziel oder von dort rueckwaerts direkt auf kuerzestem Weg zum Ausgangspunkt fahren
 * \param *data	Verhaltensdatensatz
 */
void bot_line_shortest_way_behaviour(Behaviour_t * data);

/*!
 * Startet das Verhalten
 * \param	*caller Verhaltensdatensatz des Aufrufers
 */
void bot_line_shortest_way(Behaviour_t * caller);

/*!
 * Falls Linienfolger Linie nicht findet kann hier weitergefuehrt werden nach manuellem richtigen wiederausrichten
 * \param	*caller Verhaltensdatensatz des Aufrufers
 */
void bot_line_shortest_way_continue(Behaviour_t * caller);

/*!
 * Abfahren des gefundenen Weges vorwaerts
 * \param	*caller Verhaltensdatensatz des Aufrufers
 */
void bot_line_shortest_way_forward(Behaviour_t * caller);

/*!
 * Abfahren des gefundenen Weges rueckwaerts
 * \param	*caller Verhaltensdatensatz des Aufrufers
 */
void bot_line_shortest_way_backward(Behaviour_t * caller);

/*!
 * Verhalten zum Checken auf Einnehmen der entgegengesetzten Richtung (Linienende)
 * \param *data	eigener Verhaltensdatensatz
 */
void bot_check_reverse_direction_behaviour(Behaviour_t * data);

/*!
 * Display zum Verhalten
 */
void bot_line_shortest_way_display(void);

#endif // BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
#endif // BEHAVIOUR_LINE_SHORTEST_WAY_H_
