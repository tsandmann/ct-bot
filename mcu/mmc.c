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

/*! @file 	mmc.c
 * @brief 	Routinen zum Auslesen/Schreiben einer MMC-Karte
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @date 	07.11.06
 */


/* Die (zeitkritischen) Low-Level-Funktionen read_ und write_byte bzw. _sector liegen jetzt 
 * in mmc-low.S. Der Assembler-Code ist a) wesentlich schneller und macht b) das Timing
 * unabhaengig vom verwendeten Compiler. 
 * Die Portkonfiguration findet sich in mmc-low.h.
 */

//TODO:	* hier aufraeumen :P
//		* kleine Doku machen
//		* Timeouts in mmc_init() ueberdenken
//		* Unterstuetzung fuer Hardware-SPI wieder einbauen - geht aber eh net :/ 
//		* Wo nur CLK-Flanken gebraucht werden, auch nur CLK-Flanken erzeugen


#include "ct-Bot.h"

#ifdef MCU
#ifdef MMC_AVAILABLE

#include "mmc.h"
#include <avr/io.h>
#include "ena.h"
#include "timer.h"
#include "display.h"

#include <avr/interrupt.h>
#ifndef NEW_AVR_LIB
	#include <avr/signal.h>
#endif

#include "mmc-low.h"
#include "mmc-vm.h"
#include <stdlib.h>

#define MMC_Disable()	ENA_off(ENA_MMC);
#define MMC_Enable()	ENA_on(ENA_MMC);

#define MMC_prepare()	{ MMC_DDR &=~(1<<SPI_DI);	 MMC_DDR |= (1<<SPI_DO); } 

volatile uint8 mmc_init_state=1;	/*!< Initialierungsstatus der Karte, 0: ok, 1: Fehler  */

/*!
 * Checkt die Initialisierung der Karte
 * @return	0, wenn initialisiert
 */
inline uint8 mmc_get_init_state(void){
	return mmc_init_state;	
}

/*! 
 * Schreibt ein Byte an die Karte
 * @param data	Das zu sendende Byte
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		14.11.2006
 * @see			mmc-low.s
 */
void mmc_write_byte(uint8 data);


/*!
 * Liest ein Byte von der Karte
 * @return		Das gelesene Byte
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		14.11.2006
 * @see			mmc-low.s
 */
 uint8 mmc_read_byte(void);


/*!
 * Schickt ein Kommando an die Karte
 * @param cmd 	Ein Zeiger auf das Kommando
 * @return 		Die Antwort der Karte oder 0xFF im Fehlerfall
 */
uint8 mmc_write_command(uint8 *cmd){
	uint8 result = 0xff;
	uint16 Timeout = 0;

//	//set MMC_Chip_Select to high (MMC/SD-Karte Inaktiv) 
//	MMC_Enable();
//	MMC_Disable();
//	// Da ein paar Leitungen noch von anderer Hardware mitbenutzt werden: reinit
//	MMC_prepare();
//	// sendet 8 Clock Impulse
//	mmc_write_byte(0xFF);
//	// set MMC_Chip_Select to low (MMC/SD-Karte Aktiv)
//	MMC_Enable();

	// sendet 6 Byte Kommando
	uint8 i;
	for (i=0; i<6; i++) // sendet 6 Byte Kommando zur MMC/SD-Karte
		mmc_write_byte(*cmd++);

	// Wartet auf eine gueltige Antwort von der MMC/SD-Karte
	while (result == 0xff){
		result = mmc_read_byte();
		if (Timeout++ > 500)
			break; // Abbruch da die MMC/SD-Karte nicht antwortet
	}
	
	return result;
}


/*! 
 * Initialisiere die SD/MMC-Karte
 * @return 0 wenn allles ok, sonst Nummer des Kommandos bei dem abgebrochen wurde
 */
uint8 mmc_init(void){
	uint16 timeout = 0;
	uint8 i;
	mmc_init_state = 1;	// Nicht initialisiert, bis wir diese Funktion komplett durchlaufen haben
	
	// Konfiguration des Ports an der die MMC/SD-Karte angeschlossen wurde
	MMC_CLK_DDR |= _BV(SPI_CLK);				// Setzen von Pin MMC_Clock auf Output

	MMC_prepare();
	
	MMC_Enable();
	MMC_Disable();
	
	// Initialisiere MMC/SD-Karte in den SPI-Mode
	for (i=0; i<0x0f; i++) // Sendet min 74+ Clocks an die MMC/SD-Karte
		mmc_write_byte(0xff);
	
	// MMC_Chip_Select auf low (MMC/SD-Karte aktiv)
	MMC_Enable();
	
	// Sendet Kommando CMD0 an MMC/SD-Karte
	uint8 CMD[] = {0x40,0x00,0x00,0x00,0x00,0x95};
	while(mmc_write_command (CMD) != 1){
		if (timeout++ > 1000){
			MMC_Disable();
			return 1; // Abbruch bei Kommando 1 (Return Code 1)
		}
	}
	
	// Sendet Kommando CMD1 an MMC/SD-Karte
	timeout = 0;
	CMD[0] = 0x41; // Kommando 1
	CMD[5] = 0xFF;
	while( mmc_write_command (CMD) !=0 ){
		if (timeout++ > 1000){
			MMC_Disable();
			return 2; // Abbruch bei Kommando 2 (Return Code 2)
		}
	}
	
	// set MMC_Chip_Select to high (MMC/SD-Karte Inaktiv)
	MMC_Disable();
	// Init-Status speichern
	mmc_init_state = 0;
	return 0;
}


