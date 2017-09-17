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
 * \file 	behaviour_abl.c
 * \brief 	Abstract Bot Language Interpreter
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	28.11.2007
 *
 *
 */

/* ABL in EBNF:
 *
 * letter          = 'a' | 'b' | ... | 'z' | 'A' | 'B' | ... | 'Z' ;
 * digit           = '0' | '1' | ... | '9' ;							(* true=1; false=0 *)
 * nondigit        = '_' | letter ;
 * identifier      = letter { nondigit | digit } ;
 * parameter       = [-] { digit }+ [ '.' { digit }+ ] ;				(* integer and float only *)
 * parameter_list  = parameter { ',' parameter } ;
 * call_statement  = identifier '(' [parameter_list] ')' ;				(* identifier has to be an existing behaviour-function *)
 * jump_statement  = 'jmp(' [-] { digit }+ ')' ;						(* jump to next label, backwards iff label < 0; e.g. jump(-3) jumps to last (3), { digit }+ < 128 *)
 * if_statement    = 'if(' parameter ')\n' [ program ] [ 'else()\n' ]	(* statements between if(X) and else() / fi() are executed, iff top of stack == X; *)
 * 					 [ program ] 'fi()' ;
 * for_statement   = 'for(' { digit } ')\n' [ program ] 'endf()' ;		(* for(4) \n X \n endf() executes X 4 times, { digit }+ < 256, for(0) := while(1) *)
 * stack_statement = ( 'psh(' parameter ')' ) | 'pop(' parameter ')' ;	(* pop(N) deletes the N top elements of stack and saves them as parameters, sizeof(parameter) <= 2 Byte *)
 * label_statement = 'lbl(' { digit }+ ')' ;							(* { digit }+ < 128 *)
 * comment		   = '// ' identifier '\n' ;							(* not longer than max instruction length (32 Byte)! *)
 * statement       = ( call_statement | jump_statement					(* not longer than max instruction length (32 Byte)! *)
 *                     | if_statement | for_statement
 * 					   | stack_statement | label_statement
 * 					   | comment ) ;
 * program         = { [ statement ] '\n' } ;							(* end of line terminates *every* statement (even the last one) *)
 * abl             = comment { program } ;								(* abl-script should begin with comment in first line *)
 *
 *
 *
 *** Logische Struktur des Interpreters (vereinfacht): ***
 * Die Wirklichkeit ist etwas komplizierter (z.B. Laden eines neuen Code-Segments innerhalb von
 * i_fetch(), falls aktuelle Instruktion die 512 Byte-Grenze ueberlappt).
 *
 *
 * -----------------------        -----------------------
 * |       bot_abl()     |   -->  |    load_program()   |
 * -----------------------        -----------------------
 *           v  <----------------------------------------------------------------------------|
 * -----------------------                                                                   |
 * | bot_abl_behaviour() |                                                                   |
 * -----------------------                                                                   |
 *           v                                                                               |
 * -----------------------        -----------------------        -----------------------     |
 * |      i_fetch()      |   -->  |      i_decode()     |   -->  |  parameter_parse()  |     |
 * -----------------------        -----------------------        -----------------------     |
 *                           vv                                                              |
 * -----------------------        -----------------------                                    |
 * |  keyword-handler()  |   ||   |   behaviour-call()  |                                    |
 * -----------------------        -----------------------                                    |
 *           v                                                                               |
 *           |-------------------------------------------------------------------------------|
 */


// avr-objcopy -I binary -O ihex --change-addresses 512 --set-start 0 test.txt abl.ihex
// avrdude -P net:192.168.1.30:10002 -c avr109 -p m1284p -U eeprom:w:"abl.ihex":i -u

#include "bot-logic.h"
#include "eeprom.h"

/**
 * Kleines Testprogramm, das im EEPROM-Abbild landet
 */
#define ABL_PROG { \
	"bot_turn(360)\n" \
}

#if defined __AVR_ATmega1284P__ || defined PC
char EEPROM abl_eeprom_data[3584] = ABL_PROG; /**< 3584 Byte grosser EEPROM-Bereich fuer ABL-Daten */
#elif defined MCU_ATMEGA644X // ATmega644(P)
char EEPROM abl_eeprom_data[1536] = ABL_PROG; /**< 1536 Byte grosser EEPROM-Bereich fuer ABL-Daten */
#else // ATmega32
char EEPROM abl_eeprom_data[512] = ABL_PROG; /**< 512 Byte grosser EEPROM-Bereich fuer ABL-Daten */
#endif // MCU-Typ

