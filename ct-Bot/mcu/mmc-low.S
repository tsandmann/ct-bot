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

#ifdef MCU
; Hinweis:
; Sollte der Build-Prozess an dieser Datei scheitern, bitte auch die 
; Datei ".settings/org.eclipse.core.runtime.prefs" aus dem CVS laden, oder
; selbst veranlassen, dass .S-Dateien ebenfalls vom gcc verarbeitet werden
; (und nicht direkt vom Assembler).

/*! 
 * @file 	mmc-low.s
 * @brief 	Low-Level-Routinen zum Lesen/Schreiben einer MMC / SD-Card
 * @author	Timo Sandmann (mail@timosandmann.de)
 * @date 	14.11.2006
 */

#include "mmc-low.h"
#include "ct-Bot.h"
#include "ena.h"
#ifdef MMC_AVAILABLE
#include <avr/io.h> 

; Portkonfiguration umrechnen
.equ PORT_OUT, _SFR_IO_ADDR(MMC_PORT_OUT)
.equ PORT_IN, _SFR_IO_ADDR(MMC_PORT_IN)
.equ DDR, _SFR_IO_ADDR(MMC_DDR)
.equ SPI_CLK_MASK, _BV(SPI_CLK)				; 1<<SPI_CLK

/*! 
 * Schreibt ein Byte an die Karte
 * @param data	Das zu sendende Byte
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		14.11.2006
 */
.global	mmc_write_byte
	.type mmc_write_byte, @function
mmc_write_byte:
	in r25, PORT_OUT			; MMC_PORT_OUT einlesen
	cbr r25, SPI_CLK_MASK		; CLK auf low
	bst r24, 7					; Bit 7	der Daten
	bld r25, SPI_DO				; Daten nach DO
	out PORT_OUT, r25			; Daten senden
	sbi PORT_OUT, SPI_CLK		; CLK auf high
	bst r24, 6					; Bit 6	der Daten
	bld r25, SPI_DO
	out PORT_OUT, r25
	sbi PORT_OUT, SPI_CLK
	bst r24, 5					; Bit 5	der Daten
	bld r25, SPI_DO
	out PORT_OUT, r25
	sbi PORT_OUT, SPI_CLK
	bst r24, 4					; Bit 4	der Daten
	bld r25, SPI_DO
	out PORT_OUT, r25
	sbi PORT_OUT, SPI_CLK
	bst r24, 3					; Bit 3	der Daten
	bld r25, SPI_DO
	out PORT_OUT, r25
	sbi PORT_OUT, SPI_CLK
	bst r24, 2					; Bit 2	der Daten
	bld r25, SPI_DO
	out PORT_OUT, r25
	sbi PORT_OUT, SPI_CLK
	bst r24, 1					; Bit 1	der Daten
	bld r25, SPI_DO
	out PORT_OUT, r25
	sbi PORT_OUT, SPI_CLK
	bst r24, 0					; Bit 0 der Daten
	bld r25, SPI_DO
	out PORT_OUT, r25
	sbi PORT_OUT, SPI_CLK
	sbi PORT_OUT, SPI_DO		; DO auf high
	ret

/*!
 * Liest ein Byte von der Karte
 * @return	Das gelesene Byte
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	14.11.2006
 */
.global	mmc_read_byte
	.type mmc_read_byte, @function
mmc_read_byte:
	in	r19, PORT_OUT			; MMC_PORT_OUT einlesen
	cbr r19, SPI_CLK_MASK		; CLK low
	out PORT_OUT, r19			; CLK auf low
	mov r18, r19				; CLK high
	sbr r18, SPI_CLK_MASK
	in r25, PORT_IN				; Bit 7 lesen
	out PORT_OUT, r18			; CLK Flanke
	out PORT_OUT, r19
	bst r25, SPI_DI				; Bit 7 speichern
	bld r24, 7
	in r25, PORT_IN				; Bit 6
	out PORT_OUT, r18
	out PORT_OUT, r19
	bst r25, SPI_DI
	bld r24, 6
	in r25, PORT_IN				; Bit 5
	out PORT_OUT, r18
	out PORT_OUT, r19
	bst r25, SPI_DI
	bld r24, 5
	in r25, PORT_IN				; Bit 4
	out PORT_OUT, r18
	out PORT_OUT, r19
	bst r25, SPI_DI
	bld r24, 4
	in r25, PORT_IN				; Bit 3
	out PORT_OUT, r18
	out PORT_OUT, r19
	bst r25, SPI_DI
	bld r24, 3
	in r25, PORT_IN				; Bit 2
	out PORT_OUT, r18
	out PORT_OUT, r19
	bst r25, SPI_DI
	bld r24, 2
	in r25, PORT_IN				; Bit 1
	out PORT_OUT, r18
	out PORT_OUT, r19
	bst r25, SPI_DI
	bld r24, 1
	in r25, PORT_IN				; Bit 0
	out PORT_OUT, r18
	bst r25, SPI_DI
	bld r24, 0
	clr r25						; return ist in 8 Bit und wir sind ordentlich ;)
	ret