/*!
 * Liest einen Block von der Karte
 * @param cmd 		Zeiger auf das Kommando, das erstmal an die Karte geht
 * @param Buffer 	Ein Puffer mit mindestens count Bytes
 * @param count 	Anzahl der zu lesenden Bytes
 */
uint8 mmc_read_block(uint8 *cmd,uint8 *Buffer,uint16 count){
	/* Initialisierung checken */
	if (mmc_init_state != 0) 
		if (mmc_init() != 0) return 1;	
		
	// MMC_Chip_Select auf high (MMC/SD-Karte inaktiv) 
	MMC_Enable();
	MMC_Disable();
	// Da ein paar Leitungen noch von anderer Hardware mitbenutzt werden: reinit
	MMC_prepare();
	// sendet 8 Clock Impulse
	mmc_write_byte(0xFF);
	// MMC_Chip_Select auf low (MMC/SD-Karte aktiv)
	MMC_Enable();		
		
	// Sendet Kommando cmd an MMC/SD-Karte
	if (mmc_write_command(cmd) != 0) {
		mmc_init_state = 1;
		return 1;
	}

	// Wartet auf Start Byte von der MMC/SD-Karte (FEh/Start Byte)
	uint8 timeout=1;
	while (mmc_read_byte() != 0xfe){
		if (timeout++ == 0) break;
	};

	uint16 i;
	// Lesen des Blocks (max 512 Bytes) von MMC/SD-Karte
	for (i=0; i<count; i++)
		*Buffer++ = mmc_read_byte();

	// CRC-Byte auslesen
	mmc_read_byte();	//CRC - Byte wird nicht ausgewertet
	mmc_read_byte();	//CRC - Byte wird nicht ausgewertet
	
	// set MMC_Chip_Select to high (MMC/SD-Karte inaktiv)
	MMC_Disable();
	if (timeout == 0) {
		mmc_init_state = 1;
		return 1;	// Abbruch durch Timeout
	}
	return 0;	// alles ok
}

#ifdef MMC_INFO_AVAILABLE
/*!
 * Liest das CID-Register (16 Byte) von der Karte
 * @param Buffer 	Puffer von mindestens 16 Byte
 */
void mmc_read_cid (uint8 *Buffer){
	// Kommando zum Lesen des CID Registers
	uint8 cmd[] = {0x4A,0x00,0x00,0x00,0x00,0xFF}; 
	mmc_read_block(cmd,Buffer,16);
}

/*!
 * Liest das CSD-Register (16 Byte) von der Karte
 * @param Buffer 	Puffer von mindestens 16 Byte
 */
void mmc_read_csd (uint8 *Buffer){	
	// Kommando zum lesen des CSD Registers
	uint8 cmd[] = {0x49,0x00,0x00,0x00,0x00,0xFF};
	mmc_read_block(cmd,Buffer,16);
}

/*!
 * Liefert die Groesse der Karte zurueck
 * @return Groesse der Karte in Byte. Bei einer 4 GByte-Karte kommt 0xFFFFFFFF zurueck
 */
uint32 mmc_get_size(void){
	uint8 csd[16];
	
	mmc_read_csd(csd);
	
	uint32 size = (csd[8]>>6) + (csd[7] << 2) + ((csd[6] & 0x03) << 10); // c_size
	size +=1;		// Fest in der Formel drin
	
	uint8 shift = 2;	// eine 2 ist fest in der Formel drin
	shift += (csd[10]>>7) + ((csd[9] & 0x03) <<1); // c_size_mult beruecksichtigen
	shift += csd[5] & 0x0f;	// Blockgroesse beruecksichtigen

	size = size << shift;
		
	return size;
}


#endif //MMC_INFO_AVAILABLE

