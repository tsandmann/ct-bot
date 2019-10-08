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
 * FastParser created by Rene Boellhoff; 2011
 * ------------------------------------------------------
 */

#include <stdint.h>

#include "bot-logic.h"
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "sdfat_fs.h"

#include "ubasic_config.h"

//#if USE_PROGMEM
//	#include <avr/pgmspace.h>
//#endif

#define __TOKENIZER_C__
	#include "tokenizer_access.h"
	#include "tokenizer.h"
#undef __TOKENIZER_C__
#include "ubasic_ext_proc.h"

#include <ctype.h>

#if USE_AVR
//	#include "../uart/usart.h"
#else
	#include <string.h>
//	#include <ctype.h>
	#include <stdlib.h>
	#include <stdio.h>
#endif


static PTR_TYPE line_begin_ptr;
static char last_string[MAX_STRINGLEN+1];
static int16_t  last_value;
static int16_t  last_var_num;

static int16_t current_token = TOKENIZER_ERROR;

extern PTR_TYPE program_ptr;

#if UBASIC_EXT_PROC
	extern char current_proc[MAX_PROG_NAME_LEN + 1];
#endif


#if TOKENIZER_STANDARD
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
	#if UBASIC_INPUT
	{"input", TOKENIZER_INPUT},
	#endif
	#if UBASIC_ARRAY
	{"dim", TOKENIZER_DIM},
	#endif
	#if UBASIC_DATA
	{"data", TOKENIZER_DATA},
	{"read", TOKENIZER_READ},
	{"restore", TOKENIZER_RESTORE},
	#endif
	#if UBASIC_STRING
	{"left$", TOKENIZER_LEFT},
	{"right$", TOKENIZER_RIGHT},
	{"mid$", TOKENIZER_MID},
	{"chr$", TOKENIZER_CHR},
	{"str$", TOKENIZER_STR},
	{"len", TOKENIZER_LEN},
	{"val", TOKENIZER_VAL},
	{"asc", TOKENIZER_ASC},
	{"m_strlen", TOKENIZER_MAXSTRLEN},
	{"upper$", TOKENIZER_UPPER},
	{"lower$", TOKENIZER_LOWER},
	{"instr$", TOKENIZER_INSTR},
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
	{"or", TOKENIZER_LOGOR},
	{"and", TOKENIZER_LOGAND},
	{"mod", TOKENIZER_MOD},
	{"<=", TOKENIZER_LE},
	{">=", TOKENIZER_GE},
	{"<>", TOKENIZER_NE},
	{"tab", TOKENIZER_TAB},
	{"", TOKENIZER_ERROR}
};
#endif

// Prototypen
static int16_t get_next_token(void);
#if !TOKENIZER_STANDARD
int16_t iFastParserGetKeyWord (void);
#endif


