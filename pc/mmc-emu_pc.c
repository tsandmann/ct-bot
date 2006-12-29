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
 * @file 	mmc-emu_pc.c
 * @brief 	MMC / SD-Card Emulation fuer PC
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	10.12.2006
 */
 
#include "ct-Bot.h"
#include <stdio.h>
#include "mmc-emu.h"

#ifdef PC
#ifdef MMC_VM_AVAILABLE

#define MMC_EMU_SIZE	0x2000000		/*!< Groesse der emulierten Karte in Byte */

volatile uint8 mmc_emu_init_state=1;	/*!< Initialierungsstatus der Karte, 0: ok, 1: Fehler  */
static FILE* mmc_emu_file;				/*!< Der Inhalt der emulierten Karte wird einfach in eine Datei geschrieben */

/*!
 * Checkt Initialisierung der emulierten Karte
 * @return	0, wenn initialisiert
 * @see		mcu/mmc.c
 * @date 	29.12.2006
 */
inline uint8 mmc_emu_get_init_state(void){
	return mmc_emu_init_state;	
}

/*! 
 * Initialisiere die emulierte SD/MMC-Karte
 * @return	0 wenn allles ok, sonst 1
 * @see		mcu/mmc.c
 * @date 	29.12.2006 
 */
uint8 mmc_emu_init(void){
	mmc_emu_init_state = 0;
	mmc_emu_file = fopen("mmc_emu.dat", "r+");		// Datei versuchen zu oeffnen
	if (mmc_emu_file == NULL){
		/* Datei existiert noch nicht oder kann nicht erzeugt werden */
		mmc_emu_file = fopen("mmc_emu.dat", "w+");	// Datei neu anlegen
		if (mmc_emu_file == NULL) {
			/* Datei kann nicht erzeugt werden */
			mmc_emu_init_state = 1;
			return 1;
		}	
	}
	if (mmc_emu_get_size() < MMC_EMU_SIZE){
		/* vorhandene Datei ist zu klein, also auf MMC_EMU_SIZE vergroessern */
		mmc_emu_init_state = 1;
		if (fseek(mmc_emu_file, MMC_EMU_SIZE-1, SEEK_SET) != 0) return 2;
		if (putc(0, mmc_emu_file) != 0) return 3;
		if (fflush(mmc_emu_file) != 0) return 4;
		mmc_emu_init_state = 0;
	}
	return 0;	
}

/*!
 * Liest einen Block von der emulierten Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param buffer 	Puffer von mindestens 512 Byte
 * @return 			0 wenn alles ok ist 
 * @see				mcu/mmc.c
 * @date 			10.12.2006
 */	
uint8 mmc_emu_read_sector(uint32 addr, uint8* buffer){
	if (mmc_emu_get_init_state() != 0 && mmc_emu_init() !=0) return 1;
	if (fseek(mmc_emu_file, addr<<9, SEEK_SET) != 0) return 2;	// Adresse in Byte umrechnen und an Dateiposition springen
	if (fread(buffer, 512, 1, mmc_emu_file) != 1) return 3;		// Block lesen
	return 0;	
}

/*! 
 * Schreibt einen 512-Byte Sektor auf die emulierte Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param buffer 	Zeiger auf den Puffer
 * @param async		Wird bei der PC-Version nicht ausgewertet
 * @return 			0 wenn alles ok ist
 * @date 			10.12.2006
 * @see				mcu/mmc.c
 */
uint8 mmc_emu_write_sector(uint32 addr, uint8* buffer, uint8 async){
	if (mmc_emu_get_init_state() != 0 && mmc_emu_init() !=0) return 1;
	if (fseek(mmc_emu_file, addr<<9, SEEK_SET) != 0) return 2;	// Adresse in Byte umrechnen und an Dateiposition springen
	if (fwrite(buffer, 512, 1, mmc_emu_file) != 1) return 3;	// Block schreiben
	if (fflush(mmc_emu_file) != 0) return 4;					// Puffer leeren
	return 0;	
}

/*!
 * Liefert die Groesse der Karte zurueck
 * @return	Groesse der emulierten Karte in Byte.
 * @date	29.12.2006 
 */
uint32 mmc_emu_get_size(void){
	if (mmc_emu_get_init_state() != 0 && mmc_emu_init() !=0) return 0;
	if (fseek(mmc_emu_file, 0L, SEEK_END) != 0) return 0;	// Groesse der emulierten Karte = Groesse der Datei
	return ftell(mmc_emu_file)+1;
}

uint8 mmc_emu_test(void){
	static uint32 sector = 0x0;
	/* Initialisierung checken */
	if (mmc_emu_init_state != 0 && mmc_emu_init() != 0) return 1;
	uint8 buffer[512];
	uint16 i;
	uint8 result=0;	
	/* Puffer vorbereiten */
	for (i=0; i< 512; i++)	buffer[i]= (i & 0xFF);
	/* Puffer schreiben */
	result = mmc_emu_write_sector(sector, buffer, 0);
	if (result != 0){
		return result*10 + 2;
	}
	/* Puffer vorbereiten */
	for (i=0; i< 512; i++)	buffer[i]= 255 - (i & 0xFF);	
	/* Puffer schreiben */
	result = mmc_emu_write_sector(sector+1, buffer, 0);	
	if (result != 0){
		return result*10 + 3;
	}
	/* Puffer lesen */	
	result = mmc_emu_read_sector(sector, buffer);	
	if (result != 0){
		sector--;
		return result*10 + 4;
	}
	/* Puffer vergleichen */
	for (i=0; i<512; i++)
		if (buffer[i] != (i & 0xFF)){
			return 5;
		}
	/* Puffer lesen */	
	result = mmc_emu_read_sector(sector+1, buffer);
	if (result != 0){
		sector--;
		return result*10 + 6;
	}
	/* Puffer vergleichen */
	for (i=0; i<512; i++)
		if (buffer[i] != (255- (i & 0xFF))){
			return 7;	
		}
	// hierher kommen wir nur, wenn alles ok ist		
	return 0;
}

#endif	// MMC_VM_AVAILABLE
#endif	// PC
