/*--------------------------------------------------------
 *    Implementierung Basic-Befehl "call()"
 *    =====================================
 *     Uwe Berger (bergeruw@gmx.net); 2010
 *
 * Dokumentation call_referenz.txt...!
 *
 *
 * Have fun!
 * ---------
 *
 ----------------------------------------------------------*/

/**
 * \file 	ubasic_call.c
 * \brief 	Implementierung Basic-Befehl "call()"
 * \author 	Uwe Berger (bergeruw@gmx.net)
 * \date 	10.06.2010
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "bot-logic/ubasic.h"
#include "bot-logic/ubasic_tokenizer.h"
#include "bot-logic/ubasic_config.h"
#include "bot-logic/ubasic_call.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

#if UBASIC_CALL

//#define DEBUG 1
#if DEBUG
#define DEBUG_PRINTF(...)  LOG_DEBUG(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

/**
 * Rueckgabe ob das zuletzt aufgerufene Verhalten noch aktiv ist oder nicht; festgestellt anhand der Verhaltens-Data-Struktur des ubasic-Verhaltens
 * \param *behaviour	Zeiger auf Verhaltensdatensatz zum abzufragenden Verhalten
 * \return 				1 wenn das zuletzt aufgerufene Verhalten noch laeuft; 0 wenn es nicht mehr laeuft (Achtung: wait ist auch ein Verhalten)
 */
static uint8_t behaviour_is_active(Behaviour_t * behaviour) {
	return (uint8_t) (behaviour->caller != NULL);
}

// Funktionspointertabelle
/** \todo: ins Flash */
static callfunct_t callfunct[] = {
	{ "bot_speed", .funct_ptr.VoidFuncInt2 = bot_ubasic_speed, VOID_FUNC_INT2 },
	{ "beh_active", .funct_ptr.BoolFunctBehavActive = behaviour_is_active, BOOL_FUNCT_BEHAVIOUR_ACTIVE },
//	{ "bot_delay", .funct_ptr.IntFuncBehavDel = bot_delay_ticks, INT_FUNCT_BEHAVIOUR_UINT },
	{ "RC", .funct_ptr.VoidFuncRemoteCall = bot_remotecall, VOID_FUNCT_REMOTE_CALL },
	{ NULL, { NULL }, 255 }
};

