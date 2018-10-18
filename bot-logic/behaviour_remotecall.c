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
 * \file 	behaviour_remotecall.c
 * \brief 	Ruft auf ein Kommando hin andere Verhalten auf und bestaetigt dann ihre Ausfuehrung
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	07.12.06
 * \see		<a href="../../Documentation/RemoteCall.html">RemoteCall.html</a>
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "command.h"
#include "log.h"
#include "bot-2-bot.h"
#include "trace.h"
#include "display.h"
#include "rc5.h"
#include "rc5-codes.h"
#include "gui.h"

#define REMOTE_CALL_IDLE 0
#define REMOTE_CALL_SCHEDULED 1
#define REMOTE_CALL_RUNNING 2

/** Uebergabevariable fuer Remotecall-Verhalten */
static uint8_t running_behaviour = REMOTE_CALL_IDLE;

static uint8_t function_id = 255; /**< ID der zu startenden Botenfunktion */
static uint8_t parameter_count = 0; /**< Anzahl der Paramter (ohne Zeiger auf Aufrufer) */
#ifdef MCU
static uint8_t parameter_data[8] = {0}; /**< Hier liegen die eigentlichen Parameter, derzeit brauchen wir maximal 8 Byte (2 floats, 4 (u)int16 oder 4 (u)int8 */
static uint8_t parameter_length[REMOTE_CALL_MAX_PARAM] = {0}; /**< Hier speichern wir die Laenge der jeweiligen Parameter */
#else // PC
static uint8_t parameter_data[REMOTE_CALL_MAX_PARAM * sizeof(remote_call_data_t)] = {0}; /**< Hier liegen die eigentlichen Parameter */
static const uint8_t * parameter_length = NULL; /**< Hier speichern wir die Laenge der jeweiligen Parameter */
#endif // MCU

#if REMOTE_CALL_MAX_PARAM > 3
#error "Mehr als 3 Parameter werden vom Remote-Call-Code derzeit nicht unterstuetzt! Codeanpassung noetig!"
#endif

//#define DEBUG_REMOTE_CALLS /**< Schalter um recht viel Debug-Code anzumachen */

#ifndef LOG_AVAILABLE
#undef DEBUG_REMOTE_CALLS
#endif

#ifndef DEBUG_REMOTE_CALLS
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif


/**
 * Dieses Makro bereitet eine Botenfunktion als Remote-Call-Funktion vor.
 *  - Der erste Parameter ist der Funktionsname selbst
 *  - Der zweite Parameter ist die Anzahl Parametern, die die Fkt erwartet.
 *  - Der dritte Parameter ist ein String, der die Parameter der Fkt beschreibt.
 *  - Danach folgen fuer jeden Parameter im String die Anzahl der Bytes, die der
 *    jeweilige Parameter benoetigt: Will man also einen uint16_t uebergeben steht da 2,
 *    will man einen float uebergeben 4.
 */
#define PREPARE_REMOTE_CALL(func, count, param, ...)  {count, {__VA_ARGS__}, #func, param, (Behaviour_t * (*) (Behaviour_t *, ...)) func, func##_behaviour}

#define PREPARE_REMOTE_CALL_ALIAS(func, count, param, ...)  {count, {__VA_ARGS__}, #func, param, (Behaviour_t * (*) (Behaviour_t *, ...)) func, NULL}

#define PREPARE_REMOTE_CALL_MANUAL(func, beh_func, count, param, ...)  {count, {__VA_ARGS__}, #func, param, (Behaviour_t * (*) (Behaviour_t *, ...)) func, beh_func}

/**
 * \brief Hier muessen alle Boten-Funktionen rein, die remote aufgerufen werden sollen.
 *
 * Diese stossen dann das zugehoerige Verhalten an.
 * Ein Eintrag erfolgt so:\n
 * PREPARE_REMOTE_CALL(BOTENFUNKTION, NUMBER_OF_PARAMS, STRING_DER_DIE_PARAMETER_BESCHREIBT,
 * Laenge der jeweiligen Parameter in Byte)\n
 *
 * Alle Botenfunktionen muessen folgendem Schema entsprechen:\n
 * void bot_xxx(Behaviour_t * caller, ...);\n
 *
 * \verbatim
 * Erklaerung am Bsp.:
 * PREPARE_REMOTE_CALL(bot_gotoxy, 2, "float x, float y", 4, 4),
 *   Name der Botenfunktion --^    ^   ^                  ^  ^
 *   Anzahl der Parameter ----------   |                  |  |
 *   Beschreibung der Parameter --------                  |  |
 *   Anzahl der Bytes Parameter 1 -------------------------  |
 *   Anzahl der Bytes Parameter 2 ----------------------------
 * \endverbatim
 *
 * Zur Info:
 * - 1 Byte brauchen: uint8,  int8,  char
 * - 2 Byte brauchen: uint16, int16
 * - 4 Byte brauchen: uint32, int32, float
 */