#ifdef BEHAVIOUR_ABL_AVAILABLE
#include "ui/available_screens.h"
#include "init.h"
#include "log.h"
#include "sdfat_fs.h"
#include "display.h"
#include "led.h"
#include "rc5-codes.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//#define DEBUG_ABL				/**< Schalter um recht viel Debug-Code anzumachen */
#define ERROR_CHECKS			/**< aktiviert syntaxbezogene Fehlerpruefungen */
//#define FLOAT_PARAMS			/**< aktiviert float als moeglichen Parametertyp (ca. +250 Bytes im Flash) */

#ifdef MCU
#undef DEBUG_ABL
//#undef ERROR_CHECKS
#endif // MCU

#ifndef DEBUG_ABL
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}	/**< LOG aus */
#endif

#define FOR_DEPTH	2						/**< Anzahl an max. ineinander verschachtelter for-Schleifen */

char * const abl_prg_data = (char *) GET_MMC_BUFFER(abl_buffer); /**< 512 Byte grosser Puffer des Programms */
char * p_abl_i_data = NULL;					/**< Zeiger auf Puffer des Programms */
static char * ip = NULL;					/**< Instruction Pointer des Interpreters */
static char * i_start = NULL;				/**< Adresse des Instruction-Caches fuer neue Daten (wichtig bei 512 Byte-Ueberlauf)*/
static char abl_i_cache[ABL_INSTRUCTION_LENGTH + 1]; /**< Instruction Cache, beinhaltet die aktuelle Instruktion */
remote_call_data_t abl_params[3];			/**< Parameter-Daten so wie die RemoteCalls sie erwarten (32 Bit aligned) */
uint16_t abl_stack[ABL_STACK_SIZE];			/**< Stack */
uint8_t abl_sp = ABL_STACK_SIZE - 1;		/**< Stackpointer */
static uint8_t if_state = 0;				/**< Zustandsspeicher fuer offene if/else-Bloecke */
static uint8_t for_state[FOR_DEPTH];		/**< Zustandsspeicher fuer for-Schleifen */
static uint8_t * pForState = for_state - 1;	/**< Zustandsspeicher fuer offene for-Schleifen */
#ifdef SDFAT_AVAILABLE
static pFatFile abl_file;					/**< ABL-Programmdatei */
static char last_file[ABL_PATHNAME_LENGTH + 1];	/**< letzte geladene Programmdatei */
#define ABL_FILE_NAME	"ablX.txt" 			/**< Name der Programmdateien, X wird durch 1 bis 9 ersetzt */
#define ABL_FILE_EXT	".txt"				/**< Dateinamenerweiterung (PROG_FILE_NAME muss hierauf enden) */
#else
static uint16_t addr = 0;					/**< Adresse (Offset) des aktuellen Instruktionsblocks (EEPROM) */
#endif // SDFAT_AVAILABLE

/** Instruktionstypen */
typedef enum {
	I_JUMP, I_IF, I_ELSE, I_FI, I_FOR, I_ENDF, I_PUSH, I_POP, I_LABEL, I_CALL, I_COMMENT, I_UNKNOWN
} PACKED instruction_t;

/* make Doxygen happy */
static const char jmp[];	/**< Schluesselwort fuer Spruenge */
static const char if_[];	/**< Schluesselwort fuer if() */
static const char else_[];	/**< Schluesselwort fuer else() */
static const char fi_[];	/**< Schluesselwort fuer if-Block Ende */
static const char for_[];	/**< Schluesselwort fuer for() */
static const char endf[];	/**< Schluesselwort fuer for-Block Ende */
static const char psh[];	/**< Schluesselwort fuer Stack-push */
static const char pop[];	/**< Schluesselwort fuer Stack-pop */
static const char lbl[];	/**< Schluesselwort fuer Sprungmarke */
static PGM_P const keywords[];	/**< Zeiger auf Schluesselwoerter im Flash */

static const char jmp[]		PROGMEM = "jmp";	/**< Schluesselwort fuer Spruenge */
static const char if_[]		PROGMEM = "if";		/**< Schluesselwort fuer if() */
static const char else_[]	PROGMEM = "else";	/**< Schluesselwort fuer else() */
static const char fi_[]		PROGMEM = "fi";		/**< Schluesselwort fuer if-Block Ende */
static const char for_[]	PROGMEM = "for";	/**< Schluesselwort fuer for() */
static const char endf[]	PROGMEM = "endf";	/**< Schluesselwort fuer for-Block Ende */
static const char psh[]		PROGMEM = "psh";	/**< Schluesselwort fuer Stack-push */
static const char pop[]		PROGMEM = "pop";	/**< Schluesselwort fuer Stack-pop */
static const char lbl[]		PROGMEM = "lbl";	/**< Schluesselwort fuer Sprungmarke */
static PGM_P p_keywords;						/**< Speicher zur Ablage eines Schluesselwortes im RAM */

