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


/*! @file 	behaviour_remotecall_behaviour.c
 * @brief 	ruft auf ein Kommando hin andere Verhalten auf und bestätigt dann ihre ausfuehrung
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	07.12.06
*/

#include "bot-logic/bot-logik.h"
#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
#include <stdlib.h>

#include "bot-logic/remote_calls.h"

#define REMOTE_CALL_IDLE 0
#define REMOTE_CALL_SCEDULED 1
#define REMOTE_CALL_RUNNING 2

/*! Uebergabevariable fuer Servo-Verhalten */
static uint8 running_behaviour =REMOTE_CALL_IDLE;

static char * function_name = NULL;
static uint8 parameter_len = 0;
static uint8 * parameter_data = NULL;



void * getRemoteCall(char * call){
	return NULL;
}


/*! 
 * Dieses Verhalten fuehrt ein Servo-Kommando aus und schaltet danach den Servo wieder ab
 * 
 * @param *data der Verhaltensdatensatz
 */
void bot_remotecall_behaviour(Behaviour_t *data){
	void (* func) (struct _Behaviour_t *data);
	
	switch (running_behaviour) {
		case REMOTE_CALL_SCEDULED: 
			if (function_name != NULL){
				
				
				
				
				if (parameter_len ==0){
					func =  getRemoteCal(function_name);
					func(data);
				} else {
					// Ja hier wird es spannend, denn jetzt muessen die Parameter auf den Stack
				}
				running_behaviour=REMOTE_CALL_RUNNING;
			}
			break;
			
		case REMOTE_CALL_RUNNING: 
			// TODO Antwort schicken			
			function_name=NULL
			parameter_len=0;
			parameter_data=NULL;
			running_behaviour=REMOTE_CALL_IDLE;
			return_from_behaviour(data); 	// und Verhalten auch aus
			break;
		}
		
		default:
			return_from_behaviour(data); 	// und Verhalten auch aus
			break;
		
	}
}

/*!
 * Fuehre einen remote_call aus. Es gibt KEIN aufrufendes Verhalten!!
 * @param func Zeiger auf den Namen der Fkt
 * @param len Anzahl der zu uebergebenden Bytes
 * @param data Zeiger auf die Daten
 */
void bot_remotecall(void * func, uint8 len, uint8* data){
	function_name= func;
	parameter_len=len;
	parameter_data=data;
	
	running_behaviour=REMOTE_CALL_SCEDULED;
	activateBehaviour(bot_remotecall_behaviour);
}
#endif