/*---------------------------------------------------------------------------*/
PTR_TYPE get_prog_text_pointer(void) {
	return line_begin_ptr;
}
/*---------------------------------------------------------------------------*/
static void skip_whitespaces(void){
	while((GET_CONTENT_PROG_PTR == ' '  ||
		   GET_CONTENT_PROG_PTR == '\r' ||
		   GET_CONTENT_PROG_PTR == '\t'    ) &&
		  !END_OF_PROG_TEXT
		 ) INCR_PROG_PTR;
}
/*---------------------------------------------------------------------------*/
void skip_all_whitespaces(void){
	while((GET_CONTENT_PROG_PTR == ' '  ||
		   GET_CONTENT_PROG_PTR == '\n' ||
		   GET_CONTENT_PROG_PTR == '\r' ||
		   GET_CONTENT_PROG_PTR == '\t'    ) &&
		  !END_OF_PROG_TEXT
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
#if TOKENIZER_STANDARD
static int16_t singlechar(void) {
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
		return TOKENIZER_MOD2;
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
  	#if UBASIC_STRING
  	else if(GET_CONTENT_PROG_PTR == '$') {
		return TOKENIZER_DOLLAR;
	}
  	#endif
	return 0;
}
#endif

#if UBASIC_HEX_BIN
/*---------------------------------------------------------------------------*/
static int16_t hex2int(char c) {
	if (c<='9') return (c-'0'); else return (c-'A'+10);
}
/*---------------------------------------------------------------------------*/
static char isbdigit(char c) {
	if ((c=='0') || (c=='1')) return 1; else return 0;
}
#endif

/*---------------------------------------------------------------------------*/
static int16_t get_next_token(void) {
#if !USE_PROGMEM && TOKENIZER_STANDARD
	struct keyword_token const *kt;
#endif
	uint8_t i;
	int16_t temp_token;
#if TOKENIZER_STANDARD
	char k_temp[MAX_KEYWORD_LEN+1];
#endif

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

						while (GET_CONTENT_PROG_PTR != '"') {
							last_string[i] = GET_CONTENT_PROG_PTR;
							INCR_PROG_PTR;
							i++;
							// max. zulaessige Stringlaenge?
							if (i >= MAX_STRINGLEN) return TOKENIZER_ERROR;
						}

						// String null-terminieren
						last_string[i]=0;
						INCR_PROG_PTR;
						return TOKENIZER_STRING;
				} else {
#if TOKENIZER_STANDARD

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

#else
	}
	// FastParser...
	temp_token = iFastParserGetKeyWord ();
	if (temp_token != -1) return temp_token;
#endif

	if(toupper(GET_CONTENT_PROG_PTR) >= 'A' && toupper(GET_CONTENT_PROG_PTR) <= 'Z') {
		last_var_num=toupper(GET_CONTENT_PROG_PTR)-'A';
		INCR_PROG_PTR;
#if UBASIC_STRING
		if (GET_CONTENT_PROG_PTR == '$') {
			INCR_PROG_PTR;
			return TOKENIZER_STRINGVAR;
		} else
#endif
		return TOKENIZER_VARIABLE;
	}
	return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
void tokenizer_init(PTR_TYPE program) {
	SET_PROG_PTR_ABSOLUT(program);
	line_begin_ptr=program;
#if UBASIC_NO_LINENUM_ALLOWED
	last_value = -1;
#endif
	skip_all_whitespaces();
	current_token = get_next_token();
}
/*---------------------------------------------------------------------------*/
int16_t tokenizer_token(void) {
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
void tokenizer_set_num(int16_t val) {
	last_value=val;
}
/*---------------------------------------------------------------------------*/
int16_t tokenizer_num(void) {
	return last_value;
}
/*---------------------------------------------------------------------------*/
char const *tokenizer_last_string_ptr(void) {
	return (const char*)&last_string[0];
}
/*---------------------------------------------------------------------------*/
void tokenizer_error_print(int16_t linenum, int16_t error_nr) {
	(void) linenum;
	(void) error_nr;
	PTR_TYPE current_prog_ptr;
	uint16_t source_linenum;
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
	PRINTF("\n\rerror %i at sourceline: %i (%i?) ", error_nr, source_linenum, linenum);
#if UBASIC_EXT_PROC
	PRINTF("in program %s", current_proc);
#endif
	PRINTF("\n\r");
	// Textpointer wieder auf alten Wert
	SET_PROG_PTR_ABSOLUT(current_prog_ptr);
}
/*---------------------------------------------------------------------------*/
int16_t tokenizer_finished(void) {
	return END_OF_PROG_TEXT || current_token == TOKENIZER_ENDOFINPUT;
}
/*---------------------------------------------------------------------------*/
int16_t tokenizer_variable_num(void) {
	return last_var_num;
}

/*---------------------------------------------------------------------------*/
struct tokenizer_pos_t tokenizer_get_position(void) {
	struct tokenizer_pos_t pos;
	pos.prog_ptr = PROG_PTR;
	pos.token = current_token;
	return pos;
}

/*---------------------------------------------------------------------------*/
void tokenizer_set_position(struct tokenizer_pos_t pos) {
	SET_PROG_PTR_ABSOLUT(pos.prog_ptr);
	current_token = pos.token;
}

/*---------------------------------------------------------------------------*/
/*----------------------- FASTPARSER... -------------------------------------*/
/*---------------------------------------------------------------------------*/
#if TOKENIZER_FASTPARSER

#include "tokenizer_data.inc"

int16_t iFastParserGetKeyWord (void)  {
  char            ucCh;                 // Zeichen im Basic-Quell-Text
  unsigned char   ucRC,                 // "Referenz"-Zeichen (zu vergleichendes Zeichen im Baum)
                  ucS,                  // Niedrigstes erstes ASCII-Zeichen im Baum
                  ucE;                  // Höchstes erstes ASCII-Zeichen im Baum
  const unsigned char  *pucPT = aucAVRBasic,  // Arbeitszeiger (mit Zeiger auf "Gesamtstruktur")
                 *pucTable;             // Zeiger auf Anfang des Baumes (nach Einsprungtabelle)

#ifndef SFP_USES_16BIT
  unsigned char uiN;                    // Index im Baum (8-Bit)
#else
  unsigned short uiN;                   // Index im Baum (16-Bit)
#endif

  PTR_TYPE ptr_bak = PROG_PTR;          // Position (vor der Suche)

  // Aktuelles Zeichen aus dem Text holen
  ucCh      = GET_CONTENT_PROG_PTR;

  // Niedrigstes und höchstes erstes Zeichen holen
  // (Länge der Einsprungtabelle)
#if USE_PROGMEM                         // Tabelle liegt im Flash
	ucS = pgm_read_byte (&(pucPT [0]));
	ucE = pgm_read_byte (&(pucPT [1]));
#else                                   // Tabelle liegt im RAM
	ucS = pucPT [0];
	ucE = pucPT [1];
#endif

  // Startadresse des Baums holen
#ifndef SFP_USES_16BIT                        // 8 bit Addressierung
	pucTable  = &(pucPT [2 + ucE - ucS]);
#else                                         // 16 bit Addressierung
  pucTable  = &(pucPT [2 + (((unsigned short) (ucE - ucS)) << 1)]);
#endif

   // Uppercase
	if ((ucCh >= 'A') && (ucCh <= 'Z')) {
		ucCh ^= 0x20;
	}

  // passt das erste Zeichen nicht ? dann Adios ..
	if ((ucCh < ucS) || (ucCh > ucE)) {
		SET_PROG_PTR_ABSOLUT (ptr_bak);
		return -1;
	}

  // Ja ..


  // ... dann Einsprung-Punkt in Baum holen
  ucCh = (char) (ucCh - ucS);
#ifndef SFP_USES_16BIT                        // 8 bit Addressierung
  #if USE_PROGMEM                             // Tabelle liegt im Flash
	uiN   = pgm_read_byte (&(pucPT [2 + ucCh]));
  #else                                       // Tabelle liegt im RAM
	uiN   = pucPT [2 + ucCh];
	#endif
#else                                         // 16 bit Addressierung
  #if USE_PROGMEM                             // Tabelle liegt im Flash
  uiN    = (unsigned short) (pgm_read_byte (&(pucPT [2 + (ucCh * 2)])) << 0);
  uiN   |= (unsigned short) (pgm_read_byte (&(pucPT [3 + (ucCh * 2)])) << 8);
  #else                                      // Tabelle liegt im RAM
  uiN    = (unsigned short) (&(pucPT [2 + (ucCh * 2)]) << 0);
  uiN   |= (unsigned short) (&(pucPT [3 + (ucCh * 2)]) << 8);
	#endif
#endif

  // keinen Einsprung-Punkt ? dann Tschöö ..
#ifndef SFP_USES_16BIT                        // 8 bit Addressierung
	if (uiN == 0xff) {
		SET_PROG_PTR_ABSOLUT (ptr_bak);
		return -1;
	}
#else                                         // 16 bit Addressierung
  if (uiN == 0xffff) {
    SET_PROG_PTR_ABSOLUT (ptr_bak);
    return -1;
  }
#endif

  // nächstes Zeichen im Text
  INCR_PROG_PTR;

  // Blatt beim Einsprungpunkt holen
#ifndef SFP_USES_16BIT
  pucPT = &(pucTable  [uiN << 1]);            // 8 bit Addressierung
#else
  pucPT = &(pucTable  [(uiN << 1) + uiN]);    // 16 bit Addressierung
#endif

	do
	{
    // aktuelles Zeichen aus dem Text holen
		ucCh  = GET_CONTENT_PROG_PTR;
   // zu Vergleichendes Zeichen aus dem Blatt holen
   #if USE_PROGMEM                           // Tabelle liegt im Flash
		ucRC  = pgm_read_byte (pucPT);
    #else                                     // Tabelle liegt im RAM
    ucRC  = *pucPT;
		#endif
    // Index bei Treffer holen (8-bit)
		pucPT++;
    #if USE_PROGMEM                           // Tabelle liegt im Flash
		uiN   = ((unsigned short) (pgm_read_byte (pucPT) << 0));
    #else                                     // Tabelle liegt im RAM
    uiN   = ((unsigned short) (*pucPT << 0));
		#endif
		pucPT++;

    // Index bei Treffer holen (16-bit)
#ifdef SFP_USES_16BIT
    #if USE_PROGMEM                           // Tabelle liegt im Flash
		uiN  |= ((unsigned short) (pgm_read_byte (pucPT) << 8));
		#else
    uiN  |= ((unsigned short) (*pucPT << 8)); // Tabelle liegt im RAM
		#endif
		pucPT++;
#endif

    // Solange wir kein Token gefunden haben (dann ist das Zeichen im Baum = 0)
		if (ucRC) {
      // Space im Baum hat Sonderbehandlung (kann evtl weg)
			if ((ucRC & 0x7f) == 0x20) {
        // im Text vorspulen und Space, CR, LF, Tab ignorieren
				while ((GET_CONTENT_PROG_PTR == 0x20) || (GET_CONTENT_PROG_PTR == 0x09) ||
					   (GET_CONTENT_PROG_PTR == 0x0a) || (GET_CONTENT_PROG_PTR == 0x0d)) {
					INCR_PROG_PTR;
				}
        // Arbeitszeiger auf Anfang des Baumes
				pucPT = pucTable;
        // Arbeitszeiger auf folgendes Blatt setzen
#ifdef SFP_USES_16BIT
        pucPT = &(pucPT [uiN + (uiN << 1)]);  // 16 Bit Addressierung -> pro Blatt 3 bytes
#else
        pucPT = &(pucPT [uiN << 1]);          // 8 Bit Addressierung -> pro Blatt 2 bytes
#endif
      // bei 0x01 im Baum -> Schluesselwort gefunden, weitere Zeichen irrelevant (hoffentlich)
			} else 	if ((ucRC & 0x7f) == 0x01) {
        // nächstes Blatt ist Token-Ende (ucRC = 0, uiN = Tokenwert)
        #if USE_PROGMEM                   // Tabelle liegt im Flash
						ucRC  = pgm_read_byte (pucPT);
        #else                             // Tabelle liegt im RAM
        ucRC  = *pucPT;
						#endif
						pucPT++;
        // Index (8 Bit)
        #if USE_PROGMEM                   // Tabelle liegt im Flash
						uiN   = ((unsigned short) (pgm_read_byte (pucPT) << 0));
        #else                            // Tabelle liegt im RAM
        uiN   = (unsigned short) (*pucPT << 0);
						#endif
						pucPT++;
        // Index (16 Bit)
#ifdef SFP_USES_16BIT
						#if USE_PROGMEM
						uiN  |= ((unsigned short) (pgm_read_byte (pucPT) << 8));
						#else
						uiN  |= (unsigned short) (*pucPT << 8);
						#endif
						pucPT++;
#endif
      // wenn Zeichen passt dann weiter im Baum
					} else 	if (toupper (ucCh) == toupper ((ucRC & 0x7f))) {
								INCR_PROG_PTR;
								pucPT = pucTable;
#ifdef SFP_USES_16BIT
								pucPT = &(pucPT [uiN + (uiN << 1)]);  // 16 bit addressierung -> pro blatt 3 bytes
#else
								pucPT = &(pucPT [uiN << 1]);  // 8 bit addressierung -> pro blatt 2 bytes
#endif
      // wenn Zeichen nicht passt einfach nächstes Blatt
							} else {
        // solange es nicht das Letzte Blatt ist
        if (ucRC & 0x80) { // wenn letzter in der Liste -> Ende (no match)
									SET_PROG_PTR_ABSOLUT (ptr_bak);
									return -1;
								}
							}
		}
	} while (ucRC);
  // Tokenende ist wenn das Zeichen des Blattes 0 ist. Der Tokenwert steht im Index
    return uiN;
}
#endif

#endif // BEHAVIOUR_UBASIC_AVAILABLE
