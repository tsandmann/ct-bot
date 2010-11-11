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
 * \file 	ubasic_tokenizer.c
 * \brief 	uBasic Tokenizer
 * \author	Adam Dunkels
 * \author 	Uwe Berger (bergeruw@gmx.net)
 * \date 	10.06.2010
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "log.h"
#include "botfs.h"

//#define DEBUG 1

#if DEBUG
#define DEBUG_PRINTF(...)  LOG_DEBUG(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include "bot-logic/ubasic_tokenizer.h"
#include "bot-logic/ubasic_config.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

static char const * ptr, * nextptr;
/** ct-Bot Anpassung */
static void (* next_line_hook)(const char * * p_prog) = NULL;
static void (* init_hook)(const char * * p_prog) = NULL;


#define MAX_NUMLEN 5

struct keyword_token {
	const char * keyword;
	int token;
} PACKED;

static int current_token = TOKENIZER_ERROR;

/** \todo: ins Flash */
static const struct keyword_token keywords[] = {
	{ "let", TOKENIZER_LET },
	{ "print", TOKENIZER_PRINT },
	{ "if", TOKENIZER_IF },
	{ "then", TOKENIZER_THEN },
	{ "else", TOKENIZER_ELSE },
	{ "for", TOKENIZER_FOR },
	{ "to", TOKENIZER_TO },
	{ "downto",	TOKENIZER_DOWNTO },
	{ "step", TOKENIZER_STEP },
	{ "next", TOKENIZER_NEXT },
	{ "goto", TOKENIZER_GOTO },
	{ "gosub", TOKENIZER_GOSUB },
	{ "return", TOKENIZER_RETURN },
	{ "end", TOKENIZER_END },
#if UBASIC_ABS
	{ "abs", TOKENIZER_ABS },
#endif
#if UBASIC_NOT
	{ "not", TOKENIZER_NOT },
#endif
#if UBASIC_CALL
	{ "call", TOKENIZER_CALL },
#endif
#if UBASIC_CVARS
	{ "vpoke", TOKENIZER_VPOKE },
	{ "vpeek", TOKENIZER_VPEEK },
#endif
#if UBASIC_REM
	{ "rem", TOKENIZER_REM },
#endif
#if AVR_RND
	{ "srand", TOKENIZER_SRND},
	{ "rand", TOKENIZER_RND},
#endif
#if AVR_EPOKE
	{ "epoke", TOKENIZER_EPOKE},
#endif
#if AVR_EPEEK
	{ "epeek", TOKENIZER_EPEEK},
#endif
#if AVR_WAIT
	{ "wait", TOKENIZER_WAIT},
#endif
#if BOT_WAIT
	{ "wait", TOKENIZER_WAIT },
#endif
#if AVR_DIR
	{ "dir", TOKENIZER_DIR},
#endif
#if AVR_IN
	{ "in", TOKENIZER_IN},
#endif
#if AVR_OUT
	{ "out", TOKENIZER_OUT},
#endif
#if AVR_ADC
	{ "adc", TOKENIZER_ADC},
#endif
	{ NULL, TOKENIZER_ERROR }
};

