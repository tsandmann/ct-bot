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
 * \file 	botcontrol.h
 * \brief 	High-level Steuerungsroutinen, z.B. Funktionen fuer die Bot-Hauptschleife
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.05.2011
 */

#ifndef BOTCONTROL_H_
#define BOTCONTROL_H_

/**
 * Fuehrt die Verarbeitung in der Hauptschlaufe vor dem
 * Verhaltenscode durch. Dazu gehoert beispielsweise, die Sensoren
 * abzufragen und auf Pakete des Simulators zu reagieren.
 */
void pre_behaviour(void);

/**
 * Fuehrt die Verarbeitung in der Hauptschleife nach dem
 * Verhaltenscode durch. Dazu gehoert beispielsweise, die
 * Bildschirmanzeige zu steuern und den Simulator ueber den aktuellen
 * Zustand zu informieren.
 */
void post_behaviour(void);

/**
 * Faehrt den Bot sauber herunter
 */
void ctbot_shutdown(void);

#endif // BOTCONTROL_H_
