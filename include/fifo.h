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
 * @file 	fifo.h
 * @brief 	Implementierung einer FIFO
 * @author 	http://www.roboternetz.de/wissen/index.php/FIFO_mit_avr-gcc
 * @date 	28.02.2007
 * Thread-Safe, abgesichert gegen Interrupts, solange sich Producer bzw. Consumer jeweils auf der gleichen Interrupt-Ebene befinden.
 */

#ifndef _FIFO_H_
#define _FIFO_H_

//#define DEBUG_FIFO		/*!< Schalter fuer Debug-Ausgaben */

#include "ct-Bot.h"
#include "global.h"
#include "os_thread.h"
#include "log.h"

#ifdef MCU
#include <avr/builtins.h>
#endif

#ifndef LOG_AVAILABLE
#undef DEBUG_FIFO
#endif
#ifndef DEBUG_FIFO
#define LOG_DEBUG_FIFO(a, ...) {}
#else
#define LOG_DEBUG_FIFO LOG_DEBUG
#endif

/*! FIFO-Datentyp */
typedef struct {
	uint8_t volatile count;	/*!< # Zeichen im Puffer */
	uint8_t size;			/*!< Puffer-Grosse */
	uint8_t * pread;		/*!< Lesezeiger */
	uint8_t * pwrite;		/*!< Schreibzeiger */
	uint8_t read2end;		/*!< # Zeichen bis zum Ueberlauf Lesezeiger */
	uint8_t write2end;		/*!< # Zeichen bis zum Ueberlauf Schreibzeiger */
	os_signal_t signal;		/*!< Signal das den Fifo-Status meldet */
} fifo_t;

/*!
 * Initialisiert die FIFO, setzt Lese- und Schreibzeiger, etc.
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *buffer	Zeiger auf den Puffer der Groesse size fuer die FIFO
 * @param size		Anzahl der Bytes, die die FIFO speichern soll	.
 */
void fifo_init(fifo_t * f, void * buffer, const uint8_t size);

/*!
 * Schreibt length Byte in die FIFO.
 * Achtung, wenn der freie Platz nicht ausreicht, werden die
 * aeltesten Daten verworfen!
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *data		Zeiger auf Quelldaten
 * @param length	Anzahl der zu kopierenden Bytes
 */
void fifo_put_data(fifo_t * f, void * data, uint8_t length);

/*!
 * Liefert length Bytes aus der FIFO, blockierend, falls Fifo leer!
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *data		Zeiger auf Speicherbereich fuer Zieldaten
 * @param length	Anzahl der zu kopierenden Bytes
 * @return			Anzahl der tatsaechlich gelieferten Bytes
 */
uint8_t fifo_get_data(fifo_t * f, void * data, uint8_t length);

/*!
 * Schreibt ein Byte in die FIFO.
 * @param *f	Zeiger auf FIFO-Datenstruktur
 * @param data	Das zu schreibende Byte
 * @param isr	wird die Funktion von einer ISR aus aufgerufen?
 */
static inline void _inline_fifo_put(fifo_t * f, const uint8_t data, uint8_t isr) {
	uint8_t * pwrite = f->pwrite;
	*(pwrite++) = data;

	uint8_t write2end = f->write2end;
	if (--write2end == 0) {
		write2end = f->size;
		pwrite -= write2end;
	}

	f->write2end = write2end;
	f->pwrite = pwrite;
	if (isr) {
		f->count++;
	} else {
#ifdef MCU
		uint8_t sreg = SREG;
		__builtin_avr_cli();
#else
		pthread_mutex_lock(&f->signal.mutex);
#endif
		f->count++;
#ifdef MCU
		SREG = sreg;
#else
		pthread_mutex_unlock(&f->signal.mutex);
#endif
#ifdef OS_AVAILABLE
		/* Consumer aufwecken */
		os_signal_unlock(&f->signal);
#endif	// OS_AVAILABLE
	}
}

/*!
 * Liefert das naechste Byte aus der FIFO.
 * @param *f	Zeiger auf FIFO-Datenstruktur
 * @param isr	wird die Funktion von einer ISR aus aufgerufen?
 * @return		Das Byte aus der FIFO
 */
static inline uint8_t _inline_fifo_get(fifo_t * f, uint8_t isr) {
#ifdef OS_AVAILABLE
	if (! isr) {
		uint8_t count = f->count;
		if (count == 0) {
			/* blockieren */
			LOG_DEBUG_FIFO("Fifo 0x%08x ist leer, blockiere", f);
			os_signal_lock(&f->signal);
			os_signal_set(&f->signal);
			LOG_DEBUG_FIFO("Fifo 0x%08x enthaelt wieder Daten, weiter geht's", f);
			os_signal_release(&f->signal);
		}
	}
#endif	// OS_AVAILABLE

	uint8_t * pread = f->pread;
	uint8_t data = *(pread++);
	uint8_t read2end = f->read2end;

	if (--read2end == 0) {
		read2end = f->size;
		pread -= read2end;
	}

	f->pread = pread;
	f->read2end = read2end;
	if (isr) {
		f->count--;
	} else {
#ifdef MCU
		uint8_t sreg = SREG;
		__builtin_avr_cli();
#else
		pthread_mutex_lock(&f->signal.mutex);
#endif
		f->count--;
#ifdef MCU
		SREG = sreg;
#else
		pthread_mutex_unlock(&f->signal.mutex);
#endif
	}

	return data;
}

#endif	// _FIFO_H_
