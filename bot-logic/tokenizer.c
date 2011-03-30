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

#include "bot-logic/bot-logic.h"
#ifdef BEHAVIOUR_UBASIC_AVAILABLE

#define DEBUG 0

#if DEBUG
	#define DEBUG_PRINTF(...)  usart_write(__VA_ARGS__)
//	#define DEBUG_PRINTF(...)  printf(__VA_ARGS__)

#else
	#define DEBUG_PRINTF(...)
#endif

#define __TOKENIZER_C__
	#include "bot-logic/tokenizer_access.h"
#undef __TOKENIZER_C__
#include "bot-logic/tokenizer.h"
#include "bot-logic/ubasic_config.h"
#include "bot-logic/ubasic_ext_proc.h"
#include "botfs.h"
#include <ctype.h>



#if USE_AVR
//	#include "../uart/usart.h"
#else
	#include <string.h>
	#include <ctype.h>
	#include <stdlib.h>
	#include <stdio.h>
#endif

//#if USE_PROGMEM
//	#include <avr/pgmspace.h>
//#endif


static PTR_TYPE line_begin_ptr;
static char last_string[MAX_STRINGLEN+1];
static int  last_value;
static int  last_var_num;

#define MAX_NUMLEN 5
#define MAX_HEXLEN 4
#define MAX_BINLEN 16

struct keyword_token {
#if USE_PROGMEM
	// um via strxxx_P zugreifen zu koennen, muss eine feste Laenge vorgegeben werden
	char keyword[MAX_KEYWORD_LEN+1];
#else	
	char *keyword;
#endif
  int token;
};

static int current_token = TOKENIZER_ERROR;

extern PTR_TYPE program_ptr;

#if UBASIC_EXT_PROC
	extern char current_proc[MAX_PROG_NAME_LEN];
#endif

