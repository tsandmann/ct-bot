/*--------------------------------------------------------
 *    Implementierung Basic-Befehl "vpoke" und "vpeek"
 *    =====================================
 *     Uwe Berger (bergeruw@gmx.net); 2010
 *
 *
 *
 * Have fun!
 * ---------
 *
 ----------------------------------------------------------*/

/**
 * \file 	ubasic_cvars.c
 * \brief 	Implementierung Basic-Befehl "vpoke" und "vpeek"
 * \author 	Uwe Berger (bergeruw@gmx.net)
 * \date 	10.06.2010
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "bot-logic/ubasic.h"
#include "bot-logic/ubasic_tokenizer.h"
#include "bot-logic/ubasic_config.h"
#include "bot-logic/ubasic_cvars.h"
#include "bot-logic/ubasic_call.h"
#include "log.h"
#include "sensor.h"

#if UBASIC_CVARS
#include <string.h>

//#define DEBUG_CVARS 1
#if DEBUG_CVARS
#define DEBUG_PRINTF(...)  LOG_DEBUG(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

//--------------------------------------------

// Variablenpointertabelle
static const cvars_t cvars[] PROGMEM = {
	{ "sensDistL", &sensDistL }, // Abstandssensoren
	{ "sensDistR", &sensDistR },
	{ "sensBorderL", &sensBorderL }, // Abgrundsensoren
	{ "sensBorderR", &sensBorderR },
	{ "sensLDRL", &sensLDRL }, // Lichtsensoren
	{ "sensLDRR", &sensLDRR },
	{ "sensLineL", &sensLineL }, // Liniensensoren
	{ "sensLineR", &sensLineR },
	{ "sensLineL", &sensLineL }, // Liniensensoren
	{ "sensLineR", &sensLineR },
	{ "sensDoor", (int16_t *) &sensDoor }, // Klappensensor
	{ "sensTrans", (int16_t *) &sensTrans }, // Transportfach
	{ "", NULL } };

/** ct-Bot Anpassung */
static int16_t * search_cvars(const char * var_name) {
	uint8_t idx = 0;
	// Variablenname in Tabelle suchen
	int16_t * ptr;
	while ((ptr = (int16_t *) pgm_read_word(&cvars[idx].pvar)) != NULL && strcasecmp_P(var_name, cvars[idx].var_name)) {
		idx++;
	}
	if (ptr == NULL) {
		/* keinen Tabelleneintrag gefunden! */
		tokenizer_error_print(current_linenum, UNKNOWN_CVAR_NAME);
		ubasic_break();
	}
	return ptr;
}

void ubasic_vpoke_statement(void) {
	static char var_name[MAX_NAME_LEN + 1];

	accept(TOKENIZER_VPOKE);
	accept(TOKENIZER_LEFTPAREN);
	// Funktionsname ermitteln
	if (tokenizer_token() == TOKENIZER_STRING) {
		tokenizer_string(var_name, sizeof(var_name));
		DEBUG_PRINTF("funct_name: %s", var_name);
		tokenizer_next();
	}
	/** ct-Bot Anpassung */
	int16_t * pvar = search_cvars(var_name);
	accept(TOKENIZER_RIGHTPAREN);
	accept(TOKENIZER_EQ);
	if (pvar != NULL) {
		*pvar = expr();
	}
	tokenizer_next();
}

int ubasic_vpeek_expression(void) {
	static char var_name[MAX_NAME_LEN + 1];
	int r = 0;

	accept(TOKENIZER_VPEEK);
	// Parameterliste wird durch linke Klammer eingeleitet
	accept(TOKENIZER_LEFTPAREN);
	// Funktionsname ermitteln
	if (tokenizer_token() == TOKENIZER_STRING) {
		tokenizer_string(var_name, sizeof(var_name));
		DEBUG_PRINTF("funct_name: %s", var_name);
		tokenizer_next();
	}
	/** ct-Bot Anpassung */
	int16_t * pvar = search_cvars(var_name);
	if (pvar != NULL) {
		r = *pvar;
	}
	accept(TOKENIZER_RIGHTPAREN);
	return r;
}
#endif // UBASIC_CVARS
#endif // BEHAVIOUR_UBASIC_AVAILABLE
