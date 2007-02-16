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

/*! @file 	remote_calls.h
 * @brief 	Liste mit Botenfkts, die man aus der Ferne aufrufen kann
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	19.12.06
*/

#ifndef REMOTE_CALLS_H_
#define REMOTE_CALLS_H_

#include "bot-logik.h"




#define REMOTE_CALL_FUNCTION_NAME_LEN 20
#define PARAM_TEXT_LEN 40
#define REMOTE_CALL_MAX_PARAM 3

#define REMOTE_CALL_BUFFER_SIZE (REMOTE_CALL_FUNCTION_NAME_LEN+1+REMOTE_CALL_MAX_PARAM*4)

// Die Kommandostruktur
typedef struct {
   uint8 param_count;			/*!< Anzahl der Parameter kommen Und zwar ohne den obligatorischen caller-parameter*/
   uint8 param_len[REMOTE_CALL_MAX_PARAM];	/*!< Angaben ueber die Anzahl an Bytes, die jeder einzelne Parameter belegt */
   char name[REMOTE_CALL_FUNCTION_NAME_LEN+1]; 	    /*!< Text, maximal TEXT_LEN Zeichen lang +  1 Zeichen terminierung*/
   char param_info[PARAM_TEXT_LEN+1];			/*!< String, der Angibt, welche und was fuer Parameter die Fkt erwartet */
   
   void* (*func)(void *);      /*!< Zeiger auf die auszufuehrende Funktion*/   									 
} call_t;

typedef union{
	uint32 u32;
	float fl32;
} remote_call_data_t;	/*!< uint32 und float werden beide gleich ausgelesen, daher stecken wir sie in einen Speicherbereich */

/*! Dieses Makro bereitet eine Botenfunktion als Remote-Call-Funktion vor. 
 * Der erste parameter ist der Funktionsname selbst
 * Der zweite Parameter ist die Anzahl an Bytes, die die Fkt erwartet.
 * Und zwar unabhaengig vom Datentyp. will man also einen uin16 uebergeben steht da 2
 * Will man einen Float uebergeben eine 4. Fuer zwei Floats eine 8, usw.
 */
#define PREPARE_REMOTE_CALL(func,count,param,param_len...)  {count, {param_len}, #func,param,(void*)func }


/*! 
 * Dieses Verhalten kuemmert sich darum die Verhalten, die von außen angefragt wurden zu starten und liefert ein feedback zurueck, wenn sie beendet sind.
 * @param *data der Verhaltensdatensatz
 */
void bot_remotecall_behaviour(Behaviour_t *data);

/*!
 * Fuehre einen remote_call aus. Es gibt KEIN aufrufendes Verhalten!!
 * @param func Zeiger auf den Namen der Fkt
 * @param data Zeiger auf die Daten
 */
void bot_remotecall(char* func, remote_call_data_t* data);

/*!
 * Fuehre einen remote_call aus. Es gibt KEIN aufrufendes Verhalten!!
 * @param data Zeiger die Payload eines Kommandos. Dort muss zuerst ein String mit dem Fkt-Namen stehen. ihm folgen die Nutzdaten
 */
void bot_remotecall_from_command(uint8 * data);

/*! Listet alle verfuegbaren Remote-Calls auf und verschickt sie als einzelne Kommanods
 */
void remote_call_list(void);


#endif /*REMOTE_CALLS_H_*/
