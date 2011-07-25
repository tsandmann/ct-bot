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

#ifdef BEHAVIOUR_UBASIC_AVAILABLE

#define ACCESS_VIA_BOTFS 1

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

/* Basic-Programm wird ueber ct-Bot Framework geladen */
#if ACCESS_VIA_BOTFS
#include "botfs.h"
#include "init.h"

typedef uint16_t PTR_TYPE;
#define PROG_PTR 					ubasic_get_ptr()
#define GET_CONTENT_PROG_PTR		ubasic_get_content()
#define SET_PROG_PTR_ABSOLUT(param)	ubasic_set_ptr(param)
#define INCR_PROG_PTR				ubasic_incr_ptr()
#define END_OF_PROG_TEXT			ubasic_is_eof()

/**
 * \return Offset des aktuellen Program-Pointers in der Datei
 */
static inline PTR_TYPE ubasic_get_ptr(void) {
	return ubasic_ptr;
}

/**
 * \return Das Zeichen, auf das get_ptr() zeigt
 */
static inline char ubasic_get_content(void) {
	return ubasic_content;
}

/**
 * Hilfsfunktion fuer set_ptr() und incr_ptr()
 * \param offset neuer Wert fuer ubasic_ptr
 */
static inline void _ubasic_update_ptr(uint16_t offset) __attribute__((always_inline));
static inline void _ubasic_update_ptr(uint16_t offset) {
	const uint16_t last_block = ubasic_ptr / BOTFS_BLOCK_SIZE;
	ubasic_ptr = offset;
	const uint16_t block = ubasic_ptr / BOTFS_BLOCK_SIZE;
	const uint16_t index = ubasic_ptr % BOTFS_BLOCK_SIZE;

	if (block != last_block) {
		botfs_seek(&ubasic_prog_file, (int16_t) block, SEEK_SET);
		botfs_read(&ubasic_prog_file, GET_MMC_BUFFER(ubasic_buffer));
	}

	ubasic_content = (char) GET_MMC_BUFFER(ubasic_buffer)[index];
}

/**
 * Setzt den Program-Pointer auf einen neuen Wert
 * \param offset Neuer Wert (Offset innerhalb der Datei)
 */
static inline void ubasic_set_ptr(uint16_t offset) {
	_ubasic_update_ptr(offset);
}

/**
 * Erhoeht den Wert des Program-Pointers um eins
 */
static inline void ubasic_incr_ptr(void) {
	_ubasic_update_ptr(ubasic_ptr + 1);
}

/**
 * \return True, falls Programmende erreicht
 */
static inline char ubasic_is_eof(void) {
	return ubasic_get_content() == 0;
}
#endif // ACCESS_VIA_BOTFS

#endif // BEHAVIOUR_UBASIC_AVAILABLE

#endif /* __TOKENIZER_ACCESS_H__ */