/*---------------------------------------------------------------------------*/
static int singlechar(void) {
	if (*ptr == '\n') {
		return TOKENIZER_CR;
	} else if (*ptr == ',') {
		return TOKENIZER_COMMA;
	} else if (*ptr == ';') {
		return TOKENIZER_SEMICOLON;
	} else if (*ptr == '+') {
		return TOKENIZER_PLUS;
	} else if (*ptr == '-') {
		return TOKENIZER_MINUS;
	} else if (*ptr == '&') {
		return TOKENIZER_AND;
	} else if (*ptr == '|') {
		return TOKENIZER_OR;
	} else if (*ptr == '*') {
		return TOKENIZER_ASTR;
	} else if (*ptr == '/') {
		return TOKENIZER_SLASH;
	} else if (*ptr == '%') {
		return TOKENIZER_MOD;
	} else if (*ptr == '(') {
		return TOKENIZER_LEFTPAREN;
	} else if (*ptr == ')') {
		return TOKENIZER_RIGHTPAREN;
	} else if (*ptr == '<') {
		return TOKENIZER_LT;
	} else if (*ptr == '>') {
		return TOKENIZER_GT;
	} else if (*ptr == '=') {
		return TOKENIZER_EQ;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
static int get_next_token(void) {
	struct keyword_token const * kt;
	int i;

	if (*ptr == 0) {
		return TOKENIZER_ENDOFINPUT;
	}

	if (isdigit(*ptr)) {
		for (i = 0; i < MAX_NUMLEN; ++i) {
			if (!isdigit(ptr[i])) {
				if (i > 0) {
					nextptr = ptr + i;
					return TOKENIZER_NUMBER;
				} else {
					DEBUG_PRINTF("get_next_token: error due to too short number");
					return TOKENIZER_ERROR;
				}
			}
			if (!isdigit(ptr[i])) {
				DEBUG_PRINTF("get_next_token: error due to malformed number");
				return TOKENIZER_ERROR;
			}
		}
		DEBUG_PRINTF("get_next_token: error due to too long number");
		return TOKENIZER_ERROR;
	} else if (singlechar()) {
		nextptr = ptr + 1;
		return singlechar();
	} else if (*ptr == '"') {
		nextptr = ptr;
		do {
			++nextptr;
		} while (*nextptr != '"');
		++nextptr;
		return TOKENIZER_STRING;
	} else {
		for (kt = keywords; kt->keyword != NULL; ++kt) {
			if (strncasecmp(ptr, kt->keyword, strlen(kt->keyword)) == 0) {
				nextptr = ptr + strlen(kt->keyword);
				return kt->token;
			}
		}
	}

	if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z')) {
		nextptr = ptr + 1;
		return TOKENIZER_VARIABLE;
	}

	return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
void tokenizer_init(const char * program) {
/** ct-Bot Anpassung */
	if (init_hook != NULL) {
		init_hook(&program);
	}


	ptr = program;
	current_token = get_next_token();
}
/*---------------------------------------------------------------------------*/
int tokenizer_token(void) {
	return current_token;
}
/*---------------------------------------------------------------------------*/
void tokenizer_next(void) {
	if (tokenizer_finished()) {
		return;
	}

/** ct-Bot Anpassung */
	if (tokenizer_token() == TOKENIZER_CR && next_line_hook != NULL) {
		next_line_hook(&nextptr);
	}


	DEBUG_PRINTF("tokenizer_next: %p", nextptr);
	ptr = nextptr;
	while (*ptr == ' ') {
		++ptr;
	}
	current_token = get_next_token();
	DEBUG_PRINTF("tokenizer_next: '%s', %i", ptr, current_token);
}
/*---------------------------------------------------------------------------*/
int tokenizer_num(void) {
	return atoi(ptr);
}
/*---------------------------------------------------------------------------*/
void tokenizer_string(char * dest, int len) {
	char * string_end;
	int string_len;

	if (tokenizer_token() != TOKENIZER_STRING) {
		return;
	}
	string_end = strchr(ptr + 1, '"');
	if (string_end == NULL) {
		return;
	}
	string_len = string_end - ptr - 1;
	if (len < string_len) {
		string_len = len;
	}
	memcpy(dest, ptr + 1, (size_t) string_len);
	dest[string_len] = 0;
}
/*---------------------------------------------------------------------------*/
void tokenizer_error_print(int linenum, int error_nr) {
	(void) linenum;
	(void) error_nr;
	LOG_ERROR("error %i at line: '%i'", error_nr, linenum);
}
/*---------------------------------------------------------------------------*/
int tokenizer_finished(void) {
	return *ptr == 0 || current_token == TOKENIZER_ENDOFINPUT;
}
/*---------------------------------------------------------------------------*/
int tokenizer_variable_num(void) {
	if (*ptr >= 'a') {
		return *ptr - 'a';
	} else {
		return *ptr - 'A';
	}
}
/*---------------------------------------------------------------------------*/
char tokenizer_letter(void) {
	return *ptr;
}

/**
 * ct-Bot Anpassung: Setzt die Hook-Funktion zur Initialisierung
 * \param *init_fkt Zeiger auf gewuenschte Init-Funktion
 */
void tokenizer_set_init_hook(void(* init_fkt)(const char * *)) {
	init_hook = init_fkt;
}

/**
 * ct-Bot Anpassung: Setzt die Hook-Funktion zum Laden einer neuen Zeile
 * \param *next_line_fkt Zeiger auf gewuenschte Lade-Funktion
 */
void tokenizer_set_next_line_hook(void(* next_line_fkt)(const char * *)) {
	next_line_hook = next_line_fkt;
}
#endif // BEHAVIOUR_UBASIC_AVAILABLE
