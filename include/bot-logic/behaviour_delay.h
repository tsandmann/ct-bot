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
 * @file 	behaviour_delay.h
 * @brief 	Delay-Routinen als Verhalten
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	12.07.07
 */


#ifndef BEHAVIOUR_DELAY_H_
#define BEHAVIOUR_DELAY_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_DELAY_AVAILABLE
#include "timer.h"

/*!
 * Unterbreche das aktuelle Verhalten fuer mindestens X ms
 * Das Makro vereinfacht den Aufruf des Delay-Verhaltens, das funktioniert aber
 * nur von anderen Verhalten aus!
 * Die Verhaltensfunktion wird an der Stelle, wo BLOCK_BEHAVIOUR aufgerufen wird,
 * verlassen und erst nach Ablauf der Wartezeit wieder aufgerufen, diesmal wird der
 * BLOCK_BEHAVIOUR-Aufruf jedoch nicht ausgefuehrt.
 * Sollte das Delay-Verhalten bereits aktiv sein, wird gewartet, bis es fertig ist und
 * anschliessend gewartet.
 * Ein einfaches Beispiel fuer die Verwendung findet sich in bot_servo_behaviour().
 *
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param ms		Die Verzoegerungszeit in ms
 */
#define BLOCK_BEHAVIOUR(caller, ms)	{	\
	static uint8_t delay_state = 0;		\
	if (delay_state == 0) {				\
		if (bot_delay(data, ms) == 0) {	\
			delay_state = 1;			\
		}								\
		return;							\
	}									\
	delay_state = 0;					\
}

/*!
 * Rufe das Delay-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param ms		Die Verzoegerungszeit in ms
 * @return			-1 wenn was schief gelaufen ist, sonst 0
 */
#define bot_delay(caller, ms)	bot_delay_ticks(caller, MS_TO_TICKS((uint32_t)ms))

/*!
 * Rufe das Delay-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param ticks		Die Verzoegerungszeit in ticks
 * @return			-1 wenn was schief gelaufen ist, sonst 0
 */
int8_t bot_delay_ticks(Behaviour_t * caller, uint16_t ticks);

/*!
 * Verhalten fuer Delays
 * @param *data	Der Verhaltensdatensatz
 */
void bot_delay_behaviour(Behaviour_t * data);

#endif	// BEHAVIOUR_DELAY_AVAILABLE
#endif	/*BEHAVIOUR_DELAY_H_*/
