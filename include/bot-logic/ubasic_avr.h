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
#ifndef __UBASIC_AVR_H__
#define __UBASIC_AVR_H__

int epeek_expression(void);
int adc_expression(void);
int pin_in_expression(void);
void dir_statement(void);
void epoke_statement(void);
void out_statement(void);
void wait_statement(void);



#endif /* __UBASIC_AVR_H__ */
