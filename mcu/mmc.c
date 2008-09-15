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
 * @file 	mcu/mmc.c
 * @brief 	Routinen zum Auslesen/Schreiben einer MMC-Karte
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @author	Timo Sandmann (mail@timosandmann.de)
 * @date 	07.11.06
 */


/* Die (zeitkritischen) Low-Level-Funktionen read_ und write_byte bzw. _sector liegen jetzt
 * in mmc-low.S. Der Assembler-Code ist a) wesentlich schneller und macht b) das Timing
 * unabhaengig vom verwendeten Compiler.
 * Die Portkonfiguration findet sich in mmc-low.h.
 *
 * Die MMC kann auf zwei Weisen angesprochen werden:
 * Entweder per Software-Steuerung (das ist die Standard-Einstellung), dafuer muss
 * SPI_AVAILABLE in ct-Bot.h AUS sein.
 * Oder per Hardware-SPI-Steuerung, dafuer ist ein kleiner Hardware-Umbau noetig, man
 * muss die Verbindung zwischen PC5 und dem Display trennen (busy-Leitung wird vom Display-
 * Treiber eh nicht genutzt) und auf PC5 den linken Radencoder legen. Außerdem ist PB4
 * vom Radencoder zu trennen. Der PB4-Pin kann fuer andere Zwecke genutzt werden, er muss
 * jedoch immer als OUTPUT konfiguriert sein. Schalten man nun in ct-Bot.h SPI_AVAILABLE
 * AN, dann wird die Kommunikation mit der MMC per Hardware gesteuert - Vorteil ist eine
 * hoehere Transfer-Geschwindigkeit zur MMC (Faktor 2) und es sind 530 Byte weniger im
 * Flash belegt.
 */

#include "ct-Bot.h"

#ifdef MCU
#ifdef MMC_AVAILABLE

#include "mmc.h"
#include "spi.h"
#include "ena.h"
#include "timer.h"
#include "display.h"
#include "led.h"
#include "map.h"
#include "ui/available_screens.h"
#include "mmc-low.h"
#include "mmc-vm.h"
#include "os_thread.h"
#include "log.h"
#include <stdlib.h>

uint8_t mmc_init_state = 1;	/*!< Initialierungsstatus der Karte, 0: ok, 1: Fehler  */

/*!
 * Checkt die Initialisierung der Karte
 * @return	0, wenn initialisiert
 */
uint8_t mmc_get_init_state(void) {
	return mmc_init_state;
}

#ifndef SPI_AVAILABLE
/*!
 * Schreibt ein Byte an die Karte
 * @param data	Das zu sendende Byte
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		14.11.2006
 * @see			mmc-low.S
 */
void mmc_write_byte(uint8_t data);

/*!
 * Liest ein Byte von der Karte
 * @return		Das gelesene Byte
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		14.11.2006
 * @see			mmc-low.S
 */
uint8_t mmc_read_byte(void);

static uint8_t mmc_write_command(uint8_t * cmd);

/*!
 * Liest einen Block von der Karte
 * @param cmd 		Zeiger auf das Kommando, das erstmal an die Karte geht
 * @param *buffer 	Zeiger auf Puffer mit mindestens count Bytes
 * @param count 	Anzahl der zu lesenden Bytes
 */