const remotecall_entry_t remotecall_beh_list[] PROGMEM = {
#ifdef BEHAVIOUR_PROTOTYPE_AVAILABLE
	PREPARE_REMOTE_CALL(bot_prototype, 0, "", 0),
//	PREPARE_REMOTE_CALL(bot_prototype, 1, "int16 param", 2),
#endif
	/* Demo-Verhalten fuer Einsteiger */
#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
	PREPARE_REMOTE_CALL(bot_simple, 0, "", 0),
	PREPARE_REMOTE_CALL(bot_simple2, 1, "int16 light", 2),
#endif
#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
	PREPARE_REMOTE_CALL(bot_drive_square, 0, "", 0),
	PREPARE_REMOTE_CALL_ALIAS(bot_drive_square_len, 1, "int16 length", 2),
#endif

	/* Hardware-Test Verhalten */
#ifdef BEHAVIOUR_HW_TEST_AVAILABLE
	PREPARE_REMOTE_CALL(bot_hw_test, 1, "uint8 mode", 1),
#endif
	/* Kalibrierungs-Verhalten fuer Bot-Setup */
#ifdef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
	PREPARE_REMOTE_CALL(bot_calibrate_pid, 1, "int16 speed", 2),
#endif
#ifdef BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
	PREPARE_REMOTE_CALL(bot_calibrate_sharps, 0, "", 0),
#endif

	/* Anwendungs-Verhalten, die komplexere Aufgaben erfuellen */
#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
	PREPARE_REMOTE_CALL(bot_solve_maze, 0, "", 0),
#endif
#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
	PREPARE_REMOTE_CALL(bot_follow_line, 0, "", 0),
#endif
#ifdef BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE
	PREPARE_REMOTE_CALL(bot_follow_line_enh, 0, "", 0),
#endif
#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
	PREPARE_REMOTE_CALL(bot_line_shortest_way, 0, "", 0),
#endif
#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
	PREPARE_REMOTE_CALL(bot_catch_pillar, 0, "", 0),
	PREPARE_REMOTE_CALL(bot_unload_pillar, 0, "", 0),
#endif
#ifdef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
	PREPARE_REMOTE_CALL(bot_transport_pillar, 0, "", 0),
#endif
#ifdef BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE
	PREPARE_REMOTE_CALL(bot_classify_objects, 0, "", 0),
#endif
#ifdef BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE
	PREPARE_REMOTE_CALL(bot_follow_object, 0, "", 0),
#endif
#ifdef BEHAVIOUR_FOLLOW_WALL_AVAILABLE
	PREPARE_REMOTE_CALL_MANUAL(bot_do_wall_explore, bot_follow_wall_behaviour, 0, "", 0),
#endif
#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
	PREPARE_REMOTE_CALL(bot_do_slalom, 0, "", 0),
#endif
#ifdef BEHAVIOUR_DRIVE_AREA_AVAILABLE
	PREPARE_REMOTE_CALL(bot_drive_area, 0, "", 0),
#endif
#ifdef BEHAVIOUR_NEURALNET_AVAILABLE
	PREPARE_REMOTE_CALL(bot_neuralnet, 0, "", 0),
#endif
#ifdef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
	PREPARE_REMOTE_CALL(bot_drive_neuralnet, 0, "", 0),
#endif

	/* Fahr- und Positionierungs-Verhalten */
#ifdef BEHAVIOUR_TURN_AVAILABLE
	PREPARE_REMOTE_CALL(bot_turn, 1, "int16 degrees", 2),
	PREPARE_REMOTE_CALL_ALIAS(bot_turn_speed, 3, "int16 degrees, uint16 min, uint16 max", 2, 2, 2),
#endif
#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
	PREPARE_REMOTE_CALL(bot_goto_pos, 3, "int16 x, int16 y, int16 head", 2, 2, 2),
	PREPARE_REMOTE_CALL_ALIAS(bot_goto_dist, 2, "int16 distance, int8 dir", 2, 1),
	PREPARE_REMOTE_CALL_ALIAS(bot_goto_pos_rel, 3, "int16 x, int16 y, int16 head", 2, 2, 2),
#endif
#ifdef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
	PREPARE_REMOTE_CALL(bot_goto_obstacle, 2, "int16 distance, uint8 parallel", 2, 1),
#endif
#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
	PREPARE_REMOTE_CALL(bot_drive_distance, 3, "int8 curve, int16 speed, int16 cm", 1, 2, 2),
#endif
#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
	PREPARE_REMOTE_CALL(bot_gotoxy, 2, "int16 x, int16 y", 2, 2),
#endif
#ifdef BEHAVIOUR_GOTO_AVAILABLE
	PREPARE_REMOTE_CALL(bot_goto, 2, " int16 left, int16 right", 2, 2),
#endif
#ifdef BEHAVIOUR_PATHPLANNING_AVAILABLE
	PREPARE_REMOTE_CALL(bot_calc_wave, 3, "int16 dest_x, int16 dest_y, int8 compare", 2, 2, 1),
#endif
#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
	PREPARE_REMOTE_CALL_ALIAS(bot_push_actpos, 0, "", 0),
	PREPARE_REMOTE_CALL(bot_drive_stack, 0, "", 0),
	PREPARE_REMOTE_CALL_ALIAS(bot_drive_fifo, 0, "", 0),
	PREPARE_REMOTE_CALL(bot_save_waypos, 1, "uint8 optimize", 1),
#ifdef BOT_2_BOT_PAYLOAD_AVAILABLE
	PREPARE_REMOTE_CALL_ALIAS(bot_send_stack_b2b, 1, "uint8 bot", 1),
#endif
#endif // BEHAVIOUR_DRIVE_STACK_AVAILABLE

	/* Servo-Steuerung */
#ifdef BEHAVIOUR_SERVO_AVAILABLE
	PREPARE_REMOTE_CALL(bot_servo, 2, "uint8 servo, uint8 pos", 1, 1),
#endif

	/* Auswertungs- und Mess-Verhalten */
#ifdef BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE
	PREPARE_REMOTE_CALL(bot_check_distance, 2, "int16 max_dist, uint8 diff", 2, 1),
#endif
#ifdef BEHAVIOUR_SCAN_BEACONS_AVAILABLE
	PREPARE_REMOTE_CALL(bot_scan_beacons, 2, "uint8 pos_upd, uint8 mode", 1, 1),
#endif
#ifdef BEHAVIOUR_GET_UTILIZATION_AVAILABLE
	PREPARE_REMOTE_CALL(bot_get_utilization, 1, "uint8 beh", 1),
#endif

	/* Test-Verhalten */
#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
	PREPARE_REMOTE_CALL(bot_turn_test, 0, "", 0),
#endif
#ifdef BEHAVIOUR_TEST_ENCODER_AVAILABLE
	PREPARE_REMOTE_CALL(bot_test_encoder, 0, "", 0),
#endif
#ifdef BOT_2_BOT_PAYLOAD_TEST_AVAILABLE
	PREPARE_REMOTE_CALL_ALIAS(bot_2_bot_pl_test, 1, "uint8 to", 1),
#endif

#ifdef BEHAVIOUR_ABL_AVAILABLE
	PREPARE_REMOTE_CALL_ALIAS(bot_abl_check, 1, "uint16 line", 2),
#endif
	{0, {0}, "", "", NULL, NULL}
};

