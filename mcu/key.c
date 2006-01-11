/*! @file 	key.h 
 * @brief 	Routinen zum Einlesen der Taster
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/
#ifdef MCU 

#include "ct-Bot.h"

#include <avr/io.h>
#include "key.h"


#ifdef KEY_AVAILABLE
#define KEY_ENTPRELL	10	///< Schritte zum entprellen

#define KEY_GELESEN	0x01	///< Bit darf nicht in KEY_ALL enthalten sein!!! 

volatile char key_tmp=0xFE;		///< Zwischenspeicher zum Entprellen
volatile char key_last=0xFE;	///< Speichert die zuletzt gedr�ckte Taste
				//Alle Hex Werte der Keys sind 1, wenn diese
				// nicht gedr�ckt sind (d.h. alles ist negiert
				// da Taster als Pull-Downs)
volatile char key_entprell=0;	///< Hilfsvariable

/*!
 * Kümert sich regelmässig um das auslesden der Tasten
 */
void key_isr(void){
	char old_DDRD= DDRD;	// Alte Einstellungen sichern
	char old_PORTD= PORTD;
	
	char key;
	
	DDRD  &= ~ KEY_ALL;	// All Key-Ports as Input	
	PORTD |=   KEY_ALL;	// Alle Pullups an.
	
	key = PIND & KEY_ALL;	// Einlesen und maskieren
	
	if (key != (key_last&KEY_ALL)){// �nderung seit dem letzten auslesen?	
		if (key == key_tmp){	// Zustand identisch mit letztem Aufruf?
			key_entprell++;		   // Entpreller hochz�hlen
			if (key_entprell == KEY_ENTPRELL){ // Genug entprellt?
				key_last = key;	   // Ergebnis sichern
			}
		} else {		// wenn �nderung im Wert
			key_entprell=0;	// Entpreller l�schen
		}
	} else 				// wenn keine Taste gedr�ckt
		key_entprell=0;		// Entpreller l�schen
		
	key_tmp=key;	// letzten Zustand sichern
	
	//Alten Zustand rekonstruieren	
	DDRD= (DDRD & ~KEY_ALL) | (old_DDRD &KEY_ALL);
	PORTD=(PORTD & ~KEY_ALL) | (old_PORTD &KEY_ALL);
}

/*!
 * Lies alle Tasten aus. 
 * @return Liefert die Hex Werte der Keys zurück, wenn diese
 * gedrückt sind (d.h. nichts negiert, obwohl Taster als Pull-Downs)
 */
char key_read(){
	if ((key_last & KEY_GELESEN) == 0){	// Tastendruck noch nicht gelesen
		key_last|=KEY_GELESEN;		// als gelesen markieren
		return (~key_last) & KEY_ALL;
	} else 
		return KEY_TIMEOUT;
}

#endif
#endif
