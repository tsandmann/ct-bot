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
 * \file 	mmc-low.c
 * \brief 	Low-Level-Routinen zum Lesen/Schreiben einer MMC / SD-Card
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	14.11.2006
 */

#ifdef MCU
#include "ct-Bot.h"

#if defined MMC_AVAILABLE && ! defined SPI_AVAILABLE
#include "mmc.h"
#include "mmc-low.h"
#include "ena.h"
#include "led.h"
#include <avr/io.h>

/* Portkonfiguration */
#define PORT_OUT _SFR_IO_ADDR(MMC_PORT_OUT)
#define PORT_IN _SFR_IO_ADDR(MMC_PORT_IN)
#define DDR _SFR_IO_ADDR(MMC_DDR)

void mmc_write_byte(uint8_t data) __attribute__((noinline));
/**
 * Schreibt ein Byte an die Karte
 * \param data	Das zu sendende Byte
 */
void mmc_write_byte(uint8_t data) {
	uint8_t tmp;
	__asm__ __volatile__(
		"in %0, %2			; MMC_PORT_OUT einlesen	\n\t"
		"cbr %0, %5			; CLK auf low			\n\t"
		"bst %1, 7			; Bit 7	der Daten		\n\t"
		"bld %0, %3			; Daten nach DO			\n\t"
		"out %2, %0			; Daten senden			\n\t"
		"sbi %2, %4			; CLK auf high			\n\t"
		"bst %1, 6			; Bit 6	der Daten		\n\t"
		"bld %0, %3									\n\t"
		"out %2, %0									\n\t"
		"sbi %2, %4									\n\t"
		"bst %1, 5			; Bit 5	der Daten		\n\t"
		"bld %0, %3									\n\t"
		"out %2, %0									\n\t"
		"sbi %2, %4									\n\t"
		"bst %1, 4			; Bit 4	der Daten		\n\t"
		"bld %0, %3									\n\t"
		"out %2, %0									\n\t"
		"sbi %2, %4									\n\t"
		"bst %1, 3			; Bit 3	der Daten		\n\t"
		"bld %0, %3									\n\t"
		"out %2, %0									\n\t"
		"sbi %2, %4									\n\t"
		"bst %1, 2			; Bit 2	der Daten		\n\t"
		"bld %0, %3									\n\t"
		"out %2, %0									\n\t"
		"sbi %2, %4									\n\t"
		"bst %1, 1			; Bit 1	der Daten		\n\t"
		"bld %0, %3									\n\t"
		"out %2, %0									\n\t"
		"sbi %2, %4									\n\t"
		"bst %1, 0			; Bit 0 der Daten		\n\t"
		"bld %0, %3									\n\t"
		"out %2, %0									\n\t"
		"sbi %2, %4									\n\t"
		"sbi %2, %3			; DO auf high			    "
		: "=&r" (tmp) /* %0 */
		: "r" (data) /* %1 */, "M" (PORT_OUT) /* %2 */, "M" (SPI_DO) /* %3 */, "M" (SPI_CLK) /* %4 */, "M" (_BV(SPI_CLK)) /* %5 */
		: "memory"
	);
}


uint8_t mmc_read_byte(void) __attribute__((noinline));
/**
 * Liest ein Byte von der Karte
 * \return Das gelesene Byte
 */
