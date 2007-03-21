/*
 * c't-Bot
 * 
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your
 * option) any later version. 
 * This program is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 * 
 */

/*! 
 * @file 	log.c
 * @brief 	Routinen zum Loggen von Informationen. Es sollten ausschliesslich nur
 * die Log-Makros: LOG_DEBUG(), LOG_INFO(), LOG_WARN(), LOG_ERROR() und LOG_FATAL()
 * verwendet werden.
 * Eine Ausgabe kann wie folgt erzeugt werden:
 * LOG_DEBUG("Hallo Welt!");
 * LOG_INFO("Wert x=%d", x);
 * Bei den Ausgaben kann auf ein Line Feed '\n' am Ende des Strings verzichtet werden, 
 * da dies automatisch angehaengt hinzugefuegt wird.
 * 
 * <pre>
 * Die Logausgaben werden generell mit der Definition von LOG_AVAILABLE eingeschaltet
 * und sind ansonsten nicht aktiv.
 * 
 * Loggings auf dem PC:
 * --------------------
 * Hier stehen drei Arten der Ausgabeschnittstellen zur Verfuegung.
 * 1. Logging ueber ct-Sim:		LOG_CTSIM_AVAILABLE muss definiert sein.
 * 2. Logging ueber Display:	LOG_DISPLAY_AVAILABLE muss definiert sein, sowie DISPLAY_AVAILABLE.
 * 3. Logging ueber Konsole:  	Es muss LOG_STDOUT_AVAILABLE definiert sein.
 * 
 * LOG_UART_AVAILABLE steht auf dem PC nicht zur Verfuegung.
 * 
 * Loggings auf dem MCU:
 * ---------------------
 * Hier stehen drei Arten der Ausgabeschnittstellen zur Verfuegung.
 * 1. Logging ueber UART:		LOG_UART_AVAILABLE muss definiert sein.
 * 								Es darf BOT_2_PC_AVAILABLE nicht definiert sein, da ansonsten
 * 								diese Kommunikation ueber den UART laeuft.
 * 2. Logging ueber ct-Sim:		LOG_CTSIM_AVAILABLE muss definiert sein.
 * 								BOT_2_PC_AVAILABLE muss zusaetzlich definiert sein.
 * 3. Logging ueber Display:	LOG_DISPLAY_AVAILABLE muss definiert sein, sowie DISPLAY_AVAILABLE.
 * </pre>
 * 
 * @author 	Andreas Merkle (mail@blue-andi.de)
 * @date 	27.02.06
 */

#ifndef LOG_H_
#define LOG_H_

#include "ct-Bot.h"
#ifdef MCU
	#include <avr/pgmspace.h>
#else
	#define PROGMEM			// Alibideklaration hat keine Funktion, verhindert aber eine Warning
#endif

#ifdef LOG_AVAILABLE

/*!
 * Allgemeines Debugging (Methode DiesUndDas wurde mit Parameter SoUndSo 
 * aufgerufen ...)
 */
#define LOG_DEBUG(format, args...){	static const char file[] PROGMEM = __FILE__;	\
									log_begin(file, __LINE__, LOG_TYPE_DEBUG); 		\
									static const char data[] PROGMEM = format;		\
									log_printf(data, ## args);						\
									log_end();										\
}

/*!
 * Allgemeine Informationen (Programm gestartet, Programm beendet, Verbindung 
 * zu Host Foo aufgebaut, Verarbeitung dauerte SoUndSoviel Sekunden ...)
 */
#define LOG_INFO(format, args...){	static const char file[] PROGMEM = __FILE__;	\
									log_begin(file, __LINE__, LOG_TYPE_INFO); 		\
									static const char data[] PROGMEM = format;		\
									log_printf(data, ## args);						\
									log_end();										\
}

/*!
 * Auftreten einer unerwarteten Situation.
 */
#define LOG_WARN(format, args...){	static const char file[] PROGMEM = __FILE__;	\
									log_begin(file, __LINE__, LOG_TYPE_WARN); 		\
									static const char data[] PROGMEM = format;		\
									log_printf(data, ## args);						\
									log_end();										\
}

/*!
 * Fehler aufgetreten, Bearbeitung wurde alternativ fortgesetzt.
 */
#define LOG_ERROR(format, args...){	static const char file[] PROGMEM = __FILE__;	\
									log_begin(file, __LINE__, LOG_TYPE_ERROR); 		\
									static const char data[] PROGMEM = format;		\
									log_printf(data, ## args);						\
									log_end();										\
}

/*!
 * Kritischer Fehler, Programmabbruch.
 */
#define LOG_FATAL(format, args...){	static const char file[] PROGMEM = __FILE__;	\
									log_begin(file, __LINE__, LOG_TYPE_FATAL); 		\
									static const char data[] PROGMEM = format;		\
									log_printf(data, ## args);						\
									log_end();										\
}

#else

/*!
 * Allgemeines Debugging (Methode DiesUndDas wurde mit Parameter SoUndSo 
 * aufgerufen ...)
 */
#define LOG_DEBUG(format, args...)

/*!
 * Allgemeine Informationen (Programm gestartet, Programm beendet, Verbindung 
 * zu Host Foo aufgebaut, Verarbeitung dauerte SoUndSoviel Sekunden ...)
 */
#define LOG_INFO(format, args...)

/*!
 * Auftreten einer unerwarteten Situation.
 */
#define LOG_WARN(format, args...)

/*!
 * Fehler aufgetreten, Bearbeitung wurde alternativ fortgesetzt.
 */
#define LOG_ERROR(format, args...)

/*!
 * Kritischer Fehler, Programmabbruch.
 */
#define LOG_FATAL(format, args...)

#endif	/* LOG_AVAILABLE */

/*! Dieser Typ definiert die Typen der Log-Ausgaben. */
typedef enum {
	LOG_TYPE_DEBUG = 0,	/*!< Allgemeines Debugging */
	LOG_TYPE_INFO,		/*!< Allgemeine Informationen */
	LOG_TYPE_WARN,		/*!< Auftreten einer unerwarteten Situation */
	LOG_TYPE_ERROR,		/*!< Fehler aufgetreten */
	LOG_TYPE_FATAL		/*!< Kritischer Fehler */
} LOG_TYPE;

#ifdef LOG_AVAILABLE

/*!
 * Schreibt Angaben ueber Datei, Zeilennummer und den Log-Typ in den Puffer.
 * Achtung, Mutex wird gelockt und muss explizit durch log_end() wieder
 * freigegeben werden!
 * @param filename Dateiname
 * @param line Zeilennummer
 * @param log_type Log-Typ
 */
#ifdef MCU
	#define log_begin log_flash_begin
	extern void log_flash_begin(const char *filename, unsigned int line, LOG_TYPE log_type);
#else
	extern void log_begin(const char *filename, unsigned int line, LOG_TYPE log_type);
#endif

/*!
 * Schreibt die eigentliche Ausgabeinformation in den Puffer.
 * @param format Format
 */
#ifdef MCU
	#define log_printf log_flash_printf
	extern void log_flash_printf(const char *format, ...);
#else
	extern void log_printf(const char *format, ...);	
#endif

/*!
 * Gibt den Puffer entsprechend aus.
 */
extern void log_end(void);

#ifdef LOG_MMC_AVAILABLE
	uint8 log_mmc_init(void);
#endif

#ifdef LOG_DISPLAY_AVAILABLE	
	/*!
	 * @brief	Display-Handler fuer das Logging
	 */
	void log_display(void);
#endif

#endif	/* LOG_AVAILABLE */

#endif /*LOG_H_*/
