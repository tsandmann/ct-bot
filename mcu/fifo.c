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
 * Abgesichert gegen Interrups, solange sich Producer bzw. Consumer jeweils auf der gleichen Interrupt-Ebene befinden.
 */

#include "fifo.h"

#ifdef MCU
	/*!
	 * @brief			Initialisiert die FIFO, setzt Lese- und Schreibzeiger, etc. 
	 * @param f			Zeiger auf FIFO-Datenstruktur
	 * @param buffer	Zeiger auf den Puffer der Groesse size fuer die FIFO
	 * @param size		Anzahl der Bytes, die die FIFO speichern soll	.
	 */
	void fifo_init(fifo_t *f, uint8 *buffer, const uint8 size){
		f->count = 0;
		f->pread = f->pwrite = buffer;
		f->read2end = f->write2end = f->size = size;
	}
	
//	/*!
//	 * Schreibt das Byte data in die FIFO. Liefert 1 bei Erfolg und 0, falls die FIFO voll ist.
//	 */
//	uint8 fifo_put(fifo_t *f, const uint8 data){
//		return _inline_fifo_put(f, data);
//	}
	
	/*!
	 * @brief			Schreibt length Byte in die FIFO
	 * @param f			Zeiger auf FIFO-Datenstruktur
	 * @param data		Zeiger auf Quelldaten
	 * @param length	Anzahl der zu kopierenden Bytes
	 */
	void fifo_put_data(fifo_t *f, uint8* data, uint8 length){	
		uint8* pwrite = f->pwrite;
		uint8 write2end = f->write2end;
		uint8 n = length > write2end ? write2end : length;
		uint8 i,j;
		for (j=0; j<2; j++){
			for (i=0; i<n; i++){
				*(pwrite++) = *(data++);
			}
	
			write2end -= n;
			if (write2end == 0){
				write2end = f->size;
				pwrite -= write2end;
			}
			n = length - n;
		}
//		for (i=0; i<n; i++){
//			*(pwrite++) = *(data++);
//		}
//		write2end -= n;
				
		f->write2end = write2end;
		f->pwrite = pwrite;
	
		uint8 sreg = SREG;
		cli();
		f->count += length;
		SREG = sreg;		
	}
	
//	/*!
//	 * Liefert das naechste Byte aus der FIFO, bei leerer FIFO wird gewartet, bis das naechste Zeichen eintrifft.
//	 */	
//	uint8 fifo_get_wait(fifo_t *f){
//		while (!f->count);
//		return _inline_fifo_get(f);	
//	}

//	/*!
//	 * Liefert das naechste Byte aus der FIFO als int16 bzw. -1, falls die FIFO leer ist.
//	 */	
//	int16 fifo_get_nowait(fifo_t *f){
//		if (!f->count) return -1;
//		return (int16)_inline_fifo_get(f);	
//	}

	/*!
	 * @brief			Liefert length Bytes aus der FIFO, nicht blockierend.
	 * @param f			Zeiger auf FIFO-Datenstruktur
	 * @param data		Zeiger auf Speicherbereich fuer Zieldaten
	 * @param length	Anzahl der zu kopierenden Bytes
	 * @return			Anzahl der tatsaechlich gelieferten Bytes
	 */	
	uint8 fifo_get_data(fifo_t *f, uint8* data, uint8 length){
		uint8 count = f->count;
		if (count < length) length = count;
		uint8 *pread = f->pread;
		uint8 read2end = f->read2end;
		uint8 n = length > read2end ? read2end : length;
		uint8 i,j;
		for (j=0; j<2; j++){
			for (i=0; i<n; i++){
				*(data++) = *(pread++);
			}
			read2end -= n;
			if (read2end == 0){
				read2end = f->size;
				pread -= read2end;
			}
			n = length - n;
		}
//		for (i=0; i<n; i++){
//			*(data++) = *(pread++);	
//		}
//		read2end -= n;
		f->pread = pread;
		f->read2end = read2end;
		
		uint8 sreg = SREG;
		cli();
		f->count -= length;
		SREG = sreg;
		
		return length;
	}	
#endif	// MCU
