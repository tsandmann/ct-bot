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
 * Abgesichert gegen Interrups, solange sich Producer bzw. Consumer jeweils auf der gleichen Interrupt-Ebene befinden.
 */

#ifndef _FIFO_H_
#define _FIFO_H_

#include "ct-Bot.h"
#ifdef MCU
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include "global.h"
	
	typedef struct{
		uint8 volatile count;       /*!< # Zeichen im Puffer */
		uint8 size;                 /*!< Puffer-Grosse */
		uint8 *pread;               /*!< Lesezeiger */
		uint8 *pwrite;              /*!< Schreibzeiger */
		uint8 read2end, write2end;  /*!< # Zeichen bis zum Ueberlauf Lese-/Schreibzeiger */
	} fifo_t;						/*!< FIFO-Datentyp */
	
	/*!
	 * @brief			Initialisiert die FIFO, setzt Lese- und Schreibzeiger, etc. 
	 * @param f			Zeiger auf FIFO-Datenstruktur
	 * @param buffer	Zeiger auf den Puffer der Groesse size fuer die FIFO
	 * @param size		Anzahl der Bytes, die die FIFO speichern soll	.
	 */
	extern void fifo_init(fifo_t*, uint8* buf, const uint8 size);
	
//	/*!
//	 * Schreibt das Byte data in die FIFO. Liefert 1 bei Erfolg und 0, falls die FIFO voll ist.
//	 */
//	extern uint8 fifo_put(fifo_t*, const uint8 data);
	
	/*!
	 * @brief			Schreibt length Byte in die FIFO
	 * @param f			Zeiger auf FIFO-Datenstruktur
	 * @param data		Zeiger auf Quelldaten
	 * @param length	Anzahl der zu kopierenden Bytes
	 */	
	extern void fifo_put_data(fifo_t *f, uint8* data, uint8 length);
	
//	/*!
//	 * Liefert das naechste Byte aus der FIFO, bei leerer FIFO wird gewartet, bis das naechste Zeichen eintrifft.
//	 */
//	extern uint8 fifo_get_wait(fifo_t*);
	
//	/*!
//	 * Liefert das naechste Byte aus der FIFO als int16 bzw. -1, falls die FIFO leer ist.
//	 */
//	extern int16 fifo_get_nowait(fifo_t*);

	/*!
	 * @brief			Liefert length Bytes aus der FIFO, nicht blockierend.
	 * @param f			Zeiger auf FIFO-Datenstruktur
	 * @param data		Zeiger auf Speicherbereich fuer Zieldaten
	 * @param length	Anzahl der zu kopierenden Bytes
	 * @return			Anzahl der tatsaechlich gelieferten Bytes
	 */	
	extern uint8 fifo_get_data(fifo_t *f, uint8* data, uint8 length);
	
	/*!
	 * @brief		Schreibt ein Byte in die FIFO.
	 * @param f		Zeiger auf FIFO-Datenstruktur
	 * @param data	Das zu schreibende Byte
	 * @return		1 bei Erfolg und 0, falls die FIFO voll ist.
	 */
	static inline uint8 _inline_fifo_put(fifo_t *f, const uint8 data){
		if (f->count >= f->size) return 0;
			
		uint8* pwrite = f->pwrite;
		*(pwrite++) = data;
		
		uint8 write2end = f->write2end;
		if (--write2end == 0){
			write2end = f->size;
			pwrite -= write2end;
		}
		
		f->write2end = write2end;
		f->pwrite = pwrite;
	
		uint8 sreg = SREG;
		cli();
		f->count++;
		SREG = sreg;
		
		return 1;
	}
	
	/*!
	 * @brief	Liefert das naechste Byte aus der FIFO. 
	 * @param f	Zeiger auf FIFO-Datenstruktur
	 * @return	Das Byte aus der FIFO
	 * Ob überhaupt ein Byte in der FIFO ist, muss vorher extra abgeprueft werden!
	 */
	static inline uint8 _inline_fifo_get(fifo_t *f){
		uint8 *pread = f->pread;
		uint8 data = *(pread++);
		uint8 read2end = f->read2end;
		
		if (--read2end == 0){
			read2end = f->size;
			pread -= read2end;
		}
		
		f->pread = pread;
		f->read2end = read2end;
		
		uint8 sreg = SREG;
		cli();
		f->count--;
		SREG = sreg;
		
		return data;
	}
#endif	// MCU
#endif	// _FIFO_H_
