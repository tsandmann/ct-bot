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
 * \file 	ubasic.c
 * \brief 	uBasic-Interpreter
 * \author	Adam Dunkels
 * \author 	Uwe Berger (bergeruw@gmx.net)
 * \date 	10.06.2010
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "log.h"
#include "bot-logic/ubasic_call.h"

//#define DEBUG 1

#if DEBUG
#define DEBUG_PRINTF(...)  LOG_DEBUG(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include "bot-logic/ubasic.h"
#include "bot-logic/ubasic_tokenizer.h"
#include "bot-logic/ubasic_config.h"

#if UBASIC_CALL
#include "bot-logic/ubasic_call.h"
#endif

#if UBASIC_CVARS
#include "bot-logic/ubasic_cvars.h"
#endif

#if AVR_EPEEK || AVR_EPOKE
#include <avr/eeprom.h>
#endif

#if AVR_WAIT
#include <util/delay.h>
#endif

#if !BREAK_NOT_EXIT
#include <stdlib.h> /* exit() */
#endif

static char const * program_ptr;
static char string[MAX_STRINGLEN];
static int gosub_stack[MAX_GOSUB_STACK_DEPTH];
static int gosub_stack_ptr;

struct for_state {
	int line_after_for;
	int for_variable;
	int to;
	int step;
	unsigned char downto;
} PACKED;

static struct for_state for_stack[MAX_FOR_STACK_DEPTH];
static int for_stack_ptr;
static int variables[MAX_VARNUM];
static int ended;

int expr(void);
static void statement(void);
#if AVR_RND
static long unsigned int rand31_next(void);
#endif

/*---------------------------------------------------------------------------*/
void ubasic_init(const char * program) {
	int i;
	program_ptr = program;
	for_stack_ptr = gosub_stack_ptr = 0;
	for (i = 0; i < MAX_VARNUM; i++) {
		variables[i] = 0;
	}
	tokenizer_init(program);
	ended = 0;
}
/*---------------------------------------------------------------------------*/
void ubasic_break(void) {
#if BREAK_NOT_EXIT
	// zum Ende der Basic-Zeile und Ende-Merker setzen
	ended = 1;
	do {
		tokenizer_next();
	} while (tokenizer_token() != TOKENIZER_CR && tokenizer_token() != TOKENIZER_ENDOFINPUT);
#else
	exit(1);
#endif // BREAK_NOT_EXIT
}