/** Anzahl der Remote-Calls im Array */
#define STORED_CALLS (sizeof(remotecall_beh_list) / sizeof(remotecall_entry_t))

/**
 * Sucht den Index des Remote-Calls heraus
 * \param *call	String mit dem Namen der gesuchten fkt
 * \return 		Index in das remotecall_beh_list-Array. Wenn nicht gefunden, dann 255
 */
uint8_t get_remotecall_id(const char * call) {
	LOG_DEBUG("Suche nach Funktion: \"%s\"", call);

	uint8_t i;
	for (i = 0; i < STORED_CALLS; ++i) {
		if (!strcmp_P (call, remotecall_beh_list[i].name)) {
			LOG_DEBUG("calls[%u].name=\"%s\" passt", i, call);
			return i;
		}
	}
	return 255;
}

/**
 * Hilfsfunktion fuer bot_remotecall()
 * Baut einen AVR-kompatiblen Parameterstream aus einem uint32_t-Parameterarray und einem Infoarray ueber die Parameter
 * \param *dest	Zeiger auf das Ausgabearray (8 Byte gross)
 * \param count	Anzahl der Parameter
 * \param *len	Zeiger auf ein Array, das die Anzahl der Bytes fuer die jeweiligen Parameter enthaelt
 * \param *data	Zeiger auf die Daten (3 Parameter, jeweils 32 Bit)
 */
