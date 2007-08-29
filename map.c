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
 * @file 	map.c  
 * @brief 	Karte 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	19.09.06
 */
 
#include <stdio.h>
#include "ct-Bot.h"
#include "bot-local.h"
#include "sensor_correction.h"
#include "map.h"
#include "mmc.h"
#include "mini-fat.h"
#include "mmc-vm.h"

#ifdef MAP_AVAILABLE
#include <math.h>
#include <stdlib.h>


//#define DEBUG_MAP		// Schalter um recht viel Debug-Code anzumachen

/*
 * Eine Karte ist wie folgt organisiert:
 * Es gibt Sektionen zu je MAP_SECTION_POINTS * MAP_SECTION_POINTS. 
 * Diese Sektionen enthalten direkt die Pixel-Daten
 * Auf dem PC haelt (noch) ein uebergeordnetes Array haelt die einzelnen Sections 
 * So laesst sich leicht eine Karte aufbauen, 
 * ohne dass viel Platz fuer unbenutzte Felder draufgeht
 * 
 * Auf dem MCU passt eine Sektion in einen Flashblock, bzw immer 2 Sektionen in einen Block
 * Es stehen immer 2 Sections also 1 Flash-Block im SRAM und werden bei Bedarf gewechselt
 * 
 * Felder sind vom Typ int8 und haben einen Wertebereich von -128 bis 127
 * 0 bedeutet: wir wissen nichts über das Feld
 * negative Werte bedeuten: Hinderniss
 * postivie Werte bedeuten: freier Weg
 * je größer der Betrag ist, desto sicherer die Aussage über das Feld
 * Der Wert -128 ist Löchern vorbehalten und wird dann auch nicht durch die Abstandssensoren verändert
 * (Achtung, die Locherkennung ist derzeit noch nicht fertig impelementiert)
 *
 * Felder werden wie folgt aktualisiert:
 * Wird ein Punkt als frei betrachtet, erhoehen wir den Wert des Feldes um MAP_STEP_FREE
 * Wird ein Punkt als belegt erkannt, ziehen wir um ihn einen Streukreis mit dem Radius MAP_RADIUS. 
 * Das Zentrum des Kreises wird um MAP_STEP_OCCUPIED dekrementiert, nach aussen hin immer weniger
 * Wird ein Feld als Loch erkannt, setzen wir den Wert fest auf -128 (Achtung, derzeit noch nicht fertig impelementiert)
 */



#define MAP_SECTIONS ((( MAP_SIZE*MAP_RESOLUTION)/MAP_SECTION_POINTS))	/*!< Anzahl der Sections in der Map */

#define MAP_STEP_FREE_SENSOR		2	/*!< Um diesen Wert wird ein Feld inkrementiert, wenn es vom Sensor als frei erkannt wird */
#define MAP_STEP_FREE_LOCATION		20	/*!< Um diesen Wert wird ein Feld inkrementiert, wenn der Bot drüber fährt */

#define MAP_STEP_OCCUPIED	10	/*!< Um diesen Wert wird ein Feld dekrementiert, wenn es als belegt erkannt wird */

#define MAP_RADIUS			50	/*!< Umkreis eines Messpunktes, der als Besetzt aktualisiert wird (Streukreis) [mm]*/
#define MAP_RADIUS_FIELDS	(MAP_RESOLUTION*MAP_RADIUS/1000)	/*!< Umkreis einen Messpunkt, der als Besetzt aktualisiert wird (Streukreis) [Felder]*/

#define MAP_PRINT_SCALE				/*!< Soll das PGM eine Skala erhalten */
#define MAP_SCALE	(MAP_RESOLUTION/2)	/*!< Alle wieviel Punkte kommt wein Skalen-Strich */

#define USE_MACROBLOCKS	// Soll die Karte linear oder nach Macroblocks sortiert sein?

#define MACRO_BLOCK_LENGTH	512 // kantenlaenge eines Macroblocks in Punkten/Byte
//#define MACRO_BLOCK_SIZE (MACRO_BLOCK_LENGTH * MACRO_BLOCK_LENGTH)
#define MAP_LENGTH_IN_MACRO_BLOCKS ((MAP_SIZE*MAP_RESOLUTION)/MACRO_BLOCK_LENGTH)



#ifdef SHRINK_MAP_ONLINE
	uint16 map_min_x=MAP_SIZE*MAP_RESOLUTION/2; /*!< belegter Bereich der Karte [Kartenindex]: kleinste X-Koordinate */
	uint16 map_max_x=MAP_SIZE*MAP_RESOLUTION/2; /*!< belegter Bereich der Karte [Kartenindex]: groesste X-Koordinate */
	uint16 map_min_y=MAP_SIZE*MAP_RESOLUTION/2; /*!< belegter Bereich der Karte [Kartenindex]: kleinste Y-Koordinate */
	uint16 map_max_y=MAP_SIZE*MAP_RESOLUTION/2; /*!< belegter Bereich der Karte [Kartenindex]: groesste Y-Koordinate  */
#endif

/*! Datentyp fuer die elementarfelder einer Gruppe */
typedef struct {
	int8 section[MAP_SECTION_POINTS][MAP_SECTION_POINTS]; /*!< Einzelne Punkte */
} map_section_t;

typedef struct {
	map_section_t part[2];		/*!< Ein MMC-Block fasst genau 2 map_section_t*/
}mmc_block;

#ifdef MMC_VM_AVAILABLE
	map_section_t * map[2][1];	/*!< Array mit den Zeigern auf die Elemente */
	uint32 map_start_block; 	/*!< Block, bei dem die Karte auf der MMC-Karte beginnt. Derzeit nur bis 32MByte adressierbar */
	uint32 map_current_block; 	/*!< Block, der aktuell im Puffer steht. Derzeit nur bis 32MByte adressierbar */
	uint8* map_buffer;			/*!< dynamischer Puffer */
