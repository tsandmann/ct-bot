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
#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include "bot-logic/ubasic_config.h"

enum {
  TOKENIZER_ERROR,				// 0
  TOKENIZER_ENDOFINPUT,
  TOKENIZER_NUMBER,
  TOKENIZER_STRING,
  TOKENIZER_VARIABLE,
  TOKENIZER_LET,
  TOKENIZER_PRINT,
  TOKENIZER_IF,
  TOKENIZER_THEN,
  TOKENIZER_ELSE,
  TOKENIZER_FOR,				// 10
  TOKENIZER_TO,
  TOKENIZER_DOWNTO,
  TOKENIZER_STEP,
  TOKENIZER_NEXT,
  TOKENIZER_GOTO,
  TOKENIZER_GOSUB,
  TOKENIZER_RETURN,
  TOKENIZER_END,
  TOKENIZER_COMMA,
  TOKENIZER_SEMICOLON,			// 20
  TOKENIZER_PLUS,
  TOKENIZER_MINUS,
  TOKENIZER_AND,
  TOKENIZER_OR,
  TOKENIZER_ASTR,
  TOKENIZER_SLASH,
  TOKENIZER_MOD,
  TOKENIZER_LEFTPAREN,
  TOKENIZER_RIGHTPAREN,
  TOKENIZER_LT,					// 30
  TOKENIZER_GT,
  TOKENIZER_EQ,
  
  #if UBASIC_ABS
  TOKENIZER_ABS,
  #endif
  #if UBASIC_NOT
  TOKENIZER_NOT,
  #endif
  #if UBASIC_REM
  TOKENIZER_REM,				// 35
  #endif
  #if UBASIC_CALL
  TOKENIZER_CALL,
  #endif

  #if UBASIC_RND
  TOKENIZER_SRND,
  TOKENIZER_RND,
  #endif
  #if AVR_EPOKE
  TOKENIZER_EPOKE,
  #endif
  #if AVR_EPEEK
  TOKENIZER_EPEEK,				// 40
  #endif
  #if AVR_WAIT
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
  
  #if UBASIC_XOR
  TOKENIZER_XOR,
  #endif
  #if UBASIC_SHL
  TOKENIZER_SHL,
  #endif
  #if UBASIC_SHR
  TOKENIZER_SHR,				// 50
  #endif
  #if UBASIC_ARRAY
  TOKENIZER_DIM,
  #endif
  #if UBASIC_DATA
  TOKENIZER_DATA,
  TOKENIZER_READ,
  TOKENIZER_RESTORE,
  #endif
  TOKENIZER_GE,
  TOKENIZER_LE,
  TOKENIZER_NE,
  TOKENIZER_COLON,
  TOKENIZER_CR					// 55
};


void tokenizer_init(PTR_TYPE program);
void tokenizer_next(void);
int tokenizer_token(void);
int tokenizer_num(void);
int tokenizer_variable_num(void);
const char * tokenizer_last_string_ptr(void);

int tokenizer_finished(void);
void tokenizer_error_print(int linenum, int error_nr);
PTR_TYPE get_prog_text_pointer(void);
void jump_to_prog_text_pointer(PTR_TYPE jump_ptr);
void jump_to_next_linenum(void);
void skip_all_whitespaces(void);

#endif /* __TOKENIZER_H__ */
