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

#define MAP_STEP_FREE		2	/*!< Um diesen Wert wird ein Feld inkrementiert, wenn es als frei erkannt wird */
#define MAP_STEP_OCCUPIED	10	/*!< Um diesen Wert wird ein Feld dekrementiert, wenn es als belegt erkannt wird */

#define MAP_RADIUS			10	/*!< Umkreis einen Messpunkt, der als Besetzt aktualisiert wird (Streukreis) [mm]*/
#define MAP_RADIUS_FIELDS	(MAP_RESOLUTION*MAP_RADIUS/1000)	/*!< Umkreis einen Messpunkt, der als Besetzt aktualisiert wird (Streukreis) [Felder]*/

#define MAP_PRINT_SCALE				/*!< Soll das PGM eine Skala erhalten */
#define MAP_SCALE	(MAP_RESOLUTION/2)	/*!< Alle wieviel Punkte kommt wein Skalen-Strich */

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


#ifdef MMC_VM_AVAILABLE
	map_section_t * map[2][1];	/*!< Array mit den Zeigern auf die Elemente */
	uint32 map_start_block; 	/*!< Block, bei dem die Karte auf der MMC-Karte beginnt. Derzeit nur bis 32MByte adressierbar */
	uint32 map_current_block; 	/*!< Block, der aktuell im Puffer steht. Derzeit nur bis 32MByte adressierbar */
	uint8* map_buffer;			/*!< dynamischer Puffer */
#else
	#ifdef MCU
		#ifdef MMC_AVAILABLE
			// Wenn wir die MMC-Karte haben, passen immer 2 Sektionen in den SRAM
			map_section_t * map[2][1];	/*!< Array mit den Zeigern auf die Elemente */
			uint32 map_start_block; /*!< Block, bei dem die Karte auf der MMC-Karte beginnt. Derzeit nur bis 32MByte adressierbar*/
			uint32 map_current_block; /*!< Block, der aktuell im Puffer steht. Derzeit nur bis 32MByte adressierbar*/
			uint8 map_buffer[sizeof(map_section_t)*2]; /*!< statischer Puffer */
			uint8 map_current_block_updated; /*!< markiert, ob der aktuelle Block gegenueber der MMC-Karte veraendert wurde */
		#else
			// Ohne MMC-Karte nehmen wir nur 1 Sektionen in den SRAM und das wars dann
			map_section_t * map[1][1];	/*!< Array mit den Zeigern auf die Elemente */
		#endif
	#else
		map_section_t * map[MAP_SECTIONS][MAP_SECTIONS];	/*!< Array mit den Zeigern auf die Elemente */
	#endif
#endif	// MMC_VM_AVAILABLE

