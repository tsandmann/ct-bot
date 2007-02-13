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


/*! @file 	behaviour_remotecall.c
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
#include "command.h"

#include "bot-logic/remote_calls.h"

#define REMOTE_CALL_IDLE 0
#define REMOTE_CALL_SCHEDULED 1
#define REMOTE_CALL_RUNNING 2

/*! Uebergabevariable fuer Remotecall-Verhalten */
static uint8 running_behaviour =REMOTE_CALL_IDLE;

static uint8 function_id = 255;
static uint8 parameter_count = 0;		/*!< Anzahl der Paramter (ohne Zeiger auf Aufrufer) */
static uint8 parameter_data[8] = {0};	/*!< Hier liegen die eigentlichen Parameter, derzeit brauchen wir maximal 8 Byte (2 floats, 4 (u)int16 oder 4 (u)int8 */
#ifdef MCU
	static uint8 parameter_length[REMOTE_CALL_MAX_PARAM] = {0};	/*!< Hier speichern wir die Laenge der jeweiligen Parameter */
#else
	static uint8* parameter_length = NULL;	/*!< Hier speichern wir die Laenge der jeweiligen Parameter */
#endif

#ifdef MCU
	#include <avr/pgmspace.h>
#else
	#define PROGMEM			// Alibideklaration hat keine Funktion, verhindert aber eine Warning
	#define strcmp_P strcmp	// Auf dem PC gibt es keinen Flash, also auch kein eigenes Compare
#endif

/*! 
 * Hier muessen alle Boten-Funktionen rein, die Remote aufgerufen werden sollen
 * Diese stoßen dann das zugehoerige Verhalten an
 * Ein Eintrag erfolgt so:
 * PREPARE_REMOTE_CALL(BOTENFUNKTION,NUMBER_OF_PARAMS, STRING DER DIE PARAMETER BESCHREIBT,laenge der jeweiligen Parameter in Byte)
 *
 * Alle Botenfunktionen muessen folgendem Schema entsprechen
 * void bot_xxx(Behaviour_t * caller, ...);
 *
 * Erklaerung am Bsp:   
 * PREPARE_REMOTE_CALL(bot_gotoxy, 2, "float x, float y", 4, 4),
 *   Name der Botenfunktion --^    ^    ^                 ^  ^
 *   Anzahl der Parameter ---------     |                 |  |
 *   Beschreibung der Parameter --------                  |  |
 *   Anzahl der Bytes Parameter 1 ------------------------   |
 *   Anzahl der Bytes Parameter 2 ---------------------------
 * 
 * Zur Info:
 * 1 Byte brauchen: uint8,  int8,char
 * 2 Byte brauchen: uint16, int16
 * 4 Byte brauchen: uint32, int32, float
 */
const call_t calls[] PROGMEM = {
	#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
		PREPARE_REMOTE_CALL(bot_drive_distance,3, "int8 curve, int16 speed, int16 cm", 1,2,2),
	#endif
	#ifdef BEHAVIOUR_GOTOXY_AVAILABLE	
		PREPARE_REMOTE_CALL(bot_gotoxy, 2, "float x, float y", 4, 4),
	#endif
	#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
		PREPARE_REMOTE_CALL(bot_solve_maze,0,""),
	#endif
	#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
		PREPARE_REMOTE_CALL(bot_catch_pillar,0,""),
	#endif
	#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
		PREPARE_REMOTE_CALL(bot_drive_square,0,""),
	#endif
	#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
		PREPARE_REMOTE_CALL(bot_follow_line,0,""),
	#endif
	#ifdef BEHAVIOUR_GOTO_AVAILABLE
		PREPARE_REMOTE_CALL(bot_goto,2," int16 left, int16 right",2,2),
	#endif
	#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
		PREPARE_REMOTE_CALL(bot_do_slalom,0,""),
	#endif
	#ifdef BEHAVIOUR_SCAN_AVAILABLE
		PREPARE_REMOTE_CALL(bot_scan,0,""),
	#endif
	#ifdef BEHAVIOUR_SERVO_AVAILABLE
		PREPARE_REMOTE_CALL(bot_servo,2,"uint8 servo, uint8 pos",1,1),
	#endif
	#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
		PREPARE_REMOTE_CALL(bot_simple,0,""),
		PREPARE_REMOTE_CALL(bot_simple2,1,"int16 light",2),
	#endif
	#ifdef BEHAVIOUR_TURN_AVAILABLE
		PREPARE_REMOTE_CALL(bot_turn,1,"int16 degrees",2)   
	#endif
};

