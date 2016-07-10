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
 * \file 	mmc-vm.h
 * \brief 	Virtual Memory Management mit MMC / SD-Card
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	30.11.2006
 * \see		Documentation/mmc-vm.html
 */

#ifndef MMC_VM_H_
#define MMC_VM_H_

#ifdef MMC_VM_AVAILABLE
#ifdef MCU
#include <avr/pgmspace.h>
#else
#define PSTR(x)	x
#endif // MCU

//#define VM_STATS_AVAILABLE		/**< Schaltet die Leistungsdatensammlung ein und ermoeglicht die Ausgabe einer Statistik */

#ifdef VM_STATS_AVAILABLE
	typedef struct {
		uint32_t page_access;		/**< Anzahl der Seitenzugriffe seit Systemstart */
		uint32_t swap_ins;		/**< Anzahl der Seiteneinlagerungen seit Systemstart */
		uint32_t swap_outs;		/**< Anzahl der Seitenauslagerungen seit Systemstart */
		uint32_t vm_used_bytes;	/**< Anzahl der vom VM belegten Bytes auf der MMC / SD-Card */
		uint32_t device_size;		/**< Groesse des Speichervolumes */
		uint32_t vm_size;			/**< Groesse des Virtuellen Speichers */
		uint8_t cache_size;		/**< Groesse des Caches */
		int8_t cache_load;		/**< Belegter Speicher des Caches */
		uint16_t page_access_s;	/**< Seitenzugriffe pro Sekunde */
		uint16_t swap_ins_s;		/**< Pagefaults pro Sekunde */
		uint16_t swap_outs_s;		/**< Seitenauslagerungen pro Sekunde */
		uint16_t delta_t;			/**< Sekunden seit Beginn der Messung */
	} vm_extern_stats_t;	

	/**
	 * Gibt die Anzahl der Pagefaults seit Systemstart bzw. Ueberlauf zurueck
	 * \return		#Pagefaults
	 */
	uint32_t mmc_get_pagefaults(void);
	
	/**
	 * Erstellt eine kleine Statistik ueber den VM
	 * \return		Zeiger auf Statistikdaten
	 */	
	vm_extern_stats_t * mmc_get_vm_stats(void);
	
	/**
	 * Gibt eine kleine Statistik ueber den VM aus (derzeit nur am PC)
	 */		
	void mmc_print_statistic(void);
#endif // VM_STATS_AVAILABLE

/**
 * Fordert virtuellen Speicher an
 * \param size		Groesse des gewuenschten Speicherblocks
 * \param aligned	ignored
 * \return			Virtuelle Anfangsadresse des angeforderten Speicherblocks in Byte, 0 falls Fehler
 */
uint32_t mmcalloc(uint32_t size, uint8_t aligned);

/**
 * Gibt einen Zeiger auf einen Speicherblock im RAM zurueck
 * \param addr	Eine virtuelle Adresse in Byte
 * \return		Zeiger auf uint8_t, NULL falls Fehler
 */
uint8_t * mmc_get_data(uint32_t addr);

/**
 * Erzwingt das Zurueckschreiben einer eingelagerten Seite auf die MMC / SD-Card   
 * \param addr	Eine virtuelle Adresse in Byte
 * \return		0: ok, 1: Seite zurzeit nicht eingelagert, 2: Fehler beim Zurueckschreiben
 */
uint8_t mmc_page_write_back(uint32_t addr);

/**
 * Schreibt alle eingelagerten Seiten auf die MMC / SD-Card zurueck  
 * \return		0: alles ok, sonst: Fehler beim Zurueckschreiben
 */
uint8_t mmc_flush_cache(void);

/**
 * Oeffnet eine Datei im FAT16-Dateisystem auf der MMC / SD-Card und gibt eine virtuelle Adresse zurueck,
 * mit der man per mmc_get_data() einen Pointer auf die gewuenschten Daten bekommt. Das Ein- / Auslagern
 * macht das VM-System automatisch. Der Dateiname muss derzeit am Amfang in der Datei stehen.
 * Achtung: Irgendwann muss man die Daten per mmc_flush_cache() oder mmc_page_write_back() zurueckschreiben! 
 * \param filename	Dateiname als 0-terminierter String im Flash
 * \return			Virtuelle Anfangsadresse der angeforderten Datei in Byte, 0 falls Fehler
 */
uint32_t mmc_fopen_P(const char * filename);

/**
 * Oeffnet eine Datei im FAT16-Dateisystem auf der MMC / SD-Card und gibt eine virtuelle Adresse zurueck,
 * mit der man per mmc_get_data() einen Pointer auf die gewuenschten Daten bekommt. Das Ein- / Auslagern
 * macht das VM-System automatisch. Der Dateiname muss derzeit am Amfang in der Datei stehen.
 * Achtung: Irgendwann muss man die Daten per mmc_flush_cache() oder mmc_page_write_back() zurueckschreiben! 
 * \param filename	Dateiname als 0-terminierter String
 * \return			Virtuelle Anfangsadresse der angeforderten Datei in Byte, 0 falls Fehler
 */
#define mmc_fopen(filename)	mmc_fopen_P(PSTR(filename))

/**
 * Leert eine Datei im FAT16-Dateisystem auf der MMC / SD-Card, die zuvor mit mmc_fopen() geoeffnet wurde.
 * \param file_start	(virtuelle) Anfangsadresse der Datei in Byte
 */
void mmc_clear_file(uint32_t file_start);

/**
 * Liest die Groesse einer Datei im FAT16-Dateisystem auf der MMC / SD-Card aus, die zu zuvor mit 
 * mmc_fopen() geoeffnet wurde.
 * \param file_start	(virtuelle) Anfangsadresse der Datei in Byte
 * \return				Groesse der Datei in Byte
 */
uint32_t mmc_get_filesize(uint32_t file_start);

#endif // MMC_VM_AVAILABLE
#endif // MMC_VM_H_