#else
	#ifdef MCU
		#ifdef MMC_AVAILABLE
			// Wenn wir die MMC-Karte haben, passen immer 2 Sektionen in den SRAM
			map_section_t * map[2];	/*!< Array mit den Zeigern auf die Elemente */
			uint32 map_start_block; /*!< Block, bei dem die Karte auf der MMC-Karte beginnt. Derzeit nur bis 32MByte adressierbar*/
			uint32 map_current_block; /*!< Block, der aktuell im Puffer steht. Derzeit nur bis 32MByte adressierbar*/
			uint8 map_buffer[sizeof(map_section_t)*2]; /*!< statischer Puffer */
			uint8 map_current_block_updated; /*!< markiert, ob der aktuelle Block gegenueber der MMC-Karte veraendert wurde */
		#else
			// Ohne MMC-Karte nehmen wir nur 1 Sektionen in den SRAM und das wars dann
			map_section_t * map[1];	/*!< Array mit den Zeigern auf die Elemente */
		#endif	// MMC_AVAILABLE
	#else
//		map_section_t * map[MAP_SECTIONS][MAP_SECTIONS];	/*!< Array mit den Zeigern auf die Elemente */
		map_section_t * map[MAP_SECTIONS * MAP_SECTIONS];	/*!< Array mit den Zeigern auf die Elemente */
	#endif	// MCU
#endif	// MMC_VM_AVAILABLE

/*!
 * initialisiere die Karte
 * @return 0 wenn alles ok ist
 */
int8 map_init(void){
	#ifndef MMC_VM_AVAILABLE
		#ifdef MMC_AVAILABLE 
			map_start_block=0xFFFFFFFF;
			map_current_block_updated = 0xFF;	// Die MMC-Karte ist erstmal nicht verfuegbar

			if (mmc_get_init_state() != 0) return 1;
			map_start_block= mini_fat_find_block("MAP",map_buffer);
		
			// Die Karte auf den Puffer biegen
			map[0]=(map_section_t*)map_buffer;
			map[1]=(map_section_t*)(map_buffer+sizeof(map_section_t));		
		
			if (map_start_block != 0xFFFFFFFF){
				map_current_block_updated = False;	// kein Block geladen und daher auch nicht veraendert
				map_current_block = map_start_block;
				return 1;
			}	
		#endif
	#else
		map_start_block = mmc_fopen("MAP")>>9;	// TODO: Mit Bytes arbeiten und Shiften sparen
//		printf("Startaddress of map: 0x%lx \n\r", map_start_block<<9);
		if (map_start_block == 0) return 1;
		map_buffer = mmc_get_data(map_start_block<<9);
		if (map_buffer == NULL) return 1;
		
		// Die Karte auf den Puffer biegen
		map[0][0]=(map_section_t*)map_buffer;
		map[1][0]=(map_section_t*)(map_buffer+sizeof(map_section_t));		
	#endif	// MMC_VM_AVAILABLE	
		
		
	#ifdef DEBUG_MAP
		#ifdef PC		
			map_info();				// Verrate uns was über die Karte
			map_draw_test_scheme();	// zeichne Das Testmuster in die Karte
			print_map();			// Und karte gleich ausgeben	
		#endif
	#endif
			
	return 0;
}

/*!
 * liefert einen Zeiger auf die Section zurueck, in der der Punkt liegt.
 * Auf dem MCU kuemmert sie sich darum, die entsprechende Karte aus der MMC-Karte zu laden
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param create Soll das Segment erzeugt werden, falls es noch nicht existiert?
 * @return einen zeiger auf die Karte
 */
static map_section_t * map_get_section(uint16 x, uint16 y, uint8 create) {

	uint16 section_x, section_y;
		
	// Berechne in welcher Sektion sich der Punkt befindet
	section_x=x/ MAP_SECTION_POINTS;
	section_y=y/ MAP_SECTION_POINTS;

	// Da imemr 2 Sections in einem Block stehen: richtige der beiden sections raussuchen
	uint8 index= section_x & 0x01;

		
	if ((section_x>= MAP_SECTIONS) || (section_y >= MAP_SECTIONS)){
		#ifdef PC
			printf("Versuch auf in Feld ausserhalb der Karte zu zugreifen!! x=%d y=%d\n",x,y);
		#endif
		return NULL;
	}
	
	#ifndef USE_MACROBLOCKS
		// Berechne den gesuchten Block
		uint32 block = section_x + section_y*MAP_SECTIONS;
		block = block >>1;	// es passen immer 2 Sections in einen Block
	#else		
		uint16 macroblock = x / MACRO_BLOCK_LENGTH + (y / MACRO_BLOCK_LENGTH)* MAP_LENGTH_IN_MACRO_BLOCKS; 
		
//		printf("Macroblock= %d ",macroblock);
		
		// Berechne den gesuchten Block
		uint32 block; 

		uint16 local_x = x % MACRO_BLOCK_LENGTH;	// wenn MACRO_BLOCK_LENGTH eine 2er Potenz ist kann man hier optimieren
		uint16 local_y = y % MACRO_BLOCK_LENGTH;

//		printf("\tlocal_x= %d, local_y= %d ",local_x,local_y);

		block= local_x / MAP_SECTION_POINTS + (local_y/MAP_SECTION_POINTS)* (MACRO_BLOCK_LENGTH/MAP_SECTION_POINTS);
		
		block = block >>1;	// es passen immer 2 Sections in einen Block

		block += macroblock * MACRO_BLOCK_LENGTH * (MACRO_BLOCK_LENGTH / 512);	// noch in den richtigen Macroblock springen

//		printf("block= %d ",block);

	#endif	
	
	
	
	#ifdef MMC_AVAILABLE		// MMC ist vorhanden
		// Auf der MMC beginnt die Karte nicht bei 0, sondern irgendwo		
		block += map_start_block;						// Offset für die Lage der Karte drauf		
		
		#ifndef MMC_VM_AVAILABLE	// Keine Speicherverwaltung	
			// wenn die Karte nicht sauber initialisiert ist, mache nix!
			if (map_current_block_updated == 0xFF)
				return map[0];	// oder eher NULL?
			
			// Ist der Block noch nicht geladen
			if (map_current_block != block){
				// Wurde der Block im RAM veraendert?
				if (map_current_block_updated == True)
					mmc_write_sector(map_current_block,map_buffer,0);
				
				//Lade den neuen Block
				mmc_read_sector(block,map_buffer);
				map_current_block=block;
				map_current_block_updated = False;
			}
			
			return map[index];	
		#else
			map_buffer = mmc_get_data(block<<9);
			if (map_buffer != NULL){
				map[0]=(map_section_t*)map_buffer;
				map[1]=(map_section_t*)(map_buffer+sizeof(map_section_t));
				// richtige der beiden sections raussuchen
				return map[index];
			} else 
				return map[0];	
		#endif	// MMC_VM_AVAILABLE
		
	#else 
		// ohne MMC-Karte einfach direkt mit dem SRAM arbeiten	
		if ((map[block*2+index] == NULL) && (create==True)){
			uint16 index_x, index_y;
			map[block*2+index]= malloc(sizeof(map_section_t));
			for (index_x=0; index_x<MAP_SECTION_POINTS; index_x++)
				for (index_y=0; index_y<MAP_SECTION_POINTS; index_y++)
					map[block*2+index]->section[index_x][index_y]=0;
			
		}
		return map[block*2+index];
	#endif // MMC_AVAILABLE
	
}

