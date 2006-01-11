/*! @file 	rtc.c
 * @brief 	Real Time Clock
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include <stdlib.h>
#include <string.h>

#include "ct-Bot.h"
#include "timer.h"
#include "rtc.h"
#include "uart.h"
#include "delay.h"

#ifdef RTC_AVAILABLE

#ifndef UART_AVAILABLE	   // UART ist n�tig f�r eine HTTP-Anfrage nach der Zeit
	#undef RTC_HTTPTIME_AVAILABLE	
#endif

#define TIMEZONE	+1	// Germany ist in GMT+1

#define HTTP_TIMEOUT	5	// 5 Sekunden auf HTTP-Server warten 

volatile int rtc_msecond=0;		///< Zeit: Millisekunden
volatile char rtc_second=0;		///< Zeit: Sekunden
volatile char rtc_minute=0;		///< Zeit: Minuten
volatile char rtc_hour=0;		///< Zeit: Stunden

#ifdef RTC_FULL_AVAILABLE
	volatile char rtc_day;		///< Zeit: Tagen
	volatile char rtc_month;	///< Zeit: Monaten
	volatile int rtc_year;		///< Zeit: Jahren
#endif

volatile char rtc_weekday;	///> Zeit: Wochentag (1=Sonntag, 7= Samstag)

volatile char rtc_new_second=0;	///> Immer 1, wenn eine Sekunde um
					// um das r�cksetzen muss sich wer anders
					// k�mmern
					
/*!
 * Kommt zum Einsatz, wenn ein neuer Tag anbricht
 */
void rtc_newday(void){
	if (rtc_weekday == 7)
		rtc_weekday=1;
	else 
		rtc_weekday++;
	
	#ifdef RTC_FULL_AVAILABLE
		rtc_day++;
	
		if ((rtc_month== JAN) || (rtc_month== MRZ) || (rtc_month== MAI) ||
		    (rtc_month== JUL ) || (rtc_month== AUG ) || (rtc_month== OKT)||
		    (rtc_month== DEZ)){
			if (rtc_day>=32){	    // Monatswechsel
				rtc_day=1;
				rtc_month++;
			}
		} else if ((rtc_month== APR ) || (rtc_month== JUN ) ||
			   (rtc_month== SEP ) || (rtc_month== NOV)) { 
				if (rtc_day>=31){	   // Monatswechsel
					rtc_day=1;
					rtc_month++;
				}
			} else if (rtc_month==FEB) {	//!!!FIX-ME keine
							//Schaltjahresbetrachtung
				if (rtc_day>=29){	   // Monatswechsel
					rtc_day=1;
					rtc_month++;
				}
		 } 
			
		if (rtc_month==13){ // Jahreswechsel
			rtc_month=1;
			rtc_year++;
		}
	#endif
}

#ifdef RTC_FULL_AVAILABLE
/*!
 * Gibt 1 zurück, wenn Sommerzeit gilt
 */
char rtc_summertime(void){
	
	if( rtc_month < 3 || rtc_month > 10 )	// month 1, 2, 11, 12
		return 0;					// -> Winter
	
	// after last Sunday 2:00
	if( rtc_day - rtc_weekday >= 25 && (rtc_weekday || rtc_hour >= 2) ){ 
		if( rtc_month == 10 )		// October -> Winter
			return 0;
	}else{			// before last Sunday 2:00
		if( rtc_month == 3 )				// March -> Winter
		return 0;
	}
	
	return 1;
}

#endif

/*!
 *  RTC ISR wird einmal alle RTC_RESOLUTION ms aufgerufen
 */
void rtc_isr(void){
	rtc_msecond+= RTC_RESOLUTION;
	if (rtc_msecond == 1000){
		rtc_msecond=0;
		rtc_second++;
		rtc_new_second=1;
		if (rtc_second == 60){
			rtc_second=0;
			rtc_minute++;
			if (rtc_minute == 60){
				rtc_minute=0;
				rtc_hour++;
				if (rtc_hour == 24){
					rtc_hour=0;

					rtc_newday();//from here it's tricky
				}
			}
		}
	}
}

/*!
 * Stellt die Zeit ein
 * @param hour	Stunde
 * @param minute	Minute
 * @param second 	Sekunde
 * @param weekday	Tagf der Woche
 */
void rtc_set_time(char hour, char minute, char second, char weekday){
	cli();
	rtc_msecond=0;
	rtc_second=second;
	rtc_minute=minute;
	rtc_hour=hour;
	rtc_weekday=weekday;
	sei();
}

#ifdef RTC_FULL_AVAILABLE

/*!
 * Stellt das Datum ein
 * @param day	Tag
 * @param month	Monat
 * @param year	Jahr
 */
void rtc_set_date(char day, char month, int year){
	cli();
	rtc_day=day;
	rtc_month=month;
	rtc_year=year;
	sei();
}
#endif


