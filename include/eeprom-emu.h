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
 * @file 	eeprom-emu.h  
 * @brief 	Low-Level Routinen fuer den Zugriff auf das EEPROM des c't-Bots
 * @author 	Achim Pankalla (achim.pankalla@gmx.de)
 * @date 	01.03.07
 */
 
#ifndef eeprom_emu_H_
#define eeprom_emu_H_

#ifdef PC
#include <stddef.h>

/*! 
 * Diese Funktion initialisiert die eeprom-emulation. Sie sorgt fuer die Erstellung der
 * eeprom.bin, falls nicht vorhanden und erstellt ueber eine Hilfsfunktion eine Adress-
 * konvertierungstabelle fuer die EEPROM-Adressen, wenn die benoetigten Daten vorliegen.
 * Statusinformationen werden ueber DEBUG_INFO angezeigt.
 * @param init	gibt an, ob das EEPROM mit Hilfer einer eep-Datei initialisiert werden soll (0 nein, 1 ja)
 * @return		0: alles ok, 1: Fehler
 */
uint8_t init_eeprom_man(uint8_t init);

/*!
 * Speichert ein Byte im EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @param value	Das abzulegende Byte
 */ 
void eeprom_write_byte(uint8_t * addr, uint8_t value);

/*!
 * Speichert ein Word im EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @param value	Das abzulegende Word
 */ 
void eeprom_write_word(uint16_t * addr, uint16_t value);

/*! 
 * Laedt ein Byte aus dem EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @return 		Wert der Speicheraddresse im EEPROM
 */ 
uint8_t eeprom_read_byte(const uint8_t * addr);

/*! 
 * Laedt ein Word aus dem EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @return 		Wert der Speicheraddresse im EEPROM
 */ 
uint16_t eeprom_read_word(const uint16_t * addr);

/*! 
 * Kopiert einen Block aus dem EEPROM ins RAM
 * @param pointer_ram		Adresse im RAM
 * @param pointer_eeprom	Adresse im EEPROM
 * @param size				Groesse des Blocks in Byte
 */ 
void eeprom_read_block(void *pointer_ram, const void *pointer_eeprom, size_t size);

/*! 
 * Kopiert einen Block vom RAM in das EEPROM
 * @param pointer_ram		Adresse im RAM
 * @param pointer_eeprom	Adresse im EEPROM
 * @param size				Groesse des Blocks in Byte
 */ 
void eeprom_write_block(const void *pointer_ram, void *pointer_eeprom, size_t size);

#endif	// PC
#endif	// eeprom_emu_H_
