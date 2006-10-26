/*
 * c't-Bot - Robotersimulator fuer den c't-Bot
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

/*! @file 	map.c  
 * @brief 	Karte 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	19.09.06
*/
#include <stdio.h>
#include "ct-Bot.h"
#include "bot-local.h"
#include "sensor_correction.h"
#include "map.h"

#ifdef MAP_AVAILABLE
#include <math.h>
#include <stdlib.h>

/*
 * Eine Karte ist wie folgt organisiert:
 * Es gibt Sektionen zu je MAP_SECTION_WIDTH cm * MAP_SECTION_HEIGHT cm. Diese Sektionen enthalten direkt die Pixel-Daten
 * Die Sektionen sind in Hierarchsichen Gruppen organisiert. Jede Gruppe hat MAP_GROUP_X * MAP_GROUP_Y Sektionen
 * Die Gruppen können entweder direkt Sektionen oder wieder Gruppen enthalten. So laesst sich leicht eine Karte aufbauen, 
 * ohne dass viel Platz fuer unbenutzte Felder draufgeht
 * Das Zentrum (0,0) liegt immer in der Mitte der obersten Gruppe
 */

#define MAP_RESOLUTION 	500	/*!< Aufloesung der Karte in Punkte pro Meter */

#define MAP_SECTION_WIDTH_CM	80	/*!< Breite eines Kartenfeldes in cm (entsptricht der Sichtweite des Bots) */
#define MAP_SECTION_HEIGHT_CM 80	/*!< Hoehe eines Kartenfeldes in cm (entsptricht der Sichtweite des Bots) */

#define MAP_SECTION_WIDTH	((MAP_SECTION_WIDTH_CM*MAP_RESOLUTION)/100)	/*!< Breite eines Kartenfeldes in Punkten */
#define MAP_SECTION_HEIGHT ((MAP_SECTION_HEIGHT_CM*MAP_RESOLUTION)/100)	/*!< Hoehe eines Kartenfeldes in Punkten */


typedef struct {
	uint8 depth; /*!< Tiefe der Hierarchie. 0 bedeutet: diese gruppe enthaelt nur noch sections. 1 bedeutet diese gruppe enmthaelt nur noch gruppen mit der depth 0. usw.*/
	void * elements[3][3]; /*!< zeiger auf die Elemente */
} map_group_t; /*!< Datentyp fuer die Gruppen eine Map */

typedef struct {
	int8 section[MAP_SECTION_WIDTH][MAP_SECTION_HEIGHT]; /*!< Einzelne Punkte */
} map_section_t;   /*!< Datentyp fuer die elementarfelder einer Gruppe */


//#define MAP_WIDTH	3			/*!< Breite (X-Dim) der Karte in Meter */
//#define MAP_HEIGHT	3			/*!< Hoehe (Y-Dim) der Karte in Meter */

//uint8 map[MAP_WIDTH*MAP_RESOLUTION][MAP_HEIGHT*MAP_RESOLUTION];

#define MAP_STEP_FREE		2	/*!< Stufenbreite, wenn ein Feld als frei erkannt wird */
#define MAP_STEP_OCCUPIED	10	/*!< Stufenbreite, wenn ein Feld als belegt erkannt wird */

#define MAP_RADIUS			5	/*!< Umkreis um einen Punkt, der aktualisiert wird (Streukreis) [Felder]*/

map_group_t* map = NULL;	/*!< Der Einstieg in die Karte*/

/*! Erweitere die Karte solange, bis sie new_depth-Tiefe hat.
 * @param neue tiefe der Karte 
 */
void expand_map(uint8 new_depth);

/*!
 * aendert den Wert eines Feldes um den angegebenen Betrag 
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param value Betrag um den das Feld veraendert wird (>= heisst freier, <0 heisst belegter
 */
void update_field (int16 x, int16 y, int16 value) {
	int16 section_x, section_y;
	
	if (map == NULL) 
		init_map();
	
	// Berechne in welcher Sektion sich der Punkt befindet
	section_x=x/ MAP_SECTION_WIDTH;
	section_y=y/ MAP_SECTION_HEIGHT;

	uint8 depth =0;
	uint16 sections =1;
	uint16 sections_per_group=3;
	
	// finde noetige hierarchie-ebene, in der das Feld liegt
	while ((section_x > sections) ||(section_y > sections)) {
		sections+=sections_per_group;
		sections_per_group*=3;
		depth++;
	}

	// Pruefe, ob die karte ueberhaupt schon die noetige Tiefe hat	
	if (depth > map->depth)
		expand_map(depth); // Wenn nein, erweitere sie
		
	
	map_group_t * group = map;
	int16 index_x;
	// finde die passende Gruppe
	for (;depth >0;depth--){	// Steige Ebene fuer Ebene ab
		if (abs(section_x) < sections_per_group/2)
			index_x=1;
		else if (section_x < 0)
			index_x=0;
			else index_x=2;		

		if (abs(section_y) < sections_per_group/2)
			index_y=1;
		else if (section_y < 0)
			index_y=0;
			else index_y=2;		

		group= group->elements[index_x][index_y];
		sections_per_group /= 3;
	}
	

  if ((x > 0) && (x<MAP_WIDTH*MAP_RESOLUTION-1) && 
      (y > 0) && (y<MAP_HEIGHT*MAP_RESOLUTION-1)){
      if ((value + (int16) map[x][y] >= 0) && (value + (int16) map[x][y] < 256)) 
     	map[x][y]+=value;
  } else {
//		printf("Versuch ein Feld ausserhalb der Karte zu schreiben!! x=%d y=%d\n",x,y);	
  }
}

