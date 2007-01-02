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
 * @file 	mmc_vm.h
 * @brief 	Virtual Memory Management mit MMC / SD-Card
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	30.11.2006
 */

#ifndef MMC_VM_H_
#define MMC_VM_H_

#include "ct-Bot.h"  

#ifdef MMC_VM_AVAILABLE

#define VM_STATS_AVAILABLE

#ifdef VM_STATS_AVAILABLE
	typedef struct{
		uint32 page_access;		/*!< Anzahl der Seitenzugriffe seit Systemstart */
		uint32 swap_ins;		/*!< Anzahl der Seiteneinlagerungen seit Systemstart */
		uint32 swap_outs;		/*!< Anzahl der Seitenauslagerungen seit Systemstart */
		uint32 vm_used_bytes;	/*!< Anzahl der vom VM belegten Bytes auf der MMC / SD-Card */
		uint32 device_size;		/*!< Groesse des Speichervolumes */
		uint32 vm_size;			/*!< Groesse des Virtuellen Speichers */
		uint8 cache_size;		/*!< Groesse des Caches */
		int8 cache_load;		/*!< Belegter Speicher des Caches */
		uint16 page_access_s;	/*!< Seitenzugriffe pro Sekunde */
		uint16 swap_ins_s;		/*!< Pagefaults pro Sekunde */	
		uint16 swap_outs_s;		/*!< Seitenauslagerungen pro Sekunde */		
		uint16 delta_t;			/*!< Sekunden seit Beginn der Messung */
	} vm_extern_stats_t;	

	/*! 
	 * Gibt die Anzahl der Pagefaults seit Systemstart bzw. Ueberlauf zurueck
	 * @return		#Pagefaults
	 * @author 		Timo Sandmann (mail@timosandmann.de)
	 * @date 		30.11.2006
	 */
	uint32 mmc_get_pagefaults(void);
	
	/*! 
	 * Erstellt eine kleine Statistik ueber den VM
	 * @return		Zeiger auf Statistikdaten
	 * @date 		01.01.2007
	 */	
	vm_extern_stats_t* mmc_get_vm_stats(void);
	
	/*! 
	 * Gibt eine kleine Statistik ueber den VM aus (derzeit nur am PC)
	 * @date 		01.01.2007
	 */		
	void mmc_print_statistic(void);
#endif

/*! 
 * Fordert virtuellen Speicher an
 * @param size		Groesse des gewuenschten Speicherblocks
 * @param aligned	0: egal, 1: 512 Byte ausgerichtet
 * @return			Virtuelle Anfangsadresse des angeforderten Speicherblocks, 0 falls Fehler 
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			30.11.2006
 */
uint32 mmcalloc(uint32 size, uint8 aligned);

/*! 
 * Gibt einen Zeiger auf einen Speicherblock im RAM zurueck
 * @param addr	Eine virtuelle Adresse
 * @return		Zeiger auf uint8, NULL falls Fehler
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		30.11.2006
 */
uint8* mmc_get_data(uint32 addr);

/*! 
 * Gibt die letzte Adresse einer Seite zurueck
 * @param addr	Eine virtuelle Adresse
 * @return		Adresse 
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		30.11.2006
 */
inline uint32 mmc_get_end_of_page(uint32 addr);

/*! 
 * Erzwingt das Zurueckschreiben einer eingelagerten Seite auf die MMC / SD-Card   
 * @param addr	Eine virtuelle Adresse
 * @return		0: ok, 1: Seite zurzeit nicht eingelagert, 2: Fehler beim Zurueckschreiben
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		15.12.2006
 */
uint8 mmc_page_write_back(uint32 addr);

/*! 
 * Erzwingt das Zurueckschreiben aller eingelagerten Seiten auf die MMC / SD-Card   
 * @return		0: alles ok, sonst: Fehler beim Zurueckschreiben
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		21.12.2006
 */
uint8 mmc_flush_cache(void);

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
uint32 mmc_fopen(const char *filename);

#endif	// MMC_VM_AVAILABLE
#endif	// MMC_VM_H_