static void remotecall_convert_params(uint8_t * dest, const uint8_t count, const uint8_t * len, const void * data) {
	const uint8_t * ptr = data;
	(void) ptr;
	LOG_DEBUG("p_data=%x %x %x %x", ptr[0], ptr[1], ptr[2], ptr[3]);
	LOG_DEBUG("%x %x %x %x", ptr[4], ptr[5], ptr[6], ptr[7]);
	LOG_DEBUG("%x %x %x %x", ptr[8], ptr[9], ptr[10], ptr[11]);
#ifdef MCU
	dest += 8; // ans Ende springen
	uint8_t i;
	/* Parameter rueckwaerts einlesen */
	for (i = 0; i < count; ++i) {
		uint8_t pos = (uint8_t) (len[i] > 1 ? len[i] : 2); // alle Parameter sind 16-Bit-aligned
		dest -= pos;
		memcpy(dest, ptr, len[i]);
		ptr += sizeof(remote_call_data_t);
	}
#else // PC
	(void) len;
#if BYTE_ORDER == LITTLE_ENDIAN
	/* Daten einfach kopieren */
	memcpy(dest, data, count * sizeof(remote_call_data_t));
#else // BIG_EDIAN
	uint8_t i;
	for (i = 0; i < count; ++i) {
		/* Parameter i von little-endian nach big-endian konvertieren*/
		remote_call_data_t in, out;
		in = *((remote_call_data_t *) data);
		out.u32 = ((in.u32 & 0xff) << 24) | ((in.u32 & 0xff00) << 8) | ((in.u32 & 0xff0000) >> 8) | ((in.u32 & 0xff000000) >> 24);
		memcpy(dest, &out, sizeof(remote_call_data_t));
		dest += sizeof(remote_call_data_t);
		data += sizeof(remote_call_data_t);
	}
#endif // LITTLE_EDIAN

#endif // MCU
}

#ifdef PC
void bot_remotecall_fl_dummy(Behaviour_t * caller, ...) __attribute__((noinline));

/**
 * Dummy-Funktion, die nur dafuer sorgt, dass die Parameterdaten auch in den
 * Floating-Point Registern stehen (PPC)
 */
void bot_remotecall_fl_dummy(Behaviour_t * caller, ...) {
	(void) caller; // kein warning
	__asm__ __volatile__("nop");
}
#endif // PC

/**
 * Dieses Verhalten kuemmert sich darum die Verhalten, die von aussen angefragt wurden zu starten
 * und liefert ein Feedback zurueck, wenn sie beendet sind.
 * \param *data der Verhaltensdatensatz
 */
