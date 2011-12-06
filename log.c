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
 * \file 	log.c
 * \brief 	Routinen zum Loggen von Informationen.
 *
 * Es sollten ausschliesslich nur die Log-Makros: LOG_DEBUG(), LOG_INFO(), LOG_WARN(), LOG_ERROR() und
 * LOG_FATAL() verwendet werden.
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
 * 4. Logging in txt auf MMC:	MMC_AVAILABLE und MMC_VM_AVAILABLE muessen an sein.
 * </pre>
 *
 * Alternativ schlankere Variante fuer MCU und CTSIM, indem man USE_MINILOG aktiviert.
 * Das spart viel Platz in Flash und RAM.
 *
 * \author 	Andreas Merkle (mail@blue-andi.de)
 * \date 	27.02.2006
 */

#include "ct-Bot.h"

#ifdef LOG_AVAILABLE
#ifndef USE_MINILOG

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "display.h"
#include "command.h"
#include "uart.h"
#include "mmc-vm.h"

#ifdef PC
#include <pthread.h>
#endif

#ifdef LOG_DISPLAY_AVAILABLE
/*! Groesse des Puffers fuer die Logausgaben bei Verwendung des LCD-Displays. */
#define LOG_BUFFER_SIZE		(DISPLAY_LENGTH + 1)
#else
/*! Groesse des Puffers fuer die Logausgaben ueber UART und ueber TCP/IP. */
#define LOG_BUFFER_SIZE		200
#endif // LOG_DISPLAY_AVAILABLE


#ifdef PC
/*! Schuetzt den Ausgabepuffer */
#define LOCK()		pthread_mutex_lock(&log_buffer_mutex);
/*! Hebt den Schutz fuer den Ausgabepuffer wieder auf */
#define UNLOCK()	pthread_mutex_unlock(&log_buffer_mutex);
#else
/*! Schuetzt den Ausgabepuffer */
#define LOCK()
/*! Hebt den Schutz fuer den Ausgabepuffer wieder auf */
#define UNLOCK()
#endif	/* PC */

#ifdef LOG_MMC_AVAILABLE
static uint32_t log_file;
static uint32_t log_file_end = 0;
#define LOG_FILENAME "log.txt"
#endif // LOG_MMC_AVAILABLE

/* Log-Typen als String, auf MCU im Flash */
static const char debug_str[] PROGMEM = "- DEBUG -";
static const char info_str[] PROGMEM = "- INFO -";
static const char warn_str[] PROGMEM = "- WARNING -";
static const char error_str[] PROGMEM = "- ERROR -";
static const char fatal_str[] PROGMEM = "- FATAL -";

/*!
 * Liefert den Log-Typ als String (auf MCU als Flash-Referenz).
 * @param log_type Log-Typ
 * @return char*
 */
static const char * log_get_type_str(LOG_TYPE log_type);

/*! Puffer fuer das Zusammenstellen einer Logausgabe */
static char log_buffer[LOG_BUFFER_SIZE];

#ifdef PC
/*! Schuetzt den Ausgabepuffer */
static pthread_mutex_t log_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif // PC

#ifdef LOG_DISPLAY_AVAILABLE
/*! Zeile in der die naechste Logausgabe erfolgt. */
static uint16_t log_line = 1;
static char screen_output[4][LOG_BUFFER_SIZE];	/*!< Puffer, damit mehr als eine Zeile pro Hauptschleifendurchlauf geloggt werden kann */
#endif // LOG_DISPLAY_AVAILABLE

#ifdef PC
/*!
 * Schreibt Angaben ueber Datei, Zeilennummer und den Log-Typ in den Puffer.
 * Achtung, Mutex wird gelockt und muss explizit durch log_end() wieder
 * freigegeben werden!
 * @param filename Dateiname
 * @param line Zeilennummer
 * @param log_type Log-Typ
 */