uint8_t mmc_read_byte(void) {
	uint8_t clk_high, clk_low, port_data, data;
	__asm__ __volatile__(
		"in	%2, %4		; MMC_PORT_OUT einlesen	\n\t"
		"cbr %2, %7		; CLK low				\n\t"
		"out %4, %2		; CLK auf low			\n\t"
		"mov %1, %2		; CLK high				\n\t"
		"sbr %1, %7								\n\t"
		"in %3, %5		; Bit 7 lesen			\n\t"
		"out %4, %1		; CLK Flanke			\n\t"
		"out %4, %2								\n\t"
		"bst %3, %6		; Bit 7 speichern		\n\t"
		"bld %0, 7								\n\t"
		"in %3, %5		; Bit 6					\n\t"
		"out %4, %1								\n\t"
		"out %4, %2								\n\t"
		"bst %3, %6								\n\t"
		"bld %0, 6								\n\t"
		"in %3, %5		; Bit 5					\n\t"
		"out %4, %1								\n\t"
		"out %4, %2								\n\t"
		"bst %3, %6								\n\t"
		"bld %0, 5								\n\t"
		"in %3, %5		; Bit 4					\n\t"
		"out %4, %1								\n\t"
		"out %4, %2								\n\t"
		"bst %3, %6								\n\t"
		"bld %0, 4								\n\t"
		"in %3, %5		; Bit 3					\n\t"
		"out %4, %1								\n\t"
		"out %4, %2								\n\t"
		"bst %3, %6								\n\t"
		"bld %0, 3								\n\t"
		"in %3, %5		; Bit 2					\n\t"
		"out %4, %1								\n\t"
		"out %4, %2								\n\t"
		"bst %3, %6								\n\t"
		"bld %0, 2								\n\t"
		"in %3, %5		; Bit 1					\n\t"
		"out %4, %1								\n\t"
		"out %4, %2								\n\t"
		"bst %3, %6								\n\t"
		"bld %0, 1								\n\t"
		"in %3, %5		; Bit 0					\n\t"
		"out %4, %1								\n\t"
		"bst %3, %6								\n\t"
		"bld %0, 0								    "
		: "=&r" (data) /* %0 */, "=&r" (clk_high) /* %1 */, "=&r" (clk_low) /* %2 */, "=&r" (port_data) /* %3 */
		: "M" (PORT_OUT) /* %4 */, "M" (PORT_IN) /* %5 */, "M" (SPI_DI) /* %6 */, "M" (_BV(SPI_CLK)) /* %7 */
		: "memory"
	);

	return data;
}

/**
 * Schreibt einen 512-Byte Sektor auf die Karte
 * \param addr 		Nummer des 512-Byte Blocks (r22-r25)
 * \param buffer 	Zeiger auf den Puffer (r20/r21)
 * \return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich oder Timeout vor / nach Kommando 24, 2 wenn Timeout bei busy
 */
