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
 * @file 	behaviour_drive_stack.h
 * @brief 	Anfahren aller auf dem Stack befindlichen Punkte
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	13.12.2007
 */

#ifndef BEHAVIOUR_DRIVESTACK_H_
#define BEHAVIOUR_DRIVESTACK_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"


#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE

/*!
 * Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte, wobei das Fahr-Unterverhalten bot_goto_pos benutzt wird
 * @param *data	Der Verhaltensdatensatz
 */
void bot_drive_stack_behaviour(Behaviour_t * data);

/*!
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack X befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param stack		Nummer des Stacks, der abgefahren werden soll
 * @param mode		0: Stack, 1: FIFO
 */
void bot_drive_stack_x(Behaviour_t * caller, uint8_t stack, uint8_t mode);

/*!
 * Botenfunktion: Verhalten zum Anfahren aller auf dem Stack befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_stack(Behaviour_t * caller);

/*!
 * Botenfunktion: Verhalten zum Anfahren aller in der FIFO-Queue befindlichen Punkte
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_drive_fifo(Behaviour_t * caller);

/*!
 * Sichern der aktuellen Botposition auf den Stack
 * @param *caller	einfach nur Zeiger, damit remotecall verwendbar
 * @param stack		Nummer des Stacks, auf den gepusht werden soll
 */
void bot_push_actpos(Behaviour_t * caller, uint8_t stack) __attribute__((noinline));

/*!
 * Verhalten um sich entlang des Fahrweges relevante Koordinaten auf dem Stack zu merken; Verhalten ist nach Aktivierung via Botenroutine
 * ein Endlosverhalten und sammelt bis zur Deaktivierung die Wegepunkte; deaktiviert wird es via Notaus oder direkt mit Befehl zum Zurueckfahren und
 * damit Start des Stack-Fahrverhaltens
 * @param *data	Der Verhaltensdatensatz
 */
void bot_save_waypositions_behaviour(Behaviour_t * data);

/*!
 * Botenfunktion: Verhalten um sich entlang des Fahrweges relevante Koordinaten auf dem Stack zu merken
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
void bot_save_waypositions(Behaviour_t * caller);

/*!
 * Display zum Setzen und Anfahren der Stackpunkte
 */
void drive_stack_display(void);

#endif	// BEHAVIOUR_DRIVE_STACK_AVAILABLE
#endif /*BEHAVIOUR_DRIVESTACK_H_*/
