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
 * @see		Documentation/mmc-vm.html
 */


//TODO:	* Statistikausgabe fuer MCU ergaenzen
//		* Code optimieren, Groesse und Speed

#include "ct-Bot.h"  

#ifdef MMC_VM_AVAILABLE

#include "mmc-vm.h"
#include "mmc.h"
#include "mmc-low.h"
#include "mmc-emu.h"
#include "display.h"
#include "timer.h"
#include "mini-fat.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef MCU
	#define MMC_START_ADDRESS 0x2000000	// [512;2^32-1] in Byte - Sinnvoll ist z.B. Haelfte der MMC / SD-Card Groesse, der Speicherplatz davor kann dann fuer ein Dateisystem verwendet werden
	#define MAX_SPACE_IN_SRAM 5			// [1;127] - Pro Page werden 512 Byte im SRAM belegt, sobald diese verwendet wird
	#define swap_out	mmc_write_sector
	#define swap_in		mmc_read_sector
	#define swap_space	mmc_get_size()
#else
	#define MMC_START_ADDRESS 0x1000000	// [512;2^32-1]
	#define MAX_SPACE_IN_SRAM 2			// [1;127] - Pro Page werden 512 Byte im RAM belegt, sobald diese verwendet wird
	#define swap_out	mmc_emu_write_sector
	#define swap_in		mmc_emu_read_sector
	#define swap_space	mmc_emu_get_size()
#endif	

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

#ifdef VM_STATS_AVAILABLE
	typedef struct{
		uint32 page_access;		/*!< Anzahl der Seitenzugriffe seit Systemstart */
		uint32 swap_ins;		/*!< Anzahl der Seiteneinlagerungen seit Systemstart */
		uint32 swap_outs;		/*!< Anzahl der Seitenauslagerungen seit Systemstart */
		uint32 vm_used_bytes;	/*!< Anzahl der vom VM belegten Bytes auf der MMC / SD-Card */
		uint32 time;			/*!< Timestamp bei erster Speicheranforderung */
	} vm_stats_t;
	
	vm_stats_t stats_data = {0};	
#endif

static uint32 mmc_start_address = MMC_START_ADDRESS;	/*!< physische Adresse der MMC / SD-Card, wo unser VM beginnt */
static uint32 next_mmc_address = MMC_START_ADDRESS;		/*!< naechste freie virtuelle Adresse */
static uint8 pages_in_sram = MAX_PAGES_IN_SRAM;			/*!< Groesse des Caches im RAM */
static int8 allocated_pages = 0;						/*!< Anzahl bereits belegter Cachebloecke */
static uint8 oldest_cacheblock = 0;						/*!< Zeiger auf den am laengsten nicht genutzten Eintrag (LRU) */
static uint8 recent_cacheblock = 0;						/*!< Zeiger auf den letzten genutzten Eintrag (LRU) */

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

