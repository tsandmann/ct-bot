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
 * @brief 	Routinen zum erstellen von markierten Files fuer eine MMC-Karte. 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	04.01.07
*/

#include "ct-Bot.h"
#include "mini-fat.h"

#ifdef PC

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*! 
 * Erzeugt eine Datei, die an den ersten Bytes die ID enthaelt. Dann folgen 512 - sizeof(id) nullen
 * Danach kommen so viele size kByte Nullen
 * @param filename Der Dateiname der zu erzeugenden Datei
 * @param id_string Die ID des Files, wie sie zu beginn steht
 * @param size kByte Nutzdaten, die der MCU spaeter beschreiben darf
 */
void create_mini_fat_file(const char* filename, const char* id_string, uint32 size){
	printf("Erstelle eine Mini-Fat-Datei mit dem Namen %s\n",filename);
	FILE *fp = fopen(filename, "w");
	
	/* Dateiparameter vorbereiten */
	uint8 id_len = strlen(id_string) >= MMC_FILENAME_MAX ? 254 : strlen(id_string);
	file_len_t length = {size*1024 - 512};	// im ersten Block stehen interne Daten
	
	printf("Schreibe ID: \"%s\"\n",id_string);
	fwrite(id_string,id_len,1,fp);
	
	/* Dateilaenge in die Datei schreiben */
	fseek(fp, 256, SEEK_SET);
	int8 i;
	for (i=3; i>=0; i--)
		putc(length.u8[i], fp);

	printf("Erzeuge Speicherplatz fuer %lu kByte Nutzdaten\n",size);	
	fseek(fp, size*1024-1, SEEK_SET);	// Ans Dateiende springen
	putc(0, fp);	// eine Null schreiben
	fclose(fp);	
}

#endif
