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
 * \file 	ubasic_call.h
 * \brief 	Implementierung Basic-Befehl "call()"
 * \author 	Uwe Berger (bergeruw@gmx.net)
 * \date 	10.06.2010
 */

#ifndef __UBASIC_CALL_H__
#define __UBASIC_CALL_H__

#ifdef BEHAVIOUR_UBASIC_AVAILABLE

// Funktionspointertypen
#define VOID_FUNC_VOID				0
#define VOID_FUNC_INT				1
#define INT_FUNC_INT				2
#define VOID_FUNC_INT2				3
#define VOID_FUNCT_BEHAVIOUR_INT 	4
#define VOID_FUNCT_BEHAVIOUR_INT3 	5
#define BOOL_FUNCT_BEHAVIOUR_ACTIVE 6
#define VOID_FUNCT_REMOTE_CALL 		7
#define INT_FUNCT_BEHAVIOUR_UINT 	8

// Strukturdefinition fuer Funktionspointertabelle
typedef struct {
	const char funct_name[11];
	union ftp {
		void (* VoidFuncVoid)(void);
		void (* VoidFuncInt)(int);
		void (* VoidFuncInt2)(int16_t, int16_t);
		int (* IntFuncInt)(int);
		void (* VoidFuncBehavInt)(Behaviour_t *, int16_t);
		int8_t (* IntFuncBehavDel)(Behaviour_t *, uint16_t);
		void (* VoidFuncBehavInt3)(Behaviour_t *, int8_t, int16_t, int16_t);
		uint8_t (* BoolFunctBehavActive)(Behaviour_t *);
		int8_t (* VoidFuncRemoteCall)(Behaviour_t * caller, const char * func, const remote_call_data_t * data);
	} funct_ptr;
	unsigned char typ;
} PACKED callfunct_t;

// exportierbare Prototypen
int ubasic_call_statement(void);

#endif // BEHAVIOUR_UBASIC_AVAILABLE
#endif // __UBASIC_CALL_H__