void bot_remotecall_behaviour(Behaviour_t * data) {
	LOG_DEBUG("Enter bot_remotecall_behaviour");

	switch (running_behaviour) {
		case REMOTE_CALL_SCHEDULED: // Es laueft kein Auftrag, aber es steht ein Neuer an
			LOG_DEBUG("REMOTE_CALL_SCHEDULED");

			if (function_id >= STORED_CALLS) {
				LOG_DEBUG("keine Funktion gefunden. Exit");
				running_behaviour = REMOTE_CALL_IDLE;
				exit_behaviour(data, BEHAVIOUR_SUBFAIL);
				return;
			}

#ifdef PC
			Behaviour_t * (* func) (Behaviour_t * data, ...);
			// Auf dem PC liegt die remotecall_beh_list-Struktur im RAM
			func = (Behaviour_t * (*) (Behaviour_t *, ...)) remotecall_beh_list[function_id].func;
#else // MCU
			void (* func) (Behaviour_t * data, remote_call_data_t dword1, remote_call_data_t dword2);
			// Auf dem MCU liegt die remotecall_beh_list-Struktur im Flash und muss erst geholt werden
			func = (void (*) (Behaviour_t *, remote_call_data_t, remote_call_data_t))
				pgm_read_word(&remotecall_beh_list[function_id].func);
#endif // PC

			if (parameter_count > REMOTE_CALL_MAX_PARAM) {
				LOG_DEBUG("Parameteranzahl unzulaessig!");
				running_behaviour = REMOTE_CALL_IDLE;
				return;
			}

			LOG_DEBUG("function_id=%u", function_id);
			LOG_DEBUG("parameter_count=%u", parameter_count);
			remote_call_data_t * parameter = (remote_call_data_t *) parameter_data;
#ifdef PC

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
			bot_remotecall_fl_dummy(data, parameter[0].fl32, parameter[1].fl32, parameter[2].fl32);
#pragma GCC diagnostic pop

			func(data, parameter[0], parameter[1], parameter[2]);
#else // MCU
			func(data, parameter[1], parameter[0]); // "rueckwaerts", denn kleinere Parameter-Nr liegen an hoereren Register-Nr.!
#endif // PC
			running_behaviour = REMOTE_CALL_RUNNING;
			break;

		case REMOTE_CALL_RUNNING: // Es lief ein Verhalten und ist nun zuende (sonst waeren wir nicht hier)
		{
			/* Antwort schicken */
			char * function_name;
			// Auf dem MCU muessen wir die Daten erstmal aus dem Flash holen
			char tmp[REMOTE_CALL_FUNCTION_NAME_LEN + 1];
			memcpy_P(tmp, &remotecall_beh_list[function_id].name, REMOTE_CALL_FUNCTION_NAME_LEN + 1);
			function_name = (char *) &tmp;

			int16_t result = data->subResult;
			(void) result;
			if (data->caller) {
#ifdef BEHAVIOUR_ABL_AVAILABLE
				abl_push_value((uint16_t) result);
#endif
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
				ubasic_push_value((uint16_t) result);
#endif
			}
#ifdef COMMAND_AVAILABLE
			else {
				/* kein Caller, also kam der Aufruf wohl vom Sim */
				command_write_data(CMD_REMOTE_CALL, SUB_REMOTE_CALL_DONE, result, result, function_name);
			}
#else
			(void) function_name;
#endif // COMMAND_AVAILABLE
			LOG_DEBUG("RemoteCall %s beendet (%u)", function_name, result);

			// Aufrauemen
			function_id = 255;
		}
		CASE_NO_BREAK;
		default:
			running_behaviour = REMOTE_CALL_IDLE;
			return_from_behaviour(data); // und Verhalten auch aus
			break;
	}
}

/**
 * Fuehrt einen RemoteCall aus
 * \param *caller	Zeiger auf das aufrufende Verhalten
 * \param id	 	ID des Verhaltens (Index in der Liste)
 * \param *data		Zeiger auf die Daten
 * \return 			Fehlercode (0: RemoteCall gestartet, -1: noch ein RC aktiv, -2: Funktion nicht gefunden)
 */
