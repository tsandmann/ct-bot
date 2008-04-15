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
 * @file 	fifo.c
 * @brief 	Implementierung einer FIFO
 * @author 	http://www.roboternetz.de/wissen/index.php/FIFO_mit_avr-gcc
 * @date 	28.02.2007
 * Thread-Safe, abgesichert gegen Interrupts, solange sich Producer bzw. Consumer jeweils auf der gleichen Interrupt-Ebene befinden.
 */

#include "fifo.h"
#include "log.h"

/*!
 * @brief			Initialisiert die FIFO, setzt Lese- und Schreibzeiger, etc. 
 * @param f			Zeiger auf FIFO-Datenstruktur
 * @param buffer	Zeiger auf den Puffer der Groesse size fuer die FIFO
 * @param size		Anzahl der Bytes, die die FIFO speichern soll	.
 */
void fifo_init(fifo_t * f, void * buffer, const uint8_t size) {
	f->count = 0;
	f->pread = f->pwrite = buffer;
	f->read2end = f->write2end = f->size = size;
#ifdef PC
	pthread_mutex_init(&f->mutex, NULL);
	pthread_cond_init(&f->cond, NULL);
#endif
}
	
/*!
 * @brief			Schreibt length Byte in die FIFO
 * @param f			Zeiger auf FIFO-Datenstruktur
 * @param data		Zeiger auf Quelldaten
 * @param length	Anzahl der zu kopierenden Bytes
 */
void fifo_put_data(fifo_t * f, void * data, uint8_t length) {
	uint8_t * src = data;
	uint8_t * pwrite = f->pwrite;
	uint8_t write2end = f->write2end;
	uint8_t n = length > write2end ? write2end : length;
	uint8_t i,j;
	for (j=0; j<2; j++) {
		for (i=0; i<n; i++) {
			*(pwrite++) = *(src++);
		}

		write2end -= n;
		if (write2end == 0) {
			write2end = f->size;
			pwrite -= write2end;
		}
		n = length - n;
	}
			
	f->write2end = write2end;
	f->pwrite = pwrite;

#ifdef MCU
	uint8_t sreg = SREG;
	cli();
#else
	if (length == 0) return;
	pthread_mutex_lock(&f->mutex);
#endif
	f->count += length;
#ifdef MCU
	SREG = sreg;
#else
	/* Consumer aufwecken */
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
#endif
}

/*!
 * @brief			Liefert length Bytes aus der FIFO, nicht blockierend fuer MCU, aber fuer PC / phtreads
 * @param f			Zeiger auf FIFO-Datenstruktur
 * @param data		Zeiger auf Speicherbereich fuer Zieldaten
 * @param length	Anzahl der zu kopierenden Bytes
 * @return			Anzahl der tatsaechlich gelieferten Bytes
 */	
uint8_t fifo_get_data(fifo_t * f, void * data, uint8_t length) {
#ifdef PC
	pthread_mutex_lock(&f->mutex);
#endif
	uint8_t count = f->count;
#ifdef PC
	if (count == 0) {
		/* blockieren */
//		LOG_DEBUG("Fifo leer -> Thread blockiert");
		pthread_cond_wait(&f->cond, &f->mutex);
//		LOG_DEBUG("Thread laeuft weiter");
		count = f->count;
	}
	pthread_mutex_unlock(&f->mutex);
#endif
	if (count < length) length = count;
	uint8_t * pread = f->pread;
	uint8_t read2end = f->read2end;
	uint8_t n = length > read2end ? read2end : length;
	uint8_t * dest = data;
	uint8_t i,j;
	for (j=0; j<2; j++) {
		for (i=0; i<n; i++) {
			*(dest++) = *(pread++);
		}
		read2end -= n;
		if (read2end == 0) {
			read2end = f->size;
			pread -= read2end;
		}
		n = length - n;
	}

	f->pread = pread;
	f->read2end = read2end;
		
#ifdef MCU
	uint8_t sreg = SREG;
	cli();
#else
	pthread_mutex_lock(&f->mutex);
#endif
	f->count -= length;
#ifdef MCU
	SREG = sreg;
#else
	pthread_mutex_unlock(&f->mutex);
#endif
		
	return length;
}