/** Zeiger auf Schluesselwoerter im Flash */
static PGM_P const keywords[] PROGMEM = { jmp, if_, else_, fi_, for_, endf, psh, pop, lbl };

/** Handler fuer jump-Keyword */
static void jump_handler(void);

/** Handler fuer if-Keyword */
static void if_handler(void);

/** Handler fuer else-Keyword */
static void else_handler(void);

/** Handler fuer fi-Keyword */
static void fi_handler(void);

/** Handler fuer for-Keyword */
static void for_handler(void);

/** Handler fuer endf-Keyword */
static void endf_handler(void);

/** Handler fuer pop-Keyword */
static void pop_handler(void);

/** Array fuer Keyword-Handler */
static void (* keyword_handler[])(void) = {
	jump_handler, if_handler, else_handler, fi_handler, for_handler, endf_handler, abl_push, pop_handler
};

/**
 * Initialisiert den ABL-Interpreter
 * \return Fehlercode: 0, falls alles ok
 */
static int8_t init(void) {
#ifdef SDFAT_AVAILABLE
	if (! strlen(last_file)) {
		LOG_ERROR("ABL: Keine Datei gesetzt");
		return -1;
	}
	if (sdfat_open(last_file, &abl_file, SDFAT_O_READ)) {
		LOG_ERROR("ABL: Datei \"%s\" nicht vorhanden", last_file);
		return -1;
	}
	p_abl_i_data = abl_prg_data;
#else // EEPROM
	p_abl_i_data = abl_prg_data;
#endif // SDFAT_AVAILABLE
	return 0;
}

/**
 * Laedt einen Programmteil aus Datei / EEPROM
 * \param direction Richtung, in die gesprungen wird (-1 zurueck, 0 gar nicht, 1 vor)
 */
static void load_program(int8_t direction) {
#ifdef SDFAT_AVAILABLE
	if (direction > 0) {
		sdfat_seek(abl_file, SD_BLOCK_SIZE, SEEK_CUR);
	} else if (direction < 0) {
		sdfat_seek(abl_file, -SD_BLOCK_SIZE, SEEK_CUR);
	}
	const int16_t res = sdfat_read(abl_file, p_abl_i_data, SD_BLOCK_SIZE);
	if (res != SD_BLOCK_SIZE) {
		LOG_DEBUG("load_program(): sdfat_read() failed: %d", res);
		p_abl_i_data = NULL;
		return;
	}
	sdfat_seek(abl_file, -SD_BLOCK_SIZE, SEEK_CUR);
#else // EEPROM
#if defined __AVR_ATmega1284P__ || defined PC
	/* on ATmega1284P or PC we have 3584 Bytes EEPROM for ABL */
	if (direction > 0 && addr < 3584 - 512) {
		addr += 512;
	} else if (direction < 0 && addr >= 512) {
		addr -= 512;
	} else if (direction != 0) {
		LOG_ERROR("EEPROM Zugriff out of bounds, addr=%u", addr);
	}
#elif defined MCU_ATMEGA644X
	/* on ATmega644(P) we have 1536 Bytes EEPROM for ABL */
	if (direction > 0 && addr < 1536 - 512) {
		addr += 512;
	} else if (direction < 0 && addr >= 512) {
		addr -= 512;
	} else if (direction != 0) {
		LOG_ERROR("EEPROM Zugriff out of bounds, addr=%u", addr);
	}
#else
	/* on ATmega32 we have 512 Bytes EEPROM for ABL */
	(void) direction;
	if (addr > 0) {
		LOG_ERROR("EEPROM Zugriff out of bounds, addr=%u", addr);
	}
	addr = 0;
#endif // MCU Typ
	ctbot_eeprom_read_block(p_abl_i_data, &abl_eeprom_data[addr], 512);
#endif // SDFAT_AVAILABLE
}

/**
 * Konvertiert einen String in int/float Parameter.
 * float-Parameter koennen nur konvertiert werden, wenn FLOAT_PARAMS an ist!
 * \param *start Zeiger auf Stringanfang, ab dem geparsed werden soll
 * \param *end Zeiger auf Stringende, bis zu dem geparsed werden soll
 * \return Der ermittelte Parameter im RemoteCall-Format (32 Bit aligned)
 */