static int8_t bot_remotecall_from_id(Behaviour_t * caller, const uint8_t id, const remote_call_data_t * data) {
	if (running_behaviour != REMOTE_CALL_IDLE) {
		/* Verhalten noch aktiv, Abbruch */
		LOG_DEBUG("Bereits ein RemoteCall aktiv (ID=%u)!", function_id);
		return -1;
	}

	if (id >= STORED_CALLS - 1) {
		LOG_DEBUG("Funktion mit ID=%u nicht vorhanden.", id);
		return -2;
	}

	switch_to_behaviour(caller, bot_remotecall_behaviour, BEHAVIOUR_NOOVERRIDE);

	function_id = id;
	parameter_count = pgm_read_byte(&remotecall_beh_list[function_id].param_count);

	// parameter_length: Zeiger auf ein Array, das zuerst die Anzahl der Parameter und danach die Anzahl der Bytes fuer die jeweiligen Parameter enthaelt
#ifdef PC
	parameter_length = remotecall_beh_list[function_id].param_len;
#else
	// Auf dem MCU muessen wir die Daten erstmal aus dem Flash holen
	memcpy_P(parameter_length, &remotecall_beh_list[function_id].param_len, parameter_count);
#endif // PC

	LOG_DEBUG("func=%u param_count=%u Len= %u %u %u", remotecall_beh_list[function_id].name, parameter_count, parameter_length[0], parameter_length[1],
		parameter_length[2]);

	remotecall_convert_params(parameter_data, parameter_count, parameter_length, data);

	LOG_DEBUG("p_data=%x %x %x %x", parameter_data[0], parameter_data[1], parameter_data[2], parameter_data[3]);
	LOG_DEBUG("%x %x %x %x", parameter_data[4], parameter_data[5], parameter_data[6], parameter_data[7]);
#ifdef PC
	LOG_DEBUG("%x %x %x %x", parameter_data[8], parameter_data[9], parameter_data[10], parameter_data[11]);
#endif

	running_behaviour = REMOTE_CALL_SCHEDULED;

#ifdef CREATE_TRACEFILE_AVAILABLE
	trace_add_remotecall(remotecall_beh_list[function_id].name, parameter_count, (remote_call_data_t *) parameter_data);
#endif // CREATE_TRACEFILE_AVAILABLE

	return 0;
}

/**
 * Fuehrt einen RemoteCall aus
 * \param *caller	Zeiger auf das aufrufende Verhalten
 * \param *func 	Zeiger auf den Namen der Fkt
 * \param *data		Zeiger auf die Daten
 * \return 			Fehlercode (0: RemoteCall gestartet, -1: noch ein RC aktiv, -2: Funktion nicht gefunden)
 */
int8_t bot_remotecall(Behaviour_t * caller, const char * func, const remote_call_data_t * data) {
	const uint8_t id = get_remotecall_id(func);
	if (id >= STORED_CALLS - 1) {
		LOG_ERROR("Verhalten \"%s\" nicht gefunden.", func);
	}

	return bot_remotecall_from_id(caller, id, data);
}

/**
 * Fuehrt einen RemoteCall aus. Es gibt KEIN aufrufendes Verhalten!
 * \param data Zeiger die Payload eines Kommandos. Dort muss zuerst ein String mit dem Fkt-Namen stehen.
 *             Darauf folgen die Nutzdaten
 */
void bot_remotecall_from_command(const char * data) {
	const char * function_name = data;
	remote_call_data_t * params = (remote_call_data_t *) (data + strlen(function_name) + 1);
	bot_remotecall(NULL, function_name, params);
}

/**
 * Bricht einen laufenden RemoteCall ab
 */
void bot_remotecall_cancel(void) {
	Behaviour_t * const beh = get_behaviour(bot_remotecall_behaviour);
	/* gestartete Verhalten abbrechen */
	deactivate_called_behaviours(beh);
	/* RemoteCall-Verhalten korrekt beenden, um Antwort zu senden */
	bot_remotecall_behaviour(beh);
}

/**
 * Listet alle verfuegbaren Remote-Calls auf und verschickt sie als einzelne Kommandos
 */
void bot_remotecall_list(void) {
#ifdef MCU
	remotecall_entry_t call_storage;
#endif
	const remotecall_entry_t * call;

	LOG_DEBUG("Liste %u RemoteCalls", STORED_CALLS);

	uint16_t i;
	for (i = 0; i < STORED_CALLS; ++i) {
#ifdef MCU
		// Auf dem MCU muessen die Daten erstmal aus dem Flash ins RAM
		memcpy_P(&call_storage, &remotecall_beh_list[i], sizeof(remotecall_entry_t));
		call = &call_storage;
#else
		call = &remotecall_beh_list[i];
#endif // MCU

#ifdef COMMAND_AVAILABLE
		// und uebertragen
		command_write_rawdata(CMD_REMOTE_CALL, SUB_REMOTE_CALL_ENTRY, (int16_t) i, (int16_t) i, sizeof(remotecall_entry_t), call);
#else
		(void) call;
#endif
		LOG_DEBUG("%s(%s)", call->name, call->param_info);
	}
}

#if defined DISPLAY_REMOTECALL_AVAILABLE && defined KEYPAD_AVAILABLE
static remote_call_data_t keypad_params[REMOTE_CALL_MAX_PARAM]; /**< eingebene Parameter */
static uint8_t keypad_param_index= 0; /**< aktueller Parameter */
static uint8_t beh_selected = 0; /**< Status der Eingabe (0: nicht ausgewaehlt, 1: ausgewaehlt, 2: Parametereingabe aktiv */

