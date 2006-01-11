/*! @file 	event.c
 * @brief 	Routinen für die Zeitschaltuhr
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#ifdef MCU
#ifdef EVENT_AVAILABLE

#include "ct-Bot.h"

#include "event.h"
#include <avr/eeprom.h>
#include "rtc.h"
#include "uart.h"


#define DONTCARE	255		///< Ignoriere ein Feld
#define MAXEVENT 	20		///< Maximale Anzahl der Ecents

#ifdef RTC_FULL_AVAILABLE
	/*! Ein erstes Event, um 00:00:10; Tag, Monat, Jahr, Wochentag egal */
	#define GETTIMEEVENT {{10, 0,0,  DONTCARE, DONTCARE,DONTCARE,DONTCARE,{0,EVENT_SPECIAL,EVENT_GETTIME}}}
#else
	/*! Ein erstes Event, um 00:00:10; Wochentag egal */
	#define GETTIMEEVENT {{10, 0,0,  DONTCARE,{EVENT_SPECIAL,0,EVENT_GETTIME}}}
#endif	

/*! Wieviele Events stehen im EEPROM */
char eventsInEEPROM  __attribute__ ((section (".eeprom")))=1;
char events=0;		///< Anzahl der Events

/*! Adresse im EEPROM ab der wir Events ablegen, initialisieren mit 1x gettime */
event_t eventEEPROM[MAXEVENT] __attribute__ ((section (".eeprom"))) = GETTIMEEVENT;
char eventPtr =0;	///< Zeiger auf die Events

event_t event;	///< Ein Event im RAM um damit zu arbeiten

/*!
 * sichert das Arbeitsevent event an die Stelle des eventEEPROMPtr im EEPROM
 */
void event_ToEEPROM(unsigned char eventPtr){
	if ((eventPtr <= events) & (eventPtr <MAXEVENT)){
		eeprom_write_block(&event,&eventEEPROM[eventPtr],sizeof(event_t));
		
		if (eventPtr == events){ //neues Event geschrieben
			events++;	  // event-Z�hler hoch	
			eeprom_write_byte(&eventsInEEPROM,events); //sichern
		}
	}
}

/*!
 * lädt das Arbeitsevent event von der Stelle des eventEEPROMPtr des EEPROMs
 */
void event_FromEEPROM(unsigned char eventPtr){
	if (eventPtr < events)
		eeprom_read_block(&event,&eventEEPROM[eventPtr],sizeof(event_t));
}

/*! 
 * Initialisiert die Ereignisse
 */ 
void event_init(void){
	events=eeprom_read_byte(&eventsInEEPROM);
	if (events > MAXEVENT){
		events=0;
	}
}

#ifdef RTC_FULL_AVAILABLE	 
	#define day_matches \
		 (((event.day    == rtc_day)    | (event.day    == DONTCARE)) & \
		 ((event.month  == rtc_month)  | (event.month  == DONTCARE)) & \
		 ((event.year   == rtc_year)   | (event.year   == DONTCARE)))
#else
	#define day_matches (1)
#endif


#define event_matches \
	(((event.second == rtc_second) | (event.second == DONTCARE)) & \
	 ((event.minute == rtc_minute) | (event.minute == DONTCARE)) & \
	 ((event.hour   == rtc_hour)   | (event.hour   == DONTCARE)) & \
	 ((event.weekday & (1 <<(rtc_weekday-1))) >0)  & day_matches)

/*!
 * Prüft, ob gerade Events zutreffen
 */
void event_cron(){
	int i;
	for (i=0; i<events; i++){
		event_FromEEPROM(i);
		if (event_matches){
			if (event.action.special == EVENT_SPECIAL){
				#ifdef RTC_HTTPTIME_AVAILABLE
					if (event.action.data== EVENT_GETTIME){
							rtc_get_httptime();
					}
				#endif
			}else
				Relais_set_masked(event.action.data, event.action.channel);
		}
	}
}

#ifdef UART_AVAILABLE	

/*! 
 * Transmit one event via uart 
 */ 
void event_transmit(char ev){
	char i;
	char * ptr = (char *) &event;

	event_FromEEPROM(ev);	
	for (i=0; i<sizeof(event_t); i++)
		uart_send(*ptr++);		
}

/*!
 * Receive one event from uart 
 */ 
char event_receive(char ev){
	char i;
	char * ptr = (char *) &event;
	
	uart_timeout=0;
	for (i=0; i<sizeof(event_t); i++)
		*ptr++=uart_read();

	if (uart_timeout==0){
		event_ToEEPROM(ev);
		return 1;
	} else 
		return 0;
}
#endif

/*! 
 * Delete Event ev from EEPROM 
 */
void event_delete(char ev){
	if (ev < events){
		while (ev < (events-1)){	// Beim letzten m�ssen 
			event_FromEEPROM(ev+1);	//wir nicht mehr schieben
			event_ToEEPROM(ev++);
		}
		events--;
	}
}

#endif
#endif