static uint8_t mmc_read_block(uint8_t * cmd, void * buffer, uint16_t count) {
	os_enterCS();
	/* Kommando cmd an MMC/SD-Karte senden */
	if (mmc_write_command(cmd) != 0) {
		mmc_init_state = 1;
		os_exitCS();
		return 1;
	}

	#ifdef LED_AVAILABLE
	#ifndef TEST_AVAILABLE
		LED_on(LED_GRUEN);
	#endif	// TEST_AVAILABLE
	#endif	// LED_AVAILABLE
	os_exitCS();
	/* Warten auf Start Byte von der MMC/SD-Karte (FEh/Start Byte) */
	uint8_t timeout = 0;
	uint8_t result = 0;
	while (result != 0xfe) {
		os_enterCS();
		result = mmc_read_byte();
		os_exitCS();
		if (--timeout == 0) break;
	}

	os_enterCS();
	uint16_t i;
	uint8_t * ptr = buffer;
	/* Lesen des Blocks (max 512 Bytes) von MMC/SD-Karte */
	for (i=0; i<count; i++) {
		*ptr++ = mmc_read_byte();
	}

	/* CRC-Byte auslesen */
	mmc_read_byte();	//CRC - Byte wird nicht ausgewertet
	mmc_read_byte();	//CRC - Byte wird nicht ausgewertet

	/* MMC/SD-Karte inaktiv schalten */
	ENA_off(ENA_MMC);
	#ifdef LED_AVAILABLE
	#ifndef TEST_AVAILABLE
		LED_off(LED_GRUEN);
	#endif	// TEST_AVAILABLE
	#endif	// LED_AVAILABLE
	os_exitCS();
	if (timeout == 0) {
		mmc_init_state = 1;
		return 1;	// Abbruch durch Timeout
	}

	return 0;	// alles ok
}
#else

#define mmc_read_byte	SPI_MasterReceive		/*!< read_byte per SPI */
#define mmc_write_byte	SPI_MasterTransmit		/*!< write_byte per SPI */

/*
 * schaltet die MMC aktiv und initialisiert sie, falls noetig
 * @return	0: alles ok, Fehler von mmc_init() sonst
 */
uint8_t mmc_enable(void) {
	uint8_t result;
	if (mmc_init_state != 0 && (result=mmc_init()) != 0) return result;
	os_enterCS();
	ENA_off(ENA_MMC);
	SPI_MasterTransmit(-1);
	ENA_on(ENA_MMC);
	os_exitCS();
	return 0;
}

/*!
 * wartet, bis MMC mit "data" antwortet oder der Timeout (2^16 Versuche) zuschlaegt
 * @param data	Das erwartete Antwortbyte
 * @return		Rest vom Timeout oder 0 im Fehlerfall
 */
static uint16_t wait_for_byte(uint8_t data) {
	uint16_t timeout = 0xffff;
	/* warten, bis Karte mit "data" antwortet */
	uint8_t response = 0;
	do {
		SPDR = 0xff;
		if (--timeout == 0) break;
		while(!(SPSR & (1<<SPIF))) {}
		response = SPDR;
	} while (response != data);
	return timeout;
}

/*!
 * bereitet Lesen oder Schreiben eines Blocks vor
 * @param cmd	1. Byte des Kommandos an die Karte
 * @param addr	Adresse des Blocks in Byte
 * @return		0: alles ok, sonst Fehlercode
 */
static uint8_t prepare_transfer_spi(uint8_t cmd, uint32_t addr) {
	/* Init-Check */
	uint8_t result = mmc_enable();
	if (result != 0) {
		return result;
	}
#ifdef MAUS_AVAILABLE
	os_enterCS();
#endif

	/* (addr <<= 1) & 0x00ff ffff */
	asm volatile(
		"lsl %A0		\n\t"
		"rol %B0		\n\t"
		"rol %C0			"
		:	"=r"	(addr)
		:	"0"		(addr)
	);

	/* Kommando senden */
	SPI_MasterTransmit(cmd);		// 0x51: lesen, 0x schreiben
	typedef union {
		uint32_t u32;
		uint8_t u8[4];
	} mmc_addr_t;

	mmc_addr_t tmp;
	tmp.u32 = addr;
	SPI_MasterTransmit(tmp.u8[2]);	// Byteadresse des Blocks ("big-endian")
	SPI_MasterTransmit(tmp.u8[1]);
	SPI_MasterTransmit(tmp.u8[0]);
	SPI_MasterTransmit(0x00);
	SPI_MasterTransmit(0xff);		// CRC
#ifdef MAUS_AVAILABLE
	os_exitCS();
#endif
	return 0;
}