/**
 * CallBack-Funktion fuer Keypad-Eingabe
 * \param *data Eingebene Daten
 */
static void keypad_param(char * data) {
	if (keypad_param_index < REMOTE_CALL_MAX_PARAM && *data != 0) {
		keypad_params[keypad_param_index].s32 = atoi(data);
		++keypad_param_index;
	}
	beh_selected = 1;
}

/**
 * \brief Displayhandler fuer RemoteCall-Display.
 *
 * Das Display zeigt eine Liste aller verfuegbaren Verhalten an, durch die mit den Tasten "Stopp" (nach unten) und "Pause"
 * (nach oben) geblaettert werden kann. Das Zeichen '>' markiert dabei eine ausgewaehlte Zeile. Bei Verhalten ohne Parameter
 * wird das ausgewaehlte Verhalten mit der Taste "Play" gestartet.
 * Benoetigt ein Verhalten Parameter, waehlt "Play" das Verhalten aus und schaltet auf die Parametereingabe um. Ueber die
 * Zifferntasten koennen nun alle Parameter eingegeben werden, die Taste "Play" schliesst dabei jeweils die Eingabe ab und
 * schaltet zum naechsten Parameter weiter. Mit "Stopp" laesst sich die aktuelle Eingabe loeschen und korrigieren.
 * Ein Minus-Zeichen ('-') fuer negative Parameterwerte laesst sich mit der Taste "11" eingeben.
 * Nach dem letzten Parameter startet "Play" das Verhalten.
 */
