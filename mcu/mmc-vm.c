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
 * @file 	mmc_vm.c
 * @brief 	Virtual Memory Management mit MMC / SD-Card
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	30.11.2006
 */


/* Der virtuelle (MMC- / SD-Card-) Speicher ist wie folgt organisiert:
 * Man fordert per mmcalloc() eine beliebige Menge an Speicher an und bekommt eine virtuelle Adresse
 * in 32 Bit zurueck. Diese Adresse entspricht der Adresse auf der MMC / SD-Card, was den Benutzer aber
 * nicht weiter interessieren muss. 
 * Mit mmc_get_data(uint32 virtuelle_Adresse) bekommt man eine "echte" SRAM-Adresse auf einen 512 Byte gro§en Puffer zurueck,
 * dessen Inhalt auf die MMC / SC-Card geschrieben wird, sobald er nicht mehr im SRAM gehalten werden kann. 
 * mmc_get_end_of_block(uint32 virtuelle_Adresse) gibt an, bis wohin man diesen Pointer verwenden darf. Benoetigt man mehr
 * Speicher, fordert man mit mmc_get_data(uint32 virtuelle_Adresse) einen neuen Pointer an. 
 * Das Ein- und Auslagern der Seiten auf die MMC / SD-Card macht die Speicherverwaltung automatisch. 
 *
 * Ein kleines Beispiel:
 * uint32 v_addr = mmcalloc(2048, 1);		// wir wollen 2 KB Speicher
 * uint8* p_addr = mmc_get_data(v_addr);	// Pointer auf Puffer holen
 * ... // irgendwas sinnvolles tun
 * if (i < 512){	// Ziel liegt noch im gleichen Block
 * 	p_addr[i] = my_data;	// Daten speichern
 * 	i++;
 * } else {			// Blockende erreicht => zunaechst neuen Pointer holen
 *  v_addr += i;
 * 	i = 0;
 * 	p_addr = mmc_get_data(v_addr);
 * 	p_addr[i] = my_data;	// Daten koennen nun gespeichert werden
 * }
 * 
 * Das zweite Argument von mmcalloc() gibt an, ob man den angeforderten Speicher gern in einem neuen Block haette,
 * (also 512 Byte-aligned), oder ob er einfach am naechsten freien Byte auf der Karte beginnen soll. 1: aligned, 0: beliebig
 * MMC_START_ADDRESS definiert, an welcher MMC- / SD-Card-Adresse der virtuelle Speicherraum beginnt, alles davor bleibt
 * unberuehrt, z.B. fuer ein FAT-Dateisystem.
 * MAX_PAGES_IN_SRAM definiert die maximale Anzahl der Seiten, die gleichzeitig im SRAM vorgehalten werden. Dabei wird der
 * Speicher im SRAM erst belegt, sobald er gebraucht wird. Passt keine weitere Seite mehr ins SRAM, wird die Anzahl der 
 * vorzuhaltenen Seiten reduziert und eine andere Seite ausgelagert, um Platz zu schaffen.
 * 
 * Derzeitiger Status des Ganzen: Experimentell bis experimentell+1 ;-)
 */
 
 
//TODO:	* Kompatibilitaet zu mini-fat.c herstellen => Irgendwie ein Mapping von Dateien auf (dann evtl. vorbelegte?) Adressen,
// 		  Adressraum fuer dynamisch angeforderten MMC-Speicher dann dahinter => mmc_start_address umbiegen?
//		* Permanente Daten zu Beginn einlesen / nicht ueberschreiben -> mini-fat
//		* Funktion zum Rueckschreiben der Pages auf MMC (und wann? - Dirty-Bit? - Rueckschreibestrategie ueberlegen!)
//		* LRU implementieren
//		* Unsinnige Variablen- und Funktionsnamen (block != block usw.) aendern => in Sinnvolle umbenennen 
//		* Kommentare ergaenzen... kurze Erklaerung am Dateianfang einfuegen
//		* #ifdef drumrum?
//		* Code optimieren, Groesse und Speed
//		* Fehler in mmc_load_block() behandeln, nicht einfach nur return 1
//		* virtuelle Adresse auf Gueltigkeit pruefen (>= mmc_start_address)
//		* MMC / SD-Card Groesse checken in mmcalloc, denn mehr virtuellen Speicher koennen wir wirklich nicht anbieten 

#include "ct-Bot.h"  

#ifdef MCU
#ifdef MMC_AVAILABLE
#ifdef MMC_VM_AVAILABLE

#include "mmc-vm.h"
#include "mmc.h"
#include <stdlib.h>