#ifdef VM_STATS_AVAILABLE
	/*! 
	 * Gibt die Anzahl der Pagefaults seit Systemstart bzw. Ueberlauf zurueck
	 * @return		#Pagefaults
	 * @author 		Timo Sandmann (mail@timosandmann.de)
	 * @date 		30.11.2006
	 */
	inline uint32 mmc_get_pagefaults(void){
		return stats_data.swap_ins;
	}

	/*! 
	 * Erstellt eine kleine Statistik ueber den VM
	 * @return		Zeiger auf Statistikdaten
	 * @date 		01.01.2007
	 */	
	vm_extern_stats_t* mmc_get_vm_stats(void){
		static vm_extern_stats_t extern_stats = {0};
		memcpy(&extern_stats, &stats_data, sizeof(vm_stats_t));	// .time wird spaeter ueberschrieben
		uint16 delta_t = TICKS_TO_MS(TIMER_GET_TICKCOUNT_32 - stats_data.time)/1000;
		if (delta_t == 0) delta_t = 1;
		extern_stats.page_access_s = extern_stats.page_access / delta_t;
		extern_stats.swap_ins_s = extern_stats.swap_ins / delta_t;
		extern_stats.swap_outs_s = extern_stats.swap_outs / delta_t;		
		extern_stats.device_size = swap_space;
		extern_stats.vm_size = swap_space - mmc_start_address;
		extern_stats.cache_size = pages_in_sram;
		extern_stats.cache_load = allocated_pages;
		extern_stats.delta_t = delta_t;
		return &extern_stats;	
	}

	/*! 
	 * Gibt eine kleine Statistik ueber den VM aus (derzeit nur am PC)
	 * @date 		01.01.2007
	 */		
	void mmc_print_statistic(void){
		#ifdef PC
			vm_extern_stats_t* vm_stats = mmc_get_vm_stats();
			printf("\n\r*** VM-Statistik *** \n\r");
			printf("Groesse des Volumes: \t\t%lu MByte \n\r", vm_stats->device_size>>20);
			printf("Groesse des VM: \t\t%lu MByte \n\r", vm_stats->vm_size>>20);
			printf("Belegter virt. Speicher: \t%lu KByte \n\r", vm_stats->vm_used_bytes>>10);									
			printf("Groesse des Caches: \t\t%u Byte \n\r", (uint16)vm_stats->cache_size<<9);
			printf("Auslastung des Caches: \t\t%u %% \n\r", ((uint16)vm_stats->cache_load<<9)/((uint16)vm_stats->cache_size<<9)*100);
			printf("Seitenzugriffe: \t\t%lu \n\r", vm_stats->page_access);
			printf("Seiteneinlagerungen: \t\t%lu \n\r", vm_stats->swap_ins);
			printf("Seitenauslagerungen: \t\t%lu \n\r", vm_stats->swap_outs);
			printf("Seitenzugriffe / s: \t\t%u \n\r", vm_stats->page_access_s);
			printf("Seiteneinlagerungen / s: \t%u \n\r", vm_stats->swap_ins_s);
			printf("Seitenauslagerungen / s: \t%u \n\r", vm_stats->swap_outs_s);
			printf("Cache-Hit-Rate: \t\t%f %% \n\r", (100.0-((float)vm_stats->swap_ins/(float)vm_stats->page_access)*100.0));
			printf("Messdauer: \t\t\t%u s \n\r", vm_stats->delta_t);
		#else
			/* Ausgabe fuer MCU derzeit nur ueber Logging */
			#ifdef LOG_AVAILABLE 
				vm_extern_stats_t* vm_stats = mmc_get_vm_stats();
				/* Texte wie oben */
				LOG_INFO(("%lu", vm_stats->device_size>>20));
				LOG_INFO(("%lu", vm_stats->vm_size>>20));
				LOG_INFO(("%lu", vm_stats->vm_used_bytes>>10));									
				LOG_INFO(("%u", (uint16)vm_stats->cache_size<<9));
				LOG_INFO(("%u", ((uint16)vm_stats->cache_load<<9)/((uint16)vm_stats->cache_size<<9)*100));
				LOG_INFO(("%lu", vm_stats->page_access));
				LOG_INFO(("%lu", vm_stats->swap_ins));
				LOG_INFO(("%lu", vm_stats->swap_outs));
				LOG_INFO(("%u", vm_stats->page_access_s));
				LOG_INFO(("%u", vm_stats->swap_ins_s));
				LOG_INFO(("%u", vm_stats->swap_outs_s));
				LOG_INFO(("%u", (uint8)(100.0-((float)vm_stats->swap_ins/(float)vm_stats->page_access)*100.0)));
				LOG_INFO(("%u", vm_stats->delta_t));				
			#endif
			// TODO: Display-Ausgabe?
		#endif	
	}