void log_begin(const char * filename, unsigned int line, LOG_TYPE log_type) {

	/* Ausgaben ueber das LCD-Display werden ohne Dateiname und Zeilennumer
	 * gemacht. Der Log-Typ versteckt sich im ersten Buchstaben. Durch eine
	 * die Markierung mit '>' erkennt man das letzte Logging.
	 * Nur bei Ausgaben ueber UART und an ct-Sim werden Dateiname, Zeilennummer
	 * und der Log-Typ vollstaendig ausgegeben.
	 */
#ifdef LOG_DISPLAY_AVAILABLE
	(void) line;
	(void) filename;
	LOCK();
	/* Alte Markierung loeschen */
	if (log_line == 1) {
		screen_output[3][0] = ' ';
	}
	else {
		screen_output[log_line-2][0] = ' ';
	}
	snprintf(log_buffer, LOG_BUFFER_SIZE, ">%c:", *(log_get_type_str(log_type)+2));
#else // !LOG_DISPLAY_AVAILABLE

	const char * ptr = NULL;

	/* Nur den Dateinamen loggen, ohne Verzeichnisangabe */
	ptr = strrchr(filename, '/');

	if (ptr == NULL)
		ptr = filename;
	else
		ptr++;

	LOCK();

	snprintf(log_buffer, LOG_BUFFER_SIZE, "%s(%d) %s ", ptr, line, log_get_type_str(log_type));

#endif // LOG_DISPLAY_AVAILABLE

	return;
}
#else // MCU
/*!
 * @brief	Kopiert einen String wortweise vom Flash ins Ram
 * @param *flash Zeiger auf einen String im FLASH
 * @param *ram	Zeiger auf den Zielpuffer im RAM
 * @param n		Anzahl der zu kopierenden WORTE
 * Es werden maximal n Worte kopiert, ist der String schon zuvor nullterminiert,
 * wird bei Auftreten von 0 abgebrochen.
 */
static void get_str_from_flash(const char * flash, char * ram, uint8_t n) {
	uint8_t i;
	/* Zeichen des Strings wortweise aus dem Flash holen */
	uint16_t * p_ram  = (uint16_t *) ram;
	uint16_t * p_flash = (uint16_t *) flash;
	for (i=0; i<n; i++){
		uint16_t tmp = pgm_read_word(p_flash++);
		*(p_ram++) = tmp;
		if ((uint8_t) tmp == 0 || (tmp & 0xFF00) == 0) break; // Stringende erreicht
	}
	*((char *) p_ram) = 0; // evtl. haben wir ein Byte zu viel gelesen, das korrigieren wir hier
}

/*!
 * Schreibt Angaben ueber Datei, Zeilennummer und den Log-Typ in den Puffer.
 * Achtung, Mutex wird gelockt und muss explizit durch log_end() wieder
 * freigegeben werden!
 * @param *filename Dateiname
 * @param line Zeilennummer
 * @param log_type Log-Typ
 */
void log_flash_begin(const char * filename, unsigned int line, LOG_TYPE log_type) {

	/* Ausgaben ueber das LCD-Display werden ohne Dateiname und Zeilennumer
	 * gemacht. Der Log-Typ versteckt sich im ersten Buchstaben. Durch eine
	 * die Markierung mit '>' erkennt man das letzte Logging.
	 * Nur bei Ausgaben ueber UART und an ct-Sim werden Dateiname, Zeilennummer
	 * und der Log-Typ vollstaendig ausgegeben.
	 */
#ifdef LOG_DISPLAY_AVAILABLE
	line = line;
	filename = filename;
	LOCK();
	/* Alte Markierung loeschen */
	if (log_line == 1) {
		screen_output[3][0] = ' ';
	} else {
		screen_output[log_line-2][0] = ' ';
	}
	log_buffer[0] = '>';
	log_buffer[1] = (char) pgm_read_byte(log_get_type_str(log_type)+2);
	log_buffer[2] = ':';
	log_buffer[3] = '\0';
#else // !LOG_DISPLAY_AVAILABLE
	/* Zeichen des Strings fuer Dateiname wortweise aus dem Flash holen */
	char flash_filen[80];
	get_str_from_flash(filename, flash_filen, 40/2);

	/* Zeichen des Strings fuer Typ wortweise aus dem Flash holen */
	char flash_type[12];
	get_str_from_flash(log_get_type_str(log_type), flash_type, 12/2);
	const char *ptr = NULL;

	/* Nur den Dateinamen loggen, ohne Verzeichnisangabe */
	ptr = strrchr(flash_filen, '/');

	if (ptr == NULL)
		ptr = flash_filen;
	else
		ptr++;

	LOCK();

	snprintf(log_buffer, LOG_BUFFER_SIZE, "%s(%d) %s ", ptr, line, flash_type);

#endif // LOG_DISPLAY_AVAILABLE

	return;
}
#endif // PC

#ifdef PC
/*!
 * Schreibt die eigentliche Ausgabeinformation in den Puffer.
 * @param format Format
 */
void log_printf(const char * format, ...) {
	va_list	args;
	unsigned int len = strlen(log_buffer);

	va_start(args, format);
	vsnprintf(&log_buffer[len], LOG_BUFFER_SIZE - len, format, args);
	va_end(args);

	return;
}
#else // MCU
/*!
 * Schreibt die eigentliche Ausgabeinformation (aus dem Flash) in den Puffer.
 * @param format Format
 */
