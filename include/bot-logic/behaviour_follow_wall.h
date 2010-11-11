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
 * \file 	behaviour_follow_wall.h
 * \brief 	Wandfolger Explorer
 *
 * Faehrt solange vorwaerts, bis er an eine Wand kommt, an die er sich gewisse Zeit wegdreht;
 * nach links dreht er sich, wenn rechts eine Wand als naeher erkannt wird sonst nach rechts; es erfolgt
 * hier auch eine Abgrundauswertung; wird erkannt dass entweder
 * beide Abgrundsensoren den Abgrund detektieren oder der bot senkrecht zur Wand steht, so wird via Zeitzaehler
 * ein Pseudo-Zufallswert bis 255 ausgewertet und danach die neue Drehrichtung links/ rechts ermittelt;
 * zur Mindestdrehzeit wird ebenfalls immer dieser Zufallswert zuaddiert
 * sehr sinnvoll mit diesem Explorer-Verhalten ist das hang_on-Verhalten, welches durch Vergleich mit Mausdaten
 * ein Haengenbleiben erkennt, rueckwaerts faehrt und das hier registrierte Notverhalten startet. Dieses wiederum
 * sorgt dafuer, dass der bot sich wegdreht und weiterfaehrt wie an einer Wand. Gleiches gilt fuer das Abgrundverhalten.
 *
 * \author 	Frank Menzel (Menzelfr@gmx.net)
 * \date 	30.08.2007
 */

#ifndef BEHAVIOUR_FOLLOW_WALL_H_
#define BEHAVIOUR_FOLLOW_WALL_H_

#ifdef BEHAVIOUR_FOLLOW_WALL_AVAILABLE

void border_follow_wall_handler(void);

/*!
 * Faehrt vorwaerts bis zu einer Wand, von die er sich wegdreht
 * \param *data Verhaltensdatensatz
 */
void bot_follow_wall_behaviour(Behaviour_t * data);

/*! 
 * Botenfunktion
 * Faehrt vorwaerts bis zu einer Wand, von die er sich wegdreht
 * \param *caller 	Verhaltensdatensatz
 * \param *check 	Abbruchfunktion; wenn diese True liefert wird das Verhalten beendet sonst endlos
 * 			        einfach NULL uebergeben, wenn keine definiert ist 
 */
void bot_follow_wall(Behaviour_t * caller, uint8_t (* check)(void));

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
/*! 
 * Botenverhalten zum Aufruf via Remotecall ohne weitere params, d.h. da kein Abbruchverhalten
 * uebergeben wird, ist dies dann ein Endlos-Explorerverhalten 
 * \param *caller Verhaltensdatensatz
 */
void bot_do_wall_explore(Behaviour_t * caller);
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE

#endif // BEHAVIOUR_FOLLOW_WALL_AVAILABLE
#endif // BEHAVIOUR_FOLLOW_WALL_H_