/*!
 * liefert den Wert eines Feldes 
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @return Wert des Feldes (>0 heisst frei, <0 heisst belegt
 */
int8 map_get_field (uint16 x, uint16 y) {
	uint16 index_x, index_y;
	
	// Suche die Section heraus
	map_section_t * section = map_get_section(x,y,False);

	// Eventuell existiert die Section noch nicht
	if (section == NULL)
		return 0;

	// Berechne den Index innerhalb der Section
	index_x = x % MAP_SECTION_POINTS;
	index_y = y % MAP_SECTION_POINTS;

	return section->section[index_x][index_y];	
}

/*!
 * Setzt den Wert eines Feldes auf den angegebenen Wert
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param value neuer wert des Feldes (> 0 heisst frei, <0 heisst belegt
 */
void map_set_field(uint16 x, uint16 y, int8 value) {
	uint16 index_x, index_y;
			
	// Suche die Section heraus
	map_section_t * section = map_get_section(x,y,True);		

	// Wenn wir keine section zurueck kriegen, stimmt was nicht
	if (section == NULL)
		return;

	// Berechne den Index innerhalb der Section
	index_x = x % MAP_SECTION_POINTS;
	index_y =  y % MAP_SECTION_POINTS;
	
	section->section[index_x][index_y]=value;
	
	#ifdef SHRINK_MAP_ONLINE
		// Belegte Kartengroesse anpassen
		if (x < map_min_x)
			map_min_x=x;
		if (x > map_max_x)
			map_max_x= x;
		if (y < map_min_y)
			map_min_y=y;
		if (y > map_max_y)
			map_max_y= y;
	#endif

	#ifdef MMC_AVAILABLE
		#ifndef MMC_VM_AVAILABLE
			if (map_current_block_updated != 0xFF)
				map_current_block_updated = True;
		#endif
	#endif
	
}

/*!
 * liefert den Wert eines Ortes 
 * @param x x-Ordinate der Welt 
 * @param y y-Ordinate der Welt
 * @return Wert des dem Ort zugeordneten Feldes (>0 heisst frei, <0 heisst belegt
 */
int8 map_get_point (float x, float y){
	//Ort in Kartenkoordinaten
	uint16 X = world_to_map(x);
	uint16 Y = world_to_map(y);
	
	return map_get_field(X,Y);
}

/*!
 * liefert den Durschnittswert um einen Punkt der Karte herum 
 * @param x 		x-Ordinate der Karte 
 * @param y 		y-Ordinate der Karte
 * @param radius	gewuenschter Radius
 * @return 			Wert des Durchschnitts um das Feld (>0 heisst frei, <0 heisst belegt
 */

int8 map_get_average_fields (uint16 x, uint16 y, uint16 radius){
	int16 avg=0;
	int16 avg_line=0;
	
	int16 dX,dY;
    uint16 h=radius*radius;
	for(dX = -radius; dX <= radius; dX++){
      for(dY = -radius; dY <= radius; dY++) {
           if(dX*dX + dY*dY <= h)	 
           	 avg_line+=map_get_field (x + dX, y + dY);
      }
      avg+=avg_line/(radius*2);
	}

	return (int8) (avg/(radius*2));
}

/*!
 * liefert den Durschnittswert um eine Ort herum 
 * @param x x-Ordinate der Welt 
 * @param y y-Ordinate der Welt
 * @param radius Radius der Umgebung, die Beruecksichtigt wird
 * @return Durchschnitsswert im Umkreis um den Ort (>0 heisst frei, <0 heisst belegt
 */
int8 map_get_average(float x, float y, float radius){
	//Ort in Kartenkoordinaten
	uint16 X = world_to_map(x);
	uint16 Y = world_to_map(y);
	uint16 R = radius * MAP_RESOLUTION / 1000;
	
	return map_get_average_fields(X,Y,R);
}

/*!
 * aendert den Wert eines Feldes um den angegebenen Betrag 
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param value Betrag um den das Feld veraendert wird (>= heisst freier, <0 heisst belegter
 */
static void map_update_field (uint16 x, uint16 y, int8 value) {

	int16 tmp= map_get_field(x,y);
	if (tmp == -128)	// Nicht aktualiseren, wenn es sich um ein Loch handelt
		return;
			
	tmp+=value;
	
	// pruefen, ob kein ueberlauf stattgefunden hat und das Feld nicht schon als Loch (-128) markiert ist
	if ((tmp < 128) && (tmp > -128))
		map_set_field(x,y,(int8)tmp);
}

/*!
 * aendert den Wert eines Feldes um den angegebenen Betrag 
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param radius in Kartenpunkten
 * @param value Betrag um den das Feld veraendert wird (>= heisst freier, <0 heisst belegter)
 */
