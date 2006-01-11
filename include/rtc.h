/*! @file 	rtc.h
 * @brief 	Real Time Clock
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#include "timer.h"

#define JAN	1		///< Januar
#define FEB	2		///< Februar
#define	MRZ	3		///< März
#define APR	4		///< April
#define MAI	5		///< Mai
#define JUN	6		///< Juni
#define JUL	7		///< Juli
#define AUG	8		///< August
#define SEP	9		///< Septemper
#define OKT	10		///< Oktober
#define NOV	11		///< November
#define	DEZ	12		///< Dezember

#define RTC_RESOLUTION 1000/TIMER_0_CLOCK			///< Auflösung der RTC

extern volatile int rtc_msecond;		///< Millisekunden
extern volatile char rtc_second;		///< Sekunden
extern volatile char rtc_minute;		///< Minuten
extern volatile char rtc_hour;			///< Stunden
extern volatile char rtc_day;			///< Tage
extern volatile char rtc_month;			///< Monate
extern volatile int rtc_year;			///< Jahre
extern volatile char rtc_weekday;		///< Wochentage (1=Sonntag, 7= Samstag)

extern volatile char rtc_new_second;	///< Immer 1, wenn eine Sekunde um
					// um das r�cksetzen muss sich wer anders
					// k�mmern


/*!
 *  RTC ISR wird einmal alle RTC_RESOLUTION ms aufgerufen
 */
void rtc_isr(void);

/*!
 * Stellt die Zeit ein
 * @param hour	Stunde
 * @param minute	Minute
 * @param second 	Sekunde
 * @param weekday	Tagf der Woche
 */
void rtc_set_time(char hour, char minute, char second, char weekday);

/*!
 * Stellt das Datum ein
 * @param day	Tag
 * @param month	Monat
 * @param year	Jahr
 */
void rtc_set_date(char day, char month, int year);

/*!
 *  Liest die Zeit von einem HTTP-Server
 * Damit das korrekt funktioniert, muss der XPORT bei eingehenden Daten eine 
 * "Active-Connection" zu einem HTTP-Server (Port 80) aufbauen
 * Da uart_read() blockierend ist, kann sich die Funktion unter Umst�nden
 * aufh�ngen. 
 * Erreicht sie gar keinen HTTP-Server (timeout) liefert sie 0 zur�ck und l�sst
 * die rtc-zeit unver�ndert
*/ 
char rtc_get_httptime(void);