static remote_call_data_t parameter_parse(const char * start, const char * end) {
	(void) end;
	LOG_DEBUG("parameter_parse() entered");
	remote_call_data_t param;
	param.s16 = atoi(start);
#ifdef FLOAT_PARAMS
	/* check if parameter is float */
	const char * point = strchr(start, '.');
	if (point != NULL && point <= end) {
		param.fl32 = param.s16;
		int16_t tmp = atoi(++point);
		param.fl32 += (float) tmp / 10.0 / (float) (end - point);
	}
#endif // FLOAT_PARAMS

	LOG_DEBUG("parsed parameter (d) is: %d", param.s16);
	LOG_DEBUG("parsed parameter (x) is: 0x%x", param.u16);
#if defined PC && defined FLOAT_PARAMS
	LOG_DEBUG("parsed parameter (f) is: %f", param.fl32);
#endif
	return param;
}

/**
 * Dekodiert eine ABL-Instruktion
 * \return Der Instruktionstyp
 */
static instruction_t i_decode(void) {
	LOG_DEBUG("i_decode() entered");
	/* check for comment */
	if (abl_i_cache[0] == '/' || abl_i_cache[0] == '\0') {
		LOG_DEBUG("comment found: %s", abl_i_cache);
		return I_COMMENT;
	}
	/* search end of name / keyword */
	char * func_end = strchr(abl_i_cache, '(');
#ifdef ERROR_CHECKS
	if (func_end == NULL) {
		LOG_ERROR("syntax error, instruction was:");
		LOG_ERROR("%s", abl_i_cache);
		return I_UNKNOWN;
	}
#endif // ERROR_CHECKS
	*func_end = '\0';
	++func_end;
	/* find and parse all parameters between '(' and ')' */
	char * par_end = func_end; // start with first argument-char
	uint8_t i;
	for (i = 0; *par_end != ')'; ++i) {
		par_end = strchr(func_end, ',');
		if (par_end == NULL) {
			par_end = strchr(func_end, ')');
		}
#ifdef ERROR_CHECKS
		if (par_end == NULL) {
			LOG_ERROR("syntax error, instruction was:");
			LOG_ERROR("%s", abl_i_cache);
			return I_UNKNOWN;
		}
#endif // ERROR_CHECKS
		if (func_end != par_end) {
			/* convert parameter */
			LOG_DEBUG("%u. parameter found, trying to parse...", i + 1);
			abl_params[i] = parameter_parse(func_end, par_end - 1);
			func_end = par_end + 1;
		}
	}
	/* find meaning of actual keyword */
	uint8_t j;
	for (j = 0; j < sizeof(keywords) / sizeof(PGM_P); ++j) {
		memcpy_P(&p_keywords, &keywords[j], sizeof(PGM_P));
		if (strcmp_P(abl_i_cache, p_keywords) == 0) {
#ifdef PC
			LOG_DEBUG("%s decoded", keywords[j]);
#endif
			return j;
		}
	}
	/* no keyword found, so instruction must be a function call */
	LOG_DEBUG("function name decoded as \"%s\"", abl_i_cache);
	switch (i) {
	case 0:
		LOG_DEBUG("function call decoded: \"%s()\"", abl_i_cache);
		break;
	case 1:
		LOG_DEBUG("function call decoded: \"%s(%u)\"", abl_i_cache, abl_params[0].u32);
		break;
	case 2:
		LOG_DEBUG("function call decoded: \"%s(%u,%u)\"", abl_i_cache, abl_params[0].u32, abl_params[1].u32);
		break;
	case 3:
		LOG_DEBUG("function call decoded: \"%s(%u,%u,%u)\"", abl_i_cache, abl_params[0].u32, abl_params[1].u32, abl_params[2].u32);
		break;
	}
	return I_CALL;
}

/**
 * Laedt die naechste Instruktion in den Cache und dekodiert sie mit i_decode()
 * \return Der Instruktionstyp (wird durch Aufruf von i_decode() ermittelt und durchgereicht)
 */
