/*
 * Copyright (c) 2006, Adam Dunkels
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ------------------------------------------------------
 * Source modified by Uwe Berger (bergeruw@gmx.net); 2010
 * ------------------------------------------------------
 */

/**
 * \file 	ubasic_tokenizer.h
 * \brief 	uBasic Tokenizer
 * \author	Adam Dunkels
 * \author 	Uwe Berger (bergeruw@gmx.net)
 * \date 	10.06.2010
 */

#ifndef __UBASIC_TOKENIZER_H__
#define __UBASIC_TOKENIZER_H__

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "bot-logic/ubasic_config.h"

enum {
	TOKENIZER_ERROR, // 0
	TOKENIZER_ENDOFINPUT,
	TOKENIZER_NUMBER,
	TOKENIZER_STRING,
	TOKENIZER_VARIABLE,
	TOKENIZER_LET, // 5
	TOKENIZER_PRINT,
	TOKENIZER_IF,
	TOKENIZER_THEN,
	TOKENIZER_ELSE,
	TOKENIZER_FOR, // 10
	TOKENIZER_TO,
	TOKENIZER_DOWNTO,
	TOKENIZER_STEP,
	TOKENIZER_NEXT,
	TOKENIZER_GOTO, // 15
	TOKENIZER_GOSUB,
	TOKENIZER_RETURN,
	TOKENIZER_END,
	TOKENIZER_COMMA,
	TOKENIZER_SEMICOLON, // 20
	TOKENIZER_PLUS,
	TOKENIZER_MINUS,
	TOKENIZER_AND,
	TOKENIZER_OR,
	TOKENIZER_ASTR, // 25
	TOKENIZER_SLASH,
	TOKENIZER_MOD,
	TOKENIZER_LEFTPAREN,
	TOKENIZER_RIGHTPAREN,
	TOKENIZER_LT, // 30
	TOKENIZER_GT,
	TOKENIZER_EQ,

#if UBASIC_ABS
	TOKENIZER_ABS,
#endif
#if UBASIC_NOT
	TOKENIZER_NOT,
#endif
#if UBASIC_REM
	TOKENIZER_REM, // 35
#endif
#if UBASIC_CALL
	TOKENIZER_CALL,
#endif

#if AVR_RND
	TOKENIZER_SRND,
	TOKENIZER_RND,
#endif
#if AVR_EPOKE
	TOKENIZER_EPOKE,
#endif
#if AVR_EPEEK
	TOKENIZER_EPEEK,
#endif
#if AVR_WAIT
	TOKENIZER_WAIT,
#endif
#if BOT_WAIT
	TOKENIZER_WAIT,
#endif
#if AVR_DIR
	TOKENIZER_DIR,
#endif
#if AVR_IN
	TOKENIZER_IN,
#endif
#if AVR_OUT
	TOKENIZER_OUT,
#endif
#if AVR_ADC
	TOKENIZER_ADC,
#endif

#if UBASIC_CVARS
	TOKENIZER_VPOKE,
	TOKENIZER_VPEEK,
#endif

	TOKENIZER_CR, // 40
};

void tokenizer_init(const char * program);
void tokenizer_next(void);
int tokenizer_token(void);
int tokenizer_num(void);
int tokenizer_variable_num(void);
void tokenizer_string(char * dest, int len);
char tokenizer_letter(void);

int tokenizer_finished(void);
void tokenizer_error_print(int linenum, int error_nr);

/**
 * ct-Bot Anpassung: Setzt die Hook-Funktion zur Initialisierung
 * \param *init_fkt Zeiger auf gewuenschte Init-Funktion
 */
void tokenizer_set_init_hook(void(* init_fkt)(const char * *));

/**
 * ct-Bot Anpassung: Setzt die Hook-Funktion zum Laden einer neuen Zeile
 * \param *next_line_fkt Zeiger auf gewuenschte Lade-Funktion
 */
void tokenizer_set_next_line_hook(void(* next_line_fkt)(const char * *));

#endif // BEHAVIOUR_UBASIC_AVAILABLE
#endif // __UBASIC_TOKENIZER_H__