uint8_t mmc_write_sector(uint32_t addr, void * buffer) {
	(void) addr;
	(void) buffer;

	register uint8_t result asm("r24");
	__asm__ __volatile__(
		"push r13					; allgemeine Register retten				\n\t"
		"push r14																\n\t"
		"push r15																\n\t"
		"push r16																\n\t"
		"push r28																\n\t"
		"push r29																\n\t"
		"movw r14, r22				; Byte 1 und 2 von Parameter addr sichern	\n\t"
		"mov r16, r24				; Byte 3 von Parameter addr sichern  		\n\t"
		"movw r28, r20				; Parameter Buffer sichern					\n\t"
#ifdef OS_AVAILABLE
		"sts os_scheduling_allowed, r1	; os_enterCS()							\n\t"
#endif
#if defined LED_AVAILABLE && ! defined TEST_AVAILABLE
		"lds r24, led															\n\t"
		"ori r24, %0															\n\t"
		"sts led, r24															\n\t"
		"call LED_set				; rote LED an								\n\t"
#endif // LED_AVAILABLE && ! TEST_AVAILABLE
		"call mmc_enable			; Karte aktiv schalten						\n\t"
		"tst r24																\n\t"
		"breq 1f																\n\t"
		"rjmp 4f					; Karte kann nicht initialisiert werden => Fehler		\n\t"
		"1:																					\n\t"
		"lsl r14					; Block in Bytes umrechnen								\n\t"
		"rol r15																			\n\t"
		"rol r16																			\n\t"
		"ldi r24, 88				; Kommando 24 zum Schreiben eines Blocks senden, Byte 0	\n\t"
		"call mmc_write_byte																\n\t"
		"mov r24, r16				; Byte 1		\n\t"
		"call mmc_write_byte						\n\t"
		"mov r24, r15				; Byte 2		\n\t"
		"call mmc_write_byte						\n\t"
		"mov r24, r14				; Byte 3		\n\t"
		"call mmc_write_byte						\n\t"
		"ldi r24, 0					; Byte 4		\n\t"
		"call mmc_write_byte						\n\t"
		"ldi r24, lo8(-1)			; Byte 5 (CRC)	\n\t"
		"call mmc_write_byte						\n\t"
		"clr r13					; Timeout nach 256 Versuchen	\n\t"
		"1:															\n\t"
		"call mmc_read_byte											\n\t"
		"cpi r24, lo8(-1)			; !0xff = Antwort				\n\t"
		"brne 3f													\n\t"
#ifdef OS_AVAILABLE
		"call os_exitCS 				; os_exitCS()				\n\t"
		"sts os_scheduling_allowed, r1	; os_enterCS()				\n\t"
#endif
		"dec r13				; r13--		\n\t"
		"brne 1b							\n\t"
		"3:									\n\t"
		"tst r13	 			; Fehler oder Abbruch durch Timeout?	\n\t"
		"brne 1f 												\n\t"
		"rjmp 4f 				; Return-Code 1					\n\t"
		"1:														\n\t"
		"ldi r24, lo8(-2)		; Start-Byte an Karte senden	\n\t"
		"call mmc_write_byte							\n\t"
		"in r26, %1				; MMC_PORT_OUT einlesen	\n\t"
		"cbr r26, %2			; CLK auf low			\n\t"
		"ldi r18, 2				; r18 = 2				\n\t"
		"clr r27				; r27 = 0				\n\t"
		"1:												\n\t"
		"ld r25, Y+				; Byte aus SRAM lesen	\n\t"
		"bst r25, 7				; Bit 7					\n\t"
		"bld r26, %3							\n\t"
		"out %1, r26			; Daten senden	\n\t"
		"sbi %1, %4				; CLK auf high	\n\t"
		"bst r25, 6				; Bit 6	\n\t"
		"bld r26, %3					\n\t"
		"out %1, r26					\n\t"
		"sbi %1, %4						\n\t"
		"bst r25, 5				; Bit 5	\n\t"
		"bld r26, %3					\n\t"
		"out %1, r26					\n\t"
		"sbi %1, %4						\n\t"
		"bst r25, 4				; Bit 4	\n\t"
		"bld r26, %3					\n\t"
		"out %1, r26					\n\t"
		"sbi %1, %4						\n\t"
		"bst r25, 3				; Bit 3	\n\t"
		"bld r26, %3					\n\t"
		"out %1, r26					\n\t"
		"sbi %1, %4						\n\t"
		"bst r25, 2				; Bit 2	\n\t"
		"bld r26, %3					\n\t"
		"out %1, r26					\n\t"
		"sbi %1, %4						\n\t"
		"bst r25, 1				; Bit 1	\n\t"
		"bld r26, %3					\n\t"
		"out %1, r26					\n\t"
		"sbi %1, %4						\n\t"
		"bst r25, 0				; Bit 0	\n\t"
		"bld r26, %3					\n\t"
		"out %1, r26					\n\t"
		"sbi %1, %4						\n\t"
		"inc r27				; r27++		\n\t"
		"breq 2f				; r27 == 0?	\n\t"
		"rjmp 1b							\n\t"
		"2:									\n\t"
		"dec r18				; r18--		\n\t"
		"breq 3f				; r18 == 0?	\n\t"
		"rjmp 1b							\n\t"
		"3:									\n\t"
		"sbi %1, %3		; DO auf high		\n\t"
		"call mmc_write_byte		; crc-Dummy schreiben	\n\t"
		"call mmc_write_byte					\n\t"
		"clr r13					; r13 = 0	\n\t"
		"1:										\n\t"
		"clr r28					; r28 = 0	\n\t"
		"2:										\n\t"
		"call mmc_read_byte			; warten, ob Karte noch busy	\n\t"
		"cpi r24, lo8(-1)						\n\t"
		"breq 3f					; == 0xff?	\n\t"
#ifdef OS_AVAILABLE
		"call os_exitCS 				; os_exitCS()	\n\t"
		"sts os_scheduling_allowed, r1	; os_enterCS()	\n\t"
#endif
		"dec r28						; r28--			\n\t"
		"brne 2b						; r28 == 0?		\n\t"
		"dec r13						; r13--			\n\t"
		"brne 1b						; r13 == 0?		\n\t"
		"rjmp 5f						; Timeout :(	\n\t"
		"3:												\n\t"
		"ldi r16, 0						; Alles ok, also Return-Code 0 laden :)	\n\t"
		"rjmp 6f														\n\t"
		"4:																\n\t"
		"ldi r16, 1						; Fehler, Return-Code 1 laden	\n\t"
		"sts mmc_init_state, r16		; Initstatus auf Fehler setzen	\n\t"
		"rjmp 6f														\n\t"
		"5:																\n\t"
		"ldi r16, 2						; Fehler, Return-Code 2 laden	\n\t"
		"sts mmc_init_state, r16		; Initstatus auf Fehler setzen	\n\t"
		"6:																\n\t"
		"ldi r24, lo8(%5)				; Karte inaktiv schalten	\n\t"
		"call ENA_off												\n\t"
#if defined LED_AVAILABLE && ! defined TEST_AVAILABLE
		"lds r24, led									\n\t"
		"andi r24, ~%0									\n\t"
		"sts led, r24									\n\t"
		"call LED_set					; rote LED aus	\n\t"
#endif // LED_AVAILABLE && ! TEST_AVAILABLE
#ifdef OS_AVAILABLE
		"call os_exitCS 				; os_exitCS()		\n\t"
#endif
		"mov	r24, r16				; Return-Code laden	\n\t"
		"ldi r25, 0											\n\t"
		"pop r29						; allgemeine Register wiederherstellen	\n\t"
		"pop r28	\n\t"
		"pop r16	\n\t"
		"pop r15	\n\t"
		"pop r14	\n\t"
		"pop r13	\n\t"
		"ret		    "
		:: "M" (LED_ROT) /* %0 */, "M" (PORT_OUT) /* %1 */, "M" (_BV(SPI_CLK)) /* %2 */, "M" (SPI_DO) /* %3 */,
		 "M" (SPI_CLK) /* %4 */, "M" (ENA_MMC) /* %5 */
		: "memory"
	);

	return result;
}