static instruction_t i_fetch(void) {
	LOG_DEBUG("i_fetch() entered");
	/* check for valid data */
	if (p_abl_i_data == NULL) {
		ip = abl_prg_data;
		return I_UNKNOWN;
	}
	/* update ip, skip spaces and tabs */
	do {
		ip++;
	} while (ip <= &p_abl_i_data[510] /*&& ip >= p_abl_i_data*/ && (*ip == 0x20 || *ip == 0x9));
	LOG_DEBUG("ip=0x%04x i_start=0x%04x", ip - p_abl_i_data, i_start - abl_i_cache);
	/* ip becomes start of this instruction */
	const char * old_ip = ip;
	uint8_t len;
	uint8_t max_len = (uint8_t) ((ABL_INSTRUCTION_LENGTH + 1) - (size_t) i_start + (size_t) abl_i_cache);
	/* find end of this instruction (=end of line) */
	for (len = 0; len < max_len; ++len, ++ip) {
		if (*ip == '\n') {
			/* found, instruction complete */
			break;
		}
		/* --> monsters here - don't touch this code <-- */
			else if (ip >= &p_abl_i_data[511] || ip <= p_abl_i_data - 1) {
				/* actual instruction is not completely in this buffer :( */
				LOG_DEBUG("instruction overlaps code segment, loading next one...");
				/* save the part we already found */
				len = (uint8_t) (ip - old_ip + 1);
				/* skip left spaces and tabs in old segement */
				while (len > 0 && (*old_ip == 0x20 || *old_ip == 0x9)) {
					++old_ip;
					--len;
				}
				memcpy(i_start, old_ip, len);
				i_start[len] = '\0';
				LOG_DEBUG("first i-part is \"%s\"", i_start);
				/* swap-in new program-segment and adjust pointers */
				load_program(1);
				ip = p_abl_i_data;
				--ip;
				i_start = &abl_i_cache[len]; // don't overwrite already copied part of instruction!
				return i_fetch(); // complete instruction recursively
			}
		else if (*ip == '\0') {
			/* end of program reached! */
			return I_UNKNOWN;
		}
	}
	/* check length of found instruction */
	if (len >= max_len - 1) {
#ifdef ERROR_CHECKS
		LOG_ERROR("%s", old_ip);
		LOG_ERROR("instruction is longer than %u bytes, increase ABL_INSTRUCTION_LENGTH!", ABL_INSTRUCTION_LENGTH);
#endif
		return I_UNKNOWN;
	}
	/* copy instruction into i-cache */
	memcpy(i_start, old_ip, len);
	i_start[len] = '\0';
	LOG_DEBUG("fetched instruction \"%s\" is %u bytes long", abl_i_cache, &i_start[len] - abl_i_cache);
	i_start = abl_i_cache; // correct start of i-cache (if we came here recursively)
	/* decode this instruction */
	return i_decode();
}

/**
 * Laedt die vorherige Instruktion in den Cache
 * \return Der Instruktionstyp (wird durch Aufruf von i_fetch und i_decode() ermittelt und durchgereicht)
 */
static instruction_t i_fetch_back(void) {
	LOG_DEBUG("i_fetch_back() entered");
	/* go two instructions back... */
	uint8_t i;
	uint8_t n = 2;
	if (ip == p_abl_i_data) {
		/* program segment starts with newline, so we have to go three instructions back */
		n = 3;
	}
	for (i = 0; i < n; ++i) {
		do {
			--ip;
			if (ip == p_abl_i_data - 1) {
				/* begin of buffer reached, swap-in new program-segment */
				load_program(-1);
				if (p_abl_i_data == NULL) {
					return I_UNKNOWN;
				}
				ip = p_abl_i_data + 512; // start at end of buffer, because we're going backwards
			}
		} while (*ip != '\n');
	}
	/* ... and one forward via default i-fetch() routine */
	return i_fetch();
}

/**
 * Handler fuer if-Keyword
 */
static void if_handler(void) {
	LOG_DEBUG("if_handler() entered");
	/* create new if-state */
	if_state = (uint8_t) (if_state << 1);
	if (abl_stack[abl_sp] == abl_params[0].u16) {
		/* condition is true :) */
		if_state |= 1;
		LOG_DEBUG("if-block will be executed...");
	} else {
		/* condition is false :( */
		instruction_t sub_i;
		int8_t depth = 0;
		/* skip if-block, jump to "else" or "fi" */
		for (sub_i = 0; sub_i != I_ELSE || depth != 0;) {
			LOG_DEBUG("following instruction will be skipped");
			sub_i = i_fetch();	// our i-fetch routine does the trick
			if (sub_i == I_UNKNOWN) {
				return;	// something went wrong, cancel
			} else if (sub_i == I_IF) {
				++depth;
			} else if (sub_i == I_FI) {
				if (depth == 0) {
					break;
				} else {
					--depth;
				}
			}
		}
		if (sub_i == I_ELSE) {
			LOG_DEBUG("else-block will be executed...");
		}
	}
}

/**
 * Handler fuer else-Keyword
 */