void map_update_field_circle(uint16 x, uint16 y, uint16 radius, int8 value) {

	#ifdef OLD_VERSION
		int16 dX,dY;
	    uint16 h=radius*radius;
		for(dX = -radius; dX <= radius; dX++){
	      for(dY = -radius; dY <= radius; dY++) {
	           if(dX*dX + dY*dY <= h)	 
	           	 map_update_field (x + dX, y + dY,value);
	      }
		}
	#else
		const int16 square = radius*radius;
		
	//	printf("Fange an. Zentrum= %d/%d\n",x_map,y_map);
	//	map_set_field(x_map,y_map,-120);
		
		int16 sec_x_max = ((x + radius) / MAP_SECTION_POINTS);
		int16 sec_x_min = ((x - radius) / MAP_SECTION_POINTS);
		int16 sec_y_max = ((y + radius) / MAP_SECTION_POINTS);
		int16 sec_y_min = ((y - radius) / MAP_SECTION_POINTS);
	//	printf("Betroffene Sektionen X: %d-%d, Y:%d-%d\n",sec_x_min,sec_x_max,sec_y_min,sec_y_max);
				
		int16 sec_x, sec_y, X, Y,dX,dY;
		int16 starty,startx, stopx, stopy;
		// Gehe über alle betroffenen Sektionen 
		for (sec_y= sec_y_min; sec_y <= sec_y_max; sec_y++) {
			// Bereich weiter eingrenzen
			if (sec_y*MAP_SECTION_POINTS > (y-radius))	
				starty=sec_y*MAP_SECTION_POINTS;
			else
				starty=y-radius;
			if ((sec_y+1)*MAP_SECTION_POINTS < (y+radius))	
				stopy=(sec_y+1)*MAP_SECTION_POINTS;
			else
				stopy=y+radius;
	
	
			for (sec_x= sec_x_min; sec_x <= sec_x_max; sec_x++){
				if (sec_x*MAP_SECTION_POINTS > (x-radius))	
					startx=sec_x*MAP_SECTION_POINTS;
				else
					startx=x-radius;
				if ((sec_x+1)*MAP_SECTION_POINTS < (x+radius))	
					stopx=(sec_x+1)*MAP_SECTION_POINTS;
				else
					stopx=x+radius;
	
				for (Y = starty; Y < stopy; Y++) {
					dY= y-Y;	// Distanz berechnen
					dY*=dY;			// Quadrat vorberechnen
					for (X = startx; X < stopx; X++){
						dX= x-X;	// Distanz berechnen
						dX*=dX;			// Quadrat vorberechnen
						if(dX + dY <= square)	// wenn Punkt unter Bot	 
							map_update_field(X,Y,value);	// dann aktualisieren
			  		}
				
				}
			}
		}
	#endif
}

/*!
 * markiert ein Feld als belegt -- drueckt den Feldwert etwas mehr in Richtung "belegt"
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 */
void map_update_occupied (uint16 x, uint16 y) {
  // Nur wenn ein Umkreis gewuenscht ist, auch einen malen
  #if MAP_RADIUS_FIELDS > 0
	  uint8 r;
	  for (r=1; r<=MAP_RADIUS_FIELDS; r++){
	  	map_update_field_circle(x,y,r,-MAP_STEP_OCCUPIED/MAP_RADIUS_FIELDS);
	  }
  #else 
  	map_update_field(x,y,-MAP_STEP_OCCUPIED);
  #endif
}

/*!
 * Konvertiert eine Weltkoordinate in eine Kartenkoordinate
 * @param koord Weltkordiante
 * @return kartenkoordinate
 */
uint16 world_to_map(float koord){
	uint16 tmp = koord * MAP_RESOLUTION / 1000  + (MAP_SIZE*MAP_RESOLUTION/2);
		
	return tmp;
}

/*!
 * Konvertiert eine Kartenkoordinate in eine Weltkoordinate
 * @param map_koord kartenkoordinate
 * @return Weltkordiante
 */
float map_to_world(uint16 map_koord){
	float tmp = (map_koord - (MAP_SIZE*MAP_RESOLUTION/2.0)) / MAP_RESOLUTION * 1000;
			
	return tmp;
}

/*! 
 * Aktualisiert die Karte mit den Daten eines Sensors
 * @param x		X-Achse der Position des Sensors
 * @param y 	Y-Achse der Position des Sensors
 * @param h 	Blickrichtung im Bogenmaß
 * @param dist 	Sensorwert 
 */ 
static void update_map_sensor(float x, float y, float h, int16 dist){
	//Ort des Sensors in Kartenkoordinaten
	uint16 X = world_to_map(x);
	uint16 Y = world_to_map(y);
	
	uint16 d;
	if (dist==SENS_IR_INFINITE)
		d=SENS_IR_MAX_DIST;
	else 
		d=dist;
		
		
	// liefert die Mapkoordinaten im Abstand dist vom Punkt xy
	uint16 PH_X=get_mapposx_dist(x,y,h,dist);
	uint16 PH_Y=get_mapposy_dist(x,y,h,dist);

	if ((dist > 80 ) && (dist <SENS_IR_INFINITE))
			map_update_occupied(PH_X,PH_Y);
	


	// Nun markiere alle Felder vor dem Hinderniss als frei
	uint16 i;

	uint16 lX = X; 	//	Beginne mit dem Feld, in dem der Sensor steht
	uint16 lY = Y; 
	
	int8 sX = (PH_X < X ? -1 : 1);  
	uint16 dX=abs(PH_X - X);		// Laenge der Linie in X-Richtung
	
	int8 sY = (PH_Y < Y ? -1 : 1); 
	uint16 dY =abs(PH_Y - Y);	// Laenge der Linie in Y-Richtung

	if (dX >= dY) {			// Hangle Dich an der laengeren Achse entlang
	  dY--;	// stoppe ein Feld vor dem Hinderniss
	  uint16 lh = dX / 2;
	  for (i=0; i<dX; ++i) {
	    map_update_field (lX+i*sX, lY,MAP_STEP_FREE_SENSOR);
	    lh += dY;
	    if (lh >= dX) {
	      lh -= dX;
	      lY += sY;
	    }
	  }
	} else {
	  dX--; // stoppe ein Feld vor dem Hinderniss
	  uint16 lh = dY / 2;
	  for (i=0; i<dY; ++i) {
	    map_update_field (lX, lY+i*sY,MAP_STEP_FREE_SENSOR);
	    lh += dX;
	    if (lh >= dY) {
	      lh -= dY;
	      lX += sX;
	    }
	  }
	}	
}

