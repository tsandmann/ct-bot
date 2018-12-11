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
 * @file 	behaviour_follow_line.h
 * @brief 	Linienverfolger
 * @author 	Torsten Evers (tevers@onlinehome.de) Version 1
 * @author 	Timo Sandmann (mail@timosandmann.de) Version 2
 * @author 	Frank Menzel (Menzelfr@gmx.de) Version 3
 * @date 	03.11.2006
 */

#ifndef BEHAVIOUR_FOLLOW_LINE_H_
#define BEHAVIOUR_FOLLOW_LINE_H_

#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE

/* Version 1: Altes Verfahren; Version 2: auf linke Kante gefahren; Version 3: linke Kante als auch beide
 * Version 3 funktioniert auf dem echten Bot nicht sehr zuverlaessig (Probleme bei Winkeln < 120 Grad) */
#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
#define FOLLOW_LINE_VERSION	3
#else
#define FOLLOW_LINE_VERSION 2
#endif

/**
 * Folgt einer Linie, sobald beide Liniensensoren ausloesen
 * @param *data	Verhaltensdatensatz
 */
void bot_follow_line_behaviour(Behaviour_t* data);

/**
 * Folgt einer Linie. Der linke Liniensensor ist dabei auf der Linie, der Rechte daneben.
 * Der Bot faehrt also auf der rechten Kante der Linie. Sie sollte in etwa die Breite beider CNY70 haben.
 * @param *caller Verhaltensdatensatz des Aufrufers
 * @param search Falls 1, wird zunaechst nach einer Linie gesucht. Falls 0, muss der Bot auf der rechten Linienkante stehen (nur Version 2)
 */
void bot_follow_line(Behaviour_t* caller, uint8_t search);
#endif // BEHAVIOUR_FOLLOW_LINE_AVAILABLE
#endif // BEHAVIOUR_FOLLOW_LINE_H_
