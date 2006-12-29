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
 * @file 	mmc-vm.c
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
 * mmc_get_end_of_page(uint32 virtuelle_Adresse) gibt an, bis wohin man diesen Pointer verwenden darf. Benoetigt man mehr
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
 * Die Bezeichnung "page" (oder "Seite") stellt durchgaengig etwas logisches (virtuelles) dar, mit "block" hingegen ist ein
 * physischer Block auf dem verwendeten Datentraeger gemeint.
 * 
 * Ein Auruf der Funktion mmc_flush_cache() schreibt den kompletten Cache auf die Karte zurueck. Ein Verhalten sollte vor 
 * seiner Beendigung dafuer sorgen, dass das passiert, wenn die Daten erhalten bleiben sollen.
 * 
 * mmc_fopen() oeffnet eine Datei im FAT16-Dateisystem der Karte und gibt die virtuelle Startadresse zurueck, so dass man mit 
 * mmc_get_data() an die Daten kommt. Der Dateiname muss ganz am Anfang in der Datei stehen.
 * 
 */
 
 
//TODO:	* Bug im Zusammenhang mit mmc_fopen() beheben
//		* Funktion zum Anlegen von Dateien bauen
//		* Code optimieren, Groesse und Speed

#include "ct-Bot.h"  

#ifdef MMC_VM_AVAILABLE

#include "mmc-vm.h"
#include "mmc.h"
#include "mmc-low.h"
#include "mmc-emu.h"
#include "display.h"
#include <stdlib.h>

#ifdef MCU
	#define MMC_START_ADDRESS 0x2000000	// [512;2^32-1] in Byte - Sinnvoll ist z.B. Haelfte der MMC / SD-Card Groesse, der Speicherplatz davor kann dann fuer ein Dateisystem verwendet werden
	#define MAX_SPACE_IN_SRAM 3			// [1;127] - Pro Page werden 512 Byte im SRAM belegt, sobald diese verwendet wird
	#define swap_out	mmc_write_sector
	#define swap_in		mmc_read_sector
	#define swap_space	mmc_get_size()
#else
	#define MMC_START_ADDRESS 0x200		// [512;2^32-1]
	#define MAX_SPACE_IN_SRAM 127		// [1;127] - Pro Page werden 512 Byte im RAM belegt, sobald diese verwendet wird
	#define swap_out	mmc_emu_write_sector
	#define swap_in		mmc_emu_read_sector
	#define swap_space	mmc_emu_get_size()
#endif	
#define FILENAME_MAX	255		/*!< Maximale Dateienamenlaenge in Zeichen [1;255] */

#if MMC_ASYNC_WRITE == 1
	#define MAX_PAGES_IN_SRAM MAX_SPACE_IN_SRAM-1
	static uint8* swap_buffer;	/*!< Puffer fuer asynchrones write-back */
#else
	#define MAX_PAGES_IN_SRAM MAX_SPACE_IN_SRAM
#endif

typedef struct{			/*!< Struktur eines Cacheeintrags */
	uint32	addr;		/*!< Tag = MMC-Blockadresse der ins RAM geladenen Seite */ 
	uint8*	p_data;		/*!< Daten = Zeiger auf 512 Byte grosse Seite im RAM */ 
	#if MAX_PAGES_IN_SRAM > 2
		uint8	succ;	/*!< Zeiger auf Nachfolger (LRU) */
		uint8	prec;	/*!< Zeiger auf Vorgaenger (LRU) */
	#endif
	uint8	dirty;		/*!< Dirty-Bit (0: Daten wurden bereits auf die MMC / SD-Card zurueckgeschrieben) */
} vm_cache_t;

static uint32 mmc_start_address = MMC_START_ADDRESS;	/*!< physische Adresse der MMC / SD-Card, wo unser VM beginnt */
static uint32 used_mmc_blocks = 0;						/*!< Anzahl der vom VM belegten Bloecke auf der MMC / SD-Card */
static uint32 next_mmc_address = MMC_START_ADDRESS;		/*!< naechste freie virtuelle Adresse */
static uint8 pages_in_sram = MAX_PAGES_IN_SRAM;			/*!< Groesse des Caches im RAM */
static int8 allocated_pages = 0;						/*!< Anzahl bereits belegter Cachebloecke */
static uint8 oldest_cacheblock = 0;						/*!< Zeiger auf den am laengsten nicht genutzten Eintrag (LRU) */
static uint8 recent_cacheblock = 0;						/*!< Zeiger auf den letzten genutzten Eintrag (LRU) */
uint16 pagefaults = 0;									/*!< Anzahl der Pagefaults seit Systemstart bzw. Ueberlauf */

