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
#ifndef __UBASIC_H__
#define __UBASIC_H__

#ifdef BEHAVIOUR_UBASIC_AVAILABLE

#include "ubasic_config.h"
#include "tokenizer.h"

#define SYNTAX_ERROR			1
#define UNKNOWN_ADC_CHANNEL		2
#define UNKNOWN_IO_PORT			3
#define FOR_WITHOUT_TO			4
#define UNKNOWN_STATEMENT		5
#define UNKNOWN_CALL_FUNCT		6
#define UNKNOWN_CALL_FUNCT_TYP	7
#define UNKNOWN_CVAR_NAME		8
#define SHORT_IF_WITH_ELSE		9
#define GOSUB_STACK_DETH		10
#define FOR_STACK_DETH			11
#define GOSUB_STACK_INVALID		12
#define UNKNOWN_SUBPROC			13
#define GOSUB_NO_EXT_SUBPROC    14
#define ARRAY_OUT_OF_RANGE		15
#define OUT_OF_MEMORY           16
#define NOT_ENOUGH_DATA			17
#define UNKNOWN_LINENUMBER		18
#define INPUT_IS_NOT_NUMBER     19
#define UNKNOWN_VARIABLE        20
#define STRING_TO_LARGE         21
#define DATA_READ_TYPE_DIFF		22
#define STRINGVAR_NOT_INIT		23


// Typ-Definition gosub-Stack
struct gosub_stack_t {
#if UBASIC_EXT_PROC
	char p_name[MAX_PROG_NAME_LEN];
#endif
	PTR_TYPE p_ptr;
};

// Typ-Definition for-next-Stack
struct for_state_t {
  PTR_TYPE next_line_ptr;
  int for_variable;
  int to;
  int step;
  unsigned char downto;
};

// Type-Definition Zeilennummern-Cache
#if USE_LINENUM_CACHE
struct linenum_cache_t {
#if UBASIC_EXT_PROC
	char p_name[MAX_PROG_NAME_LEN];
#endif
	int linenum;
	PTR_TYPE next_line_ptr;
};
#endif

// Typ-Definition Variablen-Information
struct varinfo_t {
	int varnum;
#if UBASIC_ARRAY
	int idx;
#endif
};

// Typ-Definition BASIC-Integer-Variable
struct variables_t {
	int val;
#if UBASIC_ARRAY
	int* adr;
	int dim;
#endif
};

// Typ-Definition BASIC-String-Variable
#if UBASIC_STRING
struct strvariables_t {
	char* val_adr;
#if UBASIC_ARRAY
	char *adr;
	int dim;
#endif
};
#endif

// Typ-Definition DATA-Pointer (data/read/restore)
#if UBASIC_DATA
struct data_ptr_t {
	struct tokenizer_pos_t first;
	struct tokenizer_pos_t current;
};
#endif

extern int current_linenum;

void ubasic_init(PTR_TYPE program);
void ubasic_run(void);
int ubasic_finished(void);

struct varinfo_t ubasic_get_varinfo(void);
int ubasic_get_variable(struct varinfo_t var);
void ubasic_set_variable(struct varinfo_t varum, int value);

#if UBASIC_STRING
char* strexpr(void);
void ubasic_set_strvariable(struct varinfo_t var, char *str);
#endif

void ubasic_accept(int token);
int ubasic_expr(void);
void ubasic_break(void);

#endif // BEHAVIOUR_UBASIC_AVAILABLE

#endif /* __UBASIC_H__ */