#if USE_PROGMEM
static const struct keyword_token keywords[] PROGMEM = {
#else
static const struct keyword_token keywords[] = {
#endif
	{"let", TOKENIZER_LET},
	#if UBASIC_PRINT
	{"print", TOKENIZER_PRINT},
	#endif
	{"if", TOKENIZER_IF},
	{"then", TOKENIZER_THEN},
	{"else", TOKENIZER_ELSE},
	{"for", TOKENIZER_FOR},
	{"to", TOKENIZER_TO},
	{"downto", TOKENIZER_DOWNTO},
	{"step", TOKENIZER_STEP},
	{"next", TOKENIZER_NEXT},
	{"goto", TOKENIZER_GOTO},
	{"gosub", TOKENIZER_GOSUB},
	{"return", TOKENIZER_RETURN},
	{"end", TOKENIZER_END},
	#if UBASIC_ABS
	{"abs", TOKENIZER_ABS},
	#endif
	#if UBASIC_NOT
	{"not", TOKENIZER_NOT},
	#endif
	#if UBASIC_XOR
	{"xor", TOKENIZER_XOR},
	#endif
 	#if UBASIC_SHL
	{"shl", TOKENIZER_SHL},
	#endif
	#if UBASIC_SHR
	{"shr", TOKENIZER_SHR},
	#endif
	#if UBASIC_CALL
	{"call", TOKENIZER_CALL},
	#endif
	#if UBASIC_CVARS
	{"vpoke", TOKENIZER_VPOKE},
	{"vpeek", TOKENIZER_VPEEK},
	#endif
	#if UBASIC_REM
	{"rem", TOKENIZER_REM},
	#endif
	#if UBASIC_RND
	{"srand", TOKENIZER_SRND},
	{"rand", TOKENIZER_RND},
	#endif
	#if AVR_EPOKE
	{"epoke", TOKENIZER_EPOKE},
	#endif
	#if AVR_EPEEK
	{"epeek", TOKENIZER_EPEEK},
	#endif
	#if AVR_WAIT
	{"wait", TOKENIZER_WAIT},
	#endif
	#if AVR_DIR
	{"dir", TOKENIZER_DIR},
	#endif
	#if AVR_IN
	{"in", TOKENIZER_IN},
	#endif
	#if AVR_OUT
	{"out", TOKENIZER_OUT},
	#endif
	#if AVR_ADC
	{"adc", TOKENIZER_ADC},
	#endif
	#if UBASIC_ARRAY
	{"dim", TOKENIZER_DIM},
	#endif
	{"or", TOKENIZER_OR},
	{"and", TOKENIZER_AND},
	{"mod", TOKENIZER_MOD},
	{"<=", TOKENIZER_LE},
	{">=", TOKENIZER_GE},
	{"<>", TOKENIZER_NE},
	{"", TOKENIZER_ERROR}
};
// Prototypen
static int get_next_token(void);


/*---------------------------------------------------------------------------*/
PTR_TYPE get_prog_text_pointer(void) {
	return line_begin_ptr;
}
/*---------------------------------------------------------------------------*/
static void skip_whitespaces(void){
	while(	GET_CONTENT_PROG_PTR == ' '  || 
			GET_CONTENT_PROG_PTR == '\r' ||
			GET_CONTENT_PROG_PTR == '\t'
		 ) INCR_PROG_PTR;
}
/*---------------------------------------------------------------------------*/
void skip_all_whitespaces(void){
	while(	GET_CONTENT_PROG_PTR == ' '  || 
			GET_CONTENT_PROG_PTR == '\n' ||
			GET_CONTENT_PROG_PTR == '\r' ||
			GET_CONTENT_PROG_PTR == '\t'
		 ) INCR_PROG_PTR;
}
/*---------------------------------------------------------------------------*/
void jump_to_prog_text_pointer(PTR_TYPE jump_ptr) {
	SET_PROG_PTR_ABSOLUT(jump_ptr);
	skip_whitespaces();
	current_token = get_next_token();
}

/*---------------------------------------------------------------------------*/
void jump_to_next_linenum(void) {
	while(GET_CONTENT_PROG_PTR != '\n' && GET_CONTENT_PROG_PTR != 0) {
		INCR_PROG_PTR;  
	}
	skip_all_whitespaces();
	line_begin_ptr = PROG_PTR;
	current_token = get_next_token();
}

/*---------------------------------------------------------------------------*/
static int singlechar(void) {
	if(GET_CONTENT_PROG_PTR == '\n') {
		return TOKENIZER_CR;
	} else if(GET_CONTENT_PROG_PTR == ',') {
		return TOKENIZER_COMMA;
	} else if(GET_CONTENT_PROG_PTR == ':') {
		return TOKENIZER_COLON;
	} else if(GET_CONTENT_PROG_PTR == ';') {
		return TOKENIZER_SEMICOLON;
	} else if(GET_CONTENT_PROG_PTR == '+') {
		return TOKENIZER_PLUS;
	} else if(GET_CONTENT_PROG_PTR == '-') {
		return TOKENIZER_MINUS;
	} else if(GET_CONTENT_PROG_PTR == '&') {
		return TOKENIZER_AND;
	} else if(GET_CONTENT_PROG_PTR == '|') {
		return TOKENIZER_OR;
	} else if(GET_CONTENT_PROG_PTR == '*') {
		return TOKENIZER_ASTR;
	} else if(GET_CONTENT_PROG_PTR == '/') {
		return TOKENIZER_SLASH;
	} else if(GET_CONTENT_PROG_PTR == '%') {
		return TOKENIZER_MOD;
	} else if(GET_CONTENT_PROG_PTR == '(') {
		return TOKENIZER_LEFTPAREN;
	} else if(GET_CONTENT_PROG_PTR == ')') {
		return TOKENIZER_RIGHTPAREN;
	} else if(GET_CONTENT_PROG_PTR == '<') {
		return TOKENIZER_LT;
	} else if(GET_CONTENT_PROG_PTR == '>') {
		return TOKENIZER_GT;
	} else if(GET_CONTENT_PROG_PTR == '=') {
		return TOKENIZER_EQ;
  	}
	return 0;
}

#if UBASIC_HEX_BIN
/*---------------------------------------------------------------------------*/
static int hex2int(char c) {
	if (c<='9') return (c-'0'); else return (c-'A'+10);
}
/*---------------------------------------------------------------------------*/
static char isbdigit(char c) {
	if ((c=='0') || (c=='1')) return 1; else return 0;
}
#endif

/*---------------------------------------------------------------------------*/
static int get_next_token(void) {
#if !USE_PROGMEM
	struct keyword_token const *kt;
#endif
	uint8_t i;
	int temp_token;
	char k_temp[MAX_KEYWORD_LEN+1];

	if(END_OF_PROG_TEXT) {
		return TOKENIZER_ENDOFINPUT;
	}
	if(isdigit(GET_CONTENT_PROG_PTR)) {
		last_value=0;
		
		#if UBASIC_HEX_BIN
		// Zeiger sichern, falls doch nicht Hex oder Bin
		PTR_TYPE temp_ptr = PROG_PTR;
		if (GET_CONTENT_PROG_PTR == '0') {
			INCR_PROG_PTR;
		}
		if (toupper(GET_CONTENT_PROG_PTR)=='X' || toupper(GET_CONTENT_PROG_PTR)=='B') {
			switch (toupper(GET_CONTENT_PROG_PTR)) {
				// Hex-Format --> 0x12AB
				case 'X':
					INCR_PROG_PTR;
					for(i = 0; i <= MAX_HEXLEN; ++i) {
						if(!isxdigit(GET_CONTENT_PROG_PTR)) {
							if(i > 0) return TOKENIZER_NUMBER; else return TOKENIZER_ERROR;
						}
						last_value = 16 * last_value + hex2int((char) toupper(GET_CONTENT_PROG_PTR));
						INCR_PROG_PTR;
					}
					return TOKENIZER_ERROR;
					break;
				// Binaer-Format --> 0b1010
				case 'B':
					INCR_PROG_PTR;
					for(i = 0; i <= MAX_BINLEN; ++i) {
						if(!isbdigit(GET_CONTENT_PROG_PTR)) {
							if(i > 0) return TOKENIZER_NUMBER; else return TOKENIZER_ERROR;
						}
						last_value = 2 * last_value + GET_CONTENT_PROG_PTR - '0';
						INCR_PROG_PTR;
					}
					return TOKENIZER_ERROR;
					break;
			}
		}
		// kein Hex oder Bin, also wieder an Ursprungsstelle...
		SET_PROG_PTR_ABSOLUT(temp_ptr);
		#endif
		// Dezimal-Format
		for(i = 0; i <= MAX_NUMLEN; ++i) {
			if(!isdigit(GET_CONTENT_PROG_PTR)) {
				if(i > 0) {
					return TOKENIZER_NUMBER;
				} else {
					return TOKENIZER_ERROR;
				}
			}
			last_value = 10 * last_value + GET_CONTENT_PROG_PTR - '0';
			INCR_PROG_PTR;
    	}
		return TOKENIZER_ERROR;
	} else if(GET_CONTENT_PROG_PTR == '"') {
						INCR_PROG_PTR;
						i=0;
						do {
							last_string[i] = GET_CONTENT_PROG_PTR;
							INCR_PROG_PTR;
							i++;
							// max. zulaessige Stringlaenge?
							if (i >= MAX_STRINGLEN) return TOKENIZER_ERROR;
						} while(GET_CONTENT_PROG_PTR != '"');
						// String null-terminieren
						last_string[i]=0;
						INCR_PROG_PTR;
						return TOKENIZER_STRING;
				} else {
					// Textabschnitt aus Quelltext auf Zwischenvariable
					for (i=0; i < MAX_KEYWORD_LEN; i++) {
						k_temp[i] = GET_CONTENT_PROG_PTR;
						INCR_PROG_PTR;
					}
					// PTR wieder an Ursprungstelle setzen
					SET_PROG_PTR_ABSOLUT(PROG_PTR - i);
					k_temp[i]=0;
#if USE_PROGMEM  	
					for (i=0; i < sizeof(keywords)/sizeof(keywords[0])-1; i++) {
						if (strncasecmp_P(k_temp, keywords[i].keyword, strlen_P(keywords[i].keyword)) == 0) {
							SET_PROG_PTR_ABSOLUT(PROG_PTR + strlen_P(keywords[i].keyword));
							return pgm_read_word(&keywords[i].token);
						}
					}
#else  	
					for(kt = keywords; kt->token != TOKENIZER_ERROR; ++kt) {
						if(strncasecmp(k_temp, kt->keyword, strlen(kt->keyword)) == 0) {
							SET_PROG_PTR_ABSOLUT(PROG_PTR + strlen(kt->keyword));
							return kt->token;
						}
					}
#endif
				} 
				
	temp_token = singlechar();
	if (temp_token) {
		INCR_PROG_PTR;
		return temp_token;
	}
	if(toupper(GET_CONTENT_PROG_PTR) >= 'A' && toupper(GET_CONTENT_PROG_PTR) <= 'Z') {
		last_var_num=toupper(GET_CONTENT_PROG_PTR)-'A';
		INCR_PROG_PTR;
		return TOKENIZER_VARIABLE;
	}
	return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
void tokenizer_init(PTR_TYPE program) {
	SET_PROG_PTR_ABSOLUT(program);
#if UBASIC_NO_LINENUM_ALLOWED
	last_value = -1;
#endif
	skip_all_whitespaces();
	current_token = get_next_token();
}
/*---------------------------------------------------------------------------*/
int tokenizer_token(void) {
	return current_token;
}
/*---------------------------------------------------------------------------*/
void tokenizer_next(void) {
	if(tokenizer_finished()) return;
	if (tokenizer_token() == TOKENIZER_CR) line_begin_ptr = PROG_PTR;
	skip_whitespaces();
	current_token = get_next_token();
	return;
}
/*---------------------------------------------------------------------------*/
int tokenizer_num(void) {
	return last_value;
}
/*---------------------------------------------------------------------------*/
char const *tokenizer_last_string_ptr(void) {
	return (const char*)&last_string[0];
}
/*---------------------------------------------------------------------------*/
void tokenizer_error_print(int linenum, int error_nr) {
	(void) linenum;
	(void) error_nr;
	PTR_TYPE current_prog_ptr;
	unsigned int source_linenum;
	// alten Textpointer retten
	current_prog_ptr=PROG_PTR;
	// Quelltextzeilennummer suchen
	SET_PROG_PTR_ABSOLUT(program_ptr);
	source_linenum=1;
	while (!END_OF_PROG_TEXT && (PROG_PTR < current_prog_ptr)) {
		if (GET_CONTENT_PROG_PTR=='\n') source_linenum++;
		INCR_PROG_PTR;
	}
	// Fehlertextausgabe
	PRINTF("error %i at sourceline: %i (%i?) ", error_nr, source_linenum, linenum);
#if UBASIC_EXT_PROC
	PRINTF("in program %s", current_proc);
#endif
//	PRINTF("\n\r");
	// Textpointer wieder auf alten Wert
	SET_PROG_PTR_ABSOLUT(current_prog_ptr);
}
/*---------------------------------------------------------------------------*/
int tokenizer_finished(void) {
	return END_OF_PROG_TEXT || current_token == TOKENIZER_ENDOFINPUT;
}
/*---------------------------------------------------------------------------*/
int tokenizer_variable_num(void) {
	return last_var_num;
}

#endif // BEHAVIOUR_UBASIC_AVAILABLE