/*!
 * Liest einen Block von der Karte
 * @param cmd		Kommando zum Lesen (0x51 fuer 512-Byte Block)
 * @param addr 		Adresse des Blocks
 * @param *buffer 	Zeiger auf Puffer fuer die Daten
 * @return 			0 wenn alles ok ist, 1 wenn prepare_transfer_spi() Fehler meldet, 3 bei Timeout
 */
uint8_t mmc_read_sector_spi(uint8_t cmd, uint32_t addr, void * buffer) {
	/* cmd[] = {0x51,addr,addr,addr,0x00,0xFF} */
	if (prepare_transfer_spi(cmd, addr) != 0) {
		return 1;
	}

#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
	os_enterCS();
	LED_on(LED_GRUEN);
	os_exitCS();
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE

	/* Warten auf Start-Byte von der MMC/SD-Karte (0xfe == Start-Byte) */
	if (wait_for_byte(0xfe) == 0) {
		mmc_init_state = 1;
		return 3;	// Abbruch durch Timeout
	}

#ifdef MAUS_AVAILABLE
	os_enterCS();
#endif
	/* Lesen des Blocks (512 Bytes) von MMC/SD-Karte */
#ifdef MMC_AGGRESSIVE_OPTIMIZATION
	SPDR = 0;	// start 1st SPI-transfer
	int16_t i = cmd != 0x51 ? 15 : 511;		// num of bytes to read
	uint8_t tmp;
	asm volatile(
		"%=:						\n\t"
		"adiw %2,1	 	; 2 nop		\n\t"	// wait 16 cycles for reception complete
		"sbiw %2,1		; 2 nop		\n\t"
		"adiw %2,1		; 2 nop		\n\t"
		"sbiw %2,1		; 2 nop		\n\t"
		"nop			; 1 nop		\n\t"
		"in %0,%3		; tmp		\n\t"	// load from SPDR
		"out %3,__zero_reg__		\n\t"	// start next SPI-transfer
		"st Z+,%0	 	; tmp		\n\t"	// save to *buffer
		"sbiw %A1,1		; i			\n\t"	// i--
		"sbrs %B1,7		; i			\n\t"	// i == 0?
		"rjmp %=b						"
		: "=&r" (tmp)
		: "w" (i), "z" (buffer), "M" (_SFR_IO_ADDR(SPDR))
		: "memory"
	);
#else
	uint8_t i,j,k;
	if (cmd != 0x51) {
		k = 248;
	} else {
		k = 0;
	}
	SPDR = 0;	// start SPI-transfer
	uint8_t * ptr = buffer;
	for (i=2; i>0; i--) {
		j = k;
		uint8_t tmp;
		do {
			while(!(SPSR & (1<<SPIF))) {}	// wait for reception complete
			tmp = SPDR;						// load from SPDR
			SPDR = 0;						// start next SPI-transfer
			*ptr = tmp;					// save to *buffer
			ptr++;
		} while (++j != 0);
	}
#endif	// MMC_AGGRESSIVE_OPTIMIZATION

	/* CRC-Bytes auslesen */
	SPI_MasterReceive();	// CRC-Byte wird nicht ausgewertet
	SPI_MasterReceive();	// CRC-Byte wird nicht ausgewertet

	os_enterCS();
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
	LED_off(LED_GRUEN);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
	os_exitCS();
	return 0;
}

/*!
 * Schreibt einen 512-Byte Sektor auf die Karte
 * @param addr 		Adresse des 512-Byte Blocks
 * @param *buffer 	Zeiger auf den Puffer
 * @return 			0 wenn alles ok ist, 1 wenn prepare_transfer_spi() Fehler meldet, 3 wenn Timeout bis zur Bestaetigung
 */
