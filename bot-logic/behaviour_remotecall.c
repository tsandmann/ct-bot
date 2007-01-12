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
 * @brief 	ruft auf ein Kommando hin andere Verhalten auf und bestaetigt dann ihre Ausfuehrung
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

/*! Uebergabevariable fuer Remotecall-Verhalten */
static uint8 running_behaviour =REMOTE_CALL_IDLE;

static char * function_name = NULL;
static uint8 parameter_length[9] = {0};
static uint8 parameter_data[8] = {0};

#ifdef MCU
	#include <avr/pgmspace.h>
#else
	#define PROGMEM			// Alibideklaration hat keine Funktion, verhindert aber eine Warning
	#define strcmp_P strcmp	// Auf dem PC gibt es keinen Flash, also auch kein eigenes Compare
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
   PREPARE_REMOTE_CALL(bot_turn,1,"int16 degrees"), 
   PREPARE_REMOTE_CALL(bot_gotoxy,2,"float x, float y"),
   PREPARE_REMOTE_CALL(bot_drive_distance,3,"int8 curve, int16 speed, int16 cm"),
   PREPARE_REMOTE_CALL(bot_solve_maze,0,"") 
};

#define STORED_CALLS (sizeof(calls)/sizeof(call_t)) /*!< Anzahl der Remote calls im Array */

/*!
 * Sucht den Index des Remote-Calls heraus
 * @param call String mit dem namen der gesuchten fkt
 * @return Index in das calls-Array. Wenn nicht gefunden, dann 255
 */
uint8 getRemoteCall(char * call){
	LOG_DEBUG(("Suche nach Funktion: %s",call));
	
	uint8 i;
	for (i=0; i< (STORED_CALLS); i++){
		if (!strcmp_P (call, calls[i].name)){
			LOG_DEBUG(("calls[%d].name=%s passt",i,call));		
			return i;
		}
	}
	return 255;
}

#ifdef MCU
	/*!
	 * Hilfsfunktion fuer bot_remotecall()
	 * Baut einen AVR-kompatiblen Parameterstream aus einem uint32-Parameterarray und einem Infoarray ueber die Parameter
	 * @param dest	Zeiger auf das Ausgabearray (len[0]*2 Byte gross!)
	 * @param len	Zeiger auf ein Array, das zuerst die Anzahl der Parameter und danach die Anzahl der Bytes fuer die jeweiligen Parameter enthaelt
   	 * @param data	Zeiger auf die Daten
	 * @author 		Timo Sandmann (mail@timosandmann.de) 
   	 * @date		12.01.2007
	 */
	void remotecall_convert_params(uint8* dest, uint8* len, uint8* data){
//		uint8 k;
//		for (k=0; k<32; k+=4)
//			LOG_DEBUG(("parameter_data: %lu", *(uint32*)(data+k)));		
		uint8 i;
		for (i=1; i<=len[0]; i++){
			int8 j;
			if (len[i] == 1) *dest++ = 0;
			for (j=len[i]-1; j>=0; j--)
				*dest++ = data[j];
			data += 4;
		}	
	}
#endif	// MCU

/*! 
 * Dieses Verhalten kuemmert sich darum die Verhalten, die von aussen angefragt wurden zu starten und liefert ein feedback zurueck, wenn sie beendet sind.
 * @param *data der Verhaltensdatensatz
 */
