/*--------------------------------------------------------
*    Definition fuer Zugriff auf externe Unterprogramme
*    ==================================================
*           Uwe Berger (bergeruw@gmx.net); 2010
*
*
* Anpassungen erfolgen in ubasic_ext_proc.c
*
*
* ---------
* Have fun!
*
----------------------------------------------------------*/

#ifndef __UBASIC_EXT_PROC_H__
#define __UBASIC_EXT_PROC_H__

#if UBASIC_EXT_PROC
extern char current_proc[]; /**< aktueller Programmname */

// Prototypen
void switch_proc(char *);

#endif // UBASIC_EXT_PROC

#endif /* __UBASIC_EXT_PROC_H__ */