#endif

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
	int8 i;
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
	if (addr >= next_mmc_address) return 1;	// ungueltige virtuelle Adresse :(
	#ifdef VM_STATS_AVAILABLE
		stats_data.page_access++;
	#endif
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
	}
	/* Pager muss nun aktiv werden */
	#ifdef VM_STATS_AVAILABLE
		stats_data.swap_ins++;
	#endif
	#if MMC_ASYNC_WRITE == 1	// im asnychronen Fall holen wir erst die neue Seite, dann kann sich das Zurueckschreiben ruhig Zeit lassen
		uint8* p_tmp = page_cache[next_cacheblock].p_data;
		if (swap_in(mmc_get_mmcblock_of_page(addr), swap_buffer) != 0) return 3;
	#endif
	if (page_cache[next_cacheblock].dirty == 1){	// Seite zurueckschreiben, falls Daten veraendert wurden
		#ifdef VM_STATS_AVAILABLE
			stats_data.swap_outs++;
		#endif
		if (swap_out(page_cache[next_cacheblock].addr, page_cache[next_cacheblock].p_data, MMC_ASYNC_WRITE) != 0) return 2;
	}
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
		/* Inits */
		if (mmc_start_address > swap_space){
			mmc_start_address = swap_space-512;
			next_mmc_address = mmc_start_address;
		}
		#if MMC_ASYNC_WRITE == 1
			swap_buffer = malloc(512);
		#endif 
		#ifdef VM_STATS_AVAILABLE
			stats_data.time = TIMER_GET_TICKCOUNT_32;
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
	#ifdef VM_STATS_AVAILABLE
		stats_data.vm_used_bytes = next_mmc_address-mmc_start_address;
	#endif
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
 * Schreibt alle eingelagerten Seiten auf die MMC / SD-Card zurueck  
 * @return		0: alles ok, sonst: Summe der Fehler beim Zurueckschreiben
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		21.12.2006
 */