/*!
 * markiert ein Feld als frei -- drueckt den Feldwert etwas mehr in Richtung "frei"
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 */
void update_free (int16 x, int16 y) {
  update_field(x,y,MAP_STEP_FREE);
}

/*!
 * aendert den Wert eines Feldes um den angegebenen Betrag 
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 * @param value Betrag um den das Feld veraendert wird (>= heisst freier, <0 heisst belegter
 */
void update_field_circle(int16 x, int16 y, int16 radius, int16 value) {
	int16 dX,dY;
    int16 h=radius*radius;
	for(dX = -radius; dX <= radius; dX++){
      for(dY = -radius; dY <= radius; dY++) {
           if(dX*dX + dY*dY <= h)	 
           	 update_field (x + dX, y + dY,value);
      }
	}
}

/*!
 * markiert ein Feld als belegt -- drueckt den Feldwert etwas mehr in Richtung "belegt"
 * @param x x-Ordinate der Karte (nicht der Welt!!!)
 * @param y y-Ordinate der Karte (nicht der Welt!!!)
 */
void update_occupied (int x, int y) {
// update_field(x,y,-MAP_STEP_OCCUPIED);
  
  uint8 r;
  for (r=1; r<=MAP_RADIUS; r++){
  	update_field_circle(x,y,r,-MAP_STEP_OCCUPIED/MAP_RADIUS);
  }
}


/*! 
 * Aktualisiert die Karte mit den Daten eines Sensors
 * @param x X-Achse der Position des Sensors
 * @param y Y-Achse der Position des Sensors
 * @param head Blickrichtung im Bogenmaß
 * @param dist Sensorwert 
 */ 
void update_map_sensor(float x, float y, float h, int16 dist){
	//Ort des Sensors in Kartenkoordinaten
	int16 X = x * MAP_RESOLUTION / 1000  + (MAP_WIDTH*MAP_RESOLUTION/2);
	int16 Y = y * MAP_RESOLUTION / 1000  + (MAP_HEIGHT*MAP_RESOLUTION/2);

	int16 d;
	if (dist==SENS_IR_INFINITE)
		d=SENS_IR_MAX_DIST;
	else 
		d=dist;
		
		
	
	// Hinderniss, dass der Sensor sieht in Weltkoordinaten
	float PH_x = x + (DISTSENSOR_POS_FW + d) * cos(h);
	float PH_y = y + (DISTSENSOR_POS_FW + d) * sin(h);
	// Hinderniss, dass der linke Sensor sieht in Kartenkoordinaten
	int16 PH_X = PH_x* MAP_RESOLUTION / 1000  + (MAP_WIDTH*MAP_RESOLUTION/2);
	int16 PH_Y = PH_y* MAP_RESOLUTION / 1000  + (MAP_HEIGHT*MAP_RESOLUTION/2);
	
	if ((dist > 80 ) && (dist <SENS_IR_INFINITE))
			update_occupied(PH_X,PH_Y);
	


	// Nun markiere alle Felder vor dem Hinderniss als frei
	int16 i;

	int16 lX = X; 	//	Beginne mit dem Feld, in dem der Sensor steht
	int16 lY = Y; 
	
	int8 sX = (PH_X < X ? -1 : 1);  
	int16 dX=abs(PH_X - X);		// Laenge der Linie in X-Richtung
	
	int8 sY = (PH_Y < Y ? -1 : 1); 
	int16 dY =abs(PH_Y - Y);	// Laenge der Linie in Y-Richtung

	if (dX >= dY) {			// Hangle Dich an der laengeren Achse entlang
	  dY--;	// stoppe ein Feld vor dem Hinderniss
	  int16 lh = dX / 2;
	  for (i=0; i<dX; ++i) {
	    update_free (lX+i*sX, lY);
	    lh += dY;
	    if (lh >= dX) {
	      lh -= dX;
	      lY += sY;
	    }
	  }
	} else {
	  dX--; // stoppe ein Feld vor dem Hinderniss
	  int16 lh = dY / 2;
	  for (i=0; i<dY; ++i) {
	    update_free (lX, lY+i*sY);
	    lh += dX;
	    if (lh >= dY) {
	      lh -= dY;
	      lX += sX;
	    }
	  }
	}
	
}