vm_cache_t page_cache[MAX_PAGES_IN_SRAM];				/*!< der eigentliche Cache, vollassoziativ, LRU-Policy */

/*! 
 * Gibt die Blockadresse einer Seite zurueck
 * @param addr	Eine virtuelle Adresse
 * @return		Adresse
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		30.11.2006
 */
inline uint32 mmc_get_mmcblock_of_page(uint32 addr){
	#ifdef MCU
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
	#else
		return addr >> 9;
	#endif	// MCU		
}

/*! 
 * Gibt die Anzahl der Pagefaults seit Systemstart bzw. Ueberlauf zurueck
 * @return		#Pagefaults
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		30.11.2006
 */
inline uint16 mmc_get_pagefaults(void){
	return pagefaults;	// read-only
}

/*! 
 * Gibt die letzte Adresse einer Seite zurueck
 * @param addr	Eine virtuelle Adresse
 * @return		Adresse 
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		30.11.2006
 */
inline uint32 mmc_get_end_of_page(uint32 addr){
	return addr | 0x1ff;	// die unteren 9 Bit sind gesetzt, da Blockgroesse = 512 Byte
}

/*! 
 * Gibt den Index einer Seite im Cache zurueck
 * @param addr	Eine virtuelle Adresse
 * @return		Index des Cacheblocks, -1 falls Cache-Miss
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		30.11.2006
 */
int8 mmc_get_cacheblock_of_page(uint32 addr){
	uint32 page_addr = mmc_get_mmcblock_of_page(addr);
	int i;
	for (i=0; i<allocated_pages; i++){	// O(n)
		if (page_cache[i].addr == page_addr) return i;	// Seite gefunden :)
	}
	return -1;	// Seite nicht im Cache
}

/*! 
 * Laedt eine Seite in den Cache, falls sie noch nicht geladen ist
 * @param addr	Eine virtuelle Adresse
 * @return		0: ok, 1: ungueltige Adresse, 2: Fehler bei swap_out, 3: Fehler bei swap_in
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		30.11.2006
 */
uint8 mmc_load_page(uint32 addr){
	if (/*addr < mmc_start_address || */addr >= next_mmc_address) return 1;	// ungueltige virtuelle Adresse :(
	int8 cacheblock = mmc_get_cacheblock_of_page(addr); 
	if (cacheblock >= 0){	// Cache-Hit, Seite ist bereits geladen :)
		/* LRU */	
		#if MAX_PAGES_IN_SRAM > 2
			if (recent_cacheblock == cacheblock) page_cache[cacheblock].succ = cacheblock;	// Nachfolger des neuesten Eintrags ist die Identitaet
			if (oldest_cacheblock == cacheblock){
				oldest_cacheblock = page_cache[cacheblock].succ;	// Nachfolger ist neuer aeltester Eintrag
				page_cache[page_cache[cacheblock].succ].prec = oldest_cacheblock;	// Vorgaenger der Nachfolgers ist seine Identitaet				
			}  
			else{
				page_cache[page_cache[cacheblock].prec].succ = page_cache[cacheblock].succ;	// Nachfolger des Vorgaengers ist eigener Nachfolger
				page_cache[page_cache[cacheblock].succ].prec = page_cache[cacheblock].prec;	// Vorganeger des Nachfolgers ist eigener Vorgaenger
			}
			page_cache[cacheblock].prec = recent_cacheblock;	// alter neuester Eintrag ist neuer Vorgaenger
		#else
			oldest_cacheblock = (pages_in_sram - 1) - cacheblock;	// aeltester Eintrag ist nun der andere Cacheblock (wenn verfuegbar)
		#endif
		recent_cacheblock = cacheblock;						// neuester Eintrag ist nun die Identitaet
		return 0;
	}
	/* Cache-Miss => neue Seite einlagern, LRU Policy */
	int8 next_cacheblock = oldest_cacheblock;
	if (allocated_pages < pages_in_sram){
		/* Es ist noch Platz im Cache */
		next_cacheblock = allocated_pages;
		page_cache[next_cacheblock].p_data = malloc(512);	// Speicher anfordern
		if (page_cache[next_cacheblock].p_data == NULL){	// Da will uns jemand keinen Speicher mehr geben :(
			if (pages_in_sram == 1) return 1;	// Hier ging was schief, das wir so nicht loesen koennen
			/* Nicht mehr genug Speicher im RAM frei => neuer Versuch mit verkleinertem Cache */
			pages_in_sram--;
			return mmc_load_page(addr);
		}
		allocated_pages++;	// Cache-Fuellstand aktualisieren		
	} else{
		/* Cache bereits voll => Pager muss aktiv werden */
		pagefaults++;	// kleine Statistik
		#if MMC_ASYNC_WRITE == 1	// im asnychronen Fall holen wir erst die neue Seite, dann kann sich das Zurueckschreiben ruhig Zeit lassen
			uint8* p_tmp = page_cache[next_cacheblock].p_data;
			if (swap_in(mmc_get_mmcblock_of_page(addr), swap_buffer) != 0) return 3;
		#endif
		if (page_cache[next_cacheblock].dirty == 1)	// Seite zurueckschreiben, falls Daten veraendert wurden
			if (swap_out(page_cache[next_cacheblock].addr, page_cache[next_cacheblock].p_data, MMC_ASYNC_WRITE) != 0) return 2;
		#if MMC_ASYNC_WRITE == 1
			page_cache[next_cacheblock].p_data = swap_buffer;
			swap_buffer = p_tmp;	
		#else
			if (swap_in(mmc_get_mmcblock_of_page(addr), page_cache[next_cacheblock].p_data) != 0) return 3;
		#endif
		#if MAX_PAGES_IN_SRAM > 2
			oldest_cacheblock = page_cache[oldest_cacheblock].succ;	// Nachfolger des aeltesten Eintrags ist neuer aeltester Eintrag
		#else
			oldest_cacheblock = (pages_in_sram - 1) - next_cacheblock;	// neuer aeltester Eintrag ist nun der andere Cacheblock (wenn verfuegbar)
		#endif
	}
	page_cache[next_cacheblock].addr = mmc_get_mmcblock_of_page(addr);	// Cache-Tag aktualisieren
	/* LRU */
	#if MAX_PAGES_IN_SRAM > 2
		page_cache[next_cacheblock].prec = recent_cacheblock;	// Vorgaenger dieses Cacheblocks ist der bisher neueste Eintrag
		page_cache[recent_cacheblock].succ = next_cacheblock;	// Nachfolger des bisher neuesten Eintrags ist dieser Cacheblock  
	#endif
	recent_cacheblock = next_cacheblock;					// Dieser Cacheblock ist der neueste Eintrag
	return 0;
}

