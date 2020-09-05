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
 * \file 	fifo.c
 * \brief 	Implementierung einer FIFO
 * \author 	http://www.roboternetz.de/wissen/index.php/FIFO_mit_avr-gcc
 * \author	Timo Sandmann
 * \date 	28.02.2007
 * Thread-Safe, abgesichert gegen Interrupts, solange sich Producer bzw. Consumer jeweils auf der gleichen Interrupt-Ebene befinden.
 */

#include "ct-Bot.h"
#include "fifo.h"

/**
 * Initialisiert die FIFO, setzt Lese- und Schreibzeiger, etc.
 * \param *f		Zeiger auf FIFO-Datenstruktur
 * \param *buffer	Zeiger auf den Puffer der Groesse size fuer die FIFO
 * \param size		Anzahl der Bytes, die die FIFO speichern soll	.
 */
void fifo_init(fifo_t * f, void * buffer, const uint8_t size) {
	f->count = 0;
	f->pread = f->pwrite = buffer;
	f->read2end = f->write2end = f->size = size;
#ifdef FIFO_STATS_ENABLED
	f->written = 0;
#endif
#ifdef OS_AVAILABLE
	f->signal.value = 0; // Fifo leer
#endif
	f->overflow = 0;
	f->locked = 0;
#ifdef PC
	pthread_mutex_init(&f->signal.mutex, NULL);
#ifdef OS_AVAILABLE
	pthread_cond_init(&f->signal.cond, NULL);
#endif
#endif // PC
	LOG_DEBUG_FIFO("Fifo 0x%08x initialisiert", (unsigned int) f);
}

/**
 * Schreibt length Byte in die FIFO.
 * \param *f		Zeiger auf FIFO-Datenstruktur
 * \param *data		Zeiger auf Quelldaten
 * \param length	Anzahl der zu kopierenden Bytes
 * \param block		Flag, ob die Funktion blockieren soll, wenn der freie Platz nicht ausreicht
 * \return			Fehlercode, 0 falls kein Fehler
 */
uint8_t fifo_put_data(fifo_t * f, const void * data, uint8_t length, const uint8_t block) {
	if (length == 0) {
		return 1;
	}
	uint8_t space;
	if (length > (space = (uint8_t) (f->size - f->count))) {
		/* nicht genug Platz -> alte Daten rauswerfen */
		f->overflow = 1;
		LOG_DEBUG_FIFO("FIFO 0x%08x overflow, size=%u", (unsigned int) f, f->size);
		while (block && f->locked) {
			os_thread_yield();
		}

		return 2;

		uint8_t to_discard = (uint8_t) (length - space);
		LOG_DEBUG_FIFO("verwerfe %u Bytes in Fifo 0x%08x", to_discard, (unsigned int) f);
		LOG_DEBUG_FIFO(" size=%u, count=%u, length=%u", f->size, f->count, length);
		uint8_t read2end = f->read2end;
		uint8_t * pread = f->pread;
		if (to_discard > read2end) {
			/* Ueberlauf */
			read2end = (uint8_t) (read2end + f->size);
			pread -= f->size;
		}
		read2end = (uint8_t) (read2end - to_discard);
		pread += to_discard;
		f->read2end = read2end;
		f->pread = pread;

#ifdef MCU
		uint8_t sreg = SREG;
		__builtin_avr_cli();
#else
		pthread_mutex_lock(&f->signal.mutex);
#endif
		f->count = (uint8_t) (f->count - to_discard);
#ifdef MCU
		SREG = sreg;
#else
		pthread_mutex_unlock(&f->signal.mutex);
#endif
	}
	const uint8_t * src = data;
	uint8_t * pwrite = f->pwrite;
	uint8_t write2end = f->write2end;
	uint8_t n = length > write2end ? write2end : length;
	uint8_t i, j;
	for (j = 0; j < 2; ++j) {
		for (i = 0; i < n; ++i) {
			*(pwrite++) = *(src++);
		}

		write2end = (uint8_t) (write2end - n);
		if (write2end == 0) {
			write2end = f->size;
			pwrite -= write2end;
		}
		n = (uint8_t) (length - n);
	}

	f->write2end = write2end;
	f->pwrite = pwrite;
#ifdef FIFO_STATS_ENABLED
	f->written += length;
#endif

#ifdef MCU
	uint8_t sreg = SREG;
	__builtin_avr_cli();
#else
	pthread_mutex_lock(&f->signal.mutex);
#endif
	f->count = (uint8_t) (f->count + length);
#ifdef MCU
	SREG = sreg;
#else
	pthread_mutex_unlock(&f->signal.mutex);
#endif
#ifdef OS_AVAILABLE
	/* Consumer aufwecken */
	os_signal_unlock(&f->signal);
#endif // OS_AVAILABLE
	return 0;
}

/**
 * Liefert length Bytes aus der FIFO.
 * Wenn OS_AVAILABLE, blockierend, falls Fifo leer.
 * \param *f		Zeiger auf FIFO-Datenstruktur
 * \param *data		Zeiger auf Speicherbereich fuer Zieldaten
 * \param length	Anzahl der zu kopierenden Bytes
 * \return			Anzahl der tatsaechlich gelieferten Bytes
 */
int16_t fifo_get_data(fifo_t * f, void * data, int16_t length) {
	uint8_t l = (uint8_t) (length <= 255 ? length : 255);
	if (l == 0) {
		return 0;
	}
	uint8_t count = f->count;
#ifdef OS_AVAILABLE
	while (count < l) {
		/* blockieren */
		LOG_DEBUG_FIFO("Fifo 0x%08x ist leer, blockiere", (unsigned int) f);
		os_signal_lock(&f->signal);
		os_signal_set(&f->signal);
		LOG_DEBUG_FIFO("Fifo 0x%08x enthaelt wieder Daten, weiter geht's", (unsigned int) f);
		os_signal_release(&f->signal);
		count = f->count;
	}
#endif // OS_AVAILABLE
//	if (count < l) {
//		l = count;
//	}
	f->locked = 1;
	uint8_t* pread = f->pread;
	uint8_t read2end = f->read2end;
	uint8_t n = l > read2end ? read2end : l;
	uint8_t * dest = data;
	uint8_t i,j;
	for (j = 0; j < 2; ++j) {
		for (i = 0; i < n; ++i) {
			*(dest++) = *(pread++);
		}
		read2end = (uint8_t) (read2end - n);
		if (read2end == 0) {
			read2end = f->size;
			pread -= read2end;
		}
		n = (uint8_t) (l - n);
	}

	f->pread = pread;
	f->read2end = read2end;

#ifdef MCU
	uint8_t sreg = SREG;
	__builtin_avr_cli();
#else
	pthread_mutex_lock(&f->signal.mutex);
#endif
	f->count = (uint8_t) (f->count - l);
#ifdef MCU
	SREG = sreg;
#else
	pthread_mutex_unlock(&f->signal.mutex);
#endif
	f->locked = 0;

	return l;
}