uint8 mmc_flush_cache(void){
	uint8 i;
	uint8 result=0;
	for (i=0; i<allocated_pages; i++){
		if (page_cache[i].dirty == 1){
			if (page_cache[i].addr < mmc_get_mmcblock_of_page(swap_space))
				result += swap_out(page_cache[i].addr, page_cache[i].p_data, 0);	// synchrones Zurueckschreiben
			page_cache[i].dirty = 0;
		}
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
	if (p_data == NULL) return 0;
	/* Die Dateiadressen liegen ausserhalb des Bereichs fuer den VM, also interne Datenanpassungen hier rueckgaengig machen */
	next_mmc_address -= 512;
	#ifdef VM_STATS_AVAILABLE
		stats_data.vm_used_bytes -= 512;
	#endif
	uint8 i;
	#ifdef MCU	// Debug-Info ausgeben
		#ifdef DISPLAY_AVAILABLE
			display_cursor(2,1);
			display_printf("Find %s: 0x",filename);
			uint16 k=0, j=0;
		#endif
	#else
		printf("Find %s...",filename);
		uint16 k=0, j=0;	
	#endif
//	uint32 end, start = TIMER_GET_TICKCOUNT_32;
	/* MMC-Block suchen zwischen Kartenanfang und VM-Startadresse (<= Kartengroesse) */
	for (block=0; block<mmc_get_mmcblock_of_page(mmc_start_address); block++){
		if (swap_in(block, p_data) != 0) break;	// Abbrechen, falls Fehler
		#ifdef MCU	// Debug-Info ausgeben
			#ifdef DISPLAY_AVAILABLE
				display_cursor(2,13);
				display_printf("%02x%04x", j, k);
				if (k==65535) j++;
				k++;
			#endif		
		#else
//			printf(".");	
//			fflush(stdout);
		#endif		
		/* Blockanfang mit Dateinamen vergleichen */
		for (i=0; i<MMC_FILENAME_MAX; i++){
			if (filename[i] == '\0'){
				if (swap_in(++block, p_data) != 0) break;	// Ersten Sektor der Datei ueberspringen, dort stehen interne Daten
				page_cache[mmc_get_cacheblock_of_page(v_addr)].addr = block;	// Cache-Tag auf gefundene Datei umbiegen
//				end = TIMER_GET_TICKCOUNT_32;
				#ifdef MCU
					#ifdef DISPLAY_AVAILABLE
			  			k = block & 0xFFFF;
			  			j = (block >> 16) & 0xFFFF;
			  			display_cursor(2,1);
			  			display_printf("Found %s: 0x%02x%04x",filename,j,k);
//			  			display_cursor(3,1);
//			  			display_printf("Ticks: %u ", end-start);
					#endif
				#else
		  			k = block & 0xFFFF;
		  			j = (block >> 16) & 0xFFFF;
		  			printf("\n\rFound %s: 0x%02x%04x \n\r",filename,j,k);			
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
	#ifdef MCU
		#ifdef DISPLAY_AVAILABLE
			display_cursor(2,1);
			display_printf("%s not found ",filename);
		#endif	
	#else	
		printf("\n\r%s not found \n\r",filename);
	#endif		
	return 0;	// Datei nicht gefunden :(	
}

/*!
 * Liest die Groesse einer Datei im FAT16-Dateisystem auf der MMC / SD-Card aus, die zu zuvor mit 
 * mmc_fopen() geoeffnet wurde.
 * @param file_start	(virtuelle Anfangsadresse der Datei)
 * @return				Groesse der Datei in Byte
 * @date				12.01.2007
 */
uint32 mmc_get_filesize(uint32 file_start){
	file_len_t length;
	uint8* p_addr = mmc_get_data(file_start-512);
	/* Dateilaenge aus Block 0, Byte 256 bis 259 der Datei lesen */
	uint8 i;
	for (i=0; i<4; i++)
		length.u8[i] = p_addr[259-i];
	return length.u32;
}

/*! 
 * Leert eine Datei im FAT16-Dateisystem auf der MMC / SD-Card, die zuvor mit mmc_fopen() geoeffnet wurde.
 * @param file_start	(virtuelle) Anfangsadresse der Datei 
 * @return				0: ok, 1: ungueltige Datei oder Laenge, 2: Fehler beim Schreiben
 * @date 				02.01.2007
 */
uint8 mmc_clear_file(uint32 file_start){
	#ifdef PC
		printf("Start of file: %lu \n\r", file_start);
	#else
//		display_cursor(3,1);
//		display_printf("Start:0x%04x", file_start>>9);
	#endif
	uint32 length = mmc_get_filesize(file_start);
	if (file_start == 0 || file_start + length >= mmc_start_address) return 1;	// Datei existiert nicht oder Laenge ist ungueltig
	/* Ersten Block der Datei laden (dessen Speicherbereich benutzen wir einfach als 0-Puffer) */
	uint8* p_addr = mmc_get_data(file_start);
	memset(p_addr, 0, 512);	// leeren Puffer erzeugen
	/* Alle Bloecke der Datei mit dem 0-Puffer ueberschreiben */
	int8 cache_block;
	uint32 addr;
	for (addr=file_start; addr<file_start+length; addr+=512){
		if (swap_out(mmc_get_mmcblock_of_page(addr), p_addr, 0) != 0) return 2;
		display_cursor(3,1);
		display_printf("0x%04x", addr>>9);
		/* Falls ein Block der Datei im Cache ist, auch diesen leeren */
		cache_block = mmc_get_cacheblock_of_page(addr);
		if (cache_block >= 0){
			memset(page_cache[cache_block].p_data, 0, 512);
			page_cache[cache_block].dirty = 0;
		}
	}
	#ifdef PC
		printf("End of file: %lu \n\r", addr);
	#else
		display_cursor(3,1);
		display_printf("End:0x%04x", addr>>9);
	#endif	
	return 0;
}

#endif	// MMC_VM_AVAILABLE