/*!
 * Aktualisiert den Standkreis der internen Karte
 * @param x X-Achse der Position
 * @param y Y-Achse der Position
 */
void update_map_location(float x, float y){
	int16 x_map = world_to_map(x);
	int16 y_map = world_to_map(y);

	//#define OLD_VERSION
	#ifdef OLD_VERSION	
		// Aktualisiere zuerst die vom Bot selbst belegte Flaeche
		const int16 dim = BOT_DIAMETER/2*MAP_RESOLUTION/100;	/*!< Botradius in Map-Aufloesung */
		int16 dX,dY;
	
		for(dX = -dim; dX <= dim; dX++)
	      for(dY = -dim; dY <= dim; dY++) {
	           if(dX*dX + dY*dY <= dim*dim)	 
	           	 map_update_free (x_map + dX, y_map + dY,MAP_STEP_FREE_LOCATION);
	     }
	#else	// #ifdef OLD_VERSION
		
		// Aktualisiere zuerst die vom Bot selbst belegte Flaeche
		map_update_field_circle(x_map, y_map, BOT_DIAMETER/2*MAP_RESOLUTION/100, MAP_STEP_FREE_LOCATION);
	
	#endif //#ifdef OLD_VERSION
}

/*!
 * Aktualisiert die interne Karte anhand der Sensordaten
 * @param x X-Achse der Position
 * @param y Y-Achse der Position
 * @param head Blickrichtung in Grad
 * @param distL Sensorwert links
 * @param distR Sensorwert rechts
 */
void update_map(float x, float y, float head, int16 distL, int16 distR){
//	printf("update_map: x=%f, y=%f, head=%f, distL=%d, distR=%d\n",x,y,head,distL,distR);
	
    float h= head * M_PI /180;
	        
    //Ort des rechten Sensors in Weltkoordinaten
    float Pr_x = x + (DISTSENSOR_POS_SW * sin(h));
	float Pr_y = y - (DISTSENSOR_POS_SW * cos(h));

    //Ort des linken Sensors in Weltkoordinaten
    float Pl_x = x - (DISTSENSOR_POS_SW * sin(h));
	float Pl_y = y + (DISTSENSOR_POS_SW * cos(h));

	update_map_sensor(Pl_x,Pl_y,h,distL);
	update_map_sensor(Pr_x,Pr_y,h,distR);
}

/*!
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * @param  from_x Startort x Kartenkoordinaten
 * @param  from_y Startort y Kartenkoordinaten
 * @param  to_x Zielort x Kartenkoordinaten
 * @param  to_y Zielort y Kartenkoordinaten
 * @return 1 wenn alles frei ist
 */
uint8 map_way_free_fields(uint16 from_x, uint16 from_y, uint16 to_x, uint16 to_y){
	// gehe alle Felder der reihe nach durch
	uint16 i;

	uint16 lX = from_x; 	//	Beginne mit dem Feld, in dem der Bot steht
	uint16 lY = from_y; 
	
	int8 sX = (to_x < from_x ? -1 : 1);  
	uint16 dX=abs(to_x - from_x);		// Laenge der Linie in X-Richtung
	
	int8 sY = (to_y < from_y ? -1 : 1); 
	uint16 dY =abs(to_y - from_y);	// Laenge der Linie in Y-Richtung

	int16 width= (BOT_DIAMETER*MAP_RESOLUTION)/100;
	int16 w=0;
	
	if (dX >= dY) {			// Hangle Dich an der laengeren Achse entlang
	  uint16 lh = dX / 2;
	  for (i=0; i<dX; ++i) {
		 for (w=-width; w<= width; w++) // wir müssen die ganze Breite des absuchen
//			map_set_field(lX+i*sX, lY+w,-126);
			if (map_get_field(lX+i*sX, lY+w) < -MAPFIELD_IGNORE) // ein hinderniss reicht für den Abbruch
		       return 0;
		  
	    lh += dY;
	    if (lh >= dX) {
	      lh -= dX;
	      lY += sY;
	    }
	  }
	} else {
	  uint16 lh = dY / 2;
	  for (i=0; i<dY; ++i) {
		 for (w=-width; w<= width; w++) // wir müssen die ganze Breite des absuchen
//			map_set_field(lX+w, lY+i*sY,126);
			if (map_get_field (lX+w, lY+i*sY) <-MAPFIELD_IGNORE ) //ein hinderniss reicht für den Abbruch
		       return 0;
	    lh += dX;
	    if (lh >= dY) {
	      lh -= dY;
	      lX += sX;
	    }
	  }
	}
	
	return 1;
}


/*!
 * Prueft ob eine direkte Passage frei von Hindernissen ist
 * @param  from_x Startort x Weltkoordinaten
 * @param  from_y Startort y Weltkoordinaten
 * @param  to_x Zielort x Weltkoordinaten
 * @param  to_y Zielort y Weltkoordinaten
 * @return 1 wenn alles frei ist
 */
int8 map_way_free(float from_x, float from_y, float to_x, float to_y){
	return map_way_free_fields(world_to_map(from_x),world_to_map(from_y),world_to_map(to_x),world_to_map(to_y));
}	

/*! 
 * gibt True zurueck wenn Map-Wert value innerhalb des Umkreises radius von xy liegt sonst False;
 * wird verwendet zum Check, ob sich ein Punkt (naechster Pfadpunkt, Loch) innerhalb eines bestimmten
 * Umkreises befindet; findet nur Verwendung bei hoeherer Aufloesung
 * @param x Map-Koordinate
 * @param y Map-Koordinate
 * @param radius Radius des Umfeldes
 * @param value Mapwert des Vergleiches
 * @return True wenn Wert value gefunden
 */
uint8 map_get_value_field_circle(uint16 x, uint16 y, uint8 radius, int8 value) {
	int16 dX,dY;
	uint16 h=radius*radius;
	for(dX = -radius; dX <= radius; dX++) {
		for(dY = -radius; dY <= radius; dY++) {
			if(dX*dX + dY*dY <= h)	                    // Vergleich nur innerhalb des Umkreises
				if (map_get_field(x + dX, y + dY)==value)  // Mapwert hat Vergleichswert ?
					return True; 	           	            // dann Schluss mit True
		}
	}
	return False;                                       // kein Feldwert ist identisch im Umkreis
}
	