static void else_handler(void) {
	LOG_DEBUG("else_handler() entered");
	/* check if if-block was taken */
	if ((if_state & 1) == 1) {
		instruction_t sub_i;
		int8_t depth = 0;
		/* skip else-block, jump to "fi" */
		for (sub_i = 0; ;) {
			LOG_DEBUG("following instruction will be skipped");
			sub_i = i_fetch();
			if (sub_i == I_UNKNOWN) {
				return; // something went wrong, cancel
			} else if (sub_i == I_IF) {
				++depth;
			} else if (sub_i == I_FI) {
				if (depth == 0) {
					break;  // we found our fi()
				} else {
					--depth;
				}
			}
		}
	}
}

/**
 * Handler fuer fi-Keyword
 */
static void fi_handler(void) {
	/* delete if-state */
	if_state >>= 1;
}

/**
 * Handler fuer for-Keyword
 */
static void for_handler(void) {
	LOG_DEBUG("for_handler() entered");
	++pForState;
#ifdef ERROR_CHECKS
	if (pForState >= for_state + FOR_DEPTH) {
		LOG_ERROR("to many nested for-loops! FOR_DEPTH = %u", FOR_DEPTH);
		LOG_ERROR("following loop will only be executed once!");
		return;
	}
#endif // ERROR_CHECKS
	*pForState = abl_params[0].u8;
	LOG_DEBUG("in for-loop... body will be executed %u times.", *pForState);
}

/**
 * Handler fuer endf-Keyword
 */
static void endf_handler(void) {
	LOG_DEBUG("endf_handler() entered");
#ifdef ERROR_CHECKS
	if (pForState >= for_state + FOR_DEPTH) {
		LOG_ERROR("to many nested for-loops! FOR_DEPTH = %u", FOR_DEPTH);
		LOG_ERROR("last loop was only executed once!");
		--pForState;
		return;
	}
#endif // ERROR_CHECKS
	/* decrement actual for-state */
	(*pForState)--;
	LOG_DEBUG("new for_state = %u", *pForState);
	if (*pForState == 0) {
		/* loop done */
		--pForState;
		LOG_DEBUG("for-loop terminated");
	} else {
		if (*pForState == 255) {
			*pForState = 0; // infinite loop
		}
		/* in loop */
		instruction_t sub_i;
		/* search accordingly for-keyword */
		uint8_t i, j = 1;
		for (i = 0; i < j; ++i) {
			LOG_DEBUG("junmping back to for(%u)", i);
			do {
				LOG_DEBUG("following instruction will be skipped");
				sub_i = i_fetch_back();
				if (sub_i == I_UNKNOWN) {
					return; // something went wrong, cancel
				}
				if (sub_i == I_ENDF) {
					j++; // skip inner loop
				}
			} while (sub_i != I_FOR);
		}
	}
}

/**
 * Handler fuer jump-Keyword
 */
static void jump_handler(void) {
	LOG_DEBUG("jump_handler() entered");
	LOG_DEBUG("jump to (%d)", abl_params[0].s8);
	/* get jump-target from parameter data */
	int8_t target = abl_params[0].s8;
	instruction_t sub_i;
	/* search accordingly label (sign of target == direction) */
	do {
		LOG_DEBUG("following instruction will be skipped");
		sub_i = target < 0 ? i_fetch_back() : i_fetch();
		if (sub_i == I_UNKNOWN) {
			return; // something went wrong, cancel
		}
	} while (sub_i != I_LABEL || abl_params[0].u8 != abs(target));
}

/**
 * Handler fuer pop-Keyword
 */
static void pop_handler(void) {
	LOG_DEBUG("pop_handler() entered");
	uint8_t i = abl_params[0].u8;
	uint8_t j = 0;
	for (; i > 0; --i) {
		LOG_DEBUG("pop \"0x%04x\" from stack", abl_stack[abl_sp]);
		abl_params[j++].u16 = abl_stack[abl_sp];
		/* update sp */
		--abl_sp;
		abl_sp = (uint8_t) (abl_sp & ABL_STACK_MASK);
	}
#ifdef DEBUG_ABL
	LOG_DEBUG("done. sp=0x%x", abl_sp);
	LOG_DEBUG("stack:");
	for (i = 0; i < ABL_STACK_SIZE; ++i) {
		LOG_DEBUG("[0x%02x]:\t0x%04x", abl_sp, abl_stack[abl_sp]);
		--abl_sp;
		abl_sp &= ABL_STACK_MASK;
	}
#endif // DEBUG_ABL
}

/**
 * Pusht den Parameter Nr. 0 auf den ABL-Stack
 * \see abl_push_value(uint16_t value)
 */
