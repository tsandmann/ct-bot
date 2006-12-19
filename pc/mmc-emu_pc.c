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
 
 //TODO:	* Komplette Implementierung ;-)

#include "ct-Bot.h"

#ifdef PC
#ifdef MMC_VM_AVAILABLE

/*!
 * Liest einen Block von der emulierten Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param buffer 	Puffer von mindestens 512 Byte
 * @return 			0 wenn alles ok ist 
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @see				mcu/mmc.c
 * @date 			10.12.2006
 */	
uint8 mmc_emu_read_sector(uint32 addr, uint8* buffer){
	// TODO Implementierung
	uint16 i;
	for (i=0; i<512; i++){
		buffer[i] = 0;
	}
	return 0;	
}

/*! 
 * Schreibt einen 512-Byte Sektor auf die emulierte Karte
 * @param addr 		Nummer des 512-Byte Blocks
 * @param buffer 	Zeiger auf den Puffer
 * @param async		Wird bei der PC-Version nicht ausgewertet
 * @return 			0 wenn alles ok ist
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			10.12.2006
 * @see				mcu/mmc.c
 */
uint8 mmc_emu_write_sector(uint32 addr, uint8* buffer, uint8 async){
	// TODO Implementierung
	return 0;	
}

/*!
 * Liefert die Groesse der Karte zurueck
 * @return Groesse der emulierten Karte in Byte.
 */
uint32 mmc_emu_get_size(void){
	return 0xFFFFFFFF;	
}

#endif	// MMC_VM_AVAILABLE
#endif	// PC