/*!
 * ermittelt ob der Wert val innerhalb des Umkreises mit Radius r von xy liegt; bei geringer MCU-Aufloesung direkter
 * Vergleich mit xy
 * @param x	Map-Koordinate
 * @param y Map-Koordinate
 * @param radius Vergleichsradius (halber Suchkreis)
 * @param val Vergleichswert
 * @return True wenn Wert val gefunden
 */
uint8 value_in_circle (uint16 x, uint16 y, uint8 radius, int8 val) {

	#if MAP_RADIUS_FIELDS_GODEST > 0
		uint8 r;
		for (r=1; r<=radius; r++){          // Botradius
			if (map_get_value_field_circle(x,y,r,val))      // Schluss sobald Wert erkannt wird
				return True;
		}
		return False;                                         // Wert nicht vorhanden
	#else
		return map_get_field(x,y)==val;  	                    // niedrige Aufloesung, direkter Feldvergleich
	#endif	// MAP_RADIUS_FIELDS_GODEST > 0
}

/*!
 * Check, ob die MAP-Koordinate xy mit destxy identisch ist (bei kleinen Aufloesungen) oder sich
 * innerhalb des halben Radius-Umkreises befindet (hoehere Aufloesungen); z.B. verwendet um das
 * Zielfahren mit gewisser Toleranz zu versehen
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param destx Ziel-x-Ordinate der Karte (nicht der Welt!!!)
 * @param desty Ziel-y-Ordinate der Karte (nicht der Welt!!!)
 * @return True wenn xy im Umkreis vorhanden oder identisch ist
 */
uint8 map_in_dest (uint16 x, uint16 y, uint16 destx, uint16 desty) {
	// MAP_RADIUS_FIELDS_GODEST sind die Mapfelder im Botdurchmesser fuer Suchkreis
	#if MAP_RADIUS_FIELDS_GODEST > 1 // gilt nur fuer hoehere Aufloesungen, sonst direkter Vergleich
		//Distanzen in Mapfelder
		int16 distx=destx-x;
		int16 disty=desty-y;
		// Radius, also Abstand ermitteln; Wurzel ziehen spare ich mir
		return distx*distx + disty*disty <= MAP_RADIUS_FIELDS_GODEST_HALF_QUAD;
	#else
		//bei geringer Aufloesung (MCU) direkt mit Zielkoords vergleichen
		return  (x==destx && y==desty);
	#endif	// MAP_RADIUS_FIELDS_GODEST > 1
}

#if MAP_RADIUS_FIELDS_GODEST > 0
	/*!
	 * Map-Umfeldaktualisierung mit einem bestimmten Wert ab einer Position xy mit Radius r bei
	 * hoeherer Aufloesung; z.B. verwendet zur Lochmarkierung in einem Umkreis, Hinderniswahrscheinlichkeit
	 * @param x Map-Koordinate
	 * @param y Map-Koordinate
	 * @param radius Radius des Umfeldes
	 * @param value Mapwert; nur eingetragen wenn aktueller Mapwert groesser value ist
	 */
	static void map_set_value_field_circle(uint16 x, uint16 y, int8 radius, int8 value) {
		int16 dX,dY;
	    uint16 h=radius*radius;
		for(dX = -radius; dX <= radius; dX++){
			for(dY = -radius; dY <= radius; dY++) {
				if(dX*dX + dY*dY <= h)	                    // nur innerhalb des Umkreises
					if (map_get_field(x + dX, y + dY)>value)   // Mapwert hoeher Richtung frei ?
						map_set_field (x + dX, y + dY,value); // dann Wert eintragen
			}
		}
	}
#endif  // hoehere Aufloesung

/*!
 * setzt ein Map-Feld auf einen Wert mit Umfeldaktualisierung; Hindernis wird mit halben Botradius
 * eingetragen, da die Pfadpunkte im ganzen Umkreis liegen und nicht ueberschrieben werden duerfen;
 * Abgrund/ Loch wird aber auch im ganzen Botradius eingetragen; bei geringer MCU-Aufloesung ohne Umfeld mit
 * direktem Eintragen des Wertes auf xy; Umfeld wird dann in Richtung Hindernis gedrueckt
 * @param x Map-Koordinate
 * @param y Map-Koordinate
 * @param val im Umkreis einzutragender Wert
 */
void map_set_value_occupied (uint16 x, uint16 y, int8 val) {
	#if MAP_RADIUS_FIELDS_GODEST > 0              // bei hoeherer Aufloesung
		uint8 r;
		uint8 maxr;

		if (val==-128)
			maxr=MAP_RADIUS_FIELDS_GODEST;        // Loch wird mit ganzem Botradius eingetragen
		else
			maxr=MAP_RADIUS_FIELDS           ;   // im Normalfall halber Botradius

		for (r=1; r<=maxr; r++)                   // in Map mit dem Radius um xy eintragen
			map_set_value_field_circle(x,y,r,val);

	#else                                         // bei geringer MCU-Aufloesung direkt auf xy
		if (map_get_field(x,y)>val)                 // nur Eintragen wenn aktueller Wert freier ist
		map_set_field(x,y,val);               // damit auch -128 Lochwert eingetragen wird
	#endif	// MAP_RADIUS_FIELDS_GODEST > 0

	// Punkt xy nun in Richtung Hindernis druecken mit Umfeld; falls Wert fuer kuenstl. Hindernis erreicht
	// muss Wert aber so bleiben, damit diese Mapfelder bei Neuberechnung der Map-Potenziale erkannt und wieder
	// freigegeben werden koennen zur Neuberuecksichtigung
	if (val != -MAP_ART_HAZARD) 
		map_update_occupied(x,y);
}

/*! 
 * Loescht die Mapfelder, die in einem bestimtmen Wertebereich min_val max_val liegen, d.h. diese
 * werden auf 0 gesetzt 
 * @param min_val minimaler Wert
 * @param max_val maximaler Wert
 */ 