/*!
 * Schaltet die Karte aktiv und checkt dabei die Initialisierung
 * @return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			09.12.2006
 */
.global mmc_enable
	.type mmc_enable, @function
mmc_enable:
	lds r24, mmc_init_state		; Initialisierung der Karte checken
	tst r24
	breq 1f
	call mmc_init				; neu initialisieren
	tst r24
	breq 1f
	ret							; kein Init moeglich => Fehler
	1:
	ldi r24, lo8(ENA_MMC)		; Karte aktiv schalten
	call ENA_on	
	ldi r24, lo8(ENA_MMC)		; Karte inaktiv schalten
	call ENA_off
	cbi DDR, SPI_DI				; DI auf input
	sbi DDR, SPI_DO				; DO auf output
	ldi r24, lo8(-1)
	call mmc_write_byte			; sendet 8 CLK-Flanken
	ldi r24, lo8(ENA_MMC)		; Karte aktiv schalten		
	call ENA_on	
	ldi r24, 0	
	ret							; alles ok

/*!
 * Schreibt einen 512-Byte Sektor auf die Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param Buffer 	Zeiger auf den Puffer
 * @param async		0: synchroner, 1: asynchroner Aufruf, siehe MMC_ASYNC_WRITE in mmc-low.h  
 * @return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich oder Timeout vor / nach Kommando 24, 2 wenn Timeout bei busy 
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			16.11.2006
 */
.global	mmc_write_sector
	.type mmc_write_sector, @function
mmc_write_sector:
	push r13					; allgemeine Register retten
	push r14
	push r15
	push r16
	push r28
	push r29
	movw r14, r22				; Byte 1 und 2 von Parameter addr sichern
	mov r16, r24				; Byte 3 von Parameter addr sichern  
	movw r28, r20				; Parameter Buffer sichern
	mov r13, r18				; Parameter async sichern
	call mmc_enable				; Karte aktiv schalten
	tst r24
	breq 1f
	rjmp 4f						; Karte kann nicht initialisiert werden => Fehler
	1:	
	lsl r14						; Block in Bytes umrechnen
	rol r15
	rol r16
