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
#ifndef __UBASIC_CVARS_H__
#define __UBASIC_CVARS_H__


// Strukturdefinition fuer Variablenpointertabelle
typedef struct {
#if USE_PROGMEM
	char var_name[MAX_NAME_LEN+1];
#else	
	char *var_name;
#endif
	int16_t *pvar;
} cvars_t;


// exportierbare Prototypen
void vpoke_statement(void);		//setzen
int vpeek_expression(void);		//lesen

#endif /* __UBASIC_CVARS_H__ */
