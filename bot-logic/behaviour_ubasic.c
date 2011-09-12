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
 * \file 	behaviour_ubasic.c
 * \brief 	Basic-Interpreter als Verhalten
 * \author 	Frank Menzel (menzelfr@gmx.de)
 * \date 	10.06.2010
 */

#include "bot-logic.h"
#ifdef BEHAVIOUR_UBASIC_AVAILABLE

#include "ui/available_screens.h"
#include "ubasic_config.h"
#include "tokenizer_access.h"
#include "ubasic.h"
#include "ubasic_avr.h"
#include "ubasic_call.h"
#include "tokenizer.h"
#include "ubasic_ext_proc.h"
#include "botfs.h"
#include "init.h"
#include "log.h"
#include "rc5-codes.h"
#include "display.h"

#include <stdio.h>
#include <string.h>

//#define DEBUG_UBASIC_BEHAV /**< Debug-Schalter fuer das uBasic-Verhalten */

#define PROG_FILE_NAME	"/basic/basX.txt" /**< Name der Programmdateien, X wird durch 1 bis 9 ersetzt */
#define PROG_FILE_EXT	".txt" /**< Dateinamenerweiterung (PROG_FILE_NAME muss hierauf enden) */

#ifndef LOG_AVAILABLE
#undef DEBUG_UBASIC_BEHAV
#endif

#ifndef DEBUG_UBASIC_BEHAV
#undef LOG_DEBUG
#define LOG_DEBUG(...)
#endif // DEBUG_UBASIC_BEHAV

botfs_file_descr_t ubasic_prog_file = { 0, 0, 0, 0, { 0, 0, 0 } }; /**< Basic-Programmdatei */

// wenn Speedvariablen direkt angesprochen werden, dann muessen sie im Verhalten selbst nach jeder Zeile direkt in die echten
// speedWish-vars geschrieben werden, damit eine fluessige Bewegung erfolgt, denn sonst sind diese in den anderen Steps 0
static int16_t speedWishLeftBas = 0; /**< vom Basic-Prog gewuenschte Geschwindigkeit links */
static int16_t speedWishRightBas = 0; /**< vom Basic-Prog gewuenschste Geschwindigkeit rechts */
static uint32_t wait_until = 0; /**< Systemzeit, bis zu der gewartet werden soll (WAIT in Basic) */
Behaviour_t * ubasic_behaviour_data; /**< Verhaltensdatensatz des ubasis-Verhaltens */
char ubasic_content = 0; /**< aktuelles Zeichen des Basic-Programms */
uint16_t ubasic_ptr = 0; /**< aktuelle Position im Basic-Programm */
char current_proc[MAX_PROG_NAME_LEN]; /**< aktueller Programmname */

/**
 * Laedt ein uBasic-Programm aus deiner BotFS-Datei
 * \param *filename Dateiname des Programms
 * \param *file Zeiger auf Dateideskriptor der Programmdatei
 */
void bot_ubasic_load_file(char * filename, botfs_file_descr_t * file) {
	(void) filename;
	LOG_DEBUG("f=\"%s\"", filename);
	ubasic_prog_file = *file;
	botfs_rewind(file);
	ubasic_ptr = 0xffff;
#if UBASIC_EXT_PROC
	strncpy(current_proc, filename, MAX_PROG_NAME_LEN);
#endif
}

/**
 * Liest eine uBasic-Programmdatei ein
 * \param keynum	Ziffer im Basic-Dateinamen
 * \return			0, falls Datei korrekt geladen
 */
static int8_t read_ubasic_src(const char keynum) {
	static char fname[] = PROG_FILE_NAME;
	const size_t num = strlen(PROG_FILE_NAME) - strlen(PROG_FILE_EXT) - 1;
	fname[num] = keynum;
	LOG_DEBUG("zu ladende Datei: \"%s\"", fname);

	ubasic_prog_file.start = 0;
	if (botfs_open(fname, &ubasic_prog_file, BOTFS_MODE_r, GET_MMC_BUFFER(ubasic_buffer)) != 0) {
		LOG_ERROR("Konnte Basic-Programm nicht laden:");
		LOG_ERROR(" Datei \"%s\" nicht vorhanden", fname);
		return -1;
	}

	bot_ubasic_load_file(fname, &ubasic_prog_file);
	return 0;
}

/**
 * Hilfsroutine, um in Basic innerhalb eines Steps beide Variablen mit der Bot-Geschwindigkeit belegen zu koennen
 * \param speedLeft	 Geschwindigkeitswert fuer links
 * \param speedRight Geschwindigkeitswert fuer rechts
 */
void ubasic_set_speed(int16_t speedLeft, int16_t speedRight) {
	speedWishLeftBas = speedLeft;
	speedWishRightBas = speedRight;
}

/**
 * Rueckgabe ob das zuletzt aufgerufene Verhalten noch aktiv ist oder nicht; festgestellt anhand der Verhaltens-Data-Struktur des ubasic-Verhaltens
 * \param *behaviour	Zeiger auf Verhaltensdatensatz zum abzufragenden Verhalten
 * \return 				!= 0 wenn das zuletzt aufgerufene Verhalten noch laeuft; 0 wenn es nicht mehr laeuft (Achtung: wait ist auch ein Verhalten)
 */
