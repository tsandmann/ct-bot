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

/**
 * \file 	log.h
 * \brief 	Routinen zum Loggen von Informationen.
 *
 * Es sollten ausschliesslich nur die Log-Makros: LOG_DEBUG(), LOG_INFO(), LOG_WARN(), LOG_ERROR(),
 * LOG_FATAL() und LOG_RAW() verwendet werden.
 * Eine Ausgabe kann wie folgt erzeugt werden:
 * LOG_DEBUG("Hallo Welt!");
 * LOG_INFO("Wert x=%d", x);
 * Bei den Ausgaben kann auf ein Line Feed am Ende des Strings verzichtet werden,
 * da dies automatisch angehaengt hinzugefuegt wird.
 * Die frueher noetigen Doppelklammern sind nicht mehr noetig, einfach normale Klammern
 * verwenden, siehe Bsp. oben.
 * (Die Doppelklammern funktionieren nicht mit Var-Arg-Makros, die wir aber brauchen, da
 * nun fuer MCU alle Strings im Flash belassen werden sollen, das spart viel RAM :-) )
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
 * 								Es darf BOT_2_SIM_AVAILABLE nicht definiert sein, da ansonsten
 * 								diese Kommunikation ueber den UART laeuft.
 * 2. Logging ueber ct-Sim:		LOG_CTSIM_AVAILABLE muss definiert sein.
 * 								BOT_2_SIM_AVAILABLE muss zusaetzlich definiert sein.
 * 3. Logging ueber Display:	LOG_DISPLAY_AVAILABLE muss definiert sein, sowie DISPLAY_AVAILABLE.
 * 4. Logging in txt auf MMC:	MMC_AVAILABLE und BOT_FS_AVAILABLE muessen an sein.
 * </pre>
 *
 * Alternativ schlankere Variante fuer LOG_CTSIM_AVAILABLE oder LOG_MMC_AVAILABLE, indem man USE_MINILOG aktiviert.
 * Das spart viel Platz in Flash und RAM.
 *
 * \author 	Andreas Merkle (mail@blue-andi.de)
 * \date 	27.02.2006
 */

#ifndef LOG_H_
#define LOG_H_

#ifdef LOG_AVAILABLE

#ifndef USE_MINILOG
/** Dieser Typ definiert die Typen der Log-Ausgaben. */
typedef enum {
	LOG_TYPE_DEBUG = 0,	/**< Allgemeines Debugging */
	LOG_TYPE_INFO,		/**< Allgemeine Informationen */
	LOG_TYPE_WARN,		/**< Auftreten einer unerwarteten Situation */
	LOG_TYPE_ERROR,		/**< Fehler aufgetreten */
	LOG_TYPE_FATAL		/**< Kritischer Fehler */
} PACKED LOG_TYPE;

#define LOG_RAW LOG_INFO

#ifdef PC
/**
 * Allgemeines Debugging (Methode DiesUndDas wurde mit Parameter SoUndSo
 * aufgerufen ...)
 */
#define LOG_DEBUG(...) {	log_begin(__FILE__, __LINE__, LOG_TYPE_DEBUG); 	\
							log_printf(__VA_ARGS__);						\
							log_end();										\
}

/**
 * Allgemeine Informationen (Programm gestartet, Programm beendet, Verbindung
 * zu Host Foo aufgebaut, Verarbeitung dauerte SoUndSoviel Sekunden ...)
 */
#define LOG_INFO(...) {		log_begin(__FILE__, __LINE__, LOG_TYPE_INFO); 	\
							log_printf(__VA_ARGS__);						\
							log_end();										\
}

/**
 * Auftreten einer unerwarteten Situation.
 */
#define LOG_WARN(...) {		log_begin(__FILE__, __LINE__, LOG_TYPE_WARN); 	\
							log_printf(__VA_ARGS__);						\
							log_end();										\
}

/**
 * Fehler aufgetreten, Bearbeitung wurde alternativ fortgesetzt.
 */
#define LOG_ERROR(...) {	log_begin(__FILE__, __LINE__, LOG_TYPE_ERROR); 	\
							log_printf(__VA_ARGS__);						\
							log_end();										\
}

/**
 * Kritischer Fehler, Programmabbruch.
 */
#define LOG_FATAL(...) {	log_begin(__FILE__, __LINE__, LOG_TYPE_FATAL); 	\
							log_printf(__VA_ARGS__);						\
							log_end();										\
}

/**
 * Schreibt Angaben ueber Datei, Zeilennummer und den Log-Typ in den Puffer.
 * Achtung, Mutex wird gelockt und muss explizit durch log_end() wieder
 * freigegeben werden!
 * \param filename Dateiname
 * \param line Zeilennummer
 * \param log_type Log-Typ
 */
void log_begin(const char * filename, unsigned int line, LOG_TYPE log_type);

/**
 * Schreibt die eigentliche Ausgabeinformation in den Puffer.
 * \param format Format
 */
void log_printf(const char * format, ...);

/**
 * Gibt den Puffer entsprechend aus.
 */
void log_end(void);
#else // ! PC
/**
 * Allgemeines Debugging (Methode DiesUndDas wurde mit Parameter SoUndSo
 * aufgerufen ...)
 */
#define LOG_DEBUG(format, ...) {	static const char _file[] PROGMEM = __FILE__;		\
									log_flash_begin(_file, __LINE__, LOG_TYPE_DEBUG);	\
									static const char data[] PROGMEM = format;			\
									log_flash_printf(data, ## __VA_ARGS__);				\
									log_end();											\
}