/**
 * Liest einen Block von der Karte
 * \param addr 		Nummer des 512-Byte Blocks (r22-r25)
 * \param *buffer 	Zeiger auf Puffer von mindestens 512 Byte (r20/r21)
 * \return 			0 wenn alles ok ist, 1 wenn Init nicht moeglich oder Timeout vor / nach Kommando 17
 */
uint8_t mmc_read_sector(uint32_t addr, void * buffer) {
	(void) addr;
	(void) buffer;

	register uint8_t result asm("r24");
	__asm__ __volatile__(
		"push r14					; allgemeine Register retten	\n\t"
		"push r15	\n\t"
		"push r16	\n\t"
		"push r28	\n\t"
		"push r29	\n\t"
		"movw r14, r22				; Byte 1 und 2 von Parameter addr sichern	\n\t"
		"mov r16, r24				; Byte 3 von Parameter addr sichern			\n\t"
		"movw r28, r20				; Parameter Buffer sichern					\n\t"
	#ifdef OS_AVAILABLE
		"sts os_scheduling_allowed, r1	; os_enterCS()	\n\t"
	#endif
	#if defined LED_AVAILABLE && ! defined TEST_AVAILABLE
		"lds r24, led									\n\t"
		"ori r24, %0									\n\t"
		"sts led, r24									\n\t"
		"call LED_set				; gruene LED an		\n\t"
	#endif // LED_AVAILABLE && ! TEST_AVAILABLE
		"call mmc_enable			; Karte aktiv schalten	\n\t"
		"tst r24											\n\t"
		"breq 1f											\n\t"
		"rjmp 5f					; Karte kann nicht initialisiert werden => Fehler	\n\t"
		"1:														\n\t"
		"lsl r14					; Block in Bytes umrechnen	\n\t"
		"rol r15												\n\t"
		"rol r16												\n\t"
		"ldi r24, 81				; Kommando 17 zum Lesen eines Blocks senden, Byte 0	\n\t"
		"call mmc_write_byte					\n\t"
		"mov r24, r16				; Byte 1	\n\t"
		"call mmc_write_byte					\n\t"
		"mov r24, r15				; Byte 2	\n\t"
		"call mmc_write_byte					\n\t"
		"mov r24, r14				; Byte 3	\n\t"
		"call mmc_write_byte					\n\t"
		"ldi r24, 0					; Byte 4	\n\t"
		"call mmc_write_byte					\n\t"
		"ldi r24,lo8(-1)			; Byte 5 (CRC)	\n\t"
		"call mmc_write_byte						\n\t"
		"ldi r16, lo8(-1)			; Timeout bei 256 	\n\t"
		"1:												\n\t"
	#ifdef OS_AVAILABLE
		"sts os_scheduling_allowed, r1	; os_enterCS()	\n\t"
	#endif
		"call mmc_read_byte								\n\t"
		"cpi r24, lo8(-2)			; 0xfe = Startbyte	\n\t"
		"breq 2f										\n\t"
		"call mmc_read_byte			; Noch ein Versuch	\n\t"
		"cpi r24, lo8(-2)								\n\t"
		"breq 2f										\n\t"
	#ifdef OS_AVAILABLE
		"call os_exitCS				; os_ExitCS()		\n\t"
	#endif
		"dec r16 										\n\t"
		"brne 1b										\n\t"
		"rjmp 4f				; Timeout, also abbrechen :(	\n\t"
		"2:												\n\t"
		"in	r20, %1				; MMC_PORT_OUT einlesen	\n\t"
		"cbr r20, %2			; CLK low		\n\t"
		"out %1, r20			; CLK auf low	\n\t"
		"mov r19, r20			; CLK high		\n\t"
		"sbr r19, %2							\n\t"
		"ldi r18, 2				; Schleifenzaehler laden	\n\t"
		"clr r24									\n\t"
		"1:											\n\t"
		"in r26, %4				; Bit 7 lesen		\n\t"
		"out %1, r19			; CLK Flanke		\n\t"
		"out %1, r20								\n\t"
		"bst r26, %3			; Bit 7 speichern	\n\t"
		"bld r25, 7						\n\t"
		"in r26, %4				; Bit 6	\n\t"
		"out %1, r19					\n\t"
		"out %1, r20					\n\t"
		"bst r26, %3					\n\t"
		"bld r25, 6						\n\t"
		"in r26, %4				; Bit 5	\n\t"
		"out %1, r19					\n\t"
		"out %1 ,r20					\n\t"
		"bst r26, %3					\n\t"
		"bld r25, 5						\n\t"
		"in r26, %4				; Bit 4	\n\t"
		"out %1, r19					\n\t"
		"out %1, r20					\n\t"
		"bst r26, %3					\n\t"
		"bld r25, 4						\n\t"
		"in r26, %4				; Bit 3	\n\t"
		"out %1, r19					\n\t"
		"out %1, r20					\n\t"
		"bst r26, %3					\n\t"
		"bld r25, 3						\n\t"
		"in r26, %4				; Bit 2	\n\t"
		"out %1, r19					\n\t"
		"out %1, r20					\n\t"
		"bst r26, %3					\n\t"
		"bld r25, 2						\n\t"
		"in r26, %4				; Bit 1	\n\t"
		"out %1, r19					\n\t"
		"out %1, r20					\n\t"
		"bst r26, %3					\n\t"
		"bld r25, 1						\n\t"
		"in r26, %4				; Bit 0	\n\t"
		"out %1, r19					\n\t"
		"out %1, r20					\n\t"
		"bst r26, %3					\n\t"
		"bld r25, 0						\n\t"
		"st Y+, r25					; Byte ins SRAM kopieren 	\n\t"
		"inc r24					; r24++						\n\t"
		"breq 2f					; r24 == 0?					\n\t"
		"rjmp 1b												\n\t"
		"2:														\n\t"
		"dec r18					; r18--						\n\t"
		"breq 3f					; r18 == 0?					\n\t"
		"rjmp 1b												\n\t"
		"3:														\n\t"
		"out %1, r19				; CLK auf high				\n\t"
		"call mmc_read_byte			; crc-Dummy lesen			\n\t"
		"call mmc_read_byte										\n\t"
		"4:														\n\t"
		"tst r16					; Abbruch durch Timeout?	\n\t"
		"breq 5f					; r27 == 0?					\n\t"
		"ldi r16, 0					; Alles ok, also Return-Code 0 laden :)	\n\t"
		"rjmp 6f															\n\t"
		"5:																	\n\t"
		"ldi r16, 1					; Fehler, also Return-Code 1 laden		\n\t"
		"sts mmc_init_state, r16	; Initstatus auf Fehler setzen			\n\t"
		"6:																	\n\t"
		"ldi r24, lo8(%5)			; Karte inaktiv schalten				\n\t"
		"call ENA_off														\n\t"
	#if defined LED_AVAILABLE && ! defined TEST_AVAILABLE
		"lds r24, led			\n\t"
		"andi r24, ~%0			\n\t"
		"sts led, r24			\n\t"
		"call LED_set				; gruene LED aus	\n\t"
	#endif // LED_AVAILABLE && ! TEST_AVAILABLE
	#ifdef OS_AVAILABLE
		"call os_exitCS				; os_exitCS();		\n\t"
	#endif
		"mov r24, r16				; Return-Code laden	\n\t"
		"ldi r25, 0															\n\t"
		"pop r29					; allgemeine Register wiederherstellen	\n\t"
		"pop r28				\n\t"
		"pop r16				\n\t"
		"pop r15				\n\t"
		"pop r14				\n\t"
		"ret		 			    "
		:: "M" (LED_GRUEN) /* %0 */, "M" (PORT_OUT) /* %1 */, "M" (_BV(SPI_CLK)) /* %2 */, "M" (SPI_DI) /* %3 */,
		 "M" (PORT_IN) /* %4 */, "M" (ENA_MMC) /* %5 */
		: "memory"
	);

	return result;
}

#endif // MMC_AVAILABLE && ! SPI_AVAILABLE
#endif // MCU