#ifdef RTC_HTTPTIME_AVAILABLE
/*!
 *  Liest die Zeit von einem HTTP-Server
 * Damit das korrekt funktioniert, muss der XPORT bei eingehenden Daten eine 
 * "Active-Connection" zu einem HTTP-Server (Port 80) aufbauen
 * Da uart_read() blockierend ist, kann sich die Funktion unter Umst�nden
 * aufh�ngen. 
 * Erreicht sie gar keinen HTTP-Server (timeout) liefert sie 0 zur�ck und l�sst
 * die rtc-zeit unver�ndert
*/ 
char rtc_get_httptime(void){   //123456789 123456789 123456789 123
	char str[31]="HEAD /index.html HTTP/1.1\n\n";
	unsigned char success=0;
	for (success=0; success<30; success++){
		uart_send(str[success]);
	}
	
	//Wait for data
	success= rtc_second;	str[0]=0;
	while ((uart_data_available()!=1)&(str[0]< HTTP_TIMEOUT)){
		if (rtc_second != success){
			str[0]++;
			success= rtc_second;
		}
	};
	
	success=0;
	
	// Abbruch, wenn timeout erreicht
	if (str[0]== HTTP_TIMEOUT)	
		return 0;
	
	uart_timeout=0;
	while (uart_timeout==0){
		// Search for possible start of Date-Field			
		// go on, if Date field found
		
		if (uart_read() != 'D') continue;
		if (uart_read() != 'a') continue;
		if (uart_read() != 't') continue;
		if (uart_read() != 'e') continue;
		if (uart_read() != ':') continue;
		if (uart_read() != ' ') continue;
		
		//Extract Day of Week Field
		str[0]=uart_read(); str[1]=uart_read(); str[2]=uart_read();
		str[3]=0;
		if (strcmp(str,"Mon") ==0)		rtc_weekday= 1;
		else if (strcmp(str,"Tue") ==0)	rtc_weekday= 2;
		else if (strcmp(str,"Wed") ==0)	rtc_weekday= 3;
		else if (strcmp(str,"Thu") ==0)	rtc_weekday= 4;
		else if (strcmp(str,"Fri") ==0)	rtc_weekday= 5;
		else if (strcmp(str,"Sat") ==0)	rtc_weekday= 6;
		else if (strcmp(str,"Sun") ==0)	rtc_weekday= 7;
		else continue;
		
		#ifdef RTC_FULL_AVAILABLE
			uart_read();	uart_read();	// catch ", "	
			
			// Extract Day
			str[0]=uart_read(); str[1]=uart_read(); str[2]=0;
			rtc_day=atoi(str);
		
			uart_read();	// catch " "
			
			// Extract Month
			str[0]=uart_read(); str[1]=uart_read(); str[2]=uart_read();
			str[3]=0;
			if (strcmp(str,"Jan") ==0)		rtc_month= 1;
			else if (strcmp(str,"Feb") ==0)	rtc_month= 2;
			else if (strcmp(str,"Mar") ==0)	rtc_month= 3;
			else if (strcmp(str,"Apr") ==0)	rtc_month= 4;
			else if (strcmp(str,"May") ==0)	rtc_month= 5;
			else if (strcmp(str,"Jun") ==0)	rtc_month= 6;
			else if (strcmp(str,"Jul") ==0)	rtc_month= 7;
			else if (strcmp(str,"Aug") ==0)	rtc_month= 8;
			else if (strcmp(str,"Sep") ==0)	rtc_month= 9;
			else if (strcmp(str,"Oct") ==0)	rtc_month= 10;
			else if (strcmp(str,"Nov") ==0)	rtc_month= 11;
			else if (strcmp(str,"Dez") ==0)	rtc_month= 12;
			else continue;
			
			uart_read();	// catch " "
	
			// Extract Year
			str[0]=uart_read(); str[1]=uart_read(); 
			str[2]=uart_read(); str[3]=uart_read();
			str[4]=0;
			rtc_year=atoi(str);		
			
			uart_read();	// catch " "
		#else
			for (success=0; success<13; success++)
				uart_read();
			success=0;
		#endif
			
		// Extract Hour
		str[0]=uart_read(); str[1]=uart_read(); str[2]=0;
		rtc_hour=atoi(str)+ TIMEZONE;
		
		if (rtc_hour > 23){	// TODO: check westerly timezones
			#if TIMEZONE <0
				#error TIMEZONES westerly of Greenwich are not implemented!!!
			#else 
				rtc_hour-=24;
				rtc_newday();
			#endif
		}
		#ifdef RTC_FULL_AVAILABLE
			rtc_hour+=rtc_summertime();
			if (rtc_hour > 23){
				rtc_hour-=24;
				rtc_newday();
			}
		#endif
		
		uart_read();	// catch " "
			
		// Extract Minute
		str[0]=uart_read(); str[1]=uart_read(); str[2]=0;
		rtc_minute=atoi(str);
		
		uart_read();	// catch " "
			
		// Extract Second
		str[0]=uart_read(); str[1]=uart_read(); str[2]=0;
		rtc_second=atoi(str);
				
		if (uart_timeout==0){
			success=1;
			break;
		}
	}
		
	delay(1000);	// Wait a second
	
	// remove everything else from Queue
	while (uart_data_available()==1)
		uart_read();
	
	return success;
}
#endif
#endif
