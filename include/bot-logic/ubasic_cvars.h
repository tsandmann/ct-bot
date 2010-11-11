/*--------------------------------------------------------
 *    Implementierung Basic-Befehl "vpeek" und "vpoke"
 *    =====================================
 *     Uwe Berger (bergeruw@gmx.net); 2010
 *
 *
 * Have fun!
 * ---------
 *
 ----------------------------------------------------------*/

/**
 * \file 	ubasic_cvars.h
 * \brief 	Implementierung Basic-Befehl "vpoke" und "vpeek"
 * \author 	Uwe Berger (bergeruw@gmx.net)
 * \date 	10.06.2010
 */

#ifndef __UBASIC_CVARS_H__
#define __UBASIC_CVARS_H__

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
// Strukturdefinition fuer Variablenpointertabelle
typedef struct {
	const char * var_name;
	int16_t * pvar;
} PACKED cvars_t;

// exportierbare Prototypen
void ubasic_vpoke_statement(void); // setzen
int ubasic_vpeek_expression(void); // lesen

#endif // BEHAVIOUR_UBASIC_AVAILABLE
#endif // __UBASIC_CVARS_H__