uint8_t mmc_write_sector_spi(uint32_t addr, void * buffer) {
	/* cmd[] = {0x58,addr,addr,addr,0x00,0xFF} */
	if (prepare_transfer_spi(0x58, addr) != 0) {
		return 1;
	}

#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
	os_enterCS();
	LED_on(LED_ROT);
	os_exitCS();
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE

#ifdef MAUS_AVAILABLE
	os_enterCS();
#endif
	/* Startbyte an MMC/SD-Karte senden */
	SPI_MasterTransmit(-2);

	/* Schreiben des Blocks (512 Bytes) auf MMC/SD-Karte */
#ifdef MMC_AGGRESSIVE_OPTIMIZATION
	int16_t i = 511;
	uint8_t tmp;
	asm volatile(
		"%=:						\n\t"
		"ld %0,Y	 	; tmp		\n\t"	// load from *buffer
		"out %3,%0					\n\t"	// start next SPI-transfer
		"adiw %2,1 		; 2 nop		\n\t"	// wait 16 cycles for reception complete
		"sbiw %2,1		; 2 nop		\n\t"
		"adiw %2,1		; 2 nop		\n\t"
		"sbiw %2,1		; 2 nop		\n\t"
		"adiw %2,1		; buffer++	\n\t"
		"sbiw %A1,1		; i			\n\t"
		"sbrs %B1,7		; i			\n\t"
		"rjmp %=b						"
		: "=&r" (tmp)
		: "w" (i), "y" (buffer), "M" (_SFR_IO_ADDR(SPDR))
		: "memory"
	);
	/* Wait for SPI ready */
	while (!(SPSR & (1<<SPIF))) {}
#else
	uint8_t i,j, tmp;
	uint8_t * ptr = buffer;
	tmp = *ptr;		// load from *buffer
	ptr++;
	for (i=2; i>0; i--) {
		j = 0;
		do {
			SPDR = tmp;		// save to SPDR
			tmp = *ptr;	// load from *buffer
			ptr++;
			j++;
			if (j == 0) {
				while(!(SPSR & (1<<SPIF))) {}
				break;		// exit inner loop
			}
			while(!(SPSR & (1<<SPIF))) {}	// wait for transmission to complete
		} while (1);
	}
#endif	// MMC_AGGRESSIVE_OPTIMIZATION

	/* CRC-Dummy schreiben */
	SPI_MasterTransmit(0xff);
	SPI_MasterTransmit(0xff);

	/* warten, bis Karte nicht mehr busy */
#ifdef MMC_AGGRESSIVE_OPTIMIZATION
#if 1
	uint16_t timeout;
	SPDR = 0xff;
	asm volatile(
		"ldi %A0,lo8(-1)	; timeout	\n\t"
		"ldi %B0,hi8(-1)	; timeout	\n\t"
		"1:								\n\t"
		"sbiw %0,1			; timeout--	\n\t"
		"breq 2f						\n\t"
		"adiw %0,1			; 2 nop		\n\t"
		"sbiw %0,1			; 2 nop		\n\t"
		"adiw %0,1			; 2 nop		\n\t"
		"sbiw %0,1			; 2 nop		\n\t"
		"nop				; 1 nop		\n\t"
		"ldi r25,lo8(-1)	; 1 nop		\n\t"
		"in r24,%1			;			\n\t"
		"out %1,r25			;			\n\t"
		"cpi r24,lo8(-1)	; == 0xff?	\n\t"
		"brne 1b						\n\t"
		"2:									"
		: "=&y"	(timeout)
		: "M" (_SFR_IO_ADDR(SPDR))
		: "r24", "r25"
	);
#else
	uint16_t timeout = 0xffff;
	/* warten, bis Karte mit "0xff" antwortet */
	uint8_t response = 0;
	SPDR = 0xff;
	do {
		if (--timeout == 0) break;
		while(!(SPSR & (1<<SPIF))) {}
		response = SPDR;
		SPDR = 0xff;
	} while (response != 0xff);
#endif
#else
	uint16_t timeout = wait_for_byte(0xff);
#endif	// MMC_AMMC_AGGRESSIVE_OPTIMIZATION

	os_enterCS();
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
	LED_off(LED_ROT);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
	os_exitCS();

	if (timeout == 0) {
		mmc_init_state = 1;
		return 3;
	}
	return 0;
}
#endif	// SPI_AVAILABLE