/*!
 *  initialisiere die Karte
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
			map[0][0]=(map_section_t*)map_buffer;
			map[1][0]=(map_section_t*)(map_buffer+sizeof(map_section_t));		
		
			if (map_start_block != 0xFFFFFFFF){
				map_current_block_updated = False;	// kein Block geladen und daher auch nicht veraendert
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
map_section_t * map_get_section(uint16 x, uint16 y, uint8 create){
	uint16 section_x, section_y;
		
	// Berechne in welcher Sektion sich der Punkt befindet
	section_x=x/ MAP_SECTION_POINTS;
	section_y=y/ MAP_SECTION_POINTS;
		
	if ((section_x>= MAP_SECTIONS) || (section_y >= MAP_SECTIONS)){
		#ifdef PC
			printf("Versuch auf in Feld ausserhalb der Karte zu zugreifen!! x=%d y=%d\n",x,y);
		#endif
		return NULL;
	}
	
	#ifndef MMC_VM_AVAILABLE	
		#ifdef MMC_AVAILABLE // Mit MMC-Karte geht hier einiges anders
			// wenn die Karte nicht sauber initialisiert ist, mache nix!
			if (map_current_block_updated == 0xFF)
				return map[0][0];
				
			// Berechne den gesuchten Block
			uint32 block = section_x + section_y*MAP_SECTIONS;
			block = block >>1;	// es passen immer 2 Sections in einen Block
			block += map_start_block;	// Offset drauf
			
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
			
			// richtige der beiden sections raussuchen
			uint8 index= section_x & 0x01;
			return map[index][0];	
		#else // ohne MMC-Karte einfach direkt mit dem SRAM arbeiten
			if ((map[section_x][section_y] == NULL) && (create==True)){
				uint16 index_x, index_y;
				map[section_x][section_y]= malloc(sizeof(map_section_t));
				for (index_x=0; index_x<MAP_SECTION_POINTS; index_x++)
					for (index_y=0; index_y<MAP_SECTION_POINTS; index_y++)
						map[section_x][section_y]->section[index_x][index_y]=0;
				
			}
			return map[section_x][section_y];
		#endif	// MMC_AVAILABLE
	#else
		// Berechne den gesuchten Block
		uint32 block = section_x + section_y*MAP_SECTIONS;
		block = block >>1;	// es passen immer 2 Sections in einen Block
		block += map_start_block;	// Offset drauf
		map_buffer = mmc_get_data(block<<9);
		if (map_buffer != NULL){
			map[0][0]=(map_section_t*)map_buffer;
			map[1][0]=(map_section_t*)(map_buffer+sizeof(map_section_t));
			// richtige der beiden sections raussuchen
			uint8 index= section_x & 0x01;
			return map[index][0];
		} else return map[0][0];	
	#endif	// MMC_VM_AVAILABLE
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
void map_update_field (uint16 x, uint16 y, int8 value) {

	int16 tmp= map_get_field(x,y);
	if (tmp == -128)	// Nicht aktualiseren, wenn es sich um ein Loch handelt
		return;
			
	tmp+=value;
	
	// pruefen, ob kein ueberlauf stattgefunden hat und das Feld nicht schon als Loch (-128) markiert ist
	if ((tmp < 128) && (tmp > -128))
		map_set_field(x,y,(int8)tmp);
}

/*!
 * markiert ein Feld als frei -- drueckt den Feldwert etwas mehr in Richtung "frei"
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 */
void map_update_free (uint16 x, uint16 y) {
  map_update_field(x,y,MAP_STEP_FREE);
}

/*!
 * aendert den Wert eines Feldes um den angegebenen Betrag 
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param value Betrag um den das Feld veraendert wird (>= heisst freier, <0 heisst belegter
 */
#if MAP_RADIUS_FIELDS > 0
	void map_update_field_circle(uint16 x, uint16 y, int8 radius, int8 value) {
		int16 dX,dY;
	    uint16 h=radius*radius;
		for(dX = -radius; dX <= radius; dX++){
	      for(dY = -radius; dY <= radius; dY++) {
	           if(dX*dX + dY*dY <= h)	 
	           	 map_update_field (x + dX, y + dY,value);
	      }
		}
	}