#define STORED_CALLS (sizeof(calls)/sizeof(call_t)) /*!< Anzahl der Remote calls im Array */

/*!
 * Sucht den Index des Remote-Calls heraus
 * @param call String mit dem namen der gesuchten fkt
 * @return Index in das calls-Array. Wenn nicht gefunden, dann 255
 */
uint8 getRemoteCall(char * call){
//	LOG_DEBUG(("Suche nach Funktion: %s",call));
	
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
	 * @param count	Anzahl der Parameter
	 * @param len	Zeiger auf ein Array, das die Anzahl der Bytes fuer die jeweiligen Parameter enthaelt
   	 * @param data	Zeiger auf die Daten (32 Bit, Laenge 8)
	 * @author 		Timo Sandmann (mail@timosandmann.de) 
   	 * @date		12.01.2007
	 */
	void remotecall_convert_params(uint8* dest, uint8 count, uint8* len, uint8* data){
		uint8 i;
		/* jeden Parameter behandeln */
		for (i=0; i<count; i++){
			int8 j;
			if (len[i] == 1) *dest++ = 0;	// auch (u)int8 beginnen immer in geraden Registern 
			/* pro Parameter LSB zuerst nach dest kopieren */
			for (j=len[i]-1; j>=0; j--)
				*dest++ = data[j];
			data += 4;	// data-Array ist immer in 32 Bit
		}	
	}
#endif	// MCU

/*! 
 * Dieses Verhalten kuemmert sich darum die Verhalten, die von aussen angefragt wurden zu starten und liefert ein feedback zurueck, wenn sie beendet sind.
 * @param *data der Verhaltensdatensatz
 */
void bot_remotecall_behaviour(Behaviour_t *data){
	uint8 call_id =255;
	
//	LOG_DEBUG(("Enter bot_remotecall_behaviour"));
	void (* func) (struct _Behaviour_t *data);
	
	switch (running_behaviour) {
		case REMOTE_CALL_SCHEDULED: 		// Es laueft kein Auftrag, aber es steht ein neuer an
//			LOG_DEBUG(("REMOTE_CALL_SCHEDULED"));

			call_id=function_id;
			if (call_id >= STORED_CALLS){
//				LOG_DEBUG(("keine Funktion gefunden. Exit"));
				running_behaviour=REMOTE_CALL_IDLE;
				return;
			}

			#ifdef PC
				// Auf dem PC liegt die calls-Struktur im RAM
				func = (void*) calls[call_id].func;
			#else
				// Auf dem MCU liegt die calls-Struktur im Flash und muss erst geholt werden
				func = (void*) pgm_read_word (& calls[call_id].func);
			#endif
		
			if (parameter_count ==0 ){		// Kommen wir ohne Parameter aus?
//				LOG_DEBUG(("call_id=%u",call_id));
				func(data);	// Die aufgerufene Botenfunktion starten
				running_behaviour=REMOTE_CALL_RUNNING;
			} else if (parameter_count <= REMOTE_CALL_MAX_PARAM){ // Es gibt gueltige Parameter
				// TODO: Ja hier wird es spannend, denn jetzt muessen die Parameter auf den Stack
				LOG_DEBUG(("call_id=%u",call_id));
				LOG_DEBUG(("parameter_count=%u", parameter_count));				
				// asm-hacks here ;)
				#ifdef PC
					/* Prinzip auf dem PC: Wir legen alle Parameter einzeln auf den Stack, springen in die Botenfunktion und raeumen anschliessend den Stack wieder auf */
					uint32 tmp;
					uint8 i;
					volatile uint8 td=1;	// verwenden wir nur, damit der Compiler unsere inline-asm-Bloecke nicht umordnet
					for (i=0; i<parameter_count*4 && td>0; i+=4,td++){	// Debug-Info ausgeben und td initialisieren (s.u.)
//						LOG_DEBUG(("parameter_data[%u-%u] = %lu",i, i+3, *(uint32*)(parameter_data+i)));
					}					
					/* Erster Wert in parameter_length ist die Anzahl der Parameter (ohne Zeiger des Aufrufers) */
					for (i=0; i<parameter_count && td>1; i++,td++){	// Check von td eigentlich sinnlos, aber wir brauchen eine echte Datenabhaengigkeit auf dieses Codestueck
						/* cdecl-Aufrufkonvention => Parameter von rechts nach links auf den Stack */
						tmp = *(uint32*)(parameter_data+(parameter_count-i-1)*4);
						/* Parameter 2 bis n pushen */
						asm volatile(	// IA32-Support only
							"pushl %0		# parameter i auf stack	"
							::	"g" (tmp)
							: "memory"
						);
					}	
					/* Parameter 1 (data) und Funktionsaufruf */
					asm volatile(
						"pushl %0			# push data			\n\t"
						"movl %1, %%eax		# adresse laden		\n\t"
						"call *%%eax		# jump to callee	\n\t"
						:: "m" (data), "m" (func)
						: "eax", "memory"
					);
					/* caller rauemt den Stack wieder auf */
					for (i=0; i<=parameter_count && td>2; i++){	// Check von td erzwingt, dass das Aufraeumen erst jetzt passiert
						asm volatile(
							"pop %%eax		# stack aufraeumen	"
							:::	"eax", "memory"
						);	
					}			
				#else
					/* Prinzip auf der MCU: Keine komplizierten Rechnungen, sondern einfach alle Register ueberschreiben.
					 * Achtung: Derzeit braucht kein Verhalten mehr als 8 Register (2*float oder 4*int16 oder 4*int8), aendert sich das,
					 * muss man den Code hier erweitern! 
					 * Die AVR-Konvention beim Funktionsaufruf: 
					 * Die Groesse in Byte wird zur naechsten geraden Zahl aufgerundet, falls sie ungerade ist.
					 * Der Registerort faengt mit 26 an.
					 * Vom Registerort wird die berechete Groesse abgezogen und das Argument in diesen Registern uebergeben (LSB first). 
					 * In r24/r25 legt der Compiler spaeter den Zeiger des Aufrufers, koennen wir hier also weglassen. */
//					LOG_DEBUG(("r22:r23 = %u:%u", parameter_data[1], parameter_data[0]));
//					LOG_DEBUG(("r21:r20 = %u:%u", parameter_data[3], parameter_data[2]));
//					LOG_DEBUG(("r18:r19 = %u:%u", parameter_data[5], parameter_data[4]));
//					LOG_DEBUG(("r16:r17 = %u:%u", parameter_data[7], parameter_data[6]));
					asm volatile(	// remotecall_convert_params() hat den Speicher bereits richtig sortiert, nur noch Werte laden
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
				func(data);	// Die aufgerufene Botenfunktion starten
				#endif
					
				running_behaviour=REMOTE_CALL_RUNNING;
				return;
			} else {
//				LOG_DEBUG(("Parameteranzahl unzulaessig!"));	
			}
			break;
			
		case REMOTE_CALL_RUNNING: // Es lief ein Verhalten und ist nun zuende (sonst waeren wir nicht hier)
		{
				// Antwort schicken			

			char * function_name;
			
			#ifdef PC
				function_name=(char*) &calls[function_id].name;
			#else
			// Auf dem MCU muessen wir die Daten erstmal aus dem Flash holen

				char tmp[REMOTE_CALL_FUNCTION_NAME_LEN+1];
				function_name=(char*)&tmp;
				
				uint8* from= (uint8*)& calls[function_id].name;

				uint8 i;	
				for (i=0; i<REMOTE_CALL_FUNCTION_NAME_LEN+1; i++)
					*function_name++ = (uint8) pgm_read_byte ( from++ );	
				function_name=(char*)&tmp;
			#endif
			
			#ifdef COMMAND_AVAILABLE
				int16 result = data->subResult;
				command_write_data(CMD_REMOTE_CALL,SUB_REMOTE_CALL_DONE,&result,&result,function_name);
			#endif

//			LOG_DEBUG(("Remote-call %s beendet",function_name));

			// Aufrauemen
			function_id=255;
			//parameter_length=NULL;
			//parameter_data=NULL;
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
 * @param data Zeiger auf die Daten
 */
void bot_remotecall(char* func, remote_call_data_t* data){

	function_id= getRemoteCall(func);
	if (function_id >= STORED_CALLS){
//		LOG_DEBUG(("Funktion %s nicht gefunden. Exit!",func));
		return;
	}

	// parameter_length: Zeiger auf ein Array, das zuerst die Anzahl der Parameter und danach die Anzahl der Bytes fuer die jeweiligen Parameter enthaelt
	#ifdef PC
		parameter_count = calls[function_id].param_count;
		parameter_length = (uint8*)calls[function_id].param_len;	
	#else
		// Auf dem MCU muessen wir die Daten erstmal aus dem Flash holen
		uint8* from= (uint8*)& calls[function_id].param_len;
		uint8 i;
		parameter_count = pgm_read_byte(&calls[function_id].param_count);	
		for (i=0; i<REMOTE_CALL_MAX_PARAM; i++)
			parameter_length[i] = (uint8) pgm_read_byte ( from++ );	
	#endif
//	LOG_DEBUG(("func=%s param_count=%d Len= %u %u %u %u",func,parameter_count,parameter_length[0],parameter_length[1],parameter_length[2]));
//	if (data != NULL){
//		LOG_DEBUG(("data= %u %u %u %u",data[0],data[1],data[2],data[3]));
//	}
	
	#ifdef MCU	// Die MCU legt die Parameter nach einem anderen Verfahren ab, diese Funktion konvertiert sie deshalb
		remotecall_convert_params(parameter_data, parameter_count, parameter_length, (uint8*)data);
	#else	// Auf dem PC kopieren wir die Daten einfach
		memcpy(parameter_data, data, parameter_count*4);
	#endif
	
	running_behaviour=REMOTE_CALL_SCHEDULED;
	activateBehaviour(bot_remotecall_behaviour);
}

/*!
 * Fuehre einen remote_call aus. Es gibt KEIN aufrufendes Verhalten!!
 * @param data Zeiger die Payload eines Kommandos. Dort muss zuerst ein String mit dem Fkt-Namen stehen. ihm folgen die Nutzdaten
 */
void bot_remotecall_from_command(uint8 * data){
	char * function_name = (char*)data;
	remote_call_data_t * params = (remote_call_data_t *)(data+ strlen(function_name)+1);
	bot_remotecall(function_name,params);
}


/*! 
 * Listet alle verfuegbaren Remote-Calls auf und verschickt sie als einzelne Kommanods
 */
void remote_call_list(void){
	#ifdef MCU
		call_t call_storage;
		uint8* to;
		uint8* from;
	#endif
	call_t* call;
	
	int16 i;
	for (i=0; i< (STORED_CALLS); i++){
		#ifdef MCU
			// Auf dem MCU muessen die Daten erstmal aus dem Flash ins RAM
			from= (uint8*)&calls[i];
			to= (uint8*)&call_storage;
			uint8 j;
			for (j=0; j< sizeof(call_t); j++){
				*to = (uint8) pgm_read_byte ( from++ );	
				to++;
			}
			call = &call_storage;
		#else
			call = (call_t*)&calls[i];
		#endif
		
		#ifdef COMMAND_AVAILABLE 
			// und uebertragen
			command_write_rawdata(CMD_REMOTE_CALL,SUB_REMOTE_CALL_ENTRY,&i,&i, sizeof(call_t),(uint8*)call);
		#endif

//		LOG_DEBUG(("%s(%s)",call->name,call->param_info));
	}
}

#endif
