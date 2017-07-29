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

#include "bot-logic.h"
#ifdef BEHAVIOUR_UBASIC_AVAILABLE

#include "tokenizer_access.h"
#include "ubasic.h"
#include "tokenizer.h"
#include "ubasic_config.h"
#include "ubasic_ext_proc.h"
#include <string.h>

#if !USE_AVR
	#include <stdio.h>
	#include <string.h>
	#if UBASIC_RND
		#include <time.h>
	#endif
#endif

#if UBASIC_CALL
	#include "ubasic_call.h"
#endif

#if UBASIC_CVARS
	#include "ubasic_cvars.h"
#endif

#if USE_AVR
//	#include "../uart/usart.h"
//	#include <avr/io.h>
	#include "ubasic_avr.h"
#else
	#include <ctype.h>
#endif

#if UBASIC_EXT_PROC
	extern char current_proc[MAX_PROG_NAME_LEN];
#endif

#if !BREAK_NOT_EXIT
	#include <stdlib.h> /* exit() */
#endif

PTR_TYPE program_ptr;
int current_linenum;

static struct gosub_stack_t gosub_stack[MAX_GOSUB_STACK_DEPTH];
static int gosub_stack_ptr;

static struct for_state_t for_stack[MAX_FOR_STACK_DEPTH];
static int for_stack_ptr;

#if USE_LINENUM_CACHE
static struct linenum_cache_t linenum_cache[MAX_LINENUM_CACHE_DEPTH];
static int linenum_cache_ptr;
#endif

static struct variables_t variables[MAX_VARNUM];

#if UBASIC_STRING
static struct strvariables_t strvariables[MAX_VARNUM];
static char str_buf[MAX_STRINGLEN+1];
#endif

static unsigned char ended;

#if UBASIC_DATA
struct data_ptr_t data_ptr = {{0,0},{0,0}};
#endif

// Prototypen
int ubasic_expr(void);
static void line_statement(void);
static void statement(void);
void ubasic_free_all_mem(void);
#if UBASIC_RND && USE_AVR
static long unsigned int rand31_next(void);
#endif
#if UBASIC_STRING
struct varinfo_t ubasic_get_strvarinfo(void);
unsigned char ubasic_is_strexpr(void);
static int strrelation(void);
#endif

/*---------------------------------------------------------------------------*/
void ubasic_init(PTR_TYPE program) {
	unsigned char i;
	program_ptr = program;
	for_stack_ptr = gosub_stack_ptr = 0;
#if USE_LINENUM_CACHE
	linenum_cache_ptr = 0;
#endif
	for (i=0; i<MAX_VARNUM; i++) variables[i].val=0;
	ubasic_free_all_mem();
	tokenizer_init(program);
	ended = 0;
}
/*---------------------------------------------------------------------------*/
void ubasic_break(void) {
	#if BREAK_NOT_EXIT
	// zum Ende der Basic-Zeile und Ende-Merker setzen
	ended = 1;
	jump_to_next_linenum();
	#else
	exit(1);
	#endif
}

