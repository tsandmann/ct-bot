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

/*! @file 	mini-fat.c
 * @brief 	Routinen zum Auffinden von markierten Files auf einer MMC-Karte. 
 *          Dies ist keine vollstaendige FAT-Unterstuetzung, sondern sucht nur eien Datei, die mit einer 3-Zeichen Sequenz beginnt. 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @date 	07.11.06
*/

#include "ct-Bot.h"

#ifdef MCU
#ifdef MINI_FAT_AVAILABLE

#include "mmc.h"
#include "display.h"

/*!
 * Sucht einen Block auf der MMC-Karte, dessen erste drei Bytes dem key entsprechen
 * liefert dann den folgenden Block zurueck.
 * Achtung das prinzip geht nur, wenn die Dateien nicht fragmentiert sind
 * @param key 3 Byte zur Identifikation
 * @param buffer Zeiger auf 512 Byte Puffer im SRAM
 */
uint32 mini_fat_find_block(const char key[3], uint8* buffer){
		
		// Suche nach der Datei fuer die Katrte
		int8 found = False;
		
		uint32 card_size= mmc_get_size() >> 9; // groesse der Karte in Bloecken
		
		#ifdef DISPLAY_AVAILABLE
		  display_cursor(2,1);
		  display_printf("Find %c%c%c: 0x",key[0],key[1],key[2]);
		  uint16 i=0, j=0;
		#endif
		
		uint32 block=0;
		while(found == False && block < card_size){
			#ifdef DISPLAY_AVAILABLE
				display_cursor(2,13);
				display_printf("%02x%04x",j,i );
				if (i==65535)
					j++;
				i++;
			#endif
			mmc_read_sector(block++,buffer);
			uint8 i;
			for (i=0; i<3; i++)
				if (buffer[i]==key[i])
					found = True;
		}
		
		if (found == False)
			 return 0xFFFFFFFF;

		#ifdef DISPLAY_AVAILABLE
		  i= block & 0xFFFF;
		  j= (block >> 16) & 0xFFFF;
		  display_cursor(2,1);
		  display_printf("Found %c%c%c: 0x%02x%04x",key[0],key[1],key[2],j,i);
		#endif

		// auf der Karte markieren, dass wir sie in der Hand hatten
		buffer[3]++;
		mmc_write_sector(block-1,buffer,0);

		return block;
}

#endif
#endif