#ifdef MMC_WRITE_TEST_AVAILABLE
	/*! Testet die MMC-Karte. Schreibt nacheinander 2 Sektoren a 512 Byte mit Testdaten voll und liest sie wieder aus
	 * !!! Achtung loescht die Karte
	 * @return 0, wenn alles ok
	 */
	uint8 mmc_test(void){
		/* Initialisierung checken */
		if (mmc_init_state != 0) 
			if (mmc_init() != 0) return 1;
		#ifdef MMC_VM_AVAILABLE	// Version mit virtuellen Aressen
			uint16 i;
			static uint16 pagefaults = 0;
			static uint16 old_pf;
			/* virtuelle Adressen holen */
			static uint32 v_addr1 = 0;
			static uint32 v_addr2 = 0;
			static uint32 v_addr3 = 0;
			static uint32 v_addr4 = 0;
			if (v_addr1 == 0) v_addr1 = mmcalloc(512, 1);	// Testdaten 1
			if (v_addr2 == 0) v_addr2 = mmcalloc(512, 1);	// Testdaten 2
			if (v_addr3 == 0) v_addr3 = mmcalloc(512, 1);	// Dummy 1
			if (v_addr4 == 0) v_addr4 = mmcalloc(512, 1);	// Dummy 2
			/* Zeitmessung starten */
			uint16 start_ticks=TIMER_GET_TICKCOUNT_16;
			uint8 start_reg=TCNT2;	
			/* Pointer auf Puffer holen */
			uint8* p_addr = mmc_get_data(v_addr1);	// Cache-Hit, CB 0
			if (p_addr == NULL) return 2;
			/* Testdaten schreiben */
			for (i=0; i<512; i++)
				p_addr[i] = (i & 0xff);
			/* Pointer auf zweiten Speicherbereich holen */
			p_addr = mmc_get_data(v_addr2);		// Cache-Hit, CB 1
			if (p_addr == NULL)	return 3;
			/* Testdaten Teil 2 schreiben */
			for (i=0; i<512; i++)
				p_addr[i] = 255 - (i & 0xff);			
			/* kleiner LRU-Test */
			p_addr = mmc_get_data(v_addr1);	// Cache-Hit, CB 0
			p_addr = mmc_get_data(v_addr4);	// Cache-Miss, => CB 1
			p_addr = mmc_get_data(v_addr1);	// Cache-Hit, CB 0						
			p_addr = mmc_get_data(v_addr3);	// Cache-Miss, => CB 1
			p_addr = mmc_get_data(v_addr1);	// Cache-Hit, CB 0
			p_addr = mmc_get_data(v_addr4);	// Cache-Miss, => CB 1						
			/* Pointer auf Testdaten Teil 1 holen */	
			p_addr = mmc_get_data(v_addr1);		// Cache-Hit, CB 0
			if (p_addr == NULL) return 4;		
			/* Testdaten 1 vergleichen */
			for (i=0; i<512; i++)
				if (p_addr[i] != (i & 0xff)) return 5;
			/* Pointer auf Testdaten Teil 2 holen */
			p_addr = mmc_get_data(v_addr2);		// Cache-Miss, => CB 1
			if (p_addr == NULL) return 6;		
			/* Testdaten 2 vergleichen */
			for (i=0; i<512; i++)
				if (p_addr[i] != (255 - (i & 0xff))) return 7;
			/* Zeitmessung beenden */
			int8 timer_reg=TCNT2;
			uint16 end_ticks=TIMER_GET_TICKCOUNT_16;
			timer_reg -= start_reg;
			/* Pagefaults merken */		
			old_pf = pagefaults;
			pagefaults = mmc_get_pagefaults();
			/* kleine Statistik ausgeben */
			display_cursor(3,1);
			display_printf("Pagefaults: %5u   ", pagefaults);
			display_cursor(4,1);
			display_printf("Bei %3u PF: %5u us", pagefaults - old_pf, (end_ticks-start_ticks)*176 + timer_reg*4);
		#else	// alte Version
			uint8 buffer[512];
			uint16 i;
			
			uint8 result=0;
			
			/* Zeitmessung starten */
			uint16 start_ticks=TIMER_GET_TICKCOUNT_16;
			uint8 start_reg=TCNT2;	
			
			// Puffer vorbereiten
			for (i=0; i< 512; i++)	buffer[i]= (i & 0xFF);
			// und schreiben
			result= mmc_write_sector(0,buffer);
			if (result != 0)
				return result*10 + 2;
			
			// Puffer vorbereiten
			for (i=0; i< 512; i++)	buffer[i]= 255 - (i & 0xFF);	
			// und schreiben
			result= mmc_write_sector(1,buffer);	
			if (result != 0)
				return result*10 + 3;
		
			// Puffer lesen	
			result= mmc_read_sector(0,buffer);	
			if (result != 0)
				return result*10 + 4;
			
			// und vergleichen
			for (i=0; i< 512; i++)
				if (buffer[i] != (i & 0xFF))
					return 5;
		
			// Puffer lesen	
			result= mmc_read_sector(1,buffer);
			if (result != 0)
				return result*10 + 6;
			// und vergleichen
			for (i=0; i< 512; i++)
				if (buffer[i] != (255- (i & 0xFF)))
					return 7;	

			/* Zeitmessung beenden */
			int8 timer_reg=TCNT2;
			uint16 end_ticks=TIMER_GET_TICKCOUNT_16;
			timer_reg -= start_reg;
			/* kleine Statistik ausgeben */
			display_cursor(3,1);
			display_printf("Dauer: %5u us     ", (end_ticks-start_ticks)*176 + timer_reg*4);							
		#endif	// MMC_VM_AVAILABLE			
		// hierher kommen wir nur, wenn alles ok ist
		return 0;
	}
#endif //MMC_WRITE_TEST_AVAILABLE

#endif
#endif
