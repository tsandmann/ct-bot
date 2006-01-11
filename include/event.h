/*! @file 	event.h
 * @brief 	Routinen für die Zeitschaltuhr
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#include "ct-Bot.h"

#define EVENT_SPECIAL  1		///< Besonderes Ereignis mit eigener Behandlungsroutine
#define EVENT_GETTIME	0x0		///< Ereignis: Hole Zeit vom HTTP-Server
			
/*! 
 * Aktionstyp
 */
typedef struct {
   unsigned char channel:7;   ///< 7 Bit f�r die Kanalmaske
   unsigned char special:1;   ///< 0 ==> Relais-Event 1 ==> anderes Event 
   unsigned char data:8; 	  ///< 1 Byte f�r die Daten
} action_t;

/*! 
 * Ereignis
 * wird ein Feld nicht benutzt, steht es auf DONTCARE
 */
typedef struct {
	char second;		///< Zeitpunkt Sekunde
	char minute;		///< Zeitpunkt Minute
	char hour;			///< Zeitpunkt Sekunde Stunde // 
	#ifdef RTC_FULL_AVAILABLE
		char day;		///< Zeitpunkt Tag
		char month;		///< Zeitpunkt Monat
		int year __attribute__ ((packed));	///< Zeitpunkt Jahr, Achtung das Jahr 255 bedeutet DONTCARE
	#endif
	char weekday;		///< Bitfeld für die Wochentage, Bit0 = Sonntag

	// Steht in channel der Wert EVENT_SPECIAL, steht in data ein EVENT_xxx-
	// Kommando, sonst geht es um die Relais. 
	// Da wir nur 7 Relais haben ist das kein Problem	

	action_t action;		///< Was soll geschehen?
} event_t;

extern event_t event;	///< Ein Event im RAM um damit zu arbeiten
extern char events;		///< Anzahl der Ereignisse

/*!
 * sichert das Arbeitsevent event an die Stelle des eventEEPROMPtr im EEPROM
 */
void event_ToEEPROM(unsigned char eventPtr);

/*!
 * lädt das Arbeitsevent event von der Stelle des eventEEPROMPtr des EEPROMs
 */
void event_FromEEPROM(unsigned char eventPtr);

/*!
 * Prüft, ob gerade Events zutreffen
 */
void event_cron(void	);

/*! 
 * Initialisiert die Ereignisse
 */ 
void event_init(void);

/*! 
 * Transmit one event via uart 
 */ 
void event_transmit(char ev);

/*!
 * Receive one event from uart 
 */ 
char event_receive(char ev);

/*! 
 * Delete Event ev from EEPROM 
 */
void event_delete(char ev);
