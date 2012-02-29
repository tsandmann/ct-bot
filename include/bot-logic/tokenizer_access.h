/*--------------------------------------------------------
*    Definition der Zugriffsroutinen auf Speichermedium
*    ==================================================
*          Uwe Berger (bergeruw@gmx.net); 2010
*
* Folgende Defines sind je nach Zugriffsmethode auf den Basic-Programm-
* Text anzupassen:
*
* PTR_TYPE                   : Type des Programm-Zeigers
*
* PROG_PTR                   : Variable des Programm-Zeigers
*
* GET_CONTENT_PROG_PTR       : Inhalt der Speicherzelle, auf die der
*                              Programm-Zeiger verweist
*
* SET_PROG_PTR_ABSOLUT(param): Programm-Zeiger auf den Wert von param
*                              setzen
*
* INCR_PROG_PTR              : Programm-Zeiger um eins weiter
*
* END_OF_PROG_TEXT           : Bedingung zur Erkennung des (phys.)
*                              Endes des Programmtextes
*
* Desweiteren sind teilweise weitere Deklarationen notwendig. Man sollte
* dabei aber mit Modifikationen dieser und der dazugehoerigen .c-Datei
* auskommen...!
*
*
* Have fun!
* ---------
*
----------------------------------------------------------*/

#ifndef __TOKENIZER_ACCESS_H__
#define __TOKENIZER_ACCESS_H__

#include "behaviour_ubasic_access.h"

// Basic-Quelltexte steht im Flash... (AVR)
#if ACCESS_VIA_PGM
	#define PTR_TYPE char const *
	#ifdef __TOKENIZER_C__
		//static PTR_TYPE ptr;
		PTR_TYPE ptr;
	#endif
	#ifdef __UBASIC_EXT_PROC_C__
		extern PTR_TYPE ptr;
	#endif
	#define PROG_PTR 					ptr
	#define GET_CONTENT_PROG_PTR		pgm_read_byte(ptr)
	#define SET_PROG_PTR_ABSOLUT(param)	(PROG_PTR = (param))
	#define INCR_PROG_PTR				++ptr
	#define END_OF_PROG_TEXT			GET_CONTENT_PROG_PTR == 0
#endif

// Basic-Quelltext steht in einer Datei auf SD-Card... (AVR)
#if ACCESS_VIA_SDCARD
	// Definitionen
	#define PTR_TYPE long int
	#ifdef __TOKENIZER_ACCESS_C__
		PTR_TYPE ptr;
	#endif
	#ifdef __TOKENIZER_C__
		extern PTR_TYPE ptr;
	#endif
	#ifdef __UBASIC_EXT_PROC_C__
		extern PTR_TYPE ptr;
	#endif
	#define PROG_PTR 					ptr
	#define GET_CONTENT_PROG_PTR		get_content()
	#define SET_PROG_PTR_ABSOLUT(param)	set_ptr(param)
	#define INCR_PROG_PTR				incr_ptr()
	#define END_OF_PROG_TEXT			is_eof()
	// Prototypen der Zugriffsroutinen
	char get_content(void);
	void set_ptr(PTR_TYPE offset);
	void incr_ptr(void);
	char is_eof(void);
#endif

// Basic-Quelltext steht in einer Datei... (Linux)
#if ACCESS_VIA_FILE
	// Definitionen
	#define PTR_TYPE long
	#ifdef __TOKENIZER_ACCESS_C__
		PTR_TYPE ptr;
	#endif
	#ifdef __TOKENIZER_C__
		extern PTR_TYPE ptr;
	#endif
	#ifdef __UBASIC_EXT_PROC_C__
		extern PTR_TYPE ptr;
	#endif
	#define PROG_PTR 					ptr
	#define GET_CONTENT_PROG_PTR		get_content()
	#define SET_PROG_PTR_ABSOLUT(param)	set_ptr(param)
	#define INCR_PROG_PTR				incr_ptr()
	#define END_OF_PROG_TEXT			is_eof()
	// Prototypen der Zugriffsroutinen
	char get_content(void);
	void set_ptr(long offset);
	void incr_ptr(void);
	char is_eof(void);
#endif

#endif /* __TOKENIZER_ACCESS_H__ */
