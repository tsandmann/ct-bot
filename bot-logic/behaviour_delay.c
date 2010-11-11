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
 * @file 	behaviour_delay.c
 * @brief 	Delay-Routinen als Verhalten
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	12.07.07
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_DELAY_AVAILABLE
#include "timer.h"
static uint32_t delay_ticks = 0; /*!< Timestamp fuer Wartezeit Ende  */

/*!
 * Verhalten fuer Delays
 * @param *data	Der Verhaltensdatensatz
 */
void bot_delay_behaviour(Behaviour_t * data) {
	uint32_t ticks = TIMER_GET_TICKCOUNT_32;
	if (ticks >= delay_ticks) {
		return_from_behaviour(data);
	}
}

/*!
 * Rufe das Delay-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param ticks		Die Verzoegerungszeit in ticks
 * @return			-1 wenn was schief gelaufen ist, sonst 0
 */
int8_t bot_delay_ticks(Behaviour_t * caller, uint16_t ticks) {
	switch_to_behaviour(caller, bot_delay_behaviour, NOOVERRIDE);
	if (caller->subResult != SUBFAIL) {
		delay_ticks = TIMER_GET_TICKCOUNT_32 + ticks;
		return 0;
	} else {
		return -1;
	}
}
#endif // BEHAVIOUR_DELAY_AVAILABLE
