/*! @file 	tools.c
 * @brief 	rtc.c
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifdef MCU
	#include <avr/io.h>
#endif

#include "ct-Bot.h"
#ifdef TOOLS_AVAILABLE

#include <stdlib.h>

/*!
 * Hilfsfunktion zur Anzeige von Hex-Zahlen
 */
void to_hex(char in, char out[3]){		
	out[0]= (in >> 4 ) + '0' ;
	if (out[0] > '9') 
		out[0]+=7;
	
	out[1]= (in & 0x0F) + '0' ;
	if (out[1] > '9') 
		out[1]+=7;
	out[2]=0x00;
}

/*!
 * Hilfsfunktion zur Anzeige von Int-Zahlen
 */
void int_to_str(int in, char out[6]){
	int tmp =in;
	if (in <0) out[0]='-'; else out[0]='0'; 
	tmp=abs(tmp);	
	out[1]= (tmp /10000)+'0';	tmp=tmp % 10000;		
	out[2]= (tmp / 1000)+'0'  ;  tmp=tmp % 1000;
	out[3]= (tmp / 100) +'0' ;   tmp=tmp % 100;
	out[4]= (tmp / 10)  +'0';    tmp=tmp % 10;
	out[5]= (tmp)       +'0';  
}
#endif
