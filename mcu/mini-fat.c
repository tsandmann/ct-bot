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
 * @file 	mcu/mini-fat.c
 * @brief 	Routinen zum Auffinden von markierten Files auf einer MMC-Karte. 
 *          Dies ist keine vollstaendige FAT-Unterstuetzung, sondern sucht nur eien Datei, die mit einer Zeichen Sequenz beginnt. 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @date 	07.11.2006
 */

#include "ct-Bot.h"

/* EEPROM-Variable immer deklarieren, damit die Adresse sich nicht veraendert je nach #define */
uint32_t EEPROM eefat[10] = {0};	/*!< EEPROM-Cache fuer FAT-Eintraege */

#ifdef MCU
#include <avr/eeprom.h>

#ifdef MMC_AVAILABLE	

#include "mmc.h"
#include "display.h"
#include "mini-fat.h"

/*!
 * @brief			Sucht die Adresse einer Mini-FAT-Datei im EERROM
 * @param filename	Datei-ID
 * @param buffer	Zeiger auf 512 Byte großen Speicherbereich (wird ueberschrieben)
 * @return			(Block-)Adresse des ersten Nutzdatenblock der gesuchten Datei oder 0, falls nicht im EEPROM
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			05.03.2007
 * Das Nachschlagen im EEPROM und durchprobieren der Adressen geht relativ schnell, somit verkuerzt sich die Wartezeit
 * beim Dateioeffnen von einer MMC erheblich durch diese Methode. Die Adressen (32 Bit) liegen in insgesamt 10 Slabs
 * im EEPROM.
 */
uint32_t mini_fat_lookup_adr(const char * filename, uint8_t * buffer) {
	uint8_t i;
	uint32_t * p_eefat = eefat;
	/* EEPROM-Slabs nach gewuenschter Datei-ID durchsuchen */
	for (i=0; i<10; i++){
		uint32 block;
		eeprom_read_block(&block, p_eefat++, sizeof(block));	// Adresse erst aus dem EEPROM laden
		if (mmc_read_sector(block, buffer) != 0) return 0;
		/* Datei-ID vergleichen */
		if (strcmp_P((char *)buffer, filename) == 0) {
//			display_cursor(1,1);
//			display_printf("eefound %u: ", i);
//			display_printf("%6x", block);
			return ++block;
		}
	}
	return 0;
}

/*!
 * @brief			Speichert die Adresse einer Mini-FAT-Datei in einem EERROM-Slab
 * @param block		(Block-)Adresse der Datei, die gespeichert werden soll
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			05.03.2007
 * Gespeichert wird die Adresse des 0. Blocks der Datei (man gibt aber die Adresse des ersten Nutzdatenblocks an, wie sie
 * z.B. mini_fat_find_block() liefert) in einem EEPROM-Slab. Derzeit gibt es 10 Slabs, sind alle belegt (d.h. != 0), speichert
 * diese Funktion die uebergebene Adresse nicht.
 */
void mini_fat_store_adr(uint32_t block) {
	uint8_t i;
	uint32_t * p_eefat = eefat;
	block--;	// Block mit der Datei-ID speichern, nicht ersten Nutzdatenblock
	/* freien Block im EEPROM suchen */
	for (i=0; i<10; i++){
		uint32_t tmp;
		eeprom_read_block(&tmp, p_eefat++, sizeof(tmp));
		if (tmp == 0){	// hier noch ist Platz :-)
			eeprom_write_block(&block, --p_eefat, sizeof(block));	// Adresse speichern
//			display_cursor(3,1);
//			display_printf("eestore %u: ", i);
//			display_printf("%6x", block);
			return;	// fertig	
		} 
	}
	eeprom_write_block(&block, &eefat[TCNT2%10], sizeof(block));	// Adresse an zufaelliger Position speichern
}

/*!
 * @brief			Sucht einen Block auf der MMC-Karte, dessen erste Bytes dem Dateinamen entsprechen
 * @param filename 	String im Flash zur Identifikation
 * @param buffer 	Zeiger auf 512 Byte Puffer im SRAM
 * @param end_addr	Byte-Adresse, bis zu der gesucht werden soll
 * @return			Anfangsblock der Nutzdaten der Datei
 * Achtung das Prinzip geht nur, wenn die Dateien nicht fragmentiert sind 
 */
uint32_t mini_fat_find_block_P(const char * filename, uint8_t * buffer, uint32_t end_addr) {
	end_addr >>= 9;	// letzte Blockadresse ermitteln
	
	/* zunaechst im EEPROM-FAT-Cache nachschauen */
	uint32_t block = mini_fat_lookup_adr(filename, buffer);
	if (block != 0)	return block;
	
#ifdef DISPLAY_AVAILABLE
	display_cursor(2,1);
	display_printf("%s:", filename);
	display_cursor(3,1);
	display_printf("0x");
	uint16_t i=0, j=0;
#endif	// DISPLAY_AVAILABLE
	
	/* sequenziell die Karte durchsuchen */
	for (block=0; block<end_addr; block++) {
#ifdef DISPLAY_AVAILABLE
		display_cursor(3,3);
		display_printf("%02x%04x",j,i);
		if (i == 65535) j++;
		i++;
#endif	// DISPLAY_AVAILABLE
		if (mmc_read_sector(block, buffer) != 0) return 0xffffffff;
		if (strcmp_P((char *)buffer, filename) == 0) {
			/* gefunden, Nutzdaten laden und Adresse ins EEPROM schreiben */
			if (mmc_read_sector(++block, buffer) != 0) return 0xffffffff;
			mini_fat_store_adr(block);
#ifdef DISPLAY_AVAILABLE
			i = block & 0xFFFF;
			j = (block >> 16) & 0xFFFF;
			display_cursor(3,1);
			display_printf("Found: 0x%02x%04x",j,i);
#endif	// DISPLAY_AVAILABLE
			return block;
		}
	}
	return 0xffffffff;
}

#endif	// MMC_AVAILABLE
#endif	// MCU