/*! 
 * Fordert virtuellen Speicher an
 * @param size		Groesse des gewuenschten Speicherblocks
 * @param aligned	0: egal, 1: 512 Byte ausgerichtet
 * @return			Virtuelle Anfangsadresse des angeforderten Speicherblocks, 0 falls Fehler 
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			30.11.2006
 */
uint32 mmcalloc(uint32 size, uint8 aligned){
	if (next_mmc_address == mmc_start_address){
		// TODO: Init-stuff here (z.B. FAT einlesen)	
		if (mmc_start_address > swap_space){
			mmc_start_address = swap_space-512;
			next_mmc_address = mmc_start_address;
		}
		#if MMC_ASYNC_WRITE == 1
			swap_buffer = malloc(512);
		#endif 
	}
	uint32 start_addr;
	if (aligned == 0 || mmc_get_end_of_page(next_mmc_address) == mmc_get_end_of_page(next_mmc_address+size-1)){
		/* Daten einfach an der naechsten freien virtuellen Adresse speichern */
		start_addr = next_mmc_address;
	} else {
		/* Rest der letzten Seite ueberspringen und Daten in neuem Block speichern */
		start_addr = mmc_get_end_of_page(next_mmc_address) + 1;
	}	
	if (start_addr+size > swap_space) return 0;	// wir haben nicht mehr virtuellen Speicher als Platz auf dem Swap-Device
	/* interne Daten aktualisieren */
	next_mmc_address = start_addr + size;
	used_mmc_blocks = mmc_get_mmcblock_of_page(next_mmc_address-1) + 1;
	return start_addr;
}

/*! 
 * Gibt einen Zeiger auf einen Speicherblock im RAM zurueck
 * @param addr	Eine virtuelle Adresse
 * @return		Zeiger auf uint8, NULL falls Fehler
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		30.11.2006
 */
uint8* mmc_get_data(uint32 addr){
	/* Seite der gewuenschten Adresse laden */
	if (mmc_load_page(addr) != 0) return NULL;
	int8 cacheblock = mmc_get_cacheblock_of_page(addr);
	page_cache[cacheblock].dirty = 1;	// Daten sind veraendert
	/* Zeiger auf Adresse in gecacheter Seite laden / berechnen und zurueckgeben */
	return page_cache[cacheblock].p_data + (addr - (mmc_get_mmcblock_of_page(addr)<<9));	// TODO: 2. Summanden schlauer berechnen
}

/*! 
 * Erzwingt das Zurueckschreiben einer eingelagerten Seite auf die MMC / SD-Card   
 * @param addr	Eine virtuelle Adresse
 * @return		0: ok, 1: Seite zurzeit nicht eingelagert, 2: Fehler beim Zurueckschreiben
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		15.12.2006
 */
