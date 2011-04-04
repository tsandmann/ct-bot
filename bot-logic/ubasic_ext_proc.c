/*--------------------------------------------------------
*    Plattformabhaengige Zugriffsroutinen fuer externe
*                    Unterprogramme 
*    =================================================
*          Uwe Berger (bergeruw@gmx.net); 2010
* 
* Folgendes ist je nach Zugriffsmethode auf den Basic-Programm-Text 
* anzupassen:
*
* current_proc  --> Definition des aktuellen Programms; diese Variable
*                   muss auch initial fuer das erste Hauptprogramm ge-
*                   setzt werden
* 
* switch_proc() --> - Schliessen des derzeitigen Programms;
*                   - Oeffnen des Programms aus Parameter;
*                   - Setzen von PROG_PTR auf Anfang des Programms;
*                   - tokenizer_init() mit neuen Programm;
*                   - Setzen von current_proc auf Programm aus Parameter
* 
*
* ---------
* Have fun!
*
----------------------------------------------------------*/

#include "bot-logic/bot-logic.h"
#ifdef BEHAVIOUR_UBASIC_AVAILABLE

#include "bot-logic/ubasic_config.h"

#define __UBASIC_EXT_PROC_C__
	#include "bot-logic/tokenizer_access.h"
#undef __UBASIC_EXT_PROC_C__

#include "bot-logic/ubasic.h"
#include "bot-logic/tokenizer.h"
#include "bot-logic/ubasic_ext_proc.h"
#include <string.h>


#if USE_AVR
//	#include "../uart/usart.h"
#else
	#include <string.h>
	#include <stdio.h> 
#endif


#if UBASIC_EXT_PROC

extern PTR_TYPE program_ptr;

// aktueller Programmname
char current_proc[MAX_PROG_NAME_LEN];

//**********************************************************************
#if ACCESS_VIA_PGM
	#include "../ubasic_tests.h"
	// Umschalten des Programm-Kontextes
	void switch_proc(char *p_name) {
		unsigned char i=0;
		i=get_program_pgm_idx(p_name);	
		//Fehlerpruefung (Programm nicht vorhanden)
		if (i<0) {
			tokenizer_error_print(current_linenum, UNKNOWN_SUBPROC);
			ubasic_break();
		} else {
			PROG_PTR=(const char*)get_program_pgm_ptr(i);
			program_ptr=(const char*)get_program_pgm_ptr(i);
			tokenizer_init(program_ptr);
			strncpy(current_proc, p_name, MAX_PROG_NAME_LEN);
		}
	}
#endif

//**********************************************************************
#if ACCESS_VIA_SDCARD
	#include "sd_card/fat.h"
	#include "sd_card/my_fat.h"
	// Filediscriptor
	extern struct fat_file_struct* fd;
	extern struct fat_fs_struct* fs;
	extern struct fat_dir_struct* dd;
	// Umschalten des Programm-Kontextes
	void switch_proc(char *p_name) {
		// aktuelle Datei schliessen
		fat_close_file(fd);
		// neue Datei oeffnen
		fd = open_file_in_dir(fs, dd, p_name);
		if(fd) {
			// diverse Pointer und Tokenizer initialisieren
			PROG_PTR=0;
			program_ptr=0;
			tokenizer_init(program_ptr);
			strncpy(current_proc, p_name, MAX_PROG_NAME_LEN);
		} else {
			tokenizer_error_print(current_linenum, UNKNOWN_SUBPROC);
			ubasic_break();
		}
	}
#endif

//**********************************************************************
#if ACCESS_VIA_FILE
	// Filediscriptor
	extern FILE *f;
	// Umschalten des Programm-Kontextes
	void switch_proc(char *p_name) {
		fclose(f);
		f = fopen(p_name, "r");
		if (!f) {
			tokenizer_error_print(current_linenum, UNKNOWN_SUBPROC);
			ubasic_break();
		} else {
			PROG_PTR=0;
			program_ptr=0;
			tokenizer_init(program_ptr);
			strncpy(current_proc, p_name, MAX_PROG_NAME_LEN);
		}

	}
#endif

#if ACCESS_VIA_BOTFS
	/**
	 * Umschalten des Programm-Kontextes
	 * \param p_name Programmname
	 */
	void switch_proc(char * p_name) {
		botfs_file_descr_t new_prog;
		if (botfs_open(p_name, &new_prog, BOTFS_MODE_r, GET_MMC_BUFFER(ubasic_buffer)) != 0) {
			tokenizer_error_print(current_linenum, UNKNOWN_SUBPROC);
			ubasic_break();
		} else {
			bot_ubasic_load_file(p_name, &new_prog);
			program_ptr = 0;
			tokenizer_init(program_ptr);
		}
	}
#endif // ACCESS_VIA_BOTFS

#endif // UBASIC_EXT_PROC

#endif // BEHAVIOUR_UBASIC_AVAILABLE