void clear_map(int8 min_val, int8 max_val) {
	int16 x,y;
	int8 tmp;
	// Mapfelder durchlaufen
	for (y=map_min_y; y<= map_max_y; y++) {
		for (x=map_max_x; x>= map_min_x; x--) {
			tmp=map_get_field(x,y);				
			if (tmp>=min_val && tmp<= max_val) // alles zwischen Intervall auf 0 setzen
				map_set_field(x,y,0); 
		}		   
	}
} 

/*! 
 * Routine ermittelt ab dem vom Mittelpunkt links/ rechts versetzten Punkt xp yp, in Blickrichtung
 * dist mm von den Abgrundsensoren voraus, die Mapkoordinaten XY
 * verwendet zur Map-Lochmarkierung; kann aber allgemeingueltig verwendet werden um
 * Mapkoordinaten zu erhalten in einem bestimmten Abstand voraus
 * @param xp Koord vom Mittelpunkt des Bots verschoben
 * @param yp Koord vom Mittelpunkt des Bots verschoben
 * @param h Blickrichtung
 * @param dist Abstand voraus in mm
 */
uint16 get_mapposx_dist(float xp, float yp, float h, uint16 dist) {

	// Hinderniss auf X-Koord, dass der Sensor sieht in Weltkoordinaten
	float PH_x = xp + (DISTSENSOR_POS_FW + dist) * cos(h);

	// Hinderniss, welches der Sensor sieht in, umgerechnet in Karten-Map-Koordinaten
	return world_to_map(PH_x);
}

uint16 get_mapposy_dist(float xp, float yp, float h, uint16 dist) {

	// Hinderniss auf Y-Koord, dass der Sensor sieht in Weltkoordinaten
	float PH_y = yp + (DISTSENSOR_POS_FW + dist) * sin(h);

	// Hinderniss, welches der Sensor sieht in, umgerechnet in Karten-Map-Koordinaten
	return  world_to_map(PH_y);
}

/*!
 * markiert die Mapkoordinaten als Loch zum entsprechenden Abgrundsensor
 * @param x bereits berechnete Koordinate nach links rechts vom Mittelpunkt in Hoehe des Sensors
 * @param y bereits berechnete Koordinate nach links rechts vom Mittelpunkt in Hoehe des Sensors
 * @param h Blickrichtung bereits umgerechnet in Bogenmass
 */
void update_map_sensor_hole(float x, float y, float h){
	uint16 PH_X=0;
	uint16 PH_Y=0;

	// 0 mm voraus Loch ab Abgrundsensoren
	PH_X=get_mapposx_dist(x,y,h,0);
	PH_Y=get_mapposy_dist(x,y,h,0);

	// nur wenn noch nicht als Loch gekennzeichnet auf Umgebungskreis Loch vermerken
	if (map_get_field(PH_X,PH_Y) > -128)
		map_set_value_occupied(PH_X,PH_Y,-128);
}