/*!
 * Schickt ein Kommando an die Karte
 * @param *cmd 	Ein Zeiger auf das Kommando
 * @return 		Die Antwort der Karte oder 0xFF im Fehlerfall
 */
static uint8_t mmc_write_command(uint8_t * cmd) {
	uint8_t result = 0xff;
	uint16_t timeout = 0;

	if (mmc_enable() != 0) {
		return 0xff; // MMC / SD-Card aktiv schalten
	}

	// sendet 6 Byte Kommando
	int8_t i;
	for (i=5; i>=0; i--) {
		// sendet 6 Byte Kommando zur MMC/SD-Karte
		mmc_write_byte(*cmd++);
	}

	// Wartet auf eine gueltige Antwort von der MMC/SD-Karte
	while (result == 0xff) {
		result = mmc_read_byte();
		if (timeout++ > MMC_TIMEOUT) {
			break; // Abbruch da die MMC/SD-Karte nicht antwortet
		}
	}

	return result;
}

/*!
 * Initialisiere die MMC/SD-Karte
 * @return	0 wenn allles ok, sonst Nummer des Kommandos bei dem abgebrochen wurde
 */
uint8_t mmc_init(void) {
	os_enterCS();
	mmc_init_state = 0;

#ifdef SPI_AVAILABLE
	SPI_MasterInit();
#else
	MMC_CLK_DDR |= _BV(SPI_CLK);
	MMC_DDR &= ~(1<<SPI_DI);
	MMC_DDR |= (1<<SPI_DO);
#endif
	ENA_on(ENA_MMC);
	ENA_off(ENA_MMC);

	/* MMC/SD-Karte in den SPI-Mode initialisieren */
	int8_t i;
	for (i=16; i>=0; i--) {
		// Sendet min 74+ Clocks an die MMC/SD-Karte
		mmc_write_byte(0xff);
	}

	/* Kommando CMD0 an MMC/SD-Karte senden */
	uint8_t cmd[] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };
	uint16_t timeout = 0;
	while (mmc_write_command(cmd) != 1) {
		if (timeout++ > MMC_TIMEOUT) {
			ENA_off(ENA_MMC);
			mmc_init_state = 1;
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
			LED_on(LED_TUERKIS);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
			os_exitCS();
			return 1; // Abbruch bei Kommando 1 (Return Code 1)
		}
	}

	/* Kommando CMD1 an MMC/SD-Karte senden */
	timeout = 0;
	cmd[0] = 0x41; // Kommando 1
	cmd[5] = 0xFF; // CRC
	while (mmc_write_command(cmd) != 0) {
		if (timeout++ > 6 * MMC_TIMEOUT) {
			ENA_off(ENA_MMC);
			mmc_init_state = 1;
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
			LED_on(LED_TUERKIS);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
			os_exitCS();
			return 2; // Abbruch bei Kommando 2 (Return Code 2)
		}
	}

#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
	LED_off(LED_TUERKIS);
#endif	// TEST_AVAILABLE
#endif	// LED_AVAILABLE
	os_exitCS();
	return 0;
}

#ifdef MMC_INFO_AVAILABLE
#ifdef DISPLAY_MMC_INFO
/*!
 * Liest das CID-Register (16 Byte) von der Karte
 * @param *buffer	Zeiger auf Puffer von mindestens 16 Byte
 */