/*!
 * Aktualisiert die interne Karte
 * @param x X-Achse der Position
 * @param y Y-Achse der Position
 * @param head Blickrichtung in Grad
 * @param distL Sensorwert links
 * @param distR Sensorwert rechts
 */
void update_map(float x, float y, float head, int16 distL, int16 distR){
//	printf("update_map: x=%f, y=%f, head=%f, distL=%d, distR=%d\n",x,y,head,distL,distR);
	
	int16 x_map = x * MAP_RESOLUTION / 1000  + (MAP_WIDTH*MAP_RESOLUTION/2);
	int16 y_map = y * MAP_RESOLUTION / 1000  + (MAP_HEIGHT*MAP_RESOLUTION/2);
    float h= head * M_PI /180;
	
	// Aktualisiere zuerst die vom Bot selbst belegte Flaeche
	#define dim BOT_DIAMETER/2*MAP_RESOLUTION/100
	int16 dX,dY;
	for(dX = -dim; dX <= dim; dX++)
      for(dY = -dim; dY <= dim; dY++) {
           if(dX*dX + dY*dY <= dim*dim)	 
           	 update_free (x_map + dX, y_map + dY);
     }
        
    //Ort des rechten Sensors in Weltkoordinaten
    float Pr_x = x + (DISTSENSOR_POS_SW * sin(h));
	float Pr_y = y - (DISTSENSOR_POS_SW * cos(h));

    //Ort des linken Sensors in Weltkoordinaten
    float Pl_x = x - (DISTSENSOR_POS_SW * sin(h));
	float Pl_y = y + (DISTSENSOR_POS_SW * cos(h));

	update_map_sensor(Pl_x,Pl_y,h,distL);
	update_map_sensor(Pr_x,Pr_y,h,distR);
}


#ifdef PC
	/*!
	 *
	 */
	void map_to_pbm(char * filename){
		FILE *fp = fopen(filename, "w");
		
		fprintf(fp,"P5\n%d %d\n255\n",MAP_WIDTH*MAP_RESOLUTION,MAP_HEIGHT*MAP_RESOLUTION);
		
		int16 x,y;
		uint8 tmp;
		for (y=MAP_HEIGHT*MAP_RESOLUTION-1; y>=0; y--)
			for (x=0; x<MAP_WIDTH*MAP_RESOLUTION; x++){
				tmp=map[x][y];
				fwrite(&tmp,1,1,fp);
			}
		fclose(fp);
		
	}
#endif

/*!
 * Zeigt die Karte an
 */
void print_map(void){
/*	int x,y;
	for (y=MAP_HEIGHT*MAP_RESOLUTION-1; y>=0; y--){
	  for (x=0; x<MAP_WIDTH*MAP_RESOLUTION; x++)
	    if (map[x][y] == 127)
	    	printf(" ");
	    else if (map[x][y] > 150)
	    	printf(".");
	    else if (map[x][y] < 100)
	    	printf("X");
	    else
	    	printf("%x",map[x][y]/16);
	  printf("\n");
	}
*/
	#ifdef PC
		map_to_pbm("map.pgm");
	#endif
}



/*! Erzeugt eine leere Gruppe 
 * @param depth tiefe der Gruppe
 */
map_group_t * newMapGroup(uint8 depth){
	map_group_t * new_group;

	new_group = malloc(sizeof(map_group_t));
	new_group->depth=depth;
	
	int x,y;
	for (x=0; x<3; x++){
		for (y=0; y<3; y++) {
			new_group->elements[x][y] = NULL;
		}
	}
	return new_group;	
}

/*! Erweitere die Karte solange, bis sie new_depth-Tiefe hat.
 * @param neue tiefe der Karte 
 */
void expand_map(uint8 new_depth){
	map_group_t * new_group;
	while (map->depth < new_depth){
		new_group= newMapGroup(map->depth+1);
		new_group->elements[1][1]=map;
		map= new_group;
	}
}

/*! 
 * initialisiere die interne Karte 
 */
void init_map(void){
	map = newMapGroup(0);
	
//	for (x=0; x<MAP_WIDTH*MAP_RESOLUTION; x++)
//	  for (y=0; y<MAP_HEIGHT*MAP_RESOLUTION; y++)
//	    map[x][y]=127;
}

#endif
