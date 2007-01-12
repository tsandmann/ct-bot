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
#include <stdio.h>
#include <string.h>
#include "log.h"

#include "bot-logic/remote_calls.h"

#define REMOTE_CALL_IDLE 0
#define REMOTE_CALL_SCEDULED 1
#define REMOTE_CALL_RUNNING 2

/*! Uebergabevariable fuer Servo-Verhalten */
static uint8 running_behaviour =REMOTE_CALL_IDLE;

static char * function_name = NULL;
static uint8 parameter_len = 0;
static uint8 * parameter_data = NULL;

#ifdef MCU
	#include <avr/pgmspace.h>
#else
	#define PROGMEM	// Alibideklaration hat keine Funktion, verhindert aber eine Warning
	#define strcmp_P strcmp
#endif

/*! Hier muessen alle Funktionen rein, die Remote aufgerufen werden sollen
 * Ein eintrag erfolgt so:
 * PREPARE_REMOTE_CALL(BOTENFUNKTION,NUMBER_OF_BYTES)
 * Der letzte Eintrag brauch natuerlich Kein komma mehr
 * Alle Botenfunktionen muessen folgendem Schema entsprechen
 * void bot_xxx(Behaviour_t * caller, ...);
 * wieviele Parameter nach dem caller kommen ist voellig unerheblich. 
 * Allerdings muss man ihre gesamtlaeng in Byte kennen
 */
const call_t calls[] PROGMEM = {
   PREPARE_REMOTE_CALL(bot_turn,2,"int16 degrees"),
   PREPARE_REMOTE_CALL(bot_gotoxy,8,"float x, float y"),
   PREPARE_REMOTE_CALL(bot_solve_maze,0,"") 
};

#define STORED_CALLS (sizeof(calls)/sizeof(call_t))


uint8 getRemoteCall(char * call){
	LOG_DEBUG(("Suche nach Funktion: %s",call));
	
	uint8 i;
	for (i=0; i< (STORED_CALLS); i++){
		if (!strcmp_P (call, calls[i].name)){
			LOG_DEBUG(("calls[%d].name=%s passt",i,calls[i].name));		
			return i;
		}
	}
	return 255;
}


/*! 
 * Dieses Verhalten kuemmert sich darum die Verhalten, die von außen angefragt wurden zu starten und liefert ein feedback zurueck, wenn sie beendet sind.
 * @param *data der Verhaltensdatensatz
 */
void bot_remotecall_behaviour(Behaviour_t *data){
	uint8 call_id =255;
	
	LOG_DEBUG(("Enter bot_remotecall_behaviour"));
	void (* func) (struct _Behaviour_t *data);
	
	switch (running_behaviour) {
		case REMOTE_CALL_SCEDULED: 		// Es laueft kein Auftrag, aber es steht ein neuer an
			LOG_DEBUG(("REMOTE_CALL_SCEDULED"));
			
			if (function_name == NULL){		// pruefe, ob uebergabeparameter ok
				LOG_DEBUG(("kein Funktionsname uebergeben. Exit"));
				running_behaviour=REMOTE_CALL_IDLE;
				return;
			}

			call_id=getRemoteCall(function_name);
			if (call_id >= STORED_CALLS){
				LOG_DEBUG(("kein Funktion gefunden. Exit"));
				running_behaviour=REMOTE_CALL_IDLE;
				return;
			}

			if (parameter_len != calls[call_id].len){
				LOG_DEBUG(("Die laenge der Parameter passt nicht. Gefordert=%d, geliefert=%d. Exit!",calls[call_id].len,parameter_len));
				running_behaviour=REMOTE_CALL_IDLE;							
				return;
			} 

			func = (void*) calls[call_id].func;
			
			if (parameter_len ==0 ){		// Kommen wir ohne Parameter aus?
				LOG_DEBUG(("call=%s",function_name));
				func(data);	// Die aufgerufene Botenfunktion starten
				running_behaviour=REMOTE_CALL_RUNNING;
			} else { // Es gibt Parameter
				if (parameter_data == NULL){
					LOG_DEBUG(("Null-Pointer fuer Parameter. Exit!"));
					running_behaviour=REMOTE_CALL_IDLE;							
					return;
				} 

				// TODO: Ja hier wird es spannend, denn jetzt muessen die Parameter auf den Stack
				
				// Push param_len bytes aus parameter_data auf den Stack
				// func(data);	// Die aufgerufene Botenfunktion starten
				
				LOG_DEBUG(("TODO: Funktionen mit Parametern noch nicht implementiert"));
				running_behaviour=REMOTE_CALL_IDLE; // So lange nicht fertig implementiert abbruch
//				running_behaviour=REMOTE_CALL_RUNNING; // Wenn es denn dann mal geht
				return;
			}
			break;
			
		case REMOTE_CALL_RUNNING: // Es lief ein Verhalten und ist nun zuende (sonst waeren wir nicht hier)
			LOG_DEBUG(("REMOTE_CALL_RUNNING"));
			// TODO Antwort schicken			

			// Aufrauemen
			function_name=NULL;
			parameter_len=0;
			parameter_data=NULL;
			running_behaviour=REMOTE_CALL_IDLE;
			
			return_from_behaviour(data); 	// und Verhalten auch aus
			break;
		
		
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
void bot_remotecall(char * func, uint8 len, uint8* data){
	printf("bot_remotecall(%s,%d,...)\n",func,len);
	function_name= func;
	parameter_len=len;
	parameter_data=data;
	
	running_behaviour=REMOTE_CALL_SCEDULED;
	activateBehaviour(bot_remotecall_behaviour);
}

#endif
