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
 * Source modified by Uwe Berger (bergeruw@gmx.net); 2010, 2011
 * ------------------------------------------------------
 */
#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include "bot-logic/ubasic_config.h"
#include "bot-logic/parser.h"

// Typ-Definition Token-Tabelle (Standard-Parser)
struct keyword_token {
#if USE_PROGMEM
	// um via strxxx_P zugreifen zu koennen, muss eine feste Laenge vorgegeben werden
	char keyword[MAX_KEYWORD_LEN+1];
#else
	char *keyword;
#endif
  int token;
};

// Typ-Definition Tokenizer-Position
struct tokenizer_pos_t {
	PTR_TYPE prog_ptr;
	int token;
};


void tokenizer_init(PTR_TYPE program);
void tokenizer_next(void);
int tokenizer_token(void);
int tokenizer_num(void);
int tokenizer_variable_num(void);
const char * tokenizer_last_string_ptr(void);
void tokenizer_set_num(int val);
int tokenizer_finished(void);
void tokenizer_error_print(int linenum, int error_nr);
PTR_TYPE get_prog_text_pointer(void);
struct tokenizer_pos_t tokenizer_get_position(void);
void tokenizer_set_position(struct tokenizer_pos_t);
void jump_to_prog_text_pointer(PTR_TYPE jump_ptr);
void jump_to_next_linenum(void);
void skip_all_whitespaces(void);

#endif /* __TOKENIZER_H__ */
