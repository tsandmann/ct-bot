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


/*! @file 	behaviour_delay.c
 * @brief 	Delay-Routinen als Verhalten
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	12.07.07
*/

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_DELAY_AVAILABLE
	#ifndef TIME_AVAILABLE
		#error BEHAVIOUR_DELAY benötigt TIME_AVAILABLE. Das ist derzeit nicht der Fall. Bitte in ct-Bot.h aktivieren
	#endif
	
	#include "timer.h"
	static uint16 delay_time_ms=0;
	
	/*!
	 * Verhalten fuer Delays
	 */
	void bot_delay_behaviour(Behaviour_t *data){
		static uint16 old_s;
		static uint16 old_ms;
	
		static uint8 state=0;
		
		switch (state){
			case 0:
				old_s=timer_get_s();
				old_ms=timer_get_ms();
				
				state++;			
				break;
	
			case 1:
				if (delay_time_ms <= timer_get_ms_since(old_s, old_ms)){
					state++;
				}
				break;
	
			default:
				state=0;
				return_from_behaviour(data);
				break;
		}	
	}
	
	/*!
	 * Rufe das Delay-Verhalten auf 
	 * @param caller		Der obligatorische Verhaltensdatensatz des Aufrufers
	 * @param delay_time	Die Verzögerungszeit in ms
	 */
	void bot_delay(Behaviour_t * caller, uint16 delay_time){
		delay_time_ms= delay_time;
		switch_to_behaviour(caller,bot_delay_behaviour,OVERRIDE);	
	}
	
#endif