#ifdef PC

	/*!
	 * verkleinert die Karte vom übergebenen auf den benutzten Bereich. Achtung, 
	 * unter Umständen muss man vorher die Puffervariablen sinnvoll initialisieren!!!
	 * @param min_x Zeiger auf einen uint16, der den miniamlen X-Wert puffert
	 * @param max_x Zeiger auf einen uint16, der den maxinmalen X-Wert puffert
	 * @param min_y Zeiger auf einen uint16, der den miniamlen Y-Wert puffert
	 * @param max_y Zeiger auf einen uint16, der den maxinmalen Y-Wert puffert
	 */
	void shrink_map(uint16 * min_x, uint16 * max_x, uint16 * min_y, uint16 * max_y){
		uint16 x,y;
	
		// lokale Variablen mit den defaults befuellen
		*min_x=map_min_x;
		*max_x=map_max_x;
		*min_y=map_min_y;
		*max_y=map_max_y;
		
		
		// Kartengroesse reduzieren
		int8 free=1;
		while ((*min_y < *max_y) && (free ==1)){
			for (x=*min_x; x<*max_x; x++){
				if (map_get_field(x,*min_y) != 0){
					free=0;
					break;
				}			
			}
			*min_y+=1;
		}		
		
		free=1;
		while ((*min_y < *max_y) && (free ==1)){
			for (x=*min_x; x<*max_x; x++){
				if (map_get_field(x,*max_y-1) != 0){
					free=0;
					break;
				}					
			}
			*max_y-=1;
		}
		
		free=1;
		while ((*min_x < *max_x) && (free ==1)){
			for (y=*min_y; y<*max_y; y++){
				if (map_get_field(*min_x,y) != 0){
					free=0;
					break;
				}					
			}
			*min_x+=1;
		}
		free=1;
		while ((*min_x < *max_x) && (free ==1)){
			for (y=*min_y; y<*max_y; y++){
				if (map_get_field(*max_x-1,y) != 0){
					free=0;
					break;
				}					
			}
			*max_x-=1;
		}
	}

	/*!
	 * Schreibt einbe Karte in eine PGM-Datei
	 * @param filename Zieldatei
	 */
	void map_to_pgm(char * filename){
		printf("Speichere Karte nach %s\n",filename);
		FILE *fp = fopen(filename, "wb");
		
		uint16 x,y;

		// lokale Variablen mit den defaults befuellen
		uint16 min_x=map_min_x;
		uint16 max_x=map_max_x;
		uint16 min_y=map_min_y;
		uint16 max_y=map_max_y;
		
		#ifdef SHRINK_MAP_OFFLINE	// nun muessen wir die Grenezn ermitteln
			shrink_map(&min_x, &max_x, &min_y, &max_y);
		#endif	// SHRINK_MAP_OFFLINE
			
		uint16 map_size_x=max_x-min_x;
		uint16 map_size_y=max_y-min_y;
		#ifdef MAP_PRINT_SCALE
			fprintf(fp,"P5 %d %d 255 ",map_size_x+10,map_size_y+10);
		#else
			fprintf(fp,"P5 %d %d 255 ",map_size_x,map_size_y);
		#endif	// MAP_PRINT_SCALE
		printf("Karte beginnt bei X=%d,Y=%d und geht bis X=%d,Y=%d (%d * %d Punkte)\n",min_x,min_y,max_x,max_y,map_size_x,map_size_y);
		
		uint8 tmp;
		for (y=max_y; y> min_y; y--){
			for (x=min_x; x<max_x; x++){
				
				tmp=map_get_field(x,y-1)+128;
				fwrite(&tmp,1,1,fp);
			}

			#ifdef MAP_PRINT_SCALE
				// und noch schnell ne Skala basteln
				for (x=0; x<10; x++){
					if (y % MAP_SCALE == 0)
						tmp=0;
					else tmp=255;
					
					fwrite(&tmp,1,1,fp);
				}
			#endif	// MAP_PRINT_SCALE

		}

		#ifdef MAP_PRINT_SCALE
			for (y=0; y< 10; y++){
				for (x=min_x; x<max_x+10; x++){
					if (x % MAP_SCALE == 0)
						tmp=0;
					else tmp=255;
					
					fwrite(&tmp,1,1,fp);
				}
			}
		#endif	// MAP_PRINT_SCALE
		fclose(fp);
		
	}


	/*! 
	 * Liest eine Map wieder ein 
	 * @param filename Quelldatei
	 */
	void read_map(char * filename){
		printf("Lese Karte (%s) von MMC/SD (Bot-Format)\n",filename);
		FILE *fp = fopen(filename, "rb");
		
		uint8 buffer[512];
		fread(buffer,512,1,fp);
		
		if (buffer[0] != 'M' || buffer[1] != 'A' || buffer[2] != 'P'){
			printf("Datei %s enthaelt keine Karte\n",filename);
			return;
		}
		
		uint16 x,y;	
		uint8 * ptr;

		for (y=0; y< MAP_SECTIONS; y++)
			for (x=0; x< MAP_SECTIONS; x++){
				ptr= (uint8*)map_get_section(x*MAP_SECTION_POINTS, y*MAP_SECTION_POINTS, True);
				fread(ptr,MAP_SECTION_POINTS*MAP_SECTION_POINTS,1,fp);
			}
			
		fclose(fp);
		
		#ifdef SHRINK_MAP_ONLINE
			// Groesse neu initialisieren
			map_min_x=0;
			map_max_x=MAP_SIZE*MAP_RESOLUTION; 
			map_min_y=0;
			map_max_y=MAP_SIZE*MAP_RESOLUTION;
		
			// und Karte verkleinern
			shrink_map(&map_min_x, &map_max_x, &map_min_y, &map_max_y);
		#endif
		
	}


	/*!
	 * Zeigt ein Paar Infos über dioe Karte an
	 */
	void map_info(void){
		uint32 points_in_map = MAP_SECTION_POINTS*MAP_SECTIONS;	// Leider kann der precompiler des MCUs keine großen Zahlen 
		points_in_map*=MAP_SECTION_POINTS*MAP_SECTIONS;
		
		printf("MAP: \n");
		printf("\t%d\t Punkte pro Section (MAP_SECTIONS)\n",MAP_SECTIONS);
		printf("\t%d\t Sections (MAP_SECTION_POINTS)\n",MAP_SECTION_POINTS);
		printf("\t%d\t Punkte Kantenlänge (MAP_SECTION_POINTS*MAP_SECTIONS)\n",MAP_SECTION_POINTS*MAP_SECTIONS);
		printf("\t%d%d\t Punkte gesamt\n", (uint16)(points_in_map/10000), (uint16) (points_in_map % 10000)	);
		points_in_map /= 1024;		// Umrechnen in KByte
		printf("\t%d%d\t KByte\n",(uint16)(points_in_map/10000), (uint16) (points_in_map % 10000));
		printf("\t%d\t Punkte pro Meter (MAP_RESOLUTION)\n",MAP_RESOLUTION);
		printf("\t%d\t Meter Kantenlänge (MAP_SIZE)\n",MAP_SIZE);
		
		printf("\n");
		#ifdef USE_MACROBLOCKS
			printf("Die Karte verwendet Macroblocks\n");
			printf("\t%d\t Länge eine Macroblocks in Punkten (MACRO_BLOCK_LENGTH)\n",MACRO_BLOCK_LENGTH);
			printf("\t%d\t Anzahl der Macroblocks in einer Zeile(MAP_LENGTH_IN_MACRO_BLOCKS)\n",MAP_LENGTH_IN_MACRO_BLOCKS);
		#else
			printf("Die Karte verwendet keine Macroblocks\n");
		#endif
		
	}
#endif	// PC

/*!
 * Zeigt die Karte an
 */
void print_map(void){
	#ifdef PC
		map_to_pgm("map.pgm");
	#else
		// Todo: Wie soll der Bot eine Karte ausgeben ....
	#endif	// PC
}

/*!
 * zeichnet ein Testmuster in die Karte
 */
void map_draw_test_scheme(void){
	int16 x,y;

	// Erstmal eine ganz simple Linie
	for (x=0; x< MAP_SECTION_POINTS*MAP_SECTIONS; x++){
		map_set_field(x,x,-120);
		map_set_field(MAP_SECTION_POINTS*MAP_SECTIONS-x-1,x,-120);
	}	
	
	// Grenzen der Sections Zeichnen
	for (x=0; x< MAP_SECTION_POINTS*MAP_SECTIONS; x++){
		for (y=0; y< MAP_SECTIONS; y++)	{
			map_set_field(x,y*MAP_SECTION_POINTS,-10);
			map_set_field(y*MAP_SECTION_POINTS,x,-10);
		}		
	}

	#ifdef USE_MACROBLOCKS
		// Grenzen der Macroblocks einzeichnen
		for (x=0; x< MAP_SECTION_POINTS*MAP_SECTIONS; x++){
			for (y=0; y< MAP_LENGTH_IN_MACRO_BLOCKS; y++)	{
					map_set_field(x,y*MACRO_BLOCK_LENGTH,-60);
					map_set_field(y*MACRO_BLOCK_LENGTH,x,-60);
			}		
		}
	#endif


	
		
}

#endif	// MAP_AVAILABLE