uint8_t ubasic_behaviour_is_active(Behaviour_t * behaviour) {
	return (uint8_t) (behaviour->subResult == BEHAVIOUR_SUBBACKGR);
}

/**
 * Implementierung des Basic-Wait-Statements fuer ct-Bot.
 * Das eigentliche Warten erfolgt dabei ueber das Verhalten.
 */
void wait_statement(void) {
	accept(TOKENIZER_WAIT);

	const uint32_t delay = (uint32_t) expr();
	/* Wartezeit speichern, wird in bot_ubasic_behaviour() ausgewertet */
	wait_until = TIMER_GET_TICKCOUNT_32 + MS_TO_TICKS(delay);
}

/**
 * uBasic als ct-Bot Verhalten
 * \param *data Zeiger auf den Datensatz des Verhaltens
 */
void bot_ubasic_behaviour(Behaviour_t * data) {
	/* keine neue Zeile ausfuehren, falls nach letztem WAIT noch gewartet werden soll */
	if (wait_until <= TIMER_GET_TICKCOUNT_32) {
		LOG_DEBUG("neue Zeile von uBasic verarbeiten");
		ubasic_run();
		if (ubasic_finished()) {
			LOG_DEBUG("->-> Endeanforderung <-<-");
			return_from_behaviour(data);
			return;
		}
	} else {
		LOG_DEBUG("WAITing");
	}

	/* Speedvariable setzen */
	speedWishLeft = speedWishLeftBas;
	speedWishRight = speedWishRightBas;
}

/**
 * Startet das uBasic-Verhalten
 * \param *caller Zeiger auf den Verhaltensdatensatz des Aufrufers
 */
void bot_ubasic(Behaviour_t * caller) {
	if (ubasic_prog_file.start == 0) {
		LOG_ERROR("Kein Programm geladen");
		return;
	}
	switch_to_behaviour(caller, bot_ubasic_behaviour, BEHAVIOUR_OVERRIDE);
	ubasic_behaviour_data = get_behaviour(bot_ubasic_behaviour);
	ubasic_set_speed(BOT_SPEED_IGNORE, BOT_SPEED_IGNORE); // alte Speed-Wuensche neutralisieren

	ubasic_init(0);
}

/**
 * bricht das aktuelle Basic-Programm ab
 */
void bot_ubasic_break(void) {
	ubasic_break();
}

/**
 * Keyhandler fuer das Basic-Verhalten; laedt nach druecken einer Taste 1-9
 * das Basic-Prog /basic/bas[1-9].txt
 */
#ifdef DISPLAY_UBASIC_AVAILABLE
static void ubasic_disp_key_handler(void) {
	if (RC5_Code >= RC5_CODE_1 && RC5_Code <= RC5_CODE_9) {
		/* Programm bas1 bis bas9 laden */
		const char key = (char) (RC5_Code - RC5_CODE_1 + '1');
		if (read_ubasic_src(key) == 0) {
			bot_ubasic(NULL);
		}
		RC5_Code = 0;
		return;
	}
	switch (RC5_Code) {
#ifdef RC5_CODE_PLAY
	case RC5_CODE_PLAY:
		/* Abspielen eines geladenen Basic-Programms */
		bot_ubasic(NULL);
		RC5_Code = 0;
		break;
#endif // RC5_CODE_PLAY

#ifdef RC5_CODE_STOP
	case RC5_CODE_STOP:
		LOG_DEBUG("Breche uBasic-Programm ab");
		ubasic_break();
		RC5_Code = 0;
		break;
#endif // RC5_CODE_STOP
	}
}

/**
 * Display fuer das uBasic-Verhalten
 */
void ubasic_display(void) {
	display_cursor(1, 1);
	display_puts("- uBasic -");
#ifdef RC5_CODE_PLAY
	display_cursor(2, 1);
	display_puts("PLAY:Run Prog");
#endif
	display_cursor(3, 1);
	display_puts("1-9:bas1..9.txt");
#ifdef RC5_CODE_STOP
	display_cursor(4, 1);
	display_puts("STOPP:Prog abbrechen");
#endif

	ubasic_disp_key_handler(); // Aufruf des Key-Handlers
}


/**
 * Umschalten des Programm-Kontextes
 * \param p_name Programmname
 */
void switch_proc(char * p_name) {
	botfs_file_descr_t new_prog;
	if (botfs_open(p_name, &new_prog, BOTFS_MODE_r, GET_MMC_BUFFER(ubasic_buffer)) != 0) {
		tokenizer_error_print(current_linenum, UNKNOWN_SUBPROC);
		ubasic_break();
	} else {
		bot_ubasic_load_file(p_name, &new_prog);
		program_ptr = 0;
		tokenizer_init(program_ptr);
	}
}
#endif // DISPLAY_UBASIC_AVAILABLE
#endif // BEHAVIOUR_UBASIC_AVAILABLE
