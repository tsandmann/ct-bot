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

#ifdef PC

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*! Erzeugt eine Datei, die an den ersten 3 Byte die ID- enthaelt. dann folgen 512 - sizeof(id) nullen
 * Danach kommen so viele size kByte Nullen
 * @param filename Der Dateiname der zu erzeugenden Datei
 * @param id_string Die ID des Files, wie sie zu beginn steht
 * @param size kByte Nutzdaten, die der MCU spaeter beschreiben darf
 */
void create_mini_fat_file(char * filename, char * id_string, uint32 size){
	// Einen 1-kByte-Block mit Nullen vorbereiten
	uint8 dummy[1024];
	uint32 i;
	for (i=0; i< sizeof(dummy); i++)
		dummy[i]=0;


	printf("Erstelle eine Mini-Fat-Datei mit dem Namen %s\n",filename);
	FILE *fp = fopen(filename, "w");
	
	char id [4] = "   ";
	
	for (i=0; (i< strlen(id_string) && i<3); i++)
		id[i]=id_string[i];
		
	printf("Schreibe ID: \"%s\"\n",id);
	fwrite(&id,3,1,fp);
	fwrite(&dummy, 512-3,1, fp);

	printf("Erzeuge Speicherplatz fuer %lu kByte Nutzdaten\n",size);	
	for (i=0; i< size; i++)
		fwrite(&dummy,1024,1,fp);
	
	fclose(fp);
	
}

#endif