void abl_push(void) {
	LOG_DEBUG("abl_push() entered");
	LOG_DEBUG("push \"0x%04x\" on stack", abl_params[0].u16);
	/* update sp */
	++abl_sp;
	abl_sp = (uint8_t) (abl_sp & ABL_STACK_MASK);
	/* store parameter on stack */
	abl_stack[abl_sp] = abl_params[0].u16;
#ifdef DEBUG_ABL
	LOG_DEBUG("done. sp=0x%x", abl_sp);
	uint8_t i;
	LOG_DEBUG("stack:");
	for (i = 0; i < ABL_STACK_SIZE; ++i) {
		LOG_DEBUG("[0x%02x]:\t0x%04x", abl_sp, abl_stack[abl_sp]);
		--abl_sp;
		abl_sp &= ABL_STACK_MASK;
	}
#endif // DEBUG_ABL
}

/**
 * Der ABL-Interpreter als Verhalten
 * \param *data Der Verhaltensdatensatz
 */
void bot_abl_behaviour(Behaviour_t* data) {
	/* get next instruction */
	LOG_DEBUG("bot_abl_behaviour(): trying to fetch next instruction...");
	const instruction_t i_type = i_fetch();

	/* execute instruction */
	if (i_type <= I_POP) {
		keyword_handler[i_type]();
	} else if (i_type == I_CALL) {
		LOG_DEBUG("bot_abl_behaviour(): calling decoded function...");
		if (bot_remotecall(data, abl_i_cache, abl_params) != 0) {
#ifdef ERROR_CHECKS
			LOG_ERROR("RemoteCall %s not found", abl_i_cache);
#ifdef SDFAT_AVAILABLE
			sdfat_close(abl_file);
#endif
			return_from_behaviour(data);
			return;
#endif // ERROR_CHECKS
		}
	}

	/* check for errors */
	else if (i_type == I_UNKNOWN) {
		LOG_DEBUG("bot_abl_behaviour(): end of program reached! exit!");
#ifdef SDFAT_AVAILABLE
		sdfat_close(abl_file);
#endif
		return_from_behaviour(data);
		return;
	}
}

/**
 * Botenfunktion des ABL-Interpreters.
 * Laedt das erste Programm-Segment, initialisiert Pointer und startet das Verhalten.
 * Nicht per Remote-Call aufrufbar, da das Verhalten selbst Remote-Calls absetzt.
 * \param *caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * \param *filename	Programmdatei oder NULL, falls EEPROM / vorherige Programmdatei
 */
void bot_abl(Behaviour_t* caller, const char* filename) {
	/* Instructionpointer- & I-Cache-init */
#ifdef SDFAT_AVAILABLE
	if (abl_load(filename) != 0) {
		LOG_ERROR("bot_abl(): can't load file \"%s\" as ABL-programm", filename);
		return;
	}
	LOG_DEBUG("bot_abl(): using file \"%s\" as ABL-programm", last_file);
#else // EEPROM
	(void) filename;
	addr = 0;
	LOG_DEBUG("bot_abl(): using EEPROM as ABL-programm");
#endif // SDFAT_AVAILABLE
	if (init() != 0) {
		LOG_ERROR("bot_abl(): can't load file \"%s\" as ABL-programm", filename);
		return;
	}
	switch_to_behaviour(caller, bot_abl_behaviour, BEHAVIOUR_OVERRIDE);
	if_state = 0;
#if __clang__ != 1 && GCC_VERSION >= 60000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
	pForState = for_state - 1;
#if __clang__ != 1 && GCC_VERSION >= 60000
#pragma GCC diagnostic pop
#endif
	abl_sp = ABL_STACK_SIZE - 1;
	LOG_DEBUG("bot_abl(): loading first segment (512 bytes) of instructions");
	load_program(0); // i_fetch() checks for NULL
	ip = p_abl_i_data; // init instruction pointer
	--ip;
	i_start = abl_i_cache;
}

/**
 * Laedt ein ABL-Programm aus einer Datei
 * \param *filename Dateiname
 * \return Fehlercode: 0, falls alles ok
 */
int8_t abl_load(const char* filename) {
#ifdef SDFAT_AVAILABLE
	if (filename) {
#ifdef ERROR_CHECKS
		if (strlen(filename) > ABL_PATHNAME_LENGTH) {
			LOG_DEBUG("abl_load(): filename \"%s\" too long", filename);
			return -1;
		}
#endif // ERROR_CHECKS
		strncpy(last_file, filename, ABL_PATHNAME_LENGTH);
		LOG_DEBUG("abl_load(): using file \"%s\" as ABL-programm", last_file);
	}
#else
	(void) filename;
#endif // SDFAT_AVAILABLE

	return 0;
}

/**
 * Syntax-Check des aktuellen ABL-Programms per RemoteCall.
 * Liefert SUBSUCCESS, falls Syntax OK, sonst SUBFAIL.
 * \param *caller Zeiger auf Verhaltensdatensatz des Aufrufers
 * \param line Zeilennummer, bis zu der geprueft werden soll, 0 fuer alle
 */