void log_flash_printf(const char * format, ...) {
	char flash_str[LOG_BUFFER_SIZE+4];	// bissel groesser, weil die % ja noch mit drin sind
	get_str_from_flash(format, flash_str, LOG_BUFFER_SIZE/2+2);	// String aus dem Flash holen

	va_list	args;
	unsigned int len = strlen(log_buffer);

	va_start(args, format);
	vsnprintf(&log_buffer[len], LOG_BUFFER_SIZE - len, flash_str, args);
	va_end(args);

	return;
}
#endif // PC

/*!
 * Gibt den Puffer entsprechend aus.
 */
void log_end(void) {
#ifdef LOG_UART_AVAILABLE
	/* String ueber UART senden, ohne '\0'-Terminierung */
	uart_write((uint8_t *) log_buffer, (uint8_t) strlen(log_buffer));
	/* Line feed senden */
	uart_write((uint8_t *) LINE_FEED, strlen(LINE_FEED));
#endif // LOG_UART_AVAILABLE

#ifdef LOG_CTSIM_AVAILABLE
	/* Kommando an ct-Sim senden, ohne Line feed am Ende. */
	command_write_data(CMD_LOG, SUB_CMD_NORM, 0, 0, log_buffer);
#endif // LOG_CTSIM_AVAILABLE

#ifdef LOG_DISPLAY_AVAILABLE
	/* aktuelle Log-Zeile in Ausgabepuffer kopieren */
	memcpy(screen_output[log_line-1], log_buffer, strlen(log_buffer));
	/* Zeile weiterschalten */
	log_line++;
	if (log_line > 4) log_line = 1;
#endif // LOG_DISPLAY_AVAILABLE

/* Wenn das Logging aktiviert und keine Ausgabeschnittstelle
 * definiert ist, dann wird auf dem PC auf die Konsole geschrieben.
 */
#ifdef LOG_STDOUT_AVAILABLE
//	printf("%s\n", log_buffer);
	puts(log_buffer);
#endif // LOG_STDOUT_AVAILABLE

#ifdef LOG_MMC_AVAILABLE
	static uint16_t f_offset = 512;	// Init erzwingt Ueberlauf im ersten Aufruf, damit p_file geholt wird
	static uint8_t * p_file;
	uint8_t len = (uint8_t) strlen(log_buffer);	// |log_buffer| < 256
	if (f_offset + len > 512){
		/* der Log-Puffer passt nicht mehr komplett in den aktuellen Block */
		f_offset = 0;
		log_file += 512;	// naechster Block
		if (log_file > log_file_end){
			/* Dateiende wurde erreicht */
			p_file = NULL;
			mmc_flush_cache();	// Logging ist "am Ende" => es ist an der Zeit alle Daten auf die Karte zurueckzuschreiben
			return;
		}
		p_file = mmc_get_data(log_file);
	}
	if (p_file == NULL) return;
	*p_file++ = '\n';	// neue Zeile fuer jeden Log-Aufruf
	/* Log-Puffer uebertragen und Dateizeiger fortschreiben */
	memcpy(p_file, log_buffer, len);
	p_file += len;
	f_offset = (uint16_t) (f_offset + len + 1);
#endif // LOG_MMC_AVAILABLE

	UNLOCK();

	return;
}

#ifdef LOG_MMC_AVAILABLE
/**
 * Initialisierung fuer MMC-Logging
 */
void log_mmc_init(void) {
	/* Log-Datei oeffnen und Dateiende merken */
	log_file = mmc_fopen(LOG_FILENAME);
	if (log_file == 0) {
		return;	// Fehler :(
	}
	log_file_end = log_file + mmc_get_filesize(log_file);
	log_file -= 512; // denn beim ersten Logging springen wir in den Block-Ueberlauf-Fall
}
#endif // LOG_MMC_AVAILABLE

#ifdef LOG_DISPLAY_AVAILABLE
/*!
 * Display-Handler fuer das Logging
 */
void log_display(void) {
	/* Strings aufs LCD-Display schreiben */
	uint8_t i;
	for (i=0; i<4; i++) {	// Das Display hat 4 Zeilen
		display_cursor((uint8_t) (i + 1), 1);
		display_printf("%s", screen_output[i]);
	}
}
#endif // LOG_DISPLAY_AVAILABLE

/*!
 * Liefert einen Zeiger auf den Log-Typ als String.
 * @param log_type Log-Typ
 * @return char* Log-Typ als String
 */
static const char * log_get_type_str(LOG_TYPE log_type) {
	switch(log_type) {
		case LOG_TYPE_DEBUG:
			return debug_str;

		case LOG_TYPE_INFO:
			return info_str;

		case LOG_TYPE_WARN:
			return warn_str;

		case LOG_TYPE_ERROR:
			return error_str;

		case LOG_TYPE_FATAL:
			return fatal_str;
	}
	return debug_str;
}
#endif // USE_MINILOG
#endif // LOG_AVAILABLE