void mmc_read_cid(void * buffer) {
	uint8_t cmd[] = {0x4A,0x00,0x00,0x00,0x00,0xFF};	// Kommando zum Lesen des CID Registers
	mmc_read_block(cmd, buffer, 16);
}
#endif	// DISPLAY_MMC_INFO

/*!
 * Liest das CSD-Register (16 Byte) von der Karte
 * @param *buffer	Zeiger auf Puffer von mindestens 16 Byte
 */
void mmc_read_csd(void * buffer) {
	uint8_t cmd[] = {0x49,0x00,0x00,0x00,0x00,0xFF};	// Kommando zum lesen des CSD Registers
	mmc_read_block(cmd, buffer, 16);
}

/*!
 * Liefert die Groesse der Karte zurueck
 * @return	Groesse der Karte in Byte. Bei einer 4 GByte-Karte kommt 0xFFFFFFFF zurueck
 */
uint32_t mmc_get_size(void) {
	uint8_t csd[16];

	mmc_read_csd(csd);

	uint32_t size = (csd[8]>>6) + (csd[7] << 2) + ((csd[6] & 0x03) << 10); // c_size
	size +=1;		// Fest in der Formel drin

	uint8_t shift = 2;	// eine 2 ist fest in der Formel drin
	shift += (csd[10]>>7) + ((csd[9] & 0x03) <<1); // c_size_mult beruecksichtigen
	shift += csd[5] & 0x0f;	// Blockgroesse beruecksichtigen

	size = size << shift;

	return size;
}

#endif // MMC_INFO_AVAILABLE

#ifdef MMC_WRITE_TEST_AVAILABLE
/*! Testet die MMC-Karte. Schreibt nacheinander 2 Sektoren a 512 Byte mit Testdaten voll und liest sie wieder aus
 * !!! Achtung loescht die Karte ab Sektor 0x20000 (^= 64 MB)
 * @param *buffer	Zeiger auf einen 512 Byte grossen Puffer
 * @return 			0, wenn alles ok
 */
