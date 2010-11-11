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

/**
 * \file 	behaviour_remotecall.h
 * \brief 	Ruft auf ein Kommando hin andere Verhalten auf und bestaetigt dann ihre Ausfuehrung
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	19.12.2006
 */

#ifndef BEHAVIOUR_REMOTECALL_H_
#define BEHAVIOUR_REMOTECALL_H_

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE

#define REMOTE_CALL_FUNCTION_NAME_LEN 20	/**< Laenge der Funktionsnamen */
#define PARAM_TEXT_LEN 40					/**< Laenge des Parameterstrings */
#define REMOTE_CALL_MAX_PARAM 3				/**< Maximale Anzahl an Parametern */

/** Groesse des Remotecall-Buffers */
#define REMOTE_CALL_BUFFER_SIZE (REMOTE_CALL_FUNCTION_NAME_LEN + 1 + REMOTE_CALL_MAX_PARAM * 4)

/** Kommandostruktur fuer Remotecalls */
typedef struct {
   uint8_t param_count;									/**< Anzahl der Parameter kommen Und zwar ohne den obligatorischen caller-Parameter */
   uint8_t param_len[REMOTE_CALL_MAX_PARAM];			/**< Angaben ueber die Anzahl an Bytes, die jeder einzelne Parameter belegt */
   const char name[REMOTE_CALL_FUNCTION_NAME_LEN + 1];	/**< Text, maximal TEXT_LEN Zeichen lang +  1 Zeichen Terminierung */
   const char param_info[PARAM_TEXT_LEN + 1];			/**< String, der Angibt, welche und was fuer Parameter die Fkt erwartet */
   void (* func) (Behaviour_t *, ...);					/**< Zeiger auf die auszufuehrende Funktion */
} PACKED remotecall_entry_t;

/** Union fuer Remotecall-Daten */
typedef union {
	uint32_t u32;	/**< 32 Bit unsigned integer */
	int32_t s32;	/**< 32 Bit signed integer */
	float fl32;		/**< 32 Bit float */
	uint16_t u16;	/**< 16 Bit unsigned integer */
	int16_t s16;	/**< 16 Bit signed integer */
	uint8_t u8;		/**<  8 Bit unsigned integer */
	int8_t s8;		/**<  8 Bit signed integer */
} remote_call_data_t;

/**
 * Dieses Makro bereitet eine Botenfunktion als Remote-Call-Funktion vor.
 * Der erste parameter ist der Funktionsname selbst
 * Der zweite Parameter ist die Anzahl an Bytes, die die Fkt erwartet.
 * Und zwar unabhaengig vom Datentyp. will man also einen uin16 uebergeben steht da 2
 * Will man einen Float uebergeben eine 4. Fuer zwei Floats eine 8, usw.
 */
#define PREPARE_REMOTE_CALL(func, count, param, ...)  {count, {__VA_ARGS__}, #func, param, (void (*) (Behaviour_t *, ...)) func}

/** Liste aller Botenfunktionen von Verhalten, die remote aufgerufen werden koennen */
extern const remotecall_entry_t remotecall_beh_list[];

/**
 * Dieses Verhalten kuemmert sich darum die Verhalten, die von aussen angefragt wurden zu starten
 * und liefert ein Feedback zurueck, wenn sie beendet sind.
 * \param *data der Verhaltensdatensatz
 */
void bot_remotecall_behaviour(Behaviour_t * data);

/**
 * Fuehrt einen RemoteCall aus
 * \param *caller	Zeiger auf das aufrufende Verhalten
 * \param *func 	Zeiger auf den Namen der Fkt
 * \param *data		Zeiger auf die Daten
 * \return 			Fehlercode (0: RemoteCall gestartet, -1: noch ein RC aktiv, -2: Funktion nicht gefunden)
 */
int8_t bot_remotecall(Behaviour_t * caller, const char * func, const remote_call_data_t * data);

/**
 * Fuehrt einen RemoteCall aus. Es gibt KEIN aufrufendes Verhalten!
 * \param *data Zeiger die Payload eines Kommandos. Dort muss zuerst ein String mit dem Fkt-Namen stehen.
 * Darauf folgen die Nutzdaten
 */
void bot_remotecall_from_command(const char * data);

/**
 * Bricht einen laufenden RemoteCall ab
 */
void bot_remotecall_cancel(void);

/**
 * Listet alle verfuegbaren Remote-Calls auf und verschickt sie als einzelne Kommanods
 */
void bot_remotecall_list(void);

/**
 * Displayhandler fuer RemoteCall-Display
 */
void remotecall_display(void);
#endif // BEHAVIOUR_REMOTECALL_AVAILABLE
#endif // BEHAVIOUR_REMOTECALL_H_