void bot_remotecall_behaviour(Behaviour_t *data){
	uint8 call_id =255;
	
	LOG_DEBUG(("Enter bot_remotecall_behaviour"));
	void (* func) (struct _Behaviour_t *data);
	uint8 len =0;	// Laenge der Parameter
	
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

			#ifdef PC
				// Auf dem PC liegt die calls-Struktur im RAM
				func = (void*) calls[call_id].func;
				len = calls[call_id].len;
			#else
				// Auf dem MCU liegt die calls-Struktur im Flash und muss erst geholt werden
				func = (void*) pgm_read_word (& calls[call_id].func);
				len = (uint8) pgm_read_byte (& calls[call_id].len);
				
			#endif


			if (parameter_length[0] != len){
				LOG_DEBUG(("Die laenge der Parameter passt nicht. Gefordert=%d, geliefert=%d. Exit!",calls[call_id].len,parameter_length[0]));
				running_behaviour=REMOTE_CALL_IDLE;							
				return;
			} 
			
			if (parameter_length[0] ==0 ){		// Kommen wir ohne Parameter aus?
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
				LOG_DEBUG(("call=%s",function_name));
				uint8 k;
				for (k=0; k<8; k+=2)
					LOG_DEBUG(("parameter_data(low:high) = %u:%u", *(uint8*)(parameter_data+k+1), *(uint8*)(parameter_data+k)));
				LOG_DEBUG(("len: %u", parameter_length[0]));				
				// asm-hacks here ;)
				#ifdef PC
					uint32 tmp;
					uint32 i;
					for (i=0; i<parameter_length[0]; i++){
						tmp = *(parameter_data+(parameter_length[0]-i-1)*4);
						asm volatile(
							"movl %%esp, %%eax	# sp holen			\n\t"
							"addl %1, %%eax		# neuer sp+index	\n\t"
							"movl %0, (%%eax)	# param auf stack		"
							::	"c" (tmp), "g" ((parameter_length[0]-i)*4)
							:	"eax"
						);
					}					
				#else
					LOG_DEBUG(("r23: %u", parameter_data[0]));
					LOG_DEBUG(("r22: %u", parameter_data[1]));
					LOG_DEBUG(("r21: %u", parameter_data[2]));
					LOG_DEBUG(("r20: %u", parameter_data[3]));
					LOG_DEBUG(("r19: %u", parameter_data[4]));
					LOG_DEBUG(("r18: %u", parameter_data[5]));
					LOG_DEBUG(("r17: %u", parameter_data[6]));
					LOG_DEBUG(("r16: %u", parameter_data[7]));										
					asm volatile(
						"ld r23, Z+		\n\t"
						"ld r22, Z+		\n\t"
						"ld r21, Z+		\n\t"
						"ld r20, Z+		\n\t"
						"ld r19, Z+		\n\t"
						"ld r18, Z+		\n\t"
						"ld r17, Z+		\n\t"
						"ld r16, Z			"
						::	"z" (parameter_data)
						:	"r23", "r22", "r21", "r20", "r19", "r18", "r17", "r16"
					);
				#endif
				func(data);	// Die aufgerufene Botenfunktion starten
				
//				LOG_DEBUG(("TODO: Funktionen mit Parametern noch nicht implementiert"));
//				running_behaviour=REMOTE_CALL_IDLE; // So lange nicht fertig implementiert abbruch
				running_behaviour=REMOTE_CALL_RUNNING; // Wenn es denn dann mal geht
				return;
			}
			break;
			
		case REMOTE_CALL_RUNNING: // Es lief ein Verhalten und ist nun zuende (sonst waeren wir nicht hier)
			LOG_DEBUG(("REMOTE_CALL_RUNNING"));
			// TODO Antwort schicken			

			// Aufrauemen
			function_name=NULL;
			//parameter_length=NULL;
			//parameter_data=NULL;
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
 * @param len Zeiger auf ein Array, das zuerst die Anzahl der Parameter und danach die Anzahl der Bytes fuer die jeweiligen Parameter enthaelt
 * @param data Zeiger auf die Daten
 */
void bot_remotecall(char* func, uint8* len, uint32* data){
	LOG_DEBUG(("bot_remotecall(%s,%u,...)\n",func,len[0]));
	function_name=func;
	memcpy(parameter_length, len, len[0]+1);
	#ifdef MCU
		remotecall_convert_params(parameter_data, parameter_length, (uint8*)data);
	#else
		memcpy(parameter_data, data, len[0]*4);
	#endif
	
	running_behaviour=REMOTE_CALL_SCEDULED;
	activateBehaviour(bot_remotecall_behaviour);
}

#endif