#if MMC_ASYNC_WRITE == 1	
	clr r27						; r27 = 0
	1:
	clr r26						; r26 = 0
	2:
	call mmc_read_byte			; warten, ob Karte noch busy
	cpi r24, lo8(-1)
	breq 3f						; == 0xff?
	dec r26						; r26--
	brne 2b						; r26 == 0?
	dec r27						; r27--
	brne 1b						; r27 == 0?
	rjmp 4f						; Timeout :(
	3:
#endif		
	ldi r24, 88					; Kommando 24 zum Schreiben eines Blocks senden, Byte 0
	call mmc_write_byte
	mov r24, r16				; Byte 1
	call mmc_write_byte
	mov r24, r15				; Byte 2
	call mmc_write_byte
	mov r24, r14				; Byte 3
	call mmc_write_byte
	ldi r24, 0					; Byte 4
	call mmc_write_byte
	ldi r24, lo8(-1)			; Byte 5 (CRC)
	call mmc_write_byte
	ldi r26, lo8(-1)			; Timeout nach 256 Versuchen
	1:	
	call mmc_read_byte
	cpi r24, lo8(-1)			; !0xff = Antwort	
	brne 3f
	dec r26						; r26--	
	brne 1b
	3:
	tst r24	 					; Fehler oder Abbruch durch Timeout?
	breq 1f 
	rjmp 4f 					; Return-Code 1
	1:	
	ldi r24, lo8(-2)			; Start-Byte an Karte senden
	call mmc_write_byte
	in r26, PORT_OUT			; MMC_PORT_OUT einlesen
	cbr r26, SPI_CLK_MASK		; CLK auf low
	ldi r18, 2					; r18 = 2
	clr r27						; r27 = 0
	1:
	ld r25, Y+					; Byte aus SRAM lesen
	bst r25, 7					; Bit 7
	bld r26, SPI_DO
	out PORT_OUT, r26			; Daten senden
	sbi PORT_OUT, SPI_CLK		; CLK auf high
	bst r25, 6					; Bit 6
	bld r26, SPI_DO
	out PORT_OUT, r26
	sbi PORT_OUT, SPI_CLK
	bst r25, 5					; Bit 5
	bld r26, SPI_DO
	out PORT_OUT, r26
	sbi PORT_OUT, SPI_CLK
	bst r25, 4					; Bit 4
	bld r26, SPI_DO
	out PORT_OUT, r26
	sbi PORT_OUT, SPI_CLK
	bst r25, 3					; Bit 3
	bld r26, SPI_DO
	out PORT_OUT, r26
	sbi PORT_OUT, SPI_CLK
	bst r25, 2					; Bit 2
	bld r26, SPI_DO
	out PORT_OUT, r26
	sbi PORT_OUT, SPI_CLK
	bst r25, 1					; Bit 1
	bld r26, SPI_DO
	out PORT_OUT, r26
	sbi PORT_OUT, SPI_CLK
	bst r25, 0					; Bit 0
	bld r26, SPI_DO
	out PORT_OUT, r26
	sbi PORT_OUT, SPI_CLK
	inc r27						; r27++
	breq 2f						; r27 == 0?	
	rjmp 1b
	2:
	dec r18						; r18--
	breq 3f						; r18 == 0?	
	rjmp 1b
	3:
	sbi PORT_OUT, SPI_DO		; DO auf high
	call mmc_write_byte			; crc-Dummy schreiben	
	call mmc_write_byte
#if MMC_ASYNC_WRITE == 1
	tst r13						; wollten wir asynchron schreiben?
	brne 3f						; wenn ja, nicht auf busy warten
#endif	
	1:
	clr r28						; r28 = 0
	2:
	call mmc_read_byte			; warten, ob Karte noch busy
	cpi r24, lo8(-1)
	breq 3f						; == 0xff?
	dec r28						; r28--
	brne 2b						; r28 == 0?
	dec r27						; r27--
	brne 1b						; r27 == 0?
	rjmp 5f						; Timeout :(
	3:
	ldi r16, 0					; Alles ok, also Return-Code 0 laden :)
	rjmp 6f
	4:
	ldi r16, 1					; Fehler, Return-Code 1 laden
	sts mmc_init_state, r16		; Initstatus auf Fehler setzen	
	rjmp 6f
	5:
	ldi r16, 2					; Fehler, Return-Code 2 laden
	sts mmc_init_state, r16		; Initstatus auf Fehler setzen		
	6:
	ldi r24, lo8(ENA_MMC)		; Karte inaktiv schalten
	call ENA_off
	mov	r24, r16				; Return-Code laden	
	ldi r25, 0
	pop r29						; allgemeine Register wiederherstellen
	pop r28
	pop r16
	pop r15
	pop r14
	pop r13
	ret 

/*!
 * Liest einen Block von der Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param Buffer 	Puffer von mindestens 512 Byte
 * @return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich oder Timeout vor / nach Kommando 17
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			17.11.2006
 */	
.global	mmc_read_sector
	.type mmc_read_sector, @function
mmc_read_sector:
	push r14					; allgemeine Register retten
	push r15
	push r16
	push r28
	push r29
	movw r14, r22				; Byte 1 und 2 von Parameter addr sichern
	mov r16, r24				; Byte 3 von Parameter addr sichern
	movw r28, r20				; Parameter Buffer sichern
	call mmc_enable				; Karte aktiv schalten
	tst r24
	breq 1f
	rjmp 5f						; Karte kann nicht initialisiert werden => Fehler
	1:
	lsl r14						; Block in Bytes umrechnen
	rol r15
	rol r16
#if MMC_ASYNC_WRITE == 1	
	clr r27						; r27 = 0
	1:
	clr r26						; r26 = 0
	2:
	call mmc_read_byte			; warten, ob Karte noch busy
	cpi r24, lo8(-1)
	breq 3f						; == 0xff?
	dec r26						; r26--
	brne 2b						; r26 == 0?
	dec r27						; r27--
	brne 1b						; r27 == 0?
	rjmp 5f						; Timeout :(	
	3:
#endif
	ldi r24, 81					; Kommando 17 zum Lesen eines Blocks senden, Byte 0	
	call mmc_write_byte
	mov r24, r16				; Byte 1
	call mmc_write_byte
	mov r24, r15				; Byte 2
	call mmc_write_byte
	mov r24, r14				; Byte 3
	call mmc_write_byte
	ldi r24, 0					; Byte 4	
	call mmc_write_byte
	ldi r24,lo8(-1)				; Byte 5 (CRC)	
	call mmc_write_byte
	ldi r16, lo8(-1)			; Timeout bei 256 
	1:
	call mmc_read_byte
	cpi r24, lo8(-2)			; 0xfe = Startbyte	
	breq 2f
	call mmc_read_byte			; Noch ein Versuch
	cpi r24, lo8(-2)	
	breq 2f	
	dec r16 
	brne 1b
	rjmp 4f						; Timeout, also abbrechen :(
	2:								
	in	r20, PORT_OUT			; MMC_PORT_OUT einlesen
	cbr r20, SPI_CLK_MASK		; CLK low
	out PORT_OUT, r20			; CLK auf low
	mov r19, r20				; CLK high
	sbr r19, SPI_CLK_MASK
	ldi r18, 2					; Schleifenzaehler laden
	clr r24		
	1:								
	in r26, PORT_IN				; Bit 7 lesen
	out PORT_OUT, r19			; CLK Flanke
	out PORT_OUT, r20
	bst r26, SPI_DI				; Bit 7 speichern
	bld r25, 7
	in r26, PORT_IN				; Bit 6
	out PORT_OUT, r19
	out PORT_OUT, r20
	bst r26, SPI_DI
	bld r25, 6
	in r26, PORT_IN				; Bit 5
	out PORT_OUT, r19
	out PORT_OUT ,r20
	bst r26, SPI_DI
	bld r25, 5
	in r26, PORT_IN				; Bit 4
	out PORT_OUT, r19
	out PORT_OUT, r20
	bst r26, SPI_DI
	bld r25, 4
	in r26, PORT_IN				; Bit 3
	out PORT_OUT, r19
	out PORT_OUT, r20
	bst r26, SPI_DI
	bld r25, 3
	in r26, PORT_IN				; Bit 2
	out PORT_OUT, r19
	out PORT_OUT, r20
	bst r26, SPI_DI
	bld r25, 2
	in r26, PORT_IN				; Bit 1
	out PORT_OUT, r19
	out PORT_OUT, r20
	bst r26, SPI_DI
	bld r25, 1
	in r26, PORT_IN				; Bit 0
	out PORT_OUT, r19
	out PORT_OUT, r20
	bst r26, SPI_DI
	bld r25, 0
	st Y+, r25					; Byte ins SRAM kopieren 
	inc r24						; r24++
	breq 2f						; r24 == 0?
	rjmp 1b
	2:
	dec r18						; r18--
	breq 3f						; r18 == 0?
	rjmp 1b
	3:
	out PORT_OUT, r19			; CLK auf high
	call mmc_read_byte			; crc-Dummy lesen
	call mmc_read_byte
	4:
	tst r16						; Abbruch durch Timeout?
	breq 5f						; r27 == 0?
	ldi r16, 0					; Alles ok, also Return-Code 0 laden :) 
	rjmp 6f
	5:
	ldi r16, 1					; Fehler, also Return-Code 1 laden
	sts mmc_init_state, r16		; Initstatus auf Fehler setzen	
	6:
	ldi r24, lo8(ENA_MMC)		; Karte inaktiv schalten
	call ENA_off
	mov r24, r16				; Return-Code laden
	ldi r25, 0
	pop r29						; allgemeine Register wiederherstellen
	pop r28
	pop r16
	pop r15
	pop r14
	ret
#endif	// MMC_AVAILABLE
#endif	// MCU