#endif

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
void update_map_sensor(float x, float y, float h, int16 dist){
	//Ort des Sensors in Kartenkoordinaten
	uint16 X = world_to_map(x);
	uint16 Y = world_to_map(y);
	
	uint16 d;
	if (dist==SENS_IR_INFINITE)
		d=SENS_IR_MAX_DIST;
	else 
		d=dist;
		
		
	
	// Hinderniss, dass der Sensor sieht in Weltkoordinaten
	float PH_x = x + (DISTSENSOR_POS_FW + d) * cos(h);
	float PH_y = y + (DISTSENSOR_POS_FW + d) * sin(h);
	// Hinderniss, dass der linke Sensor sieht in Kartenkoordinaten
	uint16 PH_X = world_to_map(PH_x);
	uint16 PH_Y = world_to_map(PH_y);
	
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
	    map_update_free (lX+i*sX, lY);
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
	    map_update_free (lX, lY+i*sY);
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
	
	// Aktualisiere zuerst die vom Bot selbst belegte Flaeche
	const int16 dim = BOT_DIAMETER/2*MAP_RESOLUTION/100;	/*!< Botradius in Map-Aufloesung */
	int16 dX,dY;
	for(dX = -dim; dX <= dim; dX++)
      for(dY = -dim; dY <= dim; dY++) {
           if(dX*dX + dY*dY <= dim*dim)	 
           	 map_update_free (x_map + dX, y_map + dY);
     }
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
 * Prüft ob eine direkte Passage frei von Hindernissen ist
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
//			 map_set_field(lX+i*sX, lY+w,-126);
		     if (map_get_field(lX+i*sX, lY+w) <0) // ein hinderniss reicht für den Abbruch
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
//			 map_set_field(lX+w, lY+i*sY,126);
			 if (map_get_field (lX+w, lY+i*sY) <0 ) //ein hinderniss reicht für den Abbruch
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
 * Prüft ob eine direkte Passage frei von Hindernissen ist
 * @param  from_x Startort x Weltkoordinaten
 * @param  from_y Startort y Weltkoordinaten
 * @param  to_x Zielort x Weltkoordinaten
 * @param  to_y Zielort y Weltkoordinaten
 * @return 1 wenn alles frei ist
 */
int8 map_way_free(float from_x, float from_y, float to_x, float to_y){
	return map_way_free_fields(world_to_map(from_x),world_to_map(from_y),world_to_map(to_x),world_to_map(to_y));
}	

#ifdef PC
	/*!
	 * Schreibt einbe Karte in eine PGM-Datei
	 * @param filename Zieldatei
	 */
	void map_to_pgm(char * filename){
		printf("Speichere Karte nach %s\n",filename);
		FILE *fp = fopen(filename, "w");
		
		uint16 x,y;

		// lokale Variablen mit den defaults befuellen
		uint16 min_x=map_min_x;
		uint16 max_x=map_max_x;
		uint16 min_y=map_min_y;
		uint16 max_y=map_max_y;
		
		#ifdef 	SHRINK_MAP_OFFLINE	// nun muessen wir die Grenezn ermitteln
			// Kartengroesse reduzieren
			int8 free=1;
			while ((min_y < max_y) && (free ==1)){
				for (x=min_x; x<max_x; x++){
					if (map_get_field(x,min_y) != 0){
						free=0;
						break;
					}					
				}
				min_y++;
			}
			free=1;
			while ((min_y < max_y) && (free ==1)){
				for (x=min_x; x<max_x; x++){
					if (map_get_field(x,max_y-1) != 0){
						free=0;
						break;
					}					
				}
				max_y--;
			}
			
			free=1;
			while ((min_x < max_x) && (free ==1)){
				for (y=min_y; y<max_y; y++){
					if (map_get_field(min_x,y) != 0){
						free=0;
						break;
					}					
				}
				min_x++;
			}
			free=1;
			while ((min_x < max_x) && (free ==1)){
				for (y=min_y; y<max_y; y++){
					if (map_get_field(max_x-1,y) != 0){
						free=0;
						break;
					}					
				}
				max_x--;
			}
		
		#endif
			
		uint16 map_size_x=max_x-min_x;
		uint16 map_size_y=max_y-min_y;
		#ifdef MAP_PRINT_SCALE
			fprintf(fp,"P5\n%d %d\n255\n",map_size_x+10,map_size_y+10);
		#else
			fprintf(fp,"P5\n%d %d\n255\n",map_size_x,map_size_y);
		#endif
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
			#endif

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
		#endif
		fclose(fp);
		
	}


	/*! Liest eine Map wieder ein 
	 * @param filename Quelldatei
	 */
	void read_map(char * filename){
		printf("Lese Karte (%s) von MMC/SD (Bot-Format)\n",filename);
		FILE *fp = fopen(filename, "r");
		
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
		
	}
#endif



/*!
 * Zeigt die Karte an
 */
void print_map(void){
	#ifdef PC
		map_to_pgm("map.pgm");
	#else
		// Todo: Wie soll der Bot eine Karte ausgeben ....
	#endif
}



#endif
