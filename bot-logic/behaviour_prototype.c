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
 * @file 	behaviour_prototype.c
 * @brief 	Rohling für eigene Verhalten
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	13.04.2011
 */


#include "bot-logic/bot-logic.h"
#ifdef BEHAVIOUR_PROTOTYPE_AVAILABLE

static uint8_t prototype_state = 0;	/*!< Status des Prototype-Verhaltens */

// Optionaler Puffer fuer den Uebergabeparameter
//static int16_t prototype_parameter = 0;	/*!< Uebergabevariable fuer Prototype */

#define STATE_PROTOTYPE_INIT 0
#define STATE_PROTOTYPE_WORK 1
#define STATE_PROTOTYPE_DONE 3

/*! 
 * Prototyp für ein Verhalten
 *
 * Bitte umbenennen.
 * Siehe auch include/bot-logik/available_behaviours
 * und
 * bot-logic/bot-logic.c in der Funktion bot_behave_init(void)
 * 
 * @param *data der Verhaltensdatensatz
 */
void bot_prototype_behaviour(Behaviour_t * data) {
	switch (prototype_state) {
	case STATE_PROTOTYPE_INIT:
		prototype_state = 1;
		break;
	case STATE_PROTOTYPE_WORK:
		prototype_state = 0;
		break;
	default:
		return_from_behaviour(data);
		break;
	}

}

/*!
 * Rufe das Prototyp-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_prototype(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_prototype_behaviour, BEHAVIOUR_OVERRIDE);
	prototype_state = STATE_PROTOTYPE_INIT;
}

// Alternative Botenfunktion mit Uebergabeparameter
/*!
 * Rufe das Prototyp-Verhalten auf
 * und nutze dabei einen Uebergabeparameter
 * @param *caller Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param param Uebergabeparameter
 */
/*
void bot_prototype(Behaviour_t * caller, int16_t param) {
	switch_to_behaviour(caller, bot_prototype_behaviour, BEHAVIOUR_OVERRIDE);
	prototype_parameter = param;
	prototyp_state = STATE_PROTOTYPE_INIT;
}
*/
#endif // BEHAVIOUR_PROTOTYPE_AVAILABLE
