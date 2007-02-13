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

/*! 
 * Konvertiert eine (binaere) mini-fat-Datei ("AVR-Endian") mit Speed-Log-Daten in eine Textdatei.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	10.02.2007  
 * @param filename Der Dateiname mini-fat-Datei
 */
void convert_slog_file(const char* input_file){
	typedef struct{
		uint8 encRate;
		uint8 targetRate;
		int16 err;
		int16 pwm;
		uint32 time;
	} slog_t;				/*!< Datentyp der Logbloecke auf der MMC */
		
	printf("Konvertiere die SpeedLog-Datei %s ins txt-Format\n", input_file);
	FILE *fp_input = fopen(input_file, "r");
	FILE *fp_output = fopen("slog.txt", "w");
	if (fseek(fp_input, 0L, SEEK_END) != 0) return;
	uint32 filesize = ftell(fp_input)+1;
	uint32 j;
	uint8 k;
	uint8 data[512];
	for (k=0; k<2; k++){	// einmal fuer links und einmal fuer rechts
		fseek(fp_input, 512L, SEEK_SET);	// erster Sektor enthaelt nur Dateiname und Groesse
		fwrite("Ist-Geschw.\tSoll-Geschw.\tFehler\tPWM\tTimestemp\n", 46, 1, fp_output); 
		/* blockweise Daten einlesen */
		for (j=0; j<=filesize/512; j++){
			fread(data, 512, 1, fp_input);
			uint8 i;
			for (i=0; i<25; i++){	// 25 Daten pro Block
				char buffer[255];
				slog_t tmp;
				/* AVR-Speicherformat einlesen, Prinzip hackhack, ist so aber unabhaengig von der Zielplattform */
				tmp.encRate = data[k*250+i*10];
				if (tmp.encRate == 0) break;	// 0-Zeile
				tmp.targetRate = data[k*250+i*10+1];
				tmp.err = data[k*250+i*10+2] | data[k*250+i*10+3]<<8;
				tmp.pwm = data[k*250+i*10+4] | data[k*250+i*10+5]<<8;
				tmp.time = data[k*250+i*10+6] | data[k*250+i*10+7]<<8 | data[k*250+i*10+8]<<16 | data[k*250+i*10+9]<<24;
				/* Ausgabezeile bauen und schreiben */
				sprintf(buffer, "%u\t%u", tmp.encRate*2, tmp.targetRate*2);
				fwrite(buffer, strlen(buffer), 1, fp_output);
				sprintf(buffer, "\t%d\t%u", tmp.err*2, tmp.pwm);
				fwrite(buffer, strlen(buffer), 1, fp_output);
				sprintf(buffer, "\t%lu\n", tmp.time);
				fwrite(buffer, strlen(buffer), 1, fp_output);									
			}
		}
		fwrite("\n", 1, 1, fp_output);	// Leerzeile trennt links und rechts
	}
	fclose(fp_input);
	fclose(fp_output);
	printf("done.\n");	// fertig :)
}

#endif