void bot_abl_check(Behaviour_t * caller, uint16_t line) {
	uint8_t result = BEHAVIOUR_SUBFAIL;
#ifdef ERROR_CHECKS
	bot_abl(NULL, NULL); // do inits...
	deactivateBehaviour(bot_abl_behaviour); // ... but don't start behaviour
	instruction_t ins;
	uint16_t n = 0;
	if (line == 0) {
		--line; // set to maxint for all lines
	}
	do {
		ins = i_fetch();
		++n; // count lines
		if (ins == I_CALL) {
			/* check for valid behaviour-call */
			if (get_remotecall_id(abl_i_cache) == 255) {
				LOG_DEBUG("error, behaviour or keyword");
				LOG_DEBUG("\"%s\"", abl_i_cache);
				LOG_DEBUG("not available");
				break; // error :(
			}
		}
		if (n > line) {
			break; // end for check reached
		}
	} while (ins != I_UNKNOWN);

	if (*ip == '\0' || n > line) {
		/* end of program */
		LOG_DEBUG("end of program/check reached! exit!");
		result = BEHAVIOUR_SUBSUCCESS; // ok :)
	}
#endif // ERROR_CHECKS
	union {
		uint8_t byte;
		unsigned bits:3;
	} tmp = {result};
	caller->subResult = tmp.bits; // save check-result at caller's data
}

/**
 * Bricht ein laufendes ABL-Programm ab
 */
void abl_cancel(void) {
	Behaviour_t* const beh = get_behaviour(bot_abl_behaviour);
	deactivate_called_behaviours(beh);
	deactivate_behaviour(beh);
	/* evtl. hatte ABL einen RemoteCall gestartet, daher dort aufraeumen */
	activateBehaviour(NULL, bot_remotecall_behaviour);
}

#ifdef DISPLAY_ABL_STACK_AVAILABLE
/**
 * Displayhandler fuer ABL-Stack Ausgabe
 */
void abl_stack_trace(void) {
	/* walk through first 9 stack-entries and display each of them */
	uint8_t i, j;
	for (i = 1; i <= 20; i = (uint8_t) (i + 7)) { // col
		for (j = 1; j <= 3; ++j) { // row
			display_cursor(j, i);
			display_printf("%x:%04x ", abl_sp, abl_stack[abl_sp]);
			--abl_sp;
			abl_sp = (uint8_t) (abl_sp & ABL_STACK_MASK);
		}
	}
	abl_sp = (uint8_t) (abl_sp - (ABL_STACK_SIZE - 9));
	abl_sp = (uint8_t) (abl_sp & ABL_STACK_MASK);
	/* display last instruction */
	display_cursor(4, 1);
	abl_i_cache[ABL_INSTRUCTION_LENGTH] = '\0'; // keeps strlen and strchr in this array
	abl_i_cache[strlen(abl_i_cache)] = '(';
	char * const ptr = strchr(abl_i_cache, ')');
	if (ptr != NULL) {
		*(ptr + 1) = '\0';
		display_printf("%- 20s", abl_i_cache);
	}

#ifdef RC5_AVAILABLE
	/* Keyhandler */
	if (RC5_Code >= RC5_CODE_1 && RC5_Code <= RC5_CODE_9) {
#ifdef SDFAT_AVAILABLE
		/* Programm abl1.txt bis abl9.txt laden */
		const char key = (char) (RC5_Code - RC5_CODE_1 + '1');
		static char fname[] = ABL_FILE_NAME;
		const size_t num = strlen(ABL_FILE_NAME) - strlen(ABL_FILE_EXT) - 1;
		fname[num] = key;
		LOG_DEBUG("zu ladende Datei: \"%s\"", fname);

		bot_abl(NULL, fname);
#else // EEPROM
		bot_abl(NULL, NULL);
#endif // SDFAT_AVAILABLE

		RC5_Code = 0;
		return;
	}

	switch (RC5_Code) {
#ifdef RC5_CODE_PLAY
	case RC5_CODE_PLAY:
		bot_abl(NULL, NULL);
		RC5_Code = 0;
		break;
#endif // PLAY

#ifdef RC5_CODE_STOP
	case RC5_CODE_STOP:
		abl_cancel();
		RC5_Code = 0;
		break;
#endif // STOP
	}
#endif // RC5_AVAILABLE
}
#endif // DISPLAY_ABL_STACK_AVAILABLE
#endif // BEHAVIOUR_ABL_AVAILABLE