uint8 mmc_test(uint8_t * buffer) {
	static uint32_t sector = 0x20000;
	/* Initialisierung checken */
	if (mmc_init_state != 0 && mmc_init() != 0) {
		return 1;
	}
#ifdef MMC_VM_AVAILABLE	// Version mit virtuellen Aressen
	uint16 i;
	static uint16 pagefaults = 0;
	static uint16 old_pf;
	/* virtuelle Adressen holen */
	static uint32 v_addr1 = 0;
	static uint32 v_addr2 = 0;
	static uint32 v_addr3 = 0;
	static uint32 v_addr4 = 0;
	if (v_addr1 == 0) v_addr1 = mmcalloc(512, 1); // Testdaten 1
	if (v_addr2 == 0) v_addr2 = mmcalloc(512, 1); // Testdaten 2
	if (v_addr3 == 0) v_addr3 = mmcalloc(512, 1); // Dummy 1
	if (v_addr4 == 0) v_addr4 = mmcalloc(512, 1); // Dummy 2
	/* Zeitmessung starten */
	uint16 start_ticks=TIMER_GET_TICKCOUNT_16;
	uint8 start_reg=TCNT2;
	/* Pointer auf Puffer holen */
	uint8* p_addr = mmc_get_data(v_addr1); // Cache-Hit, CB 0
	if (p_addr == NULL) return 2;
	/* Testdaten schreiben */
	for (i=0; i<512; i++)
	p_addr[i] = (i & 0xff);
	/* Pointer auf zweiten Speicherbereich holen */
	p_addr = mmc_get_data(v_addr2); // Cache-Hit, CB 1
	if (p_addr == NULL) return 3;
	/* Testdaten Teil 2 schreiben */
	for (i=0; i<512; i++)
	p_addr[i] = 255 - (i & 0xff);
	/* kleiner LRU-Test */
	//			p_addr = mmc_get_data(v_addr1);	// Cache-Hit, CB 0
	//			p_addr = mmc_get_data(v_addr4);	// Cache-Miss, => CB 1
	//			p_addr = mmc_get_data(v_addr1);	// Cache-Hit, CB 0
	//			p_addr = mmc_get_data(v_addr3);	// Cache-Miss, => CB 1
	//			p_addr = mmc_get_data(v_addr1);	// Cache-Hit, CB 0
	//			p_addr = mmc_get_data(v_addr4);	// Cache-Miss, => CB 1
	/* Pointer auf Testdaten Teil 1 holen */
	p_addr = mmc_get_data(v_addr1); // Cache-Hit, CB 0
	if (p_addr == NULL) return 4;
	/* Testdaten 1 vergleichen */
	for (i=0; i<512; i++)
	if (p_addr[i] != (i & 0xff)) return 5;
	/* Pointer auf Testdaten Teil 2 holen */
	p_addr = mmc_get_data(v_addr2); // Cache-Miss, => CB 1
	if (p_addr == NULL) return 6;
	/* Testdaten 2 vergleichen */
	for (i=0; i<512; i++)
	if (p_addr[i] != (255 - (i & 0xff))) return 7;

	p_addr = mmc_get_data(v_addr4);
	/* Zeitmessung beenden */
	int8 timer_reg=TCNT2;
	uint16 end_ticks=TIMER_GET_TICKCOUNT_16;
	timer_reg -= start_reg;
#ifdef VM_STATS_AVAILABLE
	/* Pagefaults merken */
	old_pf = pagefaults;
	pagefaults = mmc_get_pagefaults();
#endif
	/* kleine Statistik ausgeben */
	display_cursor(3,1);
	display_printf("Pagefaults: %5u   ", pagefaults);
	display_cursor(4,1);
	display_printf("Bei %3u PF: %5u us", pagefaults - old_pf, (end_ticks-start_ticks)*176 + timer_reg*4);
#else	// alte Version
	uint16_t i;
	uint8_t result = 0;

	/* Zeitmessung */
	uint16_t start_ticks;
	uint8_t start_reg;
	int8_t timer_reg;
	uint16_t end_ticks;
	static uint32_t time_read = 0;
	static uint32_t time_write = 0;
	static uint32_t n = 0;

	// Puffer vorbereiten
	for (i = 0; i < 512; i++) {
		buffer[i] = (i & 0xFF);
	}

	/* Zeitmessung starten */
	start_ticks = TIMER_GET_TICKCOUNT_16;
	start_reg = TCNT2;

	// und schreiben
	result = mmc_write_sector(sector, buffer);

	/* Zeitmessung beenden */
	timer_reg = TCNT2;
	end_ticks = TIMER_GET_TICKCOUNT_16;
	timer_reg -= start_reg;
	time_write += (end_ticks - start_ticks) * 176 + timer_reg * 4;

	if (result != 0) {
		return result * 10 + 2;
	}

	// Puffer vorbereiten
	for (i = 0; i < 512; i++) {
		buffer[i] = 255 - (i & 0xFF);
	}

	/* Zeitmessung starten */
	start_ticks = TIMER_GET_TICKCOUNT_16;
	start_reg = TCNT2;

	// und schreiben
	result = mmc_write_sector(sector + 1, buffer);

	/* Zeitmessung beenden */
	timer_reg = TCNT2;
	end_ticks = TIMER_GET_TICKCOUNT_16;
	timer_reg -= start_reg;
	time_write += (end_ticks - start_ticks) * 176 + timer_reg * 4;

	if (result != 0) {
		return result * 10 + 3;
	}

	/* Zeitmessung starten */
	start_ticks = TIMER_GET_TICKCOUNT_16;
	start_reg = TCNT2;

	// Puffer lesen
	result = mmc_read_sector(sector++, buffer);

	/* Zeitmessung beenden */
	timer_reg = TCNT2;
	end_ticks = TIMER_GET_TICKCOUNT_16;
	timer_reg -= start_reg;
	time_read += (end_ticks - start_ticks) * 176 + timer_reg * 4;

	if (result != 0) {
		sector--;
		return result * 10 + 4;
	}

	// und vergleichen
	for (i = 0; i < 512; i++) {
		if (buffer[i] != (i & 0xFF)) {
			LOG_ERROR("i:%u\tread:0x%x\texpected:0x%x", i, buffer[i], i & 0xff);
			return 5;
		}
	}

	/* Zeitmessung starten */
	start_ticks = TIMER_GET_TICKCOUNT_16;
	start_reg = TCNT2;

	// Puffer lesen
	result = mmc_read_sector(sector++, buffer);

	/* Zeitmessung beenden */
	timer_reg = TCNT2;
	end_ticks = TIMER_GET_TICKCOUNT_16;
	timer_reg -= start_reg;
	time_read += (end_ticks - start_ticks) * 176 + timer_reg * 4;

	if (result != 0) {
		sector--;
		return result * 10 + 6;
	}
	// und vergleichen
	for (i = 0; i < 512; i++) {
		if (buffer[i] != (255 - (i & 0xFF))) {
			LOG_ERROR("i:%u\tread:0x%x\texpected:0x%x", i, buffer[i], 255 - (i & 0xff));
			return 7;
		}
	}

	/* kleine Statistik ausgeben */
	os_enterCS();
	display_cursor(3, 1);
	n += 2;
	display_printf("Dauer r/w%5u/%5u", (uint16_t)(time_read/n), (uint16_t)(time_write/n));
	display_cursor(4, 1);
	display_printf("Sektor: 0x%04x", (sector-2) >> 16);
	display_printf("%04x  ", (sector-2) & 0xffff);os_exitCS();
#endif	// MMC_VM_AVAILABLE
	// hierher kommen wir nur, wenn alles ok ist
	return 0;
}
#endif //MMC_WRITE_TEST_AVAILABLE