/*---------------------------------------------------------------------------*/
void ubasic_accept(int token) {
  if(token != tokenizer_token()) {
    tokenizer_error_print(current_linenum, SYNTAX_ERROR);
    ubasic_break();
  }
  tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static int varfactor(void) {
  ubasic_accept(TOKENIZER_VARIABLE);
  return ubasic_get_variable(ubasic_get_varinfo());
}
/*---------------------------------------------------------------------------*/
static int factor(void) {
  int r=0;
  #if UBASIC_RND
  int b;
  #endif
  #if UBASIC_STRING
  char *s;
  char s1[MAX_STRINGLEN+1];
  char s2[MAX_STRINGLEN+1];
  #endif
  switch(tokenizer_token()) {

  case TOKENIZER_NUMBER:
    r = tokenizer_num();
    ubasic_accept(TOKENIZER_NUMBER);
    break;

  case TOKENIZER_MINUS:
    ubasic_accept(TOKENIZER_MINUS);
    r = ubasic_expr()*(-1);
    break;

  case TOKENIZER_PLUS:
    ubasic_accept(TOKENIZER_PLUS);
    r = ubasic_expr();
    break;

  case TOKENIZER_LEFTPAREN:
    ubasic_accept(TOKENIZER_LEFTPAREN);
    r = ubasic_expr();
    ubasic_accept(TOKENIZER_RIGHTPAREN);
    break;

  #if UBASIC_RND
  case TOKENIZER_RND:
    ubasic_accept(TOKENIZER_RND);
    ubasic_accept(TOKENIZER_LEFTPAREN);
   	b = ubasic_expr();
    #if USE_AVR
		r = (int) (rand31_next() % ((unsigned long) b + 1));
	#else
		r = rand() % (b+1);
	#endif
    ubasic_accept(TOKENIZER_RIGHTPAREN);
    break;
  #endif

  #if UBASIC_ABS
  case TOKENIZER_ABS:
    ubasic_accept(TOKENIZER_ABS);
    ubasic_accept(TOKENIZER_LEFTPAREN);
    r = ubasic_expr();
    if (r<0) r=r*(-1);
    ubasic_accept(TOKENIZER_RIGHTPAREN);
    break;
  #endif

  #if UBASIC_STRING
  case TOKENIZER_LEN:
    ubasic_accept(TOKENIZER_LEN);
    ubasic_accept(TOKENIZER_LEFTPAREN);
    r=(int)strlen(strexpr());
    ubasic_accept(TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_VAL:
    ubasic_accept(TOKENIZER_VAL);
    ubasic_accept(TOKENIZER_LEFTPAREN);
    r=atoi(strexpr());
    ubasic_accept(TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_ASC:
    ubasic_accept(TOKENIZER_ASC);
    ubasic_accept(TOKENIZER_LEFTPAREN);
    s=strexpr();
    r=(int)s[0];
    ubasic_accept(TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_MAXSTRLEN:
	ubasic_accept(TOKENIZER_MAXSTRLEN);
	r=MAX_STRINGLEN;
	break;

  case TOKENIZER_INSTR:
	ubasic_accept(TOKENIZER_INSTR);
    ubasic_accept(TOKENIZER_LEFTPAREN);
    strcpy(s1, strexpr());
    ubasic_accept(TOKENIZER_COMMA);
    strcpy(s2, strexpr());
    s=strstr(s1, s2);
    if (s) r=s-s1; else r=-1;
    ubasic_accept(TOKENIZER_RIGHTPAREN);
	break;

  #endif

  #if UBASIC_NOT
  case TOKENIZER_NOT:
    ubasic_accept(TOKENIZER_NOT);
    ubasic_accept(TOKENIZER_LEFTPAREN);
    r = ~ubasic_expr();
    ubasic_accept(TOKENIZER_RIGHTPAREN);
    break;
  #endif

  #if UBASIC_CALL
  case TOKENIZER_CALL:
  	r=call_statement();
  	break;
  #endif

  #if UBASIC_CVARS
  case TOKENIZER_VPEEK:
  	r=vpeek_expression();
  	break;
  #endif

  #if AVR_EPEEK
  case TOKENIZER_EPEEK:
    r=epeek_expression();
    break;
  #endif

  #if AVR_ADC
  case TOKENIZER_ADC:
	r=adc_expression();
    break;
  #endif

  #if AVR_IN
  case TOKENIZER_IN:
	r=pin_in_expression();
    break;
  #endif

  default:
    r = varfactor();
    break;
  }
  return r;
}
/*---------------------------------------------------------------------------*/
static int term(void) {
  int f1, f2;
  int op;

  f1 = factor();
  op = tokenizer_token();
  while(op == TOKENIZER_ASTR  ||
		op == TOKENIZER_SLASH ||
		op == TOKENIZER_MOD   ||
		op == TOKENIZER_MOD2) {
    tokenizer_next();
    f2 = factor();
    switch(op) {
    case TOKENIZER_ASTR:
      f1 = f1 * f2;
      break;
    case TOKENIZER_SLASH:
      f1 = f1 / f2;
      break;
    case TOKENIZER_MOD2:
    case TOKENIZER_MOD:
      f1 = f1 % f2;
      break;
    }
    op = tokenizer_token();
  }
  return f1;
}
/*---------------------------------------------------------------------------*/
int ubasic_expr(void) {
  int t1, t2;
  int op;

  t1 = term();
  op = tokenizer_token();
  while(op == TOKENIZER_PLUS  ||
		op == TOKENIZER_MINUS ||
		op == TOKENIZER_AND   ||
		#if UBASIC_XOR
		op == TOKENIZER_XOR   ||
		#endif
		#if UBASIC_SHL
		op == TOKENIZER_SHL   ||
		#endif
		#if UBASIC_SHR
		op == TOKENIZER_SHR   ||
		#endif
		op == TOKENIZER_OR) {
    tokenizer_next();
    t2 = term();
    switch(op) {
    case TOKENIZER_PLUS:
      t1 = t1 + t2;
      break;
    case TOKENIZER_MINUS:
      t1 = t1 - t2;
      break;
    case TOKENIZER_AND:
      t1 = t1 & t2;
      break;
    case TOKENIZER_OR:
      t1 = t1 | t2;
      break;
#if UBASIC_XOR
    case TOKENIZER_XOR:
      t1 = t1 ^ t2;
      break;
#endif
#if UBASIC_SHL
    case TOKENIZER_SHL:
      t1 = t1 << t2;
      break;
#endif
#if UBASIC_SHR
    case TOKENIZER_SHR:
      t1 = t1 >> t2;
      break;
#endif

    }
    op = tokenizer_token();
  }
  return t1;
}
/*---------------------------------------------------------------------------*/
static int relation(void) {
  int r1, r2;
  int op;

  r1 = ubasic_expr();
  op = tokenizer_token();
/*
  while(op == TOKENIZER_LT ||
		op == TOKENIZER_GT ||
		op == TOKENIZER_GE ||
		op == TOKENIZER_LE ||
		op == TOKENIZER_NE ||
		op == TOKENIZER_EQ) {
*/
    tokenizer_next();
    r2 = ubasic_expr();
    switch(op) {
    case TOKENIZER_LT:
      r1 = r1 < r2;
      break;
    case TOKENIZER_GT:
      r1 = r1 > r2;
      break;
    case TOKENIZER_EQ:
      r1 = r1 == r2;
      break;
    case TOKENIZER_LE:
      r1 = r1 <= r2;
      break;
    case TOKENIZER_GE:
      r1 = r1 >= r2;
      break;
    case TOKENIZER_NE:
      r1 = r1 != r2;
      break;
    }
    op = tokenizer_token();
/*
  }
*/
  return r1;
}
/*---------------------------------------------------------------------------*/
static void jump_linenum(int linenum) {

#if USE_LINENUM_CACHE
	unsigned char i;
	// zuerst die Zeilennummer im Cache suchen
	for (i=0; i<linenum_cache_ptr; i++){
		if (linenum_cache[i].linenum == linenum
			#if UBASIC_EXT_PROC
				&& strncmp(current_proc, linenum_cache[i].p_name, MAX_PROG_NAME_LEN) == 0
			#endif
			) {
			jump_to_prog_text_pointer(linenum_cache[i].next_line_ptr);
			return;
		}
	}
#endif
	tokenizer_init(program_ptr);
	while(tokenizer_num() != linenum && (tokenizer_token() != TOKENIZER_ENDOFINPUT)) {
		do {
			jump_to_next_linenum();
		} while(tokenizer_token() != TOKENIZER_NUMBER  && tokenizer_token() != TOKENIZER_ENDOFINPUT);
	}
	// Zeilennummer nicht gefunden
	if (tokenizer_token() == TOKENIZER_ENDOFINPUT) {
	    tokenizer_error_print(current_linenum, UNKNOWN_LINENUMBER);
		ubasic_break();
	}
#if USE_LINENUM_CACHE
	// wenn noch im Zeilennummern-Cache Platz ist, Zeilennummer/-Pointer
	// merken
	if (linenum_cache_ptr < MAX_LINENUM_CACHE_DEPTH) {
		linenum_cache[linenum_cache_ptr].next_line_ptr=get_prog_text_pointer();
		linenum_cache[linenum_cache_ptr].linenum=linenum;
		#if UBASIC_EXT_PROC
		strncpy(linenum_cache[linenum_cache_ptr].p_name, current_proc, MAX_PROG_NAME_LEN);
		#endif
		linenum_cache_ptr++;
	}
#endif
}
/*---------------------------------------------------------------------------*/
static void goto_statement(void) {
  ubasic_accept(TOKENIZER_GOTO);
  jump_linenum(ubasic_expr());
}
/*---------------------------------------------------------------------------*/
#if UBASIC_PRINT
static void print_statement(void) {
	unsigned char nl;
	int i, j;
	ubasic_accept(TOKENIZER_PRINT);
	do {
		nl=1;
		if(tokenizer_token() == TOKENIZER_STRING) {
			#if UBASIC_STRING
			PRINTF("%s", strexpr());
			#else
			PRINTF("%s", tokenizer_last_string_ptr());
			tokenizer_next();
			#endif
		} else if(tokenizer_token() == TOKENIZER_COMMA) {
			nl=0;
			PRINTF(" ");
			tokenizer_next();
		} else if(tokenizer_token() == TOKENIZER_SEMICOLON) {
			nl=0;
			tokenizer_next();
		} else if(tokenizer_token() == TOKENIZER_TAB) {
			ubasic_accept(TOKENIZER_TAB);
			ubasic_accept(TOKENIZER_LEFTPAREN);
			j=ubasic_expr();
			//PRINTF("-->%i\n\r", j);
			for (i=0; i<j; i++) PRINTF(" ");
			ubasic_accept(TOKENIZER_RIGHTPAREN);
		} else if(tokenizer_token() == TOKENIZER_VARIABLE  ||
			tokenizer_token() == TOKENIZER_LEFTPAREN ||
			tokenizer_token() == TOKENIZER_MINUS     ||
			tokenizer_token() == TOKENIZER_PLUS      ||
			#if UBASIC_RND
			tokenizer_token() == TOKENIZER_RND       ||
			#endif
			#if UBASIC_ABS
			tokenizer_token() == TOKENIZER_ABS       ||
			#endif
			#if UBASIC_NOT
			tokenizer_token() == TOKENIZER_NOT       ||
			#endif
			#if UBASIC_CALL
			tokenizer_token() == TOKENIZER_CALL      ||
			#endif
			#if UBASIC_CVARS
			tokenizer_token() == TOKENIZER_VPEEK     ||
			#endif
			#if AVR_EPEEK
			tokenizer_token() == TOKENIZER_EPEEK     ||
			#endif
			#if AVR_IN
			tokenizer_token() == TOKENIZER_IN        ||
			#endif
			#if AVR_ADC
			tokenizer_token() == TOKENIZER_ADC       ||
			#endif
			#if UBASIC_STRING
			tokenizer_token() == TOKENIZER_LEN       ||
			tokenizer_token() == TOKENIZER_VAL       ||
			tokenizer_token() == TOKENIZER_ASC       ||
			tokenizer_token() == TOKENIZER_MAXSTRLEN ||
			tokenizer_token() == TOKENIZER_INSTR     ||
			#endif
			tokenizer_token() == TOKENIZER_NUMBER ) {
				PRINTF("%i", ubasic_expr());
#if UBASIC_STRING
			} else if(tokenizer_token() == TOKENIZER_STRINGVAR	||
				tokenizer_token() == TOKENIZER_LEFT				||
				tokenizer_token() == TOKENIZER_RIGHT			||
				tokenizer_token() == TOKENIZER_MID				||
				tokenizer_token() == TOKENIZER_CHR				||
				tokenizer_token() == TOKENIZER_LOWER			||
				tokenizer_token() == TOKENIZER_UPPER			||
				tokenizer_token() == TOKENIZER_STR) {
				PRINTF("%s", strexpr());
#endif
		} else {
			break;
		}
	} while(tokenizer_token() != TOKENIZER_CR &&
			tokenizer_token() != TOKENIZER_ENDOFINPUT);
	// wenn "," oder ";" am Zeilenende, dann kein Zeilenvorschub
	if (nl) {
		PRINTF("\n\r");
	}
}
#endif
/*---------------------------------------------------------------------------*/
static void if_statement(void) {
	int r, r1, lop;
	unsigned char no_then=0;

	ubasic_accept(TOKENIZER_IF);

#if UBASIC_STRING
		if (ubasic_is_strexpr())
			r = strrelation();
		else
#endif
			r = relation();

	lop = tokenizer_token();
	while (lop == TOKENIZER_LOGAND ||
		   lop == TOKENIZER_LOGOR)    {
		tokenizer_next();
#if UBASIC_STRING
		if (ubasic_is_strexpr())
			r1 = strrelation();
		else
#endif
			r1 = relation();
		if (lop == TOKENIZER_LOGAND)
			r = r & r1;
		else if (lop == TOKENIZER_LOGOR)
			r = r | r1;
		lop = tokenizer_token();
	}

		// Kurzform (IF ohne THEN/ELSE)?
	if (tokenizer_token() == TOKENIZER_THEN) ubasic_accept(TOKENIZER_THEN); else no_then=1;
	if(r) {
		statement();
		// bei Kurzform darf kein ELSE kommen!
		if (no_then && (tokenizer_token() == TOKENIZER_ELSE)) {
		    tokenizer_error_print(current_linenum, SHORT_IF_WITH_ELSE);
			ubasic_break();
		}
		// hmm..., hier ist man schon ein Token zu weit...
		if	((tokenizer_token() == TOKENIZER_NUMBER)||
			(tokenizer_token() != TOKENIZER_ELSE)) return;
		jump_to_next_linenum();
	} else {
		do {
			tokenizer_next();
		} while(tokenizer_token() != TOKENIZER_ELSE &&
				tokenizer_token() != TOKENIZER_CR &&
				tokenizer_token() != TOKENIZER_ENDOFINPUT);
		if(tokenizer_token() == TOKENIZER_ELSE) {
			// bei Kurzform darf kein ELSE kommen!
			if (no_then) {
				tokenizer_error_print(current_linenum, SHORT_IF_WITH_ELSE);
				ubasic_break();
			}
			tokenizer_next();
			statement();
		} else	if(tokenizer_token() == TOKENIZER_CR) tokenizer_next();
	}
}

/*---------------------------------------------------------------------------*/
static void let_statement(void) {
	struct varinfo_t var;
#if UBASIC_STRING
	unsigned char is_strvar;
	if (tokenizer_token() == TOKENIZER_VARIABLE) {
		is_strvar = 0;
#endif
		ubasic_accept(TOKENIZER_VARIABLE);
		var = ubasic_get_varinfo();
#if UBASIC_STRING
	} else {
		is_strvar = 1;
		ubasic_accept(TOKENIZER_STRINGVAR);
		var = ubasic_get_strvarinfo();
	}
#endif
	ubasic_accept(TOKENIZER_EQ);
#if UBASIC_STRING
	if (is_strvar)
		ubasic_set_strvariable(var, strexpr());
	else
#endif
		ubasic_set_variable(var, ubasic_expr());
}
/*---------------------------------------------------------------------------*/
static void gosub_statement(void){
	int linenum=0;
	ubasic_accept(TOKENIZER_GOSUB);

#if UBASIC_EXT_PROC
	char p_name[MAX_PROG_NAME_LEN]="";
	if (tokenizer_token() == TOKENIZER_STRING) {
		strncpy(p_name, tokenizer_last_string_ptr(), MAX_PROG_NAME_LEN);
		jump_to_next_linenum();
	} else
#else
	if (tokenizer_token() == TOKENIZER_STRING) {
	    tokenizer_error_print(current_linenum, GOSUB_NO_EXT_SUBPROC);
    	ubasic_break();
	} else
#endif
	{
		linenum = ubasic_expr();
		// es muss bis zum Zeilenende gelesen werden, um die Rueck-
		// sprungzeile fuer return zu ermitteln
		if (tokenizer_token() != TOKENIZER_CR) jump_to_next_linenum();
		else  tokenizer_next();
		tokenizer_next();
	}
	if(gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
		gosub_stack[gosub_stack_ptr].p_ptr = get_prog_text_pointer();
		#if UBASIC_EXT_PROC
			strncpy(gosub_stack[gosub_stack_ptr].p_name, current_proc, MAX_PROG_NAME_LEN);
		#endif
		gosub_stack_ptr++;
#if UBASIC_EXT_PROC
		if (p_name[0]) {
			switch_proc(p_name);
		} else
#endif
		jump_linenum(linenum);
	} else {
	    tokenizer_error_print(current_linenum, GOSUB_STACK_DETH);
    	ubasic_break();
	}
}
/*---------------------------------------------------------------------------*/
static void return_statement(void) {
	ubasic_accept(TOKENIZER_RETURN);
	if(gosub_stack_ptr > 0) {
		gosub_stack_ptr--;
		#if UBASIC_EXT_PROC
			// wenn nicht gleiches Programm, dann erstmal umschalten...
			if (strncmp(current_proc, gosub_stack[gosub_stack_ptr].p_name, MAX_PROG_NAME_LEN)){
				switch_proc(gosub_stack[gosub_stack_ptr].p_name);
			}
		#endif
		jump_to_prog_text_pointer(gosub_stack[gosub_stack_ptr].p_ptr);
	} else {
	    tokenizer_error_print(current_linenum, GOSUB_STACK_INVALID);
    	ubasic_break();
	}
}
/*---------------------------------------------------------------------------*/
static void next_statement(void) {
  struct varinfo_t var;

  ubasic_accept(TOKENIZER_NEXT);
  ubasic_accept(TOKENIZER_VARIABLE);
  var = ubasic_get_varinfo();
  if(for_stack_ptr > 0 && var.varnum == for_stack[for_stack_ptr - 1].for_variable) {
    ubasic_set_variable(var, ubasic_get_variable(var) + for_stack[for_stack_ptr - 1].step);
    if(((ubasic_get_variable(var) <= for_stack[for_stack_ptr - 1].to) && !for_stack[for_stack_ptr - 1].downto)||
       ((ubasic_get_variable(var) >= for_stack[for_stack_ptr - 1].to) && for_stack[for_stack_ptr - 1].downto)
      ) {
      jump_to_prog_text_pointer(for_stack[for_stack_ptr - 1].next_line_ptr);
    } else {
      for_stack_ptr--;
      ubasic_accept(TOKENIZER_CR);
    }
  } else {
    ubasic_accept(TOKENIZER_CR);
  }

}
/*---------------------------------------------------------------------------*/
static void for_statement(void) {
  int for_variable, to, step;
  unsigned char downto=0;
  struct varinfo_t var;

  ubasic_accept(TOKENIZER_FOR);
  ubasic_accept(TOKENIZER_VARIABLE);
  var = ubasic_get_varinfo();
  for_variable = var.varnum;
  ubasic_accept(TOKENIZER_EQ);
  ubasic_set_variable(var, ubasic_expr());
  if (tokenizer_token() == TOKENIZER_TO) {
  	downto = 0;
  } else if (tokenizer_token() == TOKENIZER_DOWNTO) {
  	downto = 1;
  } else tokenizer_error_print(current_linenum, FOR_WITHOUT_TO);
  tokenizer_next();
  to = ubasic_expr();
  if(tokenizer_token() == TOKENIZER_STEP) {
  	tokenizer_next();
  	step = ubasic_expr();
  } else {
  	step = 1;
  }
  if (downto) step *= -1;
  ubasic_accept(TOKENIZER_CR);
  if(for_stack_ptr < MAX_FOR_STACK_DEPTH) {
    for_stack[for_stack_ptr].next_line_ptr = get_prog_text_pointer();
    for_stack[for_stack_ptr].for_variable = for_variable;
    for_stack[for_stack_ptr].to = to;
    for_stack[for_stack_ptr].step = step;
    for_stack[for_stack_ptr].downto = downto;
    for_stack_ptr++;
  } else {
    tokenizer_error_print(current_linenum, FOR_STACK_DETH);
   	ubasic_break();
  }
}
/*---------------------------------------------------------------------------*/
static void end_statement(void) {
	ubasic_accept(TOKENIZER_END);
	ended = 1;
}

/*---------------------------------------------------------------------------*/
#if UBASIC_REM
static void rem_statement(void) {
	ubasic_accept(TOKENIZER_REM);
	// im if gekapselt wg. (z.B.) leere REM-Anweisung
	if (tokenizer_token() != TOKENIZER_CR) jump_to_next_linenum();
}
#endif

/*---------------------------------------------------------------------------*/
#if UBASIC_RND
#if USE_AVR
long unsigned int seed = 0;
static void srand_statement(void) {
	unsigned int *p = (unsigned int*) (RAMEND+1);
	extern unsigned int __heap_start;
	ubasic_accept(TOKENIZER_SRND);
	while (p >= &__heap_start + 1)
		seed ^= * (--p);
}
#else
static void srand_statement(void) {
	time_t t;
	ubasic_accept(TOKENIZER_SRND);
	time(&t);
	srand((unsigned int)t);
}
#endif
#endif

/*---------------------------------------------------------------------------*/
#if UBASIC_ARRAY
static void dim_statement(void) {
	int var, dim, i;
#if UBASIC_STRING
	int is_strvar=0;
#endif
	ubasic_accept(TOKENIZER_DIM);
	var = tokenizer_variable_num();
#if UBASIC_STRING
	if (tokenizer_token() == TOKENIZER_STRINGVAR) {
		ubasic_accept(TOKENIZER_STRINGVAR);
		is_strvar=1;
	} else
#endif
	ubasic_accept(TOKENIZER_VARIABLE);
	ubasic_accept(TOKENIZER_LEFTPAREN);
	// Dimension des Array ermitteln
	dim=ubasic_expr();
	ubasic_accept(TOKENIZER_RIGHTPAREN);
	ubasic_accept(TOKENIZER_CR);
#if UBASIC_STRING
	if (is_strvar) {
		// Stringarray anlegen/initialisieren
		strvariables[var].dim=dim;
		// wenn Speicher schon mal reserviert war, zuerst freigeben
		if (!(strvariables[var].adr == NULL)) free(strvariables[var].adr);
		strvariables[var].adr=malloc((size_t)dim * (MAX_STRINGLEN+1));
		for (i=0; i<dim; i++) strvariables[var].adr[i*(MAX_STRINGLEN+1)+1]=0;
	} else
#endif
	{
		// Integerfelder anlegen/initialisieren
		variables[var].dim=dim;
		if (!(variables[var].adr == NULL)) free(variables[var].adr);
		variables[var].adr=malloc((size_t)dim * sizeof(int));
		if (variables[var].adr == NULL	) {
			tokenizer_error_print(current_linenum, OUT_OF_MEMORY);
			ubasic_break();
		}
		for (i=0; i<dim; i++) variables[var].adr[i]=0;
	}
}
#endif

#if UBASIC_INPUT
/*---------------------------------------------------------------------------*/
static void input_statement(void) {
	#if UBASIC_STRING
	char buf[MAX_STRINGLEN];
	#else
	char buf[MAX_INPUT_LEN];
	#endif
	char* buf_ptr = buf;
	ubasic_accept(TOKENIZER_INPUT);
	if (tokenizer_token() == TOKENIZER_STRING) {
		PRINTF("%s", tokenizer_last_string_ptr());
		ubasic_accept(TOKENIZER_STRING);
		ubasic_accept(TOKENIZER_SEMICOLON);
	} else {
		PRINTF("? ");
	}
	do {
		if (tokenizer_token()==TOKENIZER_VARIABLE) {
			ubasic_accept(TOKENIZER_VARIABLE);
			GETLINE(buf_ptr, MAX_INPUT_LEN);
			ubasic_set_variable(ubasic_get_varinfo(), atoi(buf));
		#if UBASIC_STRING
		} else if (tokenizer_token()==TOKENIZER_STRINGVAR) {
			ubasic_accept(TOKENIZER_STRINGVAR);
			GETLINE(buf_ptr, MAX_STRINGLEN);
			ubasic_set_strvariable(ubasic_get_strvarinfo(), buf_ptr);
		#endif
		} else if (tokenizer_token()==TOKENIZER_COMMA) {
			PRINTF("\n\r? ");
			tokenizer_next();
		} else {
			break;
		}
	} while(tokenizer_token() != TOKENIZER_CR &&
			tokenizer_token() != TOKENIZER_ENDOFINPUT);
	PRINTF("\n\r");
}
#endif

#if UBASIC_DATA
/*---------------------------------------------------------------------------*/
static void ubasic_search_data(void) {
	struct tokenizer_pos_t save_pos;
	// Tokenizer-Position sichern
	save_pos=tokenizer_get_position();
	// DATA-Anweisung suchen
	do {
		// REM-Zeilen ueberlesen
		if (tokenizer_token() == TOKENIZER_REM) {
			jump_to_next_linenum();
		} else {
			tokenizer_next();
		}
	} while(tokenizer_token() != TOKENIZER_DATA  &&
	        tokenizer_token() != TOKENIZER_ENDOFINPUT &&
	        !tokenizer_finished());
	// kein DATA (mehr) gefunden...?
	if (tokenizer_token() == TOKENIZER_ENDOFINPUT || tokenizer_finished()) {
		// ... nein, Fehlerbehandlung
	    tokenizer_error_print(current_linenum, NOT_ENOUGH_DATA);
		ubasic_break();
	} else {
		// ... ja, Positionen merken
		if (!data_ptr.first.prog_ptr) data_ptr.first = tokenizer_get_position();
		data_ptr.current = tokenizer_get_position();
	}
	// Tokenizer-Position wieder zuruecksetzen
	tokenizer_set_position(save_pos);
}


/*---------------------------------------------------------------------------*/
static int ubasic_next_data(void) {
	struct tokenizer_pos_t save_pos;
	int val=0;
	// aktuelle Tokenizer-Position merken
	save_pos=tokenizer_get_position();
	// wenn noch nie DATA, dann suchen
	if (!data_ptr.current.prog_ptr) {
		ubasic_search_data();
	}
	// Tokenizer auf aktuelle DATA-Position setzen
	tokenizer_set_position(data_ptr.current);
	tokenizer_next();
	// ... und naechsten DATA-Wert ermitteln
	do {
		if (tokenizer_token() == TOKENIZER_COMMA) {
			tokenizer_next();
		} else if (tokenizer_token() == TOKENIZER_CR) {
			// aktuelles DATA zuende, naechstes Auftreten suchen
			ubasic_search_data();
			// ... und Position merken
			tokenizer_set_position(data_ptr.current);
			tokenizer_next();
		} else if (tokenizer_token() == TOKENIZER_NUMBER) {
			//...
		} else if (tokenizer_token() == TOKENIZER_MINUS) {
			// Interger-Werte duerfen auch negativ sein
			ubasic_accept(TOKENIZER_MINUS);
			//... dann muss aber auch eine Zahl nach dem Minus kommen!
			if (tokenizer_token()!=TOKENIZER_NUMBER) {
				tokenizer_error_print(current_linenum, SYNTAX_ERROR);
				ubasic_break();
			}
			// last_num im tokenizer manipulieren...
			tokenizer_set_num(tokenizer_num()*(-1));
#if UBASIC_STRING
		} else if (tokenizer_token() == TOKENIZER_STRING) {
			//...
#endif
		} else
			// unzulaessiges Token in DATA-Anweisung
			break;
	} while (tokenizer_token() != TOKENIZER_NUMBER &&
#if UBASIC_STRING
			 tokenizer_token() != TOKENIZER_STRING &&
#endif
	         tokenizer_token() != TOKENIZER_ENDOFINPUT &&
	         !tokenizer_finished());
	// "ausreichend" DATA-Werte vorhanden...?
	if (tokenizer_token() == TOKENIZER_ENDOFINPUT || tokenizer_finished()) {
		// ... nein, Fehlerbehandlung
	    tokenizer_error_print(current_linenum, NOT_ENOUGH_DATA);
		ubasic_break();
	} else {
		// ... ja, Position merken, Wert zuweisen
		data_ptr.current = tokenizer_get_position();
		val=tokenizer_token();
	}
	// Tokenizer-Position wieder zuruecksetzen
	tokenizer_set_position(save_pos);
	return val;
}

/*---------------------------------------------------------------------------*/
static void data_statement(void) {
	// erstes Auftreten von DATA und noch keine Pointer gesetzt
	if (!data_ptr.first.prog_ptr) {
		data_ptr.first = tokenizer_get_position();
		data_ptr.current = data_ptr.first;
	}
	// ansonsten Rest ueberlesen (wie REM)
	jump_to_next_linenum();
}
/*---------------------------------------------------------------------------*/
static void read_statement(void) {
	struct varinfo_t var;
	ubasic_accept(TOKENIZER_READ);
	do {
		if (tokenizer_token()==TOKENIZER_VARIABLE) {
			tokenizer_next();
			var = ubasic_get_varinfo();
			// naechsten DATA-Wert ermitteln
			if (ubasic_next_data()!=TOKENIZER_NUMBER) {
				// DATA-Wert passt nicht zu READ-Variablentyp (Integer)
				tokenizer_error_print(current_linenum, DATA_READ_TYPE_DIFF);
				ubasic_break();
			}
			ubasic_set_variable(var, tokenizer_num());
			tokenizer_next();
#if UBASIC_STRING
		} else if (tokenizer_token()==TOKENIZER_STRINGVAR) {
			tokenizer_next();
			var = ubasic_get_strvarinfo();
			if (ubasic_next_data()!=TOKENIZER_STRING) {
				// DATA-Wert passt nicht zu READ-Variablentyp (String)
				tokenizer_error_print(current_linenum, DATA_READ_TYPE_DIFF);
				ubasic_break();
			}
			ubasic_set_strvariable(var, (char*)tokenizer_last_string_ptr());
			tokenizer_next();
#endif
		} else if (tokenizer_token()==TOKENIZER_COMMA) {
			tokenizer_next();
		} else {
			break;
		}
	} while(tokenizer_token() != TOKENIZER_CR &&
			tokenizer_token() != TOKENIZER_ENDOFINPUT);
}
/*---------------------------------------------------------------------------*/
static void restore_statement(void) {
	if (data_ptr.first.prog_ptr) {
		// DATA wurde schon mal erkannt, also PTR auf erstes DATA setzen
		data_ptr.current = data_ptr.first;
	} else {
		// noch kein DATA, also suchen
		ubasic_search_data();
	}
	ubasic_accept(TOKENIZER_RESTORE);
}
#endif


/*---------------------------------------------------------------------------*/
static void statement(void) {
  int token;

  token = tokenizer_token();

  switch(token) {
  #if UBASIC_PRINT
  case TOKENIZER_PRINT:
    print_statement();
    break;
  #endif

  case TOKENIZER_IF:
    if_statement();
    break;

  case TOKENIZER_GOTO:
    goto_statement();
    break;

  case TOKENIZER_GOSUB:
    gosub_statement();
    break;

  case TOKENIZER_RETURN:
    return_statement();
    break;

  case TOKENIZER_FOR:
    for_statement();
    break;

  case TOKENIZER_NEXT:
    next_statement();
    break;

  case TOKENIZER_END:
    end_statement();
    break;

  #if UBASIC_CALL
  case TOKENIZER_CALL:
    call_statement();
    break;
  #endif

  #if UBASIC_REM
  case TOKENIZER_REM:
    rem_statement();
    break;
  #endif

  #if UBASIC_CVARS
  case TOKENIZER_VPOKE:
    vpoke_statement();
    break;
  #endif

  #if AVR_EPOKE
  case TOKENIZER_EPOKE:
    epoke_statement();
    break;
  #endif

  #if AVR_WAIT
  case TOKENIZER_WAIT:
    wait_statement();
    break;
  #endif

  #if AVR_DIR
  case TOKENIZER_DIR:
    dir_statement();
    break;
  #endif

  #if AVR_OUT
  case TOKENIZER_OUT:
    out_statement();
    break;
  #endif

  #if UBASIC_RND
  case TOKENIZER_SRND:
    srand_statement();
    break;
  #endif

  #if UBASIC_ARRAY
  case TOKENIZER_DIM:
    dim_statement();
    break;
  #endif

  #if UBASIC_INPUT
  case TOKENIZER_INPUT:
    input_statement();
    break;
  #endif

  #if UBASIC_DATA
  case TOKENIZER_DATA:
    data_statement();
    break;

  case TOKENIZER_READ:
    read_statement();
    break;

  case TOKENIZER_RESTORE:
    restore_statement();
    break;
  #endif

  case TOKENIZER_LET:
    ubasic_accept(TOKENIZER_LET);
    /* Fall through. */
    CASE_NO_BREAK;
  case TOKENIZER_VARIABLE:
#if UBASIC_STRING
  case TOKENIZER_STRINGVAR:
#endif
    let_statement();
    break;

  default:
  	PRINTF("-->%i\n\r", token);
    tokenizer_error_print(current_linenum, UNKNOWN_STATEMENT);
    ubasic_break();
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void line_statement(void) {
	if (tokenizer_token() != TOKENIZER_NUMBER && UBASIC_NO_LINENUM_ALLOWED) {
		//....
	} else {
		current_linenum = tokenizer_num();
		ubasic_accept(TOKENIZER_NUMBER);
#if UBASIC_NO_LINENUM_ALLOWED
		if (tokenizer_token() == TOKENIZER_COLON) tokenizer_next();
#endif
	}
	// Zeilennummer/Label allein auf einer Zeile?
	if (tokenizer_token() == TOKENIZER_CR) {
		skip_all_whitespaces();
		tokenizer_next();
	}
	statement();
	return;
}
/*---------------------------------------------------------------------------*/
void ubasic_free_all_mem(void) {
	// dynamischer RAM fuer Arrays und Zeichenketten
#if UBASIC_ARRAY || UBASIC_STRING
	unsigned char i;
	for (i=0; i<MAX_VARNUM; i++) {
	#if UBASIC_ARRAY
		if (variables[i].adr) free(variables[i].adr);
		variables[i].adr=NULL;
		variables[i].dim=0;
	#endif
	#if UBASIC_STRING
		if (strvariables[i].val_adr) free(strvariables[i].val_adr);
		strvariables[i].val_adr=NULL;
		#if UBASIC_ARRAY
		if (strvariables[i].adr) free(strvariables[i].adr);
		strvariables[i].adr=NULL;
		strvariables[i].dim=0;
		#endif
	#endif
	}
#endif
}
/*---------------------------------------------------------------------------*/
void ubasic_run(void) {
	if(tokenizer_finished()) {
		return;
	}
	line_statement();
	// wenn "Leerzeile(n)", dann diese schon mal ueberlesen
	if (tokenizer_token() == TOKENIZER_CR) {
		skip_all_whitespaces();
		tokenizer_next();
	}
	// Programm zuende, allozierten Speicher frei machen
	if (ended || tokenizer_finished()) ubasic_free_all_mem();
}

/*---------------------------------------------------------------------------*/
int ubasic_finished(void)
{
	return ended || tokenizer_finished();
}

/*---------------------------------------------------------------------------*/
void ubasic_set_variable(struct varinfo_t var, int value) {
#if UBASIC_ARRAY
	if (variables[var.varnum].adr)
		variables[var.varnum].adr[var.idx]=value;
	else
#endif
		variables[var.varnum].val = value;
}

/*---------------------------------------------------------------------------*/
struct varinfo_t ubasic_get_varinfo(void) {
	struct varinfo_t var;
	var.varnum = tokenizer_variable_num();
#if UBASIC_ARRAY
	var.idx = 0;
#endif
	if(var.varnum >= 0 && var.varnum < MAX_VARNUM) {
#if UBASIC_ARRAY
		// handelt es sich um ein Array?
		if (variables[var.varnum].adr) {
			// Index in den Klammern ermitteln
			ubasic_accept(TOKENIZER_LEFTPAREN);
			var.idx=ubasic_expr();
			// damit die Dimension abpruefen
			if (var.idx<variables[var.varnum].dim) {
				ubasic_accept(TOKENIZER_RIGHTPAREN);
			} else {
				// Array-Index ausserhalb der Definition (DIM)
			    tokenizer_error_print(current_linenum, ARRAY_OUT_OF_RANGE);
    			ubasic_break();
			}
		}
#endif
	} else {
			// Variablenname unbekannt (MAX_VARNUM)
		    tokenizer_error_print(current_linenum, UNKNOWN_VARIABLE);
   			ubasic_break();
	}
	return var;
}

#if UBASIC_STRING
/*---------------------------------------------------------------------------*/
static char* ubasic_get_strvariable(struct varinfo_t var) {
# if UBASIC_ARRAY
	if (strvariables[var.varnum].adr)
		return &strvariables[var.varnum].adr[var.idx*(MAX_STRINGLEN + 1)];
	else
#endif
	if (strvariables[var.varnum].val_adr)
		return strvariables[var.varnum].val_adr;
	else {
		// String-Variable nicht initialisiert
	    tokenizer_error_print(current_linenum, STRINGVAR_NOT_INIT);
		ubasic_break();
	}
	return NULL;
}

/*---------------------------------------------------------------------------*/
struct varinfo_t ubasic_get_strvarinfo(void) {
	struct varinfo_t var;
	var.varnum = tokenizer_variable_num();
#if UBASIC_ARRAY
	var.idx = 0;
#endif
	if(var.varnum >= 0 && var.varnum < MAX_VARNUM) {
#if UBASIC_ARRAY
		// handelt es sich um ein Array?
		if (strvariables[var.varnum].adr) {
			// Index in den Klammern ermitteln
			ubasic_accept(TOKENIZER_LEFTPAREN);
			var.idx=ubasic_expr();
			// damit die Dimension abpruefen
			if (var.idx<strvariables[var.varnum].dim) {
				ubasic_accept(TOKENIZER_RIGHTPAREN);
			} else {
				// Array-Index ausserhalb der Definition (DIM)
			    tokenizer_error_print(current_linenum, ARRAY_OUT_OF_RANGE);
    			ubasic_break();
			}
		}
#endif
	} else {
			// Variablenname unbekannt (MAX_VARNUM)
		    tokenizer_error_print(current_linenum, UNKNOWN_VARIABLE);
   			ubasic_break();
	}
	return var;
}

/*---------------------------------------------------------------------------*/
static int strrelation(void)
{
	char s1[MAX_STRINGLEN+1];
	char s2[MAX_STRINGLEN+1];
	int r;
	int op;

	strcpy(s1, strexpr());
	op = tokenizer_token();
/*
	while(op == TOKENIZER_LT ||
		  op == TOKENIZER_GT ||
		  op == TOKENIZER_GE ||
		  op == TOKENIZER_LE ||
		  op == TOKENIZER_NE ||
		  op == TOKENIZER_EQ) {
*/
		tokenizer_next();
		strcpy(s2, strexpr());
		r = strcmp(s1, s2);
		// 0  --> s1==s2
		// >0 --> s1 > s2
		// <0 --> s1 < s2
		if (r == 0 &&
			(op == TOKENIZER_EQ ||
			 op == TOKENIZER_GE ||
			 op == TOKENIZER_LE)
			) r = 1;
		else if (r > 0 &&
				 (op == TOKENIZER_GT ||
				  op == TOKENIZER_GE)
				 ) r = 1;
		else if (r < 0 &&
				 (op == TOKENIZER_LT ||
				  op == TOKENIZER_LE)
				 ) r = 1;
		else if (r != 0 &&
				 op == TOKENIZER_NE
				 ) r = 1;
		else r = 0;

		op = tokenizer_token();
/*
	}
*/
	return r;
}

/*---------------------------------------------------------------------------*/
unsigned char ubasic_is_strexpr() {
	unsigned char r;
	struct tokenizer_pos_t save_pos;
	// Tokenizer-Position sichern
	save_pos=tokenizer_get_position();
	// ...und vorwaerts suchen
	r = 0;
	while (r == 0 &&
		tokenizer_token() != TOKENIZER_CR 			&&
		tokenizer_token() != TOKENIZER_LOGAND		&&
		tokenizer_token() != TOKENIZER_LOGOR		&&
		tokenizer_token() != TOKENIZER_VARIABLE		&&
		tokenizer_token() != TOKENIZER_NUMBER		&&
		tokenizer_token() != TOKENIZER_LEN			&&
		tokenizer_token() != TOKENIZER_VAL			&&
		tokenizer_token() != TOKENIZER_ASC			&&
		#if UBASIC_ABS
		tokenizer_token() != TOKENIZER_ABS			&&
		#endif
		#if UBASIC_NOT
		tokenizer_token() != TOKENIZER_NOT			&&
		#endif
		#if UBASIC_RND
		tokenizer_token() != TOKENIZER_RND			&&
		#endif
		#if AVR_EPEEK
		tokenizer_token() != TOKENIZER_EPEEK		&&
		#endif
		#if AVR_IN
		tokenizer_token() != TOKENIZER_IN			&&
		#endif
		#if AVR_ADC
		tokenizer_token() != TOKENIZER_ADC			&&
		#endif
		#if UBASIC_CVARS
		tokenizer_token() != TOKENIZER_VPEEK		&&
		#endif
	    tokenizer_token() != TOKENIZER_ENDOFINPUT) {
			if (tokenizer_token() == TOKENIZER_STRING	||
			tokenizer_token() == TOKENIZER_STRINGVAR	||
			tokenizer_token() == TOKENIZER_LEFT			||
			tokenizer_token() == TOKENIZER_RIGHT		||
			tokenizer_token() == TOKENIZER_MID			||
			tokenizer_token() == TOKENIZER_CHR			||
			tokenizer_token() == TOKENIZER_STR
			) r = 1;
		else tokenizer_next();
	}
	// Tokenizer-Position wieder zuruecksetzen
	tokenizer_set_position(save_pos);
	return r;
}

/*---------------------------------------------------------------------------*/
static char* strfactor(void) {
	char *r, *s;
	char c;
	int l, i, o;
	struct varinfo_t var;
	r=0;
	switch (tokenizer_token()) {

		case TOKENIZER_LEFTPAREN:
			ubasic_accept(TOKENIZER_LEFTPAREN);
			r=strexpr();
			ubasic_accept(TOKENIZER_RIGHTPAREN);
			break;

		case TOKENIZER_STRING:
			if (strlen(tokenizer_last_string_ptr()) >= MAX_STRINGLEN) {
				// Stringlaenge wird zu gross
				tokenizer_error_print(current_linenum, STRING_TO_LARGE);
				ubasic_break();
			}
			r = (char*)tokenizer_last_string_ptr();
			tokenizer_next();
			break;

		case TOKENIZER_STRINGVAR:
			ubasic_accept(TOKENIZER_STRINGVAR);
			var = ubasic_get_strvarinfo();
			r = ubasic_get_strvariable(var);
			break;

		case TOKENIZER_LEFT:
			ubasic_accept(TOKENIZER_LEFT);
			ubasic_accept(TOKENIZER_LEFTPAREN);
			s=strexpr();
			ubasic_accept(TOKENIZER_COMMA);
			l=ubasic_expr();
			if (l>(int)strlen(s)) r = s;
			else {
				for (i=0; i<l; i++) {
					str_buf[i] = s[i];
				}
				str_buf[i] = 0;
				r=(char*)&str_buf[0];
			}
			ubasic_accept(TOKENIZER_RIGHTPAREN);
			break;

		case TOKENIZER_RIGHT:
			ubasic_accept(TOKENIZER_RIGHT);
			ubasic_accept(TOKENIZER_LEFTPAREN);
			s=strexpr();
			ubasic_accept(TOKENIZER_COMMA);
			l=ubasic_expr();
			if (l>(int)strlen(s)) r = s;
			else {
				for (i=0; i<l; i++) {
					str_buf[i] = s[(int)strlen(s)-l+i];
				}
				str_buf[i] = 0;
				r=(char*)&str_buf[0];
			}
			ubasic_accept(TOKENIZER_RIGHTPAREN);
			break;

		case TOKENIZER_MID:
			ubasic_accept(TOKENIZER_MID);
			ubasic_accept(TOKENIZER_LEFTPAREN);
			s=strexpr();
			ubasic_accept(TOKENIZER_COMMA);
			o=ubasic_expr();
			ubasic_accept(TOKENIZER_COMMA);
			l=ubasic_expr();
			if (l>((int)strlen(s)-o)) r = s;
			else {
				for (i=0; i<l; i++) {
					str_buf[i] = s[o+i];
				}
				str_buf[i] = 0;
				r=(char*)&str_buf[0];
			}
			ubasic_accept(TOKENIZER_RIGHTPAREN);
			break;

		case TOKENIZER_CHR:
			ubasic_accept(TOKENIZER_CHR);
			ubasic_accept(TOKENIZER_LEFTPAREN);
			str_buf[0]=(char)ubasic_expr();
			str_buf[1]=0;
			r=(char*)&str_buf[0];
			ubasic_accept(TOKENIZER_RIGHTPAREN);
			break;

		case TOKENIZER_STR:
			ubasic_accept(TOKENIZER_STR);
			ubasic_accept(TOKENIZER_LEFTPAREN);
			l=ubasic_expr();
			i=0;
			while (l != 0) {
				str_buf[i]=(char)((l%10)+'0');
				l=l/10;
				i++;
			}
			str_buf[i]=0;
			// String umdrehen
			l=(int)strlen(str_buf);
			for (i=0; i<l/2; i++) {
				c=str_buf[i];
				str_buf[i]=str_buf[l-i-1];
				str_buf[l-i-1]=c;
			}
			r=(char*)&str_buf[0];
			ubasic_accept(TOKENIZER_RIGHTPAREN);
			break;

		case TOKENIZER_UPPER:
			ubasic_accept(TOKENIZER_UPPER);
			ubasic_accept(TOKENIZER_LEFTPAREN);
			s=strexpr();
			while (*s != '\0') {
				if (islower (*s)) *s = (char)toupper (*s);
				++s;
			}
			r=(char*)&str_buf[0];
			ubasic_accept(TOKENIZER_RIGHTPAREN);
			break;

		case TOKENIZER_LOWER:
			ubasic_accept(TOKENIZER_LOWER);
			ubasic_accept(TOKENIZER_LEFTPAREN);
			s=strexpr();
			while (*s != '\0') {
				if (isupper (*s)) *s = (char)tolower (*s);
				++s;
			}
			r=(char*)&str_buf[0];
			ubasic_accept(TOKENIZER_RIGHTPAREN);
			break;

		default:
			break;
	}

	return r;
}

/*---------------------------------------------------------------------------*/
char* strexpr(void) {
	char s1[MAX_STRINGLEN+1];
	char *s2;
	int op, str_len;

	// Argument in lokalen Puffer kopieren
	strcpy(s1, strfactor());
	str_len = (int)strlen(s1);
	op = tokenizer_token();
	while(op == TOKENIZER_PLUS) {
		tokenizer_next();
		s2 = strfactor();
		str_len = str_len + (int)strlen(s2);
		if (str_len >= MAX_STRINGLEN+1) {
			// Stringlaenge wird zu gross
		    tokenizer_error_print(current_linenum, STRING_TO_LARGE);
   			ubasic_break();
		}
		strcat(s1, s2);
		op = tokenizer_token();
	}
	// Ergebnis in globalen Stringpuffer kopieren
	strcpy(str_buf, s1);
	return str_buf;
}

/*---------------------------------------------------------------------------*/
void ubasic_set_strvariable(struct varinfo_t var, char *str) {
#if UBASIC_ARRAY
	if (strvariables[var.varnum].adr)
		strcpy(&strvariables[var.varnum].adr[var.idx*(MAX_STRINGLEN+1)], str);
	else
#endif
	{
		// Speicher fuer Variable reservieren
		if (strvariables[var.varnum].val_adr == NULL)
			strvariables[var.varnum].val_adr=malloc(MAX_STRINGLEN+1);
		// genug Speicher vorhanden?
		if (strvariables[var.varnum].val_adr == NULL	) {
			tokenizer_error_print(current_linenum, OUT_OF_MEMORY);
			ubasic_break();
		}
		strcpy(strvariables[var.varnum].val_adr, str);
	}
}
#endif


/*---------------------------------------------------------------------------*/
int ubasic_get_variable(struct varinfo_t var) {
# if UBASIC_ARRAY
	if (variables[var.varnum].adr)
		return variables[var.varnum].adr[var.idx];
	else
#endif
		return variables[var.varnum].val;
}


/*---------------------------------------------------------------------------*/
// Park-Miller "minimal standard" 31Bit pseudo-random generator
// http://www.firstpr.com.au/dsp/rand31/
#if UBASIC_RND && USE_AVR
long unsigned int rand31_next(void)
{
	long unsigned int hi, lo;
	lo  = 16807 * (seed & 0xffff);
	hi  = 16807 * (seed >> 16);
	lo += (hi & 0x7fff) << 16;
	lo += hi >> 15;
	if (lo > 0x7fffffff) lo -= 0x7fffffff;
	return (seed = (long)lo);
}
#endif
/*---------------------------------------------------------------------------*/

#endif // BEHAVIOUR_UBASIC_AVAILABLE