void remotecall_display(void) {
	static uint8_t first_row = 0; // erste Verhaltens-ID, die auf dem Display angezeigt wird
	static int8_t selected_row = 0; // Offset der markierten Zeile (relativ zu first_row)
	static uint8_t show_running_beh = 0;

	if (running_behaviour == REMOTE_CALL_IDLE) {
		show_running_beh = 0;
	}

	/* Keyhandler */
	switch (RC5_Code) {
	case RC5_CODE_UP: // zurueck blaettern
		--selected_row;
		keypad_param_index = 0;
		beh_selected = 0;
		RC5_Code = 0;
		break;

	case RC5_CODE_DOWN: // vor blaettern oder laufenden RC abbrechen
		if (running_behaviour == REMOTE_CALL_RUNNING && show_running_beh) {
			bot_remotecall_cancel();
		} else {
			++selected_row;
			keypad_param_index = 0;
			beh_selected = 0;
		}
		RC5_Code = 0;
		break;

	case RC5_CODE_PLAY: // auswahlen / starten
		if (running_behaviour == REMOTE_CALL_RUNNING) {
			show_running_beh = ! show_running_beh;
			break;
		}

		show_running_beh = 1;
		beh_selected = 1;
		first_row = (uint8_t) (first_row + selected_row);
		if (first_row > STORED_CALLS - 2) {
			first_row = (uint8_t) (first_row - (STORED_CALLS - 1));
		}
		selected_row = 0;
		RC5_Code = 0;
		break;
	}

	/* Anzeige */
	display_clear();
	if (selected_row > 2) {
		selected_row = 2;
		++first_row;
		if (first_row > STORED_CALLS - 2) {
			first_row = 0;
		}
	} else if (selected_row < 0) {
		selected_row = 0;
		--first_row;
		if (first_row > STORED_CALLS - 2) {
			first_row = STORED_CALLS - 2;
		}
	}

	uint8_t row;
	const remotecall_entry_t * ptr_call;
	const uint8_t n = (uint8_t) (beh_selected > 0 || (running_behaviour == REMOTE_CALL_RUNNING && show_running_beh) ? 1 : 3);
	uint8_t i;
	for (i = 1; i <= n; ++i) {
		if (running_behaviour == REMOTE_CALL_RUNNING && show_running_beh) {
			row = function_id;
		} else {
			row = (uint8_t) (first_row + (i - 1));
			if (row > STORED_CALLS - 2) {
				row = (uint8_t) (row - (STORED_CALLS - 1));
			}
		}

		ptr_call = &remotecall_beh_list[row];
		display_cursor(i, 1);
		const Behaviour_t* p_beh = ptr_call->beh_func ? get_behaviour(ptr_call->beh_func) : NULL;
		if (p_beh) {
//			LOG_DEBUG("p_beh->prio = %u", p_beh->priority);
//			LOG_DEBUG("p_beh->active = %d", p_beh->active);
			if (! show_running_beh) {
				if (p_beh->active == BEHAVIOUR_ACTIVE) {
					display_puts("A");
				} else if (p_beh->subResult == BEHAVIOUR_SUBRUNNING || p_beh->subResult == BEHAVIOUR_SUBBACKGR) {
					display_puts("S");
				}
			}
			if (i == selected_row + 1) {
				display_cursor(4, 16);
				display_printf("P=%3u", p_beh->priority);
			}
		}
		display_cursor(i, 2);
		if (! show_running_beh) {
			const char tmp = (char) (i == (selected_row + 1) ? '>' : ' ');
			display_printf("%c", tmp);
		}
		display_flash_puts(ptr_call->name + (sizeof("bot_") - 1));
	}

	display_cursor(4, 1);
	if (running_behaviour == REMOTE_CALL_RUNNING && show_running_beh) {
		display_puts("Abbruch: Stopp");
		display_cursor(3, 1);
		display_puts(" gestartet.");
		display_cursor(2, 1);
		display_puts(" (");
		uint8_t j;
		for (j = 0; j < parameter_count; ++j) {
			if (j != 0) {
				display_puts(",");
			}
#ifdef MCU
			int16_t * ptr = (int16_t *) parameter_data;
			int16_t tmp = ptr[(sizeof(parameter_data) / sizeof(int16_t) - 1) - j];
			if ((tmp & 0x80) && pgm_read_byte(&ptr_call->param_len[j]) == 1) {
				/* Vorzeichenerweiterung */
				tmp = (int16_t) (tmp | (int16_t) 0xff00);
			}
#else // PC
			remote_call_data_t * ptr = (remote_call_data_t * ) parameter_data;
			const int16_t tmp = ptr[j].s16;
#endif // MCU
			display_printf("%d", tmp);
		}
		display_puts(")");
		return;
	}

	const uint8_t par_count = pgm_read_byte(&ptr_call->param_count);
	if (keypad_param_index < par_count && beh_selected > 0) {
		display_puts("ok:Play | Korr:Stopp");
		display_cursor(2, 1);
		const char * p_info = ptr_call->param_info;
		uint8_t j;
		for (j = 0; j < keypad_param_index; ++j) {
			p_info = strchr_P(p_info, ',');
			p_info += 2; // ", " weg
		}
		display_flash_puts(p_info);
		const char * p_info_end = strchr_P(p_info, ',');
		if (p_info_end != NULL) {
			uint8_t len = (uint8_t) (p_info_end - p_info);
			display_cursor(2, (int16_t) (len + 1));
			for (j = len; j < DISPLAY_LENGTH; ++j) {
				display_puts(" ");
			}
		}
		display_cursor(3, 1);
		display_printf("par%1u/%1u: ", keypad_param_index + 1, par_count);
		if (beh_selected == 1) {
			keypad_params[keypad_param_index].u32 = 0;
			beh_selected = 2;
			gui_keypad_request(keypad_param, 1, 3, 9);
		}
	} else if (par_count - keypad_param_index == 0 && running_behaviour == REMOTE_CALL_IDLE) {
		display_puts("Start: Play");
		if (beh_selected == 1) {
			beh_selected = 0;
			keypad_param_index = 0;
			LOG_DEBUG("first_row=%u", first_row);
#ifdef PC
			LOG_DEBUG("starte \"%s\"", remotecall_beh_list[first_row].name);
#endif
			Behaviour_t* const beh = get_behaviour(bot_remotecall_behaviour);
			bot_remotecall_from_id(beh, first_row, keypad_params);
			show_running_beh = 1;
		}
	} else if (beh_selected == 0 && running_behaviour == REMOTE_CALL_IDLE) {
		display_puts("Auswahl: Play");
	} else if (running_behaviour == REMOTE_CALL_RUNNING) {
		display_puts("Details: Play");
	}
}
#endif // DISPLAY_REMOTECALL_AVAILABLE

#endif // BEHAVIOUR_REMOTECALL_AVAILABLE