int ubasic_call_statement(void) {
	static char funct_name[MAX_NAME_LEN + 1];
	uint8_t idx = 0;
	int r = 0;

	accept(TOKENIZER_CALL);
	// Parameterliste wird durch linke Klammer eingeleitet
	accept(TOKENIZER_LEFTPAREN);
	// Funktionsname ermitteln
	if (tokenizer_token() == TOKENIZER_STRING) {
		tokenizer_string(funct_name, sizeof(funct_name) - 1);
		DEBUG_PRINTF("funct_name: %s", funct_name);
		tokenizer_next();
	}
	// Funktionsname in Tabelle suchen
	/** ct-Bot Anpassung (Bugfix) */
	while (callfunct[idx].funct_name != NULL && strcasecmp(callfunct[idx].funct_name, funct_name)) {
		idx++;
	}
	if (callfunct[idx].funct_name == NULL) {
		/* keinen Tabelleneintrag gefunden! */
		DEBUG_PRINTF("funct_name: %s nicht gefunden!", funct_name);
		tokenizer_error_print(current_linenum, UNKNOWN_CALL_FUNCT);
		ubasic_break();
	} else {
		DEBUG_PRINTF("funct_name: %s hat Index: %i", funct_name, idx);
		// je nach Funktionstyp (3.Spalte in Funktionspointertabelle) 
		// Parameterliste aufbauen und Funktion aufrufen
		switch (callfunct[idx].typ) {
		case VOID_FUNC_INT2: {
			// zwei Integer und kein Rueckgabewert
			accept(TOKENIZER_COMMA);
			const int16_t p1 = expr();
			accept(TOKENIZER_COMMA);
			const int16_t p2 = expr();
			callfunct[idx].funct_ptr.VoidFuncInt2(p1, p2);
			break;
		}

//		case INT_FUNC_INT: {
//			// ein Integer und Rueckgabewert
//			accept(TOKENIZER_COMMA);
//			const int16_t p1 = expr();
//			r = callfunct[idx].funct_ptr.IntFuncInt(p1);
//			break;
//		}

//		case INT_FUNCT_BEHAVIOUR_UINT: {
//			// ein Integer und Verhaltensdatensatz sowie Rueckgabewert
//			accept(TOKENIZER_COMMA);
//			const uint16_t p1 = (uint16_t) expr();
//			r = callfunct[idx].funct_ptr.IntFuncBehavDel(ubasic_behaviour_data, p1);
//			break;
//		}

//		case VOID_FUNCT_BEHAVIOUR_INT: {
//			// ein Integer und Rueckgabewert sowie Verhaltensdatensatz
//			accept(TOKENIZER_COMMA);
//			const int16_t p1 = expr();
//			callfunct[idx].funct_ptr.VoidFuncBehavInt(ubasic_behaviour_data, p1);
//			break;
//		}

//		case VOID_FUNCT_BEHAVIOUR_INT3: {
//			// 3 Integer sowie Verhaltensdatensatz
//			accept(TOKENIZER_COMMA);
//			const int8_t p1 = (int8_t) expr();
//			accept(TOKENIZER_COMMA);
//			const int16_t p2 = expr();
//			accept(TOKENIZER_COMMA);
//			const int16_t p3 = expr();
//			callfunct[idx].funct_ptr.VoidFuncBehavInt3(ubasic_behaviour_data, p1, p2, p3);
//			break;
//		}

		case BOOL_FUNCT_BEHAVIOUR_ACTIVE: {
			// ein Integer und Rueckgabewert
			r = callfunct[idx].funct_ptr.BoolFunctBehavActive(ubasic_behaviour_data);
			break;
		}

		case VOID_FUNCT_REMOTE_CALL: {
			// Remotecall
			accept(TOKENIZER_COMMA);
			char func[REMOTE_CALL_FUNCTION_NAME_LEN + 1];
			tokenizer_string(func, sizeof(func) - 1);
			DEBUG_PRINTF("func=\"%s\"", func);
			accept(TOKENIZER_STRING);
			remote_call_data_t params[REMOTE_CALL_MAX_PARAM] = { {0} };
			uint8_t i;
			for (i = 0; i < REMOTE_CALL_MAX_PARAM; ++i) {
				if (tokenizer_token() == TOKENIZER_RIGHTPAREN) {
					break;
				}
				accept(TOKENIZER_COMMA);
				params[i].s16 = expr();
				DEBUG_PRINTF("p%u=%d", i + 1, params[i].s16);
			}
			callfunct[idx].funct_ptr.VoidFuncRemoteCall(ubasic_behaviour_data, func, params);
			break;
		}

		default:
			DEBUG_PRINTF("Funktionspointertyp %i nicht gefunden", callfunct[idx].typ);
			tokenizer_error_print(current_linenum, UNKNOWN_CALL_FUNCT_TYP);
			ubasic_break();
		}
	}
	// abschliessende rechte Klammer
	accept(TOKENIZER_RIGHTPAREN);
	// bei Funktionspointertypen ohne Rueckgabewert ein Token weiterlesen...
	if ((callfunct[idx].typ == VOID_FUNC_VOID) || (callfunct[idx].typ == VOID_FUNC_INT) || (callfunct[idx].typ == VOID_FUNC_INT2)
		/*|| (callfunct[idx].typ == VOID_FUNCT_BEHAVIOUR_INT) || (callfunct[idx].typ == VOID_FUNCT_BEHAVIOUR_INT3) */
		|| (callfunct[idx].typ == VOID_FUNCT_REMOTE_CALL)) {
		tokenizer_next();
	}
	return r;
}

#endif // UBASIC_CALL
#endif // BEHAVIOUR_UBASIC_AVAILABLE
