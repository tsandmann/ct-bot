/*--------------------------------------------------------
*    Implementierung Basic-Befehl fuer AVR
*    =====================================
*     Uwe Berger (bergeruw@gmx.net); 2010
*
*
* Have fun!
* ---------
*
----------------------------------------------------------*/

#include <stdint.h>

#ifndef __UBASIC_AVR_H__
#define __UBASIC_AVR_H__

int16_t epeek_expression(void);
int16_t adc_expression(void);
int16_t pin_in_expression(void);
void dir_statement(void);
void epoke_statement(void);
void out_statement(void);
void wait_statement(void);



#endif /* __UBASIC_AVR_H__ */
