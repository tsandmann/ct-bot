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

/* Die PC-Emulation einer MMC / SD-Card ermoeglicht es, am PC dieselben Funktionen zu benutzen, wie bei einer echten
 * MMC / SD-Card. Als Speichermedium dient hier eine Datei (MMC_EMU_FILE), deren Groesse sich mit MMC_EMU_SIZE in Byte 
 * einstellen laesst. Ist die Datei nicht vorhanden oder derzeit kleiner als MMC_EMU_SIZE, wird sie angelegt bzw. 
 * vergroessert. Achtung, startet man den C-Bot vom Sim aus, liegt die Datei, falls kein absoluter Pfad angegeben wurde, im
 * Verzeichnis dem ct-Sims, von der Konsole aus gestartet erwartet / erzeugt der Code die Datei im Unterverzeichnis 
 * "Debug-Linux" bzw. "Debug-W32".
 * Eine sinnvolle (und die derzeit einzig moegliche) Verwendung der Emulation ergibt sich im Zusammenspiel mit dem
 * Virtual Memory Management fuer MMC. Ein Verhalten kann in diesem Fall immer auf dieselbe Art und Weise Speicher anfordern,
 * je nach System liegt dieser physisch dann entweder auf einer MMC / SD-Card (MCU) oder in einer Datei (PC). Fuer das
 * Verhalten ergibt sich kein Unterschied und es kann einfach derselbe Code verwendet werden.
 * Moechte man die Funktion mmc_fopen() benutzen, also auf FAT16-Dateien zugreifen, so ist zu beachten, dass deren "Dateiname"
 * in der Datei fuer die Emulation am Anfang eines 512 Byte grossen Blocks steht (denn auf einer echten MMC / SD-Card ezeugt
 * ein Betriebssystem eine neue Datei immer am Anfang eines Clusters und mmc_fopen() sucht nur dort nach dem "Dateinamen").
 * Im Moment gibt es noch keine Funktion zum Anlegen einer neuen Datei auf einer echten oder emulierten MMC / SD-Card. 
 * Die Code der Emulation ist voellig symmetrisch zum Code fuer eine echte MMC / SD-Card aufgebaut.  
 */
 
#include "ct-Bot.h"
#include <stdio.h>
#include <string.h>
#include "mmc-emu.h"
#include "mmc-vm.h"
#include "display.h"

#ifdef PC
#ifdef MMC_VM_AVAILABLE

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
	mmc_emu_file = fopen(MMC_EMU_FILE, "r+b");		// Datei versuchen zu oeffnen
	if (mmc_emu_file == NULL){
		/* Datei existiert noch nicht oder kann nicht erzeugt werden */
		mmc_emu_file = fopen(MMC_EMU_FILE, "w+b");	// Datei neu anlegen
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

/*!
 * @brief			Sucht die Adresse einer Mini-FAT-Datei im EERROM
 * @param filename	Datei-ID
 * @param buffer	Zeiger auf 512 Byte großen Speicherbereich (wird ueberschrieben)
 * @return			(Byte-)Adresse des ersten Nutzdatenblock der gesuchten Datei oder 0, falls nicht im EEPROM
 * @date 			05.03.2007
 * Nur DUMMY fuer MMC-Emulation am PC. Wenn es mal eine EEPROM-Emulation fuer PC gibt, kann man diese Funktion implementieren.
 */
uint32 mmc_emu_fat_lookup_adr(const char* filename, uint8* buffer){
	// absichtlich leer
	return 0;
}

/*!
 * @brief			Speichert die Adresse einer Mini-FAT-Datei in einem EERROM-Slab
 * @param block		(Block-)Adresse der Datei, die gespeichert werden soll
 * @date 			05.03.2007
 * Nur DUMMY fuer MMC-Emulation am PC. Wenn es mal eine EEPROM-Emulation fuer PC gibt, kann man diese Funktion implementieren.
 */
void mmc_emu_fat_store_adr(uint32 block){
	// absichtlich leer
}

uint32_t mmc_emu_find_block(const char * filename, uint8_t * buffer, uint32_t end_addr) {
	end_addr >>= 9;	// letzte Blockadresse ermitteln
	
	/* sequenziell die Karte durchsuchen */
	uint32_t block;
	for (block=0; block<end_addr; block++) {
		if (mmc_emu_read_sector(block, buffer) != 0) return 0xffffffff;
		if (strcmp((char *)buffer, filename) == 0) {
			/* gefunden, Nutzdaten laden */
			if (mmc_emu_read_sector(++block, buffer) != 0) return 0xffffffff;
			return block;
		}
	}
	return 0xffffffff;
}

/*!
 * Testet VM und MMC / SD-Card Emulation am PC
 * @date	30.12.2006 
 */
uint8 mmc_emu_test(void){
	/* Initialisierung checken */
	if (mmc_emu_init_state != 0 && mmc_emu_init() != 0) return 1;
	uint16 i;
	static uint16 pagefaults = 0;
	/* virtuelle Adressen holen */
	static uint32 v_addr1 = 0;
	static uint32 v_addr2 = 0;
	static uint32 v_addr3 = 0;
	static uint32 v_addr4 = 0;
	if (v_addr1 == 0) v_addr1 = mmcalloc(512, 1);	// Testdaten 1
	if (v_addr2 == 0) v_addr2 = mmcalloc(512, 1);	// Testdaten 2
	if (v_addr3 == 0) v_addr3 = mmcalloc(512, 1);	// Dummy 1
	if (v_addr4 == 0) v_addr4 = mmcalloc(512, 1);	// Dummy 2
	/* Pointer auf Puffer holen */
	uint8* p_addr = mmc_get_data(v_addr1);
	if (p_addr == NULL) return 2;
	/* Testdaten schreiben */
	for (i=0; i<512; i++)
		p_addr[i] = (i & 0xff);
	/* Pointer auf zweiten Speicherbereich holen */
	p_addr = mmc_get_data(v_addr3);
	if (p_addr == NULL)	return 3;
	/* Testdaten Teil 2 schreiben */
	for (i=0; i<512; i++)
		p_addr[i] = 255 - (i & 0xff);			
	/* kleiner LRU-Test */
		p_addr = mmc_get_data(v_addr1);
		p_addr = mmc_get_data(v_addr4);
		p_addr = mmc_get_data(v_addr1);						
		p_addr = mmc_get_data(v_addr3);
		p_addr = mmc_get_data(v_addr1);
		p_addr = mmc_get_data(v_addr4);						
	/* Pointer auf Testdaten Teil 1 holen */	
	p_addr = mmc_get_data(v_addr1);
	if (p_addr == NULL) return 4;		
	/* Testdaten 1 vergleichen */
	for (i=0; i<512; i++)
		if (p_addr[i] != (i & 0xff)) return 5;
	/* Pointer auf Testdaten Teil 2 holen */
	p_addr = mmc_get_data(v_addr3);
	if (p_addr == NULL) return 6;		
	/* Testdaten 2 vergleichen */
	for (i=0; i<512; i++)
		if (p_addr[i] != (255 - (i & 0xff))) return 7;
	#ifdef VM_STATS_AVAILABLE 
		/* Pagefaults merken */		
		pagefaults = mmc_get_pagefaults();
	#endif
	/* kleine Statistik ausgeben */
	display_cursor(3,1);
	display_printf("Pagefaults: %5u  ", pagefaults);
	// hierher kommen wir nur, wenn alles ok ist		
	return 0;
}

#endif	// MMC_VM_AVAILABLE
#endif	// PC