/*---------------------------------------------------------------------------*/
void accept(int token) {
	if (token != tokenizer_token()) {
		DEBUG_PRINTF("Unexpected token (expected %i, got %i)", token, tokenizer_token());
		DEBUG_PRINTF(" last line was:");
		DEBUG_PRINTF(" \"%s\"", program_ptr);
		tokenizer_error_print(current_linenum, SYNTAX_ERROR);
		ubasic_break();
	}
	DEBUG_PRINTF("Expected %i, got it", token);
	tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static int varfactor(void) {
	int r;
	DEBUG_PRINTF("varfactor: obtaining %i from variable %i", variables[tokenizer_variable_num()], tokenizer_variable_num());
	r = ubasic_get_variable(tokenizer_variable_num());
	accept(TOKENIZER_VARIABLE);
	return r;
}
/*---------------------------------------------------------------------------*/
static int factor(void) {
	int r = 0;
#if AVR_IN || AVR_ADC || AVR_EPEEK
	int adr;
	char port, pin;
#endif // AVR_IN || AVR_ADC || AVR_EPEEK
#if AVR_RND
	int b;
#endif

	DEBUG_PRINTF("factor: token %i", tokenizer_token());
	switch (tokenizer_token()) {
	case TOKENIZER_NUMBER:
		r = tokenizer_num();
		accept(TOKENIZER_NUMBER);
		break;

		//?????
	case TOKENIZER_MINUS:
		accept(TOKENIZER_MINUS);
		r = tokenizer_num() * (-1);
		accept(TOKENIZER_NUMBER);
		break;

	case TOKENIZER_LEFTPAREN:
		accept(TOKENIZER_LEFTPAREN);
		r = expr();
		accept(TOKENIZER_RIGHTPAREN);
		break;

#if AVR_RND
	case TOKENIZER_RND:
		accept(TOKENIZER_RND);
		accept(TOKENIZER_LEFTPAREN);
		b = expr();
		r = rand31_next() % (b+1);
		accept(TOKENIZER_RIGHTPAREN);
		break;
#endif // AVR_RND

#if UBASIC_ABS
	case TOKENIZER_ABS:
		accept(TOKENIZER_ABS);
		accept(TOKENIZER_LEFTPAREN);
		r = expr();
		if (r < 0) {
			r = r * (-1);
		}
		accept(TOKENIZER_RIGHTPAREN);
		break;
#endif // UBASIC_ABS

#if UBASIC_NOT
	case TOKENIZER_NOT:
		accept(TOKENIZER_NOT);
		accept(TOKENIZER_LEFTPAREN);
		r = ~expr();
		accept(TOKENIZER_RIGHTPAREN);
		break;
#endif // UBASIC_NOT

#if UBASIC_CALL
	case TOKENIZER_CALL:
		r = ubasic_call_statement();
		break;
#endif // UBASIC_CALL

#if UBASIC_CVARS
	case TOKENIZER_VPEEK:
		r = ubasic_vpeek_expression();
		break;
#endif // UBASIC_CVARS

#if AVR_EPEEK
	case TOKENIZER_EPEEK:
		accept(TOKENIZER_EPEEK);
		accept(TOKENIZER_LEFTPAREN);
		adr = expr();
		r = eeprom_read_byte((unsigned char *)adr);
		accept(TOKENIZER_RIGHTPAREN);
		break;
#endif // AVR_EPEEK

#if AVR_ADC
	case TOKENIZER_ADC:
		accept(TOKENIZER_ADC);
		accept(TOKENIZER_LEFTPAREN);
		pin=tokenizer_num();
		tokenizer_next();
		if ((pin < 0) || (pin > ADC_COUNT_MAX)) {
			//Fehlerfall
			DEBUG_PRINTF("adc_token: unknown channel %c", pin);
			tokenizer_error_print(current_linenum, UNKNOWN_ADC_CHANNEL);
			ubasic_break();
		} else {
			// ADC_Kanal und Referenzspannung (hier Avcc) einstellen
			ADMUX = pin;
			ADMUX |= (1 << REFS0);
			// Prescaler 8:1 und ADC aktivieren
			ADCSRA = (1 << ADEN) | (1 << ADPS1) | (1 << ADPS0);
			// eine ADC-Wandlung fuer eine Dummy-Messung
			ADCSRA |= (1 << ADSC);
			while (ADCSRA & (1 << ADSC));
			r = ADCW;
			// eigentliche Messung
			ADCSRA |= (1 << ADSC);
			while (ADCSRA & (1 << ADSC));
			r = ADCW;
			// ADC wieder ausschalten
			ADCSRA = 0;
		}
		accept(TOKENIZER_RIGHTPAREN);
		break;
#endif // AVR_ADC

#if AVR_IN
	case TOKENIZER_IN:
		accept(TOKENIZER_IN);
		accept(TOKENIZER_LEFTPAREN);
		port=tokenizer_letter();
		tokenizer_next();
		accept(TOKENIZER_COMMA);
		pin=tokenizer_num();
		tokenizer_next();
		accept(TOKENIZER_RIGHTPAREN);
		switch (port) {
#if HAVE_PORTA
		case 'a':
		case 'A':
			if (bit_is_clear(PINA, pin)) r = 0; else r = 1;
			break;
#endif
#if HAVE_PORTB
		case 'b':
		case 'B':
			if (bit_is_clear(PINB, pin)) r = 0; else r = 1;
			break;
#endif
#if HAVE_PORTC
		case 'c':
		case 'C':
			if (bit_is_clear(PINC, pin)) r = 0; else r = 1;
			break;
#endif
#if HAVE_PORTD
		case 'd':
		case 'D':
			if (bit_is_clear(PIND, pin)) r = 0; else r = 1;
			break;
#endif
		default:
			DEBUG_PRINTF("in_token: unknown port %c", port);
			tokenizer_error_print(current_linenum, UNKNOWN_IO_PORT);
			ubasic_break();
		}
		break;
#endif // AVR_IN

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
	DEBUG_PRINTF("term: token %i", op);
	while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD) {
		tokenizer_next();
		f2 = factor();
		DEBUG_PRINTF("term: %i %i %i", f1, op, f2);
		switch (op) {
		case TOKENIZER_ASTR:
			f1 = f1 * f2;
			break;
		case TOKENIZER_SLASH:
			f1 = f1 / f2;
			break;
		case TOKENIZER_MOD:
			f1 = f1 % f2;
			break;
		}
		op = tokenizer_token();
	}
	DEBUG_PRINTF("term: %i", f1);
	return f1;
}
/*---------------------------------------------------------------------------*/
int expr(void) {
	int t1, t2;
	int op;

	t1 = term();
	op = tokenizer_token();
	DEBUG_PRINTF("expr: token %i", op);
	while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS || op == TOKENIZER_AND || op == TOKENIZER_OR) {
		tokenizer_next();
		t2 = term();
		DEBUG_PRINTF("expr: %i %i %i", t1, op, t2);
		switch (op) {
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
		}
		op = tokenizer_token();
	}
	DEBUG_PRINTF("expr: %i", t1);
	return t1;
}
/*---------------------------------------------------------------------------*/
static int relation(void) {
	int r1, r2;
	int op;

	r1 = expr();
	op = tokenizer_token();
	DEBUG_PRINTF("relation: token %i", op);
	while (op == TOKENIZER_LT || op == TOKENIZER_GT || op == TOKENIZER_EQ) {
		tokenizer_next();
		r2 = expr();
		DEBUG_PRINTF("relation: %i %i %i", r1, op, r2);
		switch (op) {
		case TOKENIZER_LT:
			r1 = r1 < r2;
			break;
		case TOKENIZER_GT:
			r1 = r1 > r2;
			break;
		case TOKENIZER_EQ:
			r1 = r1 == r2;
			break;
		}
		op = tokenizer_token();
	}
	return r1;
}
/*---------------------------------------------------------------------------*/
static void jump_linenum(int linenum) {
	tokenizer_init(program_ptr);
	while (tokenizer_num() != linenum) {
		do {
			do {
				tokenizer_next();
			} while (tokenizer_token() != TOKENIZER_CR && tokenizer_token() != TOKENIZER_ENDOFINPUT);
			if (tokenizer_token() == TOKENIZER_CR) {
				tokenizer_next();
			}
		} while (tokenizer_token() != TOKENIZER_NUMBER);
		DEBUG_PRINTF("jump_linenum: Found line %i", tokenizer_num());
	}
}
/*---------------------------------------------------------------------------*/
static void goto_statement(void) {
	accept(TOKENIZER_GOTO);
	jump_linenum(tokenizer_num());
}
/*---------------------------------------------------------------------------*/
/** \todo: Ausgabe auf Display */
static void print_statement(void) {
	accept(TOKENIZER_PRINT);
	do {
		DEBUG_PRINTF("Print loop");
		if (tokenizer_token() == TOKENIZER_STRING) {
			tokenizer_string(string, sizeof(string));
			PRINTF("%s", string);
			tokenizer_next();
		} else if (tokenizer_token() == TOKENIZER_COMMA) {
			PRINTF(" ");
			tokenizer_next();
		} else if (tokenizer_token() == TOKENIZER_SEMICOLON) {
			tokenizer_next();
		} else if (tokenizer_token() == TOKENIZER_VARIABLE ||
#if AVR_RND
			tokenizer_token() == TOKENIZER_RND ||
#endif
#if UBASIC_ABS
			tokenizer_token() == TOKENIZER_ABS ||
#endif
#if UBASIC_NOT
			tokenizer_token() == TOKENIZER_NOT ||
#endif
#if UBASIC_CALL
			tokenizer_token() == TOKENIZER_CALL ||
#endif
#if UBASIC_CVARS
			tokenizer_token() == TOKENIZER_VPEEK ||
#endif
#if AVR_EPEEK
			tokenizer_token() == TOKENIZER_EPEEK ||
#endif
#if AVR_IN
			tokenizer_token() == TOKENIZER_IN ||
#endif
#if AVR_ADC
			tokenizer_token() == TOKENIZER_ADC ||
#endif
			tokenizer_token() == TOKENIZER_NUMBER) {
			PRINTF("%i", expr());
		} else {
			break;
		}
	} while (tokenizer_token() != TOKENIZER_CR && tokenizer_token() != TOKENIZER_ENDOFINPUT);
	DEBUG_PRINTF("End of print");
	tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static void if_statement(void) {
	accept(TOKENIZER_IF);
	const int r = relation();
	DEBUG_PRINTF("if_statement: relation %i", r);
	accept(TOKENIZER_THEN);
	if (r) {
		statement();

		DEBUG_PRINTF("if_statement: token=%i", tokenizer_token());

		// hmm..., hier ist man schon ein Token zu weit...
		if (tokenizer_token() == TOKENIZER_NUMBER) {
			DEBUG_PRINTF("if_statement: WEG!");
			return;
		}

		while (tokenizer_token() != TOKENIZER_CR && tokenizer_token() != TOKENIZER_ENDOFINPUT) {
			tokenizer_next();
		}
		if (tokenizer_token() == TOKENIZER_CR) {
			tokenizer_next();
		}
	} else {
		do {
			tokenizer_next();
		} while (tokenizer_token() != TOKENIZER_ELSE && tokenizer_token() != TOKENIZER_CR && tokenizer_token()
			!= TOKENIZER_ENDOFINPUT);
		if (tokenizer_token() == TOKENIZER_ELSE) {
			tokenizer_next();
			statement();
		} else if (tokenizer_token() == TOKENIZER_CR) {
			tokenizer_next();
		}
	}
	DEBUG_PRINTF("if_statement ENDE: token=%i", tokenizer_token());

}

/*---------------------------------------------------------------------------*/
static void let_statement(void) {
	int var;

	var = tokenizer_variable_num();

	accept(TOKENIZER_VARIABLE);
	accept(TOKENIZER_EQ);
	ubasic_set_variable(var, expr());
	DEBUG_PRINTF("let_statement: assign %i to %i", variables[var], var);
	tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static void gosub_statement(void) {
	int linenum;
	accept(TOKENIZER_GOSUB);
	linenum = tokenizer_num();
	accept(TOKENIZER_NUMBER);
	tokenizer_next();
	if (gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
		gosub_stack[gosub_stack_ptr] = tokenizer_num();
		gosub_stack_ptr++;
		jump_linenum(linenum);
	} else {
		DEBUG_PRINTF("gosub_statement: gosub stack exhausted");
	}
}
/*---------------------------------------------------------------------------*/
static void return_statement(void) {
	accept(TOKENIZER_RETURN);
	if (gosub_stack_ptr > 0) {
		gosub_stack_ptr--;
		jump_linenum(gosub_stack[gosub_stack_ptr]);
	} else {
		DEBUG_PRINTF("return_statement: non-matching return");
	}
}
/*---------------------------------------------------------------------------*/
static void next_statement(void) {
	int var;

	accept(TOKENIZER_NEXT);
	var = tokenizer_variable_num();
	accept(TOKENIZER_VARIABLE);
	if (for_stack_ptr > 0 && var == for_stack[for_stack_ptr - 1].for_variable) {
		ubasic_set_variable(var, ubasic_get_variable(var) + for_stack[for_stack_ptr - 1].step);
		if (((ubasic_get_variable(var) <= for_stack[for_stack_ptr - 1].to) && !for_stack[for_stack_ptr - 1].downto)
			|| ((ubasic_get_variable(var) >= for_stack[for_stack_ptr - 1].to) && for_stack[for_stack_ptr - 1].downto)) {
			jump_linenum(for_stack[for_stack_ptr - 1].line_after_for);
		} else {
			for_stack_ptr--;
			accept(TOKENIZER_CR);
		}
	} else {
		DEBUG_PRINTF("next_statement: non-matching next (expected %i, found %i)", for_stack[for_stack_ptr - 1].for_variable, var);
		accept(TOKENIZER_CR);
	}

}
/*---------------------------------------------------------------------------*/
static void for_statement(void) {
	int for_variable, to, step;
	unsigned char downto;

	accept(TOKENIZER_FOR);
	for_variable = tokenizer_variable_num();
	accept(TOKENIZER_VARIABLE);
	accept(TOKENIZER_EQ);
	ubasic_set_variable(for_variable, expr());
	if (tokenizer_token() == TOKENIZER_TO) {
		downto = 0;
	} else if (tokenizer_token() == TOKENIZER_DOWNTO) {
		downto = 1;
	} else {
		tokenizer_error_print(current_linenum, FOR_WITHOUT_TO);
	}
	tokenizer_next();
	to = expr();
	if (tokenizer_token() == TOKENIZER_STEP) {
		tokenizer_next();
		step = expr();
	} else {
		step = 1;
	}
	if (downto) {
		step *= -1;
	}
	accept(TOKENIZER_CR);

	if (for_stack_ptr < MAX_FOR_STACK_DEPTH) {
		for_stack[for_stack_ptr].line_after_for = tokenizer_num();
		for_stack[for_stack_ptr].for_variable = for_variable;
		for_stack[for_stack_ptr].to = to;
		for_stack[for_stack_ptr].step = step;
		for_stack[for_stack_ptr].downto = downto;

		DEBUG_PRINTF("for_statement: new for, var %i to %i",
			for_stack[for_stack_ptr].for_variable,
			for_stack[for_stack_ptr].to);

		for_stack_ptr++;
	} else {
		DEBUG_PRINTF("for_statement: for stack depth exceeded");
	}
}
/*---------------------------------------------------------------------------*/
static void end_statement(void) {
	accept(TOKENIZER_END);
	ended = 1;
}

/*---------------------------------------------------------------------------*/
#if UBASIC_REM
static void rem_statement(void) {
	accept(TOKENIZER_REM);
	do {
		tokenizer_next();
	} while (tokenizer_token() != TOKENIZER_CR && tokenizer_token() != TOKENIZER_ENDOFINPUT);
	tokenizer_next();
}
#endif // UBASIC_REM

/*---------------------------------------------------------------------------*/
#if AVR_EPOKE
static void epoke_statement(void) {
	int adr, val;
	accept(TOKENIZER_EPOKE);
	accept(TOKENIZER_LEFTPAREN);
	adr = expr();
	accept(TOKENIZER_RIGHTPAREN);
	accept(TOKENIZER_EQ);
	val = expr();
	eeprom_write_byte((unsigned char *) adr, val);
	tokenizer_next();
}
#endif // AVR_EPOKE

/*---------------------------------------------------------------------------*/
#if AVR_DIR
static void dir_statement(void) {
	char port, pin, val;
	accept(TOKENIZER_DIR);
	accept(TOKENIZER_LEFTPAREN);
	port=tokenizer_letter();
	tokenizer_next();
	accept(TOKENIZER_COMMA);
	pin=tokenizer_num();
	tokenizer_next();
	accept(TOKENIZER_RIGHTPAREN);
	accept(TOKENIZER_EQ);
	if (expr()) val = 1; else val = 0;
	switch (port) {
#if HAVE_PORTA
	case 'a':
	case 'A':
		if (val) DDRA |= (1 << pin); else DDRA &= ~(1 << pin);
		break;
#endif
#if HAVE_PORTB
	case 'b':
	case 'B':
		if (val) DDRB |= (1 << pin); else DDRB &= ~(1 << pin);
		break;
#endif
#if HAVE_PORTC
	case 'c':
	case 'C':
		if (val) DDRC |= (1 << pin); else DDRC &= ~(1 << pin);
		break;
#endif
#if HAVE_PORTD
	case 'd':
	case 'D':
		if (val) DDRD |= (1 << pin); else DDRD &= ~(1 << pin);
		break;
#endif
		default:
		DEBUG_PRINTF("dir_statement: unknown port %c", port);
		tokenizer_error_print(current_linenum, UNKNOWN_IO_PORT);
		ubasic_break();
	}
	tokenizer_next();
}
#endif // AVR_DIR

/*---------------------------------------------------------------------------*/
#if AVR_OUT
static void out_statement(void) {
	char port, pin, val;
	accept(TOKENIZER_OUT);
	accept(TOKENIZER_LEFTPAREN);
	port=tokenizer_letter();
	tokenizer_next();
	accept(TOKENIZER_COMMA);
	pin=tokenizer_num();
	tokenizer_next();
	accept(TOKENIZER_RIGHTPAREN);
	accept(TOKENIZER_EQ);
	val = expr();
	switch (port) {
#if HAVE_PORTA
	case 'a':
	case 'A':
		if (val) PORTA |= (1 << pin); else PORTA &= ~(1 << pin);
		break;
#endif
#if HAVE_PORTB
	case 'b':
	case 'B':
		if (val) PORTB |= (1 << pin); else PORTB &= ~(1 << pin);
		break;
#endif
#if HAVE_PORTC
	case 'c':
	case 'C':
		if (val) PORTC |= (1 << pin); else PORTC &= ~(1 << pin);
		break;
#endif
#if HAVE_PORTD
	case 'd':
	case 'D':
		if (val) PORTD |= (1 << pin); else PORTD &= ~(1 << pin);
		break;
#endif
		default:
		DEBUG_PRINTF("out_statement: unknown port %c", port);
		tokenizer_error_print(current_linenum, UNKNOWN_IO_PORT);
		ubasic_break();
	}
	tokenizer_next();
}
#endif // AVR_OUT

/*---------------------------------------------------------------------------*/
#if AVR_WAIT
static void wait_statement(void) {
	int delay;
	accept(TOKENIZER_WAIT);
	delay = expr();
	int i;
	for (i = 0; i < delay; i++) {
		_delay_ms(1);
	}
	tokenizer_next();
}
#endif // AVR_WAIT

/** ct-Bot Anpassung **/
#if BOT_WAIT
static void bot_wait_statement(void) {
	DEBUG_PRINTF("== in bot_wait -> warten ==");

	accept(TOKENIZER_WAIT);

	const uint16_t delay = (uint16_t) expr();
	DEBUG_PRINTF(" Wartezeit: %u", delay);
	/* Wartezeit speichern, wird in bot_ubasic_behaviour() ausgewertet */
	ubasic_wait_until = TIMER_GET_TICKCOUNT_32 + MS_TO_TICKS(delay);

	tokenizer_next(); // Tokenizer weitersetzen
}
#endif // BOT_WAIT

/*---------------------------------------------------------------------------*/
#if AVR_RND
long unsigned int seed = 0;
static void srand_statement(void) {
	uint16_t *p = (uint16_t *) (RAMEND + 1);
	extern uint16_t __heap_start;
	accept(TOKENIZER_SRND);
	while (p >= &__heap_start + 1)
	seed ^= * (--p);
	tokenizer_next();
}
#endif // AVR_RND

/*---------------------------------------------------------------------------*/
static void statement(void) {
	int token = tokenizer_token();

	switch (token) {
	case TOKENIZER_PRINT:
		print_statement();
		break;
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
		ubasic_call_statement();
		break;
#endif // UBASIC_CALL

#if UBASIC_REM
	case TOKENIZER_REM:
		rem_statement();
		break;
#endif // UBASIC_REM

#if UBASIC_CVARS
	case TOKENIZER_VPOKE:
		ubasic_vpoke_statement();
		break;
#endif // UBASIC_CVARS

#if AVR_EPOKE
	case TOKENIZER_EPOKE:
		epoke_statement();
		break;
#endif // AVR_EPOKE

	case TOKENIZER_WAIT:
#if AVR_WAIT
		wait_statement();
#elif BOT_WAIT
		bot_wait_statement();
#endif // AVR_WAIT || BOT_WAIT
		break;

#if AVR_DIR
	case TOKENIZER_DIR:
		dir_statement();
		break;
#endif // AVR_DIR

#if AVR_OUT
	case TOKENIZER_OUT:
		out_statement();
		break;
#endif // AVR_OUT

#if AVR_RND
	case TOKENIZER_SRND:
		srand_statement();
		break;
#endif // AVR_RND

	case TOKENIZER_LET:
		accept(TOKENIZER_LET);
		/* Fall through. */
	case TOKENIZER_VARIABLE:
		let_statement();
		break;

	default:
		DEBUG_PRINTF("ubasic.c: statement(): not implemented %i", token);
		tokenizer_error_print(current_linenum, UNKNOWN_STATEMENT);
		ubasic_break();
	}
}
/*---------------------------------------------------------------------------*/

void line_statement(void) {
	DEBUG_PRINTF("----------- Line number %i ---------", tokenizer_num());
	current_linenum = tokenizer_num();
	accept(TOKENIZER_NUMBER);
	statement();
	return;
}
/*---------------------------------------------------------------------------*/
void ubasic_run(void) {
	if (tokenizer_finished()) {
		DEBUG_PRINTF("uBASIC program finished");
		return;
	}

	line_statement();
}
/*---------------------------------------------------------------------------*/
int ubasic_finished(void) {
	return ended || tokenizer_finished();
}
/*---------------------------------------------------------------------------*/
void ubasic_set_variable(int varnum, int value) {
	if (varnum >= 0 && varnum < MAX_VARNUM) {
		variables[varnum] = value;
	}
}
/*---------------------------------------------------------------------------*/
int ubasic_get_variable(int varnum) {
	if (varnum >= 0 && varnum < MAX_VARNUM) {
		return variables[varnum];
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
// Park-Miller "minimal standard" 31Bit pseudo-random generator
// http://www.firstpr.com.au/dsp/rand31/
#if AVR_RND
long unsigned int rand31_next(void) {
	long unsigned int hi, lo;
	lo = 16807 * (seed & 0xffff);
	hi = 16807 * (seed >> 16);
	lo += (hi & 0x7fff) << 16;
	lo += hi >> 15;
	if (lo > 0x7fffffff) lo -= 0x7fffffff;
	return (seed = (long) lo);
}
#endif // AVR_RND
#endif // BEHAVIOUR_UBASIC_AVAILABLE
