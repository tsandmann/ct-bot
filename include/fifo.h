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
 * Thread-Safe, abgesichert gegen Interrups, solange sich Producer bzw. Consumer jeweils auf der gleichen Interrupt-Ebene befinden.
 */

#ifndef _FIFO_H_
#define _FIFO_H_

#include "ct-Bot.h"
#include "global.h"
#ifdef MCU
#include <avr/io.h>
#include <avr/interrupt.h>
#else
#include <pthread.h>
#endif	// MCU
	
/*! FIFO-Datentyp */
typedef struct {
	uint8_t volatile count;	/*!< # Zeichen im Puffer */
	uint8_t size;			/*!< Puffer-Grosse */
	uint8_t * pread;		/*!< Lesezeiger */
	uint8_t * pwrite;		/*!< Schreibzeiger */
	uint8_t read2end;		/*!< # Zeichen bis zum Ueberlauf Lesezeiger */
	uint8_t write2end;		/*!< # Zeichen bis zum Ueberlauf Schreibzeiger */
#ifdef PC
	pthread_mutex_t mutex;	/*!< Mutex zur Synchronisation */
	pthread_cond_t cond;	/*!< Signal zur Synchronisation */
#endif
} fifo_t;

/*!
 * @brief			Initialisiert die FIFO, setzt Lese- und Schreibzeiger, etc. 
 * @param f			Zeiger auf FIFO-Datenstruktur
 * @param buf		Zeiger auf den Puffer der Groesse size fuer die FIFO
 * @param size		Anzahl der Bytes, die die FIFO speichern soll	.
 */
extern void fifo_init(fifo_t * f, void * buf, const uint8_t size);
	
/*!
 * @brief			Schreibt length Byte in die FIFO
 * @param f			Zeiger auf FIFO-Datenstruktur
 * @param data		Zeiger auf Quelldaten
 * @param length	Anzahl der zu kopierenden Bytes
 */	
extern void fifo_put_data(fifo_t * f, void * data, uint8_t length);

/*!
 * @brief			Liefert length Bytes aus der FIFO, nicht blockierend.
 * @param f			Zeiger auf FIFO-Datenstruktur
 * @param data		Zeiger auf Speicherbereich fuer Zieldaten
 * @param length	Anzahl der zu kopierenden Bytes
 * @return			Anzahl der tatsaechlich gelieferten Bytes
 */	
extern uint8 fifo_get_data(fifo_t * f, void * data, uint8_t length);

/*!
 * @brief		Schreibt ein Byte in die FIFO.
 * @param f		Zeiger auf FIFO-Datenstruktur
 * @param data	Das zu schreibende Byte
 * @return		1 bei Erfolg und 0, falls die FIFO voll ist.
 */
static inline uint8_t _inline_fifo_put(fifo_t * f, const uint8_t data) {
	if (f->count >= f->size) return 0;
		
	uint8_t * pwrite = f->pwrite;
	*(pwrite++) = data;
	
	uint8 write2end = f->write2end;
	if (--write2end == 0){
		write2end = f->size;
		pwrite -= write2end;
	}
	
	f->write2end = write2end;
	f->pwrite = pwrite;

#ifdef MCU
	uint8_t sreg = SREG;
	cli();
#else
	pthread_mutex_lock(&f->mutex);
#endif
	f->count++;
#ifdef MCU
	SREG = sreg;
#else
	pthread_mutex_unlock(&f->mutex);
#endif
	
	return 1;
}

/*!
 * @brief	Liefert das naechste Byte aus der FIFO. 
 * @param f	Zeiger auf FIFO-Datenstruktur
 * @return	Das Byte aus der FIFO
 * Ob ueberhaupt ein Byte in der FIFO ist, muss vorher extra abgeprueft werden!
 */
static inline uint8_t _inline_fifo_get(fifo_t * f) {
	uint8_t * pread = f->pread;
	uint8_t data = *(pread++);
	uint8_t read2end = f->read2end;
	
	if (--read2end == 0) {
		read2end = f->size;
		pread -= read2end;
	}
	
	f->pread = pread;
	f->read2end = read2end;
	
#ifdef MCU
	uint8_t sreg = SREG;
	cli();
#else
	pthread_mutex_lock(&f->mutex);
#endif
	f->count--;
#ifdef MCU
	SREG = sreg;
#else
	pthread_mutex_unlock(&f->mutex);
#endif
	return data;
}

#endif	// _FIFO_H_