#define MMC_START_ADDRESS 0	// [0;2^32-2]
#define MAX_PAGES_IN_SRAM 6	// [1;127] - Pro Page werden 512 Byte im SRAM belegt, sobald die Page verwendet wird

static uint32 mmc_start_address = MMC_START_ADDRESS;
static uint32 used_mmc_blocks = 0;
static uint32 next_mmc_address = MMC_START_ADDRESS;
static uint32 loaded_blocks[MAX_PAGES_IN_SRAM];
static uint8 pages_in_sram = MAX_PAGES_IN_SRAM;
static int8 last_loaded_block = -1;
static uint8 allocated_blocks = 0;
uint16 pagefaults = 0;

uint8* data[MAX_PAGES_IN_SRAM];

// Vorsicht, "loescht" alle Daten!		TODO: Unsinn? Funktion weglassen? -> mini-fat.c 
void set_mmc_start_address(uint32 address){	// TODO: Reinit oder so noetig => was tun?! oder nix tun?
	mmc_start_address = address;	
	next_mmc_address = mmc_start_address;
}

inline uint32 mmc_get_block(uint32 addr){
	/* Eine effizientere Variante von addr >> 9 */
	asm volatile(
		"mov %A0, %B0	\n\t"
		"mov %B0, %C0	\n\t"
		"mov %C0, %D0	\n\t"
		"lsr %C0		\n\t"
		"ror %B0		\n\t"
		"ror %A0		\n\t"		
		"clr %D0			"		
		:	"=r"	(addr)
		:	"0"		(addr)
	);	
	return addr;		
	//return addr >> 9;	// TODO: das geht auch effizienter
}

inline uint16 mmc_get_pagefaults(void){
	return pagefaults;	// read-only
}

inline uint32 mmc_get_end_of_block(uint32 addr){
	return addr | 0x1ff;	// die unteren 9 Bit sind gesetzt, da Blockgroesse = 512 Byte
}

int8 mmc_get_loaded_block(uint32 addr){
	uint32 block = mmc_get_block(addr);
	int i;
	for (i=0; i<pages_in_sram; i++){	// O(n)
		if (allocated_blocks <= i) return -1;
		if (loaded_blocks[i] == block) return i;
	}
	return -1;
}

uint8 mmc_load_block(uint32 addr){
	if (mmc_get_loaded_block(addr) >= 0) return 0;
	last_loaded_block++;
	if (last_loaded_block == pages_in_sram) last_loaded_block = 0;
	if (allocated_blocks <= last_loaded_block){
		data[last_loaded_block] = malloc(512);
		if (data[last_loaded_block] == NULL){	// Da will uns jemand keinen Speicher mehr geben :(
			if (pages_in_sram == 1) return 1;	// Hier ging was schief, das wir so nicht loesen koennen
			pages_in_sram--;
			last_loaded_block--;
			return mmc_load_block(addr);
		}
		allocated_blocks++;	
	} else{
		pagefaults++;
		//if (data[last_loaded_block] == NULL) data[last_loaded_block] = malloc(512); 
		if (mmc_write_sector(loaded_blocks[last_loaded_block], data[last_loaded_block]) != 0) return 1;	// TODO: korrekte Werte wiederherstellen
		if (mmc_read_sector(mmc_get_block(addr), data[last_loaded_block]) != 0) return 1;
	}
	loaded_blocks[last_loaded_block] = mmc_get_block(addr);
	return 0;
}

uint32 mmcalloc(uint32 size, uint8 aligned){
	uint32 start;
	if (aligned == 0 || mmc_get_end_of_block(next_mmc_address) == mmc_get_end_of_block(next_mmc_address+size-1)){
		start = next_mmc_address;
	} else {
		start = mmc_get_end_of_block(next_mmc_address) + 1;
	}	
	next_mmc_address = start + size;
	//uint32 new_block = last_loaded_block + 1;	//Unsinn
	//if (new_block == pages_in_sram) new_block = 0;
	//loaded_blocks[new_block] = mmc_get_block(start);
	used_mmc_blocks = mmc_get_block(next_mmc_address-1) + 1;
	return start;
}

uint8* mmc_get_data(uint32 addr){
	if (mmc_load_block(addr) != 0) return NULL;
	return data[mmc_get_loaded_block(addr)] + (addr - (mmc_get_block(addr)<<9));	//TODO: 2. Summanden schlauer berechnen
}

#endif	// MMC_VM_AVAILABLE
#endif	// MMC_AVAILABLE
#endif	// MCU