uint8 mmc_page_write_back(uint32 addr){
	int8 cacheblock = mmc_get_cacheblock_of_page(addr);
	if (cacheblock < 0) return 1;	// Seite nicht eingelagert
	if (swap_out(page_cache[cacheblock].addr, page_cache[cacheblock].p_data, MMC_ASYNC_WRITE) != 0) return 2;	// Seite (evtl. asynchron) zurueckschreiben
	page_cache[cacheblock].dirty = 0;	// Dirty-Bit zuruecksetzen
	return 0;
}

/*! 
 * Erzwingt das Zurueckschreiben aller eingelagerten Seiten auf die MMC / SD-Card   
 * @return		0: alles ok, sonst: Summe der Fehler beim Zurueckschreiben
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		21.12.2006
 */
uint8 mmc_flush_cache(void){
	uint8 i;
	uint8 result=0;
	for (i=0; i<allocated_pages; i++){
		if (page_cache[i].addr < mmc_get_mmcblock_of_page(swap_space))
			result += swap_out(page_cache[i].addr, page_cache[i].p_data, 0);	// synchrones Zurueckschreiben
		page_cache[i].dirty = 0;
	}
	return result;	
}

/*! 
 * Oeffnet eine Datei im FAT16-Dateisystem auf der MMC / SD-Card und gibt eine virtuelle Adresse zurueck,
 * mit der man per mmc_get_data() einen Pointer auf die gewuenschten Daten bekommt. Das Ein- / Auslagern
 * macht das VM-System automatisch. Der Dateiname muss derzeit am Amfang in der Datei stehen.
 * Achtung: Irgendwann muss man die Daten per mmc_flush_cache() oder mmc_page_write_back() zurueckschreiben! 
 * @param filename	Dateiname als 0-terminierter String   
 * @return			Virtuelle Anfangsadresse der angeforderten Datei, 0 falls Fehler 
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			21.12.2006
 */
uint32 mmc_fopen(const char *filename){
	uint32 block;
	/* Pufferspeicher organisieren */
	uint32 v_addr = mmcalloc(512, 0);
	uint8* p_data = mmc_get_data(v_addr);	// hier speichern wir im Folgenden den ersten Block der gesuchten Datei, der ist dann gleich im Cache ;)
	/* Die Dateiadressen liegen ausserhalb des Bereichs fuer den VM, also interne Datenanpassungen hier rueckgaengig machen */
	next_mmc_address -= 512;
	used_mmc_blocks--;
	uint8 i;
	#ifdef DISPLAY_AVAILABLE		// Debug-Info ausgben
		display_cursor(2,1);
		display_printf("Find %c%c%c: 0x",filename[0],filename[1],filename[2]);
		uint16 k=0, j=0;
	#endif
	/* MMC-Block suchen zwischen Kartenanfang und VM-Startadresse (<= Kartengroesse) */
	for (block=0; block<mmc_get_mmcblock_of_page(mmc_start_address); block++){
		#ifdef DISPLAY_AVAILABLE	// Debug-Info ausgben
			display_cursor(2,13);
			display_printf("%02x%04x", j, k);
			if (k==65535)
				j++;
			k++;
		#endif
		if (swap_in(block, p_data) != 0) break;	// Abbrechen, falls Fehler
		/* Blockanfang mit Dateinamen vergleichen */
		for (i=0; i<FILENAME_MAX; i++){
			if (filename[i] == '\0'){
				page_cache[mmc_get_cacheblock_of_page(v_addr)].addr = block;	// Cache-Tag auf gefundene Datei umbiegen
				#ifdef DISPLAY_AVAILABLE
		  			k = block & 0xFFFF;
		  			j = (block >> 16) & 0xFFFF;
		  			display_cursor(2,1);
		  			display_printf("Found %c%c%c: 0x%02x%04x",filename[0],filename[1],filename[2],j,k);
				#endif				
				return block<<9;	// gesuchte Datei beginnt hier :)
			}
			if (filename[i] != p_data[i]) break;	// gesuchte Datei beginnt nicht in diesem Block
		}
	}
	/* Suche erfolglos, aber der Cache soll konsistent bleiben */
	// TODO: ordentlich aufraeumen im Fehlerfall!
	page_cache[mmc_get_cacheblock_of_page(v_addr)].addr = 0x800000;	// Diesen Sektor gibt es auf keiner Karte <= 4 GB 
	page_cache[mmc_get_cacheblock_of_page(v_addr)].dirty = 0;	// HackHack, aber so wird der ungueltige Inhalt beim Pagefault niemals versucht auf die Karte zu schreiben		
	return 0;	// Datei nicht gefunden :(	
}

#endif	// MMC_VM_AVAILABLE
