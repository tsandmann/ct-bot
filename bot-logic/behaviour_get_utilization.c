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
 * @file 	behaviour_get_utilization.c
 * @brief 	Misst die CPU-Auslastung eines anderen Verhaltens
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	14.12.2008
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_GET_UTILIZATION_AVAILABLE
#include <stdlib.h>
#include <string.h>
#include "os_scheduler.h"
#include "os_thread.h"
#include "log.h"
#include "map.h"

static uint8_t state = 0;
static Behaviour_t * beh = NULL;

/*!
 * Verhalten, das die CPU-Auslastung eines anderen Verhaltens misst
 * @param *data	Der Verhaltensdatensatz
 */
void bot_get_utilization_behaviour(Behaviour_t * data) {
	switch (state) {
	case 0:
		/* Warten bis Verhaten aktiv wird */
		if (beh->active == BEHAVIOUR_ACTIVE) {
			os_clear_utilization();
			state = 1;
			/* no break */
		} else {
			break;
		}
	case 1:
		/* Daten speichern */
		os_calc_utilization();
		if (beh->active != BEHAVIOUR_ACTIVE /*&& beh->subResult != BEHAVIOUR_SUBRUNNING*/) {
			state = 2;
		}
		break;
	case 2:
#ifdef MAP_AVAILABLE
		/* Verhalten wurde beendet */
		if (map_update_fifo.count > 0 || map_locked() == 1) {
			/* Map-Update abwarten */
			os_calc_utilization();
		} else
#endif
		{
			state = 3;
		}
		break;
	case 3:
		BLOCK_BEHAVIOUR(data, 200);
		os_calc_utilization();
		BLOCK_BEHAVIOUR(data, 200);
		os_calc_utilization();
		/* Daten ausgeben */
		os_print_utilization();
		state = 0;
		break;
	}
}

/*!
 * Verhalten, das die CPU-Auslastung eines anderen Verhaltens misst
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 * @param behaviour	Prioritaet des zu ueberwachenden Verhaltens
 */
void bot_get_utilization(Behaviour_t * caller, uint8_t behaviour) {
	beh = get_behaviour_from_prio(behaviour);
	if (beh == NULL) {
		if (caller) {
			caller->subResult = BEHAVIOUR_SUBFAIL;
		}
		return;
	}
	switch_to_behaviour(caller, bot_get_utilization_behaviour, BEHAVIOUR_OVERRIDE | BEHAVIOUR_BACKGROUND);
	state = 0;
}

#endif // BEHAVIOUR_GET_UTILIZATION_AVAILABLE