/**
 * Allgemeine Informationen (Programm gestartet, Programm beendet, Verbindung
 * zu Host Foo aufgebaut, Verarbeitung dauerte SoUndSoviel Sekunden ...)
 */
#define LOG_INFO(format, ...) {		static const char _file[] PROGMEM = __FILE__;	\
									log_flash_begin(_file, __LINE__, LOG_TYPE_INFO);	\
									static const char data[] PROGMEM = format;		\
									log_flash_printf(data, ## __VA_ARGS__);			\
									log_end();										\
}

/**
 * Auftreten einer unerwarteten Situation.
 */
#define LOG_WARN(format, ...) {	static const char _file[] PROGMEM = __FILE__;		\
									log_flash_begin(_file, __LINE__, LOG_TYPE_WARN); \
									log_flash_printf(data, ## __VA_ARGS__);			\
									static const char data[] PROGMEM = format;		\
									log_end();										\
}

/**
 * Fehler aufgetreten, Bearbeitung wurde alternativ fortgesetzt.
 */
#define LOG_ERROR(format, ...) {	static const char _file[] PROGMEM = __FILE__;		\
									log_flash_begin(_file, __LINE__, LOG_TYPE_ERROR); 	\
									static const char data[] PROGMEM = format;			\
									log_flash_printf(data, ## __VA_ARGS__);				\
									log_end();											\
}

/**
 * Kritischer Fehler, Programmabbruch.
 */
#define LOG_FATAL(format, ...) { 	static const char _file[] PROGMEM = __FILE__;		\
									log_flash_begin(_file, __LINE__, LOG_TYPE_FATAL); 	\
									static const char data[] PROGMEM = format;			\
									log_flash_printf(data, ## __VA_ARGS__);				\
									log_end();											\
}

/**
 * Schreibt Angaben ueber Datei, Zeilennummer und den Log-Typ in den Puffer.
 * \param filename Dateiname
 * \param line Zeilennummer
 * \param log_type Log-Typ
 */
void log_flash_begin(const char * filename, unsigned int line, LOG_TYPE log_type);

/**
 * Schreibt die eigentliche Ausgabeinformation in den Puffer.
 * \param format Format-String
 */
void log_flash_printf(const char * format, ...);

/**
 * Gibt den Puffer entsprechend aus.
 */
void log_end(void);
#endif // PC

#ifdef LOG_DISPLAY_AVAILABLE
/**
 * Display-Handler fuer das Logging
 */
void log_display(void);
#endif // LOG_DISPLAY_AVAILABLE

#else // USE_MINILOG
/** Dieser Typ definiert die Typen der Log-Ausgaben. */
typedef enum {
	LOG_TYPE_DEBUG = 0,	/**< Allgemeines Debugging */
	LOG_TYPE_INFO,		/**< Allgemeine Informationen */
	LOG_TYPE_ERROR,		/**< Fehler aufgetreten */
	LOG_TYPE_RAW,		/**< Nur Datenausgabe */
} PACKED LOG_TYPE;

#define LOG_WARN	LOG_INFO
#define LOG_FATAL	LOG_ERROR

/**
 * Allgemeines Debugging
 */
#define LOG_DEBUG(format, ...) { minilog_begin(__LINE__, LOG_TYPE_DEBUG); 		\
								static const char __data[] PROGMEM = format;	\
								minilog_printf(__data, ## __VA_ARGS__);			\
}

/**
 * Info-Logging
 */
#define LOG_INFO(format, ...) {	minilog_begin(__LINE__, LOG_TYPE_INFO);			\
								static const char __data[] PROGMEM = format;	\
								minilog_printf(__data, ## __VA_ARGS__);			\
}

/**
 * Fehler-Logging
 */
#define LOG_ERROR(format, ...) { minilog_begin(__LINE__, LOG_TYPE_ERROR); 		\
								 static const char __data[] PROGMEM = format;	\
								 minilog_printf(__data, ## __VA_ARGS__);		\
}

/**
 * Reine Datenausgabe
 */
#define LOG_RAW(format, ...) {	minilog_begin(0, LOG_TYPE_RAW); 				\
								static const char __data[] PROGMEM = format;	\
								minilog_printf(__data, ## __VA_ARGS__);			\
}

/**
 * Schreibt die Zeilennummer und den Log-Typ in den Puffer
 * \param line		Zeilennummer
 * \param log_type	Log-Typ {DEBUG, INFO, ERROR}
 */
void minilog_begin(uint16_t line, LOG_TYPE log_type);

/**
 * Schreibt den Log-Text in den Log-Puffer und versendet die Daten
 * \param format	Format-String, wie bei printf
 * \param ... 		Variable Argumentenliste, wie bei printf
 */
void minilog_printf(const char * format, ...);
#endif // USE_MINILOG

#ifdef LOG_MMC_AVAILABLE
/**
 * Initialisierung fuer MMC-Logging
 */
void log_mmc_init(void);

/**
 * Schreibt den aktuellen Inhalt des Log-Puffers auf die MMC
 */
void log_flush(void);

/**
 * Display-Handler fuer MMC-LOG
 */
void log_mmc_display(void);
#endif // LOG_MMC_AVAILABLE

#else // ! LOG_AVAILABLE

#define LOG_DEBUG(a, ...)
#define LOG_INFO(a, ...)
#define LOG_WARN(a, ...)
#define LOG_ERROR(a, ...)
#define LOG_FATAL(a, ...)
#define LOG_RAW(a, ...)
#endif // LOG_AVAILABLE
#endif // LOG_H_
