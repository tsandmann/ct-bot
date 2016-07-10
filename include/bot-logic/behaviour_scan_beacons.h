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
 * \file 	behaviour_scan_beacons.h
 * \brief 	Verhalten, das Landmarken im Umkreis des Bots sucht und die Bot-Position
 * 			aktualisiert, falls drei oder mehr Landmarken gefunden wurden.
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	01.05.2009
 */

#ifndef BEHAVIOUR_SCAN_BEACONS_H_
#define BEHAVIOUR_SCAN_BEACONS_H_

#ifdef BEHAVIOUR_SCAN_BEACONS_AVAILABLE
/**
 * Verhalten, das Landmarken im Umkreis des Bots sucht und die Bot-Position
 * aktualisiert, falls drei oder mehr Landmarken gefunden wurden.
 * \param *data	Der Verhaltensdatensatz
 */
void bot_scan_beacons_behaviour(Behaviour_t * data);

/**
 * Verhalten, das Landmarken im Umkreis des Bots sucht und die Bot-Position
 * aktualisiert, falls drei oder mehr Landmarken gefunden wurden.
 * \param *caller Der Verhaltensdatensatz des Aufrufers
 * \param position_update Sollen die Positionsdaten aktualisiert werden? 1: ja
 * \param mode 0: Auf der Stelle drehen, 1: Kreis um das linke Rad fahren
 */
void bot_scan_beacons(Behaviour_t * caller, uint8_t position_update, uint8_t mode);

#endif // BEHAVIOUR_SCAN_BEACONS_AVAILABLE
#endif // BEHAVIOUR_SCAN_BEACONS_H_
