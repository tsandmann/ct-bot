/*--------------------------------------------------------
*    Implementierung Basic-Befehl "call()"
*    =====================================
*     Uwe Berger (bergeruw@gmx.net); 2010
*
*
*
* Have fun!
* ---------
*
----------------------------------------------------------*/

#include "bot-logic.h"
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "sensor.h"

#include "tokenizer_access.h"
#include "ubasic.h"
#include "tokenizer.h"
#include "ubasic_config.h"
#include "ubasic_cvars.h"


#if USE_AVR
//	#include "../uart/usart.h"
#else
	#include <string.h>
	#include <stdio.h>
#endif

#if UBASIC_CVARS

//--------------------------------------------

// Variablenpointertabelle
#if USE_PROGMEM
const cvars_t cvars[] PROGMEM = {
#else
cvars_t cvars[] = {
#endif
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
    {"", NULL}
};

static int search_cvars(const char *var_name) {
	int idx=0;
	// Variablenname in Tabelle suchen
#if USE_PROGMEM
	while((int *)pgm_read_word(&cvars[idx].pvar) != NULL &&
	      strncasecmp_P(var_name, cvars[idx].var_name, MAX_NAME_LEN)) {
    	idx++;
    }
#else
	while(cvars[idx].pvar != NULL &&
	      strncasecmp(cvars[idx].var_name, var_name, MAX_NAME_LEN)) {
    	idx++;
    }
#endif
    // keinen Tabelleneintrag gefunden!
#if USE_PROGMEM
    if ((int *)pgm_read_word(&cvars[idx].pvar) == NULL) {
#else
    if (cvars[idx].pvar == NULL) {
#endif
    	tokenizer_error_print(current_linenum, UNKNOWN_CVAR_NAME);
		ubasic_break();
    }
	return idx;
}

void vpoke_statement(void) {
	int idx=0;
#if USE_PROGMEM
	int *var_temp;
#endif

	accept(TOKENIZER_VPOKE);
    accept(TOKENIZER_LEFTPAREN);
	// Variablenname ermitteln
	if(tokenizer_token() == TOKENIZER_STRING) {
		tokenizer_next();
	}
	idx=search_cvars(tokenizer_last_string_ptr());
	accept(TOKENIZER_RIGHTPAREN);
	accept(TOKENIZER_EQ);
#if USE_PROGMEM
	var_temp=(int *)pgm_read_word(&cvars[idx].pvar);
	*var_temp=expr();
#else
	*cvars[idx].pvar = expr();
#endif
	//tokenizer_next();
}

int vpeek_expression(void) {
	int idx=0;
	int r=0;
#if USE_PROGMEM
	int16_t *var_temp;
#endif

	accept(TOKENIZER_VPEEK);
	// Parameterliste wird durch linke Klammer eingeleitet
    accept(TOKENIZER_LEFTPAREN);
	// Variablenname ermitteln
	if(tokenizer_token() == TOKENIZER_STRING) {
		tokenizer_next();
	}
	idx=search_cvars(tokenizer_last_string_ptr());
#if USE_PROGMEM
	var_temp=(int16_t *)pgm_read_word(&cvars[idx].pvar);
	r=*var_temp;
#else
	r = *cvars[idx].pvar;
#endif
    accept(TOKENIZER_RIGHTPAREN);
	return r;
}
#endif

#endif // BEHAVIOUR_UBASIC_AVAILABLE