#ifdef DISPLAY_MMC_INFO
/*!
 * Zeigt die Daten der MMC-Karte an
 */
void mmc_display(void) {
#ifdef MMC_INFO_AVAILABLE
	static uint8_t mmc_state = 0xFF;

	uint8_t dummy = mmc_init();
	/* hat sich was geaendert? */
	if (dummy != mmc_state) {
		mmc_state = dummy;

		display_cursor(1, 1);
		if (mmc_state != 0) {
			display_clear();
			display_printf("MMC not init (%u)", mmc_state);
			return;
		}
		uint32_t size = mmc_get_size();
		display_printf("  MMC: %4u MByte ", size >> 20);

#ifndef MMC_WRITE_TEST_AVAILABLE
		uint8_t csd[16];
		mmc_read_csd(csd);
		display_cursor(3, 1);
		uint8_t i;
		for (i=0; i<16; i++) {
			if (i == 8) display_cursor(4, 1);
			if (i%2 == 0) display_printf(" ");
			display_printf("%02x", csd[i]);
		}
#endif	// MMC_WRITE_TEST_AVAILABLE
	}
#ifdef MMC_WRITE_TEST_AVAILABLE
	if (mmc_state == 0) {
		static uint8_t buffer[512];
		uint8_t result = mmc_test(buffer);
		if (result != 0) {
			display_cursor(3, 1);
			display_printf("mmc_test()=%u :(", result);
		}
	}
#endif	// MMC_WRITE_TEST_AVAILABLE
#else
#ifdef MMC_VM_AVAILABLE
#ifdef PC
	display_cursor(3, 1);
	display_printf("mmc_emu_test() = %u ", mmc_emu_test());
#endif	// PC
#endif	// MMC_VM_AVAILABLE
#endif	// MMC_INFO_AVAILABLE
}
#endif	// DISPLAY_MMC_INFO

#endif	// MMC_AVAILABLE
#endif	// MCU
