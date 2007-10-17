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
 * @file 	behaviour_map_go_destination.c
 * @brief 	Bot faehrt nach der erstellten Karte (bzw. wird online erstellt) zu einer bestimmten
 *          Zielkoordinate und umgeht durch vorherige Pfadplanung nach der Potenzialfeldmethode 
 *          vorhandene Hindernisse; die Zielkoordinate ist zu Beginn die 
 *          Startposition und kann mittels Tastendruck(3) auf die aktuelle 
 *          Botposition geaendert werden; einfach den Bot im Raum umherfahren und bei Druecken der 4
 *          findet der bot den Weg zur Startposition zurueck (oder falls 3 betaetigt wurde zu dieser
 *          neuen Zielposition)
 *
 * @author	Frank Menzel (-alias achiem -Menzelfr@gmx.net)
 * @date	30.04.07
 */


#include "bot-logic/bot-logik.h"
#include "map.h"
#include "ui/available_screens.h"
#include "display.h"
#include "rc5.h"
#include "rc5-codes.h"

#include <stdlib.h>
#include <math.h>
#include "math_utils.h"

#ifdef BEHAVIOUR_MAP_GO_DESTINATION_AVAILABLE

static uint16 next_x=0;		/*!< naechstes anzufahrendes Ziel (x) laut Pfadplanung */
static uint16 next_y=0;		/*!< naechstes anzufahrendes Ziel (y) laut Pfadplanung */
static uint16 target_x=0;	/*!< Zwischenzielkoordinaten X des xy-Fahrverhaltens */				
static uint16 target_y=0;	/*!< Zwischenzielkoordinaten Y des xy-Fahrverhaltens */			

// Teilerwert zur Potenzialberechnung des attraktiven Potenzials je nach Aufloesung und Suchumkreis
#if MAP_RADIUS_FIELDS_GODEST <= 1  // MCU-Aufloesung
	#define DIV_ATTPOT 1  
#else
	#if MAP_RADIUS_FIELDS_GODEST > 9   // sehr hohe Aufloesung 
		#define DIV_ATTPOT 1000
	#else
		#define DIV_ATTPOT 10
	#endif
#endif

#define MAXCOUNTER 100		/*!< Zielsuche bis zu dieser Tiefe, hier wird Routine spaetestens beendet */
  
/*! Werte, wie bestimmte Mapfelder behandelt werden */
#define PATHNODES  127	// aus Pfadplanung 
#define GOAL  127		// Ziel bekommt freieste Feldwahrscheinlichkeit
 
 
/*! 
 * globale Zielkoordinaten fuer Pfadplanung; ist zuerst immer das Bot-Map-Startfeld, da dieses sonst nirgendwo anders
 * gespeichert ist; kann sonst via bot_set_destination auf beliebige Map-Pos gesetzt werden
 */
static uint16 dest_x_map=0;
static uint16 dest_y_map=0;
 
/*! Zustandsvar der Verhalten nach aussen gezogen, um dieses im Botenverhalten korrekt init. zu koennen;nach Notaus
 *  koennte diese sonst einen undef. Zustand haben
 */
static uint8 gotoStatexy=0;  // Statusvar des xy-Fahr-Verhaltens

#ifdef MEASURE_MOUSE_AVAILABLE

#endif

/*! 
 * True wenn Border_map_behaviour einen Abgrund erkannt und markiert hatte;
 * muss vom Anwenderprog auf False gesetzt werden und dient zur Erkennung ob Verhalten
 * zugeschlagen hat; ueber ueber registrierter Prozedur gesetzt vom Abgrundverhalten
 */
static uint8 border_fired=False;


 /*! Elternposition, d.h. Botposition vor neuer Position */
static uint16 x_parent=0;
static uint16 y_parent=0;


/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung; muss registriert werden
 * Notfallroutine wird jetzt auch aufgerufen vom globalen Hang_On-Verhalten mit
 * derselben Behandlung wie im Falle des Notfalls Abgrund
 */
void border_mapgo_handler(void) {
	// Routine muss zuerst checken, ob map_go_destination auch gerade aktiv ist, da nur in diesem
	// Fall etwas gemacht werden muss
	if (!behaviour_is_activated(bot_gotoxy_behaviour_map)) 
		return;

	border_fired=True;  // Setzen der Syncvar des Verhaltens, die immer abgefragt wird

	bot_drive_distance(0,0,-BOT_SPEED_FOLLOW,10);// 10cm rueckwaerts nur bei Hindernis	
}



// ============= eigenstaendiges Parallelverhalten zum eigentlichen Fahrverhalten =======


#ifdef MEASURE_MOUSE_AVAILABLE 

#endif	// MEASURE_MOUSE_AVAILABLE


/*! 
 * Routine zum Setzen der Zielkoordinaten; wird 0 uebergeben, dann auf aktuelle Botposition setzen 
 * @param x X-Map-Zielkoordinate
 * @param y Y-Map-Zielkoordinate
 */
void bot_set_destination(uint16 x, uint16 y) {	
	if (x==0 && y==0) {
		dest_x_map=map_world_to_map(x_pos);
		dest_y_map=map_world_to_map(y_pos);
	}
	else {
		dest_x_map = x;
		dest_y_map = y;
	}
}




// ===== notwendige Routinen zur Ermittlung der Potenziale fuer das eigentliche Fahrverhalten =======

/*!
 * Ermittlung des Abstandes zwischen 2 Koordinaten; verwendet zur Potenzial-Abstandsermittlung zum Zielpunkt,
 * um eine heuristische Schaetzung vorzunehmen
 * @param xs Map-Koordinate des zu untersuchenden Punktes
 * @param ys Map-Koordinate des zu untersuchenden Punktes
 * @param xd Map-Koordinate des Zielpunktes
 * @param yd Map-Koordinate des Zielpunktes
 * @return liefert Quadrat-Abstand zwischen den Map-Punkten als heuristische Funktion zur Entfernungsschaetzung;
 * je kleiner, desto mehr wird dieser Punkt bevorzugt behandelt
 */
static uint16 get_dist(uint16 xs, uint16 ys, uint16 xd, uint16 yd){
	int16 xt=xd - xs;	
	int16 yt=yd - ys;
	
	/* Abstandsermittlung nach dem guten alten Pythagoras ohne Ziehen der Wurzel */
	return (xt*xt+yt*yt);   
}



/*! 
 * ermittelt Abstossungs-Potenzial des Nodes xy, der bereits mit der Wahrscheinlichkeit eines 
 * Hindernisses laut Map belegt ist (-128 - +127 sind moegliche Map-Werte)
 * das repulsive Potenzial ist am groessten umso naeher sich der Punkt am Hindernis befindet und
 * liefert das kleinste Potenzial fuer am weitesten vom Hindernis weg 
 * @param x x-Koord des zu untersuchenden Punktes
 * @param y y-Koord des zu untersuchenden Punktes
 * @param mapval Wert laut Map an Position xy
 * @return repulsives Potenzial fuer Map-Hinderniswahrscheinlichkeit
 */
static uint16 get_rep_pot(uint16 x, uint16 y, int8 mapval) {
	
	// bereits markierter Pfadpunkt bekommt Hinderniswert, sonst wuerde Richtung des schon markierten
	// Weges bevorzugt werden; so also nicht genommen
    if (mapval==PATHNODES) mapval=-MAP_ART_HAZARD;
	
	// alles groesser 0 ist Freiwahrscheinlichkeit, die beim Umherfahren fuer Freifelder eingetragen wird;
	// daher auf 0 setzen mit Gleichbehandlung
	if (mapval > 0) mapval = 0;
	
    // kleinerer Hinderniswert bekommt hiermit groesseres Abstossungspotenzial und wird in den positiven Bereich
    // transferiert	 
    return mapval * (-1) + 127;  
}




/*!
 * liefert Wert fuer das attraktive Potenzial des Zieles aus Abstandsfunktion; bewirkt das 
 * Anziehen des Bots und Ablenken in Richtung Ziel; ist umso groesser je naeher der Punkt
 * sich am Ziel befindet; wie ein Trichter liegt diese Kraft auf der Karte; am groessten
 * ist die Kraft am Zielpunkt selbst, das Potenzial dort also am kleinsten; Potenzial ist 
 * entgegengesetzt der Kraft, wird also in Richtung Ziel immer kleiner
 * @param x x-Koord des zu untersuchenden Punktes
 * @param y y-Koord des zu untersuchenden Punktes
 * @param xdest x-Koord des Zieles
 * @param ydest y-Koord des Zieles
 * @return attraktives Potenzial aus Abstandsermittlung
 */
static uint16 get_att_pot(uint16 x, uint16 y, uint16 xdest, uint16 ydest) {
	/* Als heuristische Potenzialfunktion dient die Bestimmung des Abstandes zum Ziel, wobei der Punkt  
	 * xy das kleinste Potential hat, welcher sich am nahesten am Ziel befindet und konvergiert dort gegen 0
  	 * hoher Wert fuer hohen Abstand
  	 */
	return (DIV_ATTPOT <= 1)  ? 4 * get_dist(x,y,xdest,ydest) : (4 * get_dist(x,y,xdest,ydest)) / DIV_ATTPOT;
}

/*! 
 * liefert verdoppeltes repulsives Potenzial, falls der zu untersuchende Punkt auf den Elternnode verweist
 * somit Wahrscheinlichkeit hoeher, dass ein anderer Weg gegangen wird als wo er gerade herkommt
 * @param xs betrachtete Map-Kooordinate
 * @param ys betrachtete Map-Kooordinate
 * @param reppot bereits ermitteltes Abstossungspotenzial von xs ys
 * @return Verdopplung von reppot oder 0 
 */
static uint16 pot_off_to_parent(uint16 xs, uint16 ys, uint16 reppot) {
	return (map_in_dest(xs,ys,x_parent,y_parent)/*||map_in_dest(xs,ys,x_parent_par,y_parent_par)*/)/*(xs==x_parent && ys==y_parent)*/  ? reppot * 2 : 0;
}



/*! 
 * liefert die Summe aller abstossenden Potenziale ohne anziehendes Zielpotenzial
 * @param xs betrachtete Koordinaten
 * @param ys betrachtete Koordinaten
 * @param xd Zielkoordinaten
 * @param yd Zielkoordinaten
 * @return Summe aller repulsiven Potenziale
 */
static uint16 get_all_rep_pot(uint16 xs, uint16 ys, uint16 xd, uint16 yd) {
	
	uint16 sumpot=0;
	
	//falls der zu untersuchende Knoten der Zielknoten ist, keine Abstossungskraft mit Pot 0
	if (!map_in_dest (xs,ys, xd,yd)) {
		int16 avg=map_get_average_fields(xs,ys,MAP_RADIUS_FIELDS_GODEST_HALF);
		sumpot = get_rep_pot(xs,ys,avg);
	}
	  
	sumpot+=pot_off_to_parent(xs,ys,sumpot);  // Potenzial verdoppelt falls Elternnode derjenige ist
	
    return sumpot;  // Rueckgabe des repulsiven Summenpotenzials
}



/*! 
 * ab dem Knoten x,y wird rundum fuer jeden Knoten das Potenzial ermittelt und der Knoten mit dem kleinsten 
 * Wert bestimmt und in nextx y zurueckgegeben
 * dies ist die Hauptroutine zur Ermittlung des Potenzials in einem beliebigen Punkt
 * @param xdest 	Zielkoordinate (zur Bestimmung des attraktiven Potenzials)
 * @param ydest 	Zielkoordinate (zur Bestimmung des attraktiven Potenzials)
 * @param XP_START	Startkoordinate des Bots
 * @param YP_START	Startkoordinate des Bots
 * @param nextx 	naechster Zwischenzielpunkt mit geringstem Potenzial
 * @param nexty		naechster Zwischenzielpunkt mit geringstem Potenzial
 * @param get_next_pathpoint	Bei True wird der bereits mit Kennung versehene Pfad zum Ziel verfolgt und der 
 * 					naechste bereits als Pfad gekennzeichnete Punkt zurueckgegeben; bei False wird Nachbar mit 
 * 					geringstem Potenzial ermittelt fuer Pathplaning
 * @param roff		Offsetradius zur Umkreissuche  
 */
static void get_gradient(uint16 xdest, uint16 ydest,uint16 XP_START, uint16 YP_START, 
                  uint16 *nextx, uint16 *nexty, int8 get_next_pathpoint, uint8 roff) {
	int8 i;
	int8 j;
	uint16 sumpot=0;
  
	//minimalstes Potenzial wird sich hier gemerkt
	uint16 minpot=9999;  //auf maximalen Wert setzen
	uint16 x_minpos=0;
	uint16 y_minpos=0;
   
	// vorberechnete Spalten und Zeilenzaehler
	uint16 xj=0;
	uint16 yi=0;
	uint16 sumpotrep=0;
  
	uint16 x=XP_START;
	uint16 y=YP_START;
 
	// Rueckgabewerte erst mal initialisieren
	*nextx=0;
	*nexty=0;
  
	int8 mapval=0;  // Wert des Mapfeldes
  
	if (map_in_dest(dest_x_map,dest_y_map,XP_START,YP_START)) { // Start- ist schon Zielpunkt
		*nextx=dest_x_map;
		*nexty=dest_y_map;
		return;  
	}

	// Umkreisermittlung macht nur Sinn bei hoeherer Aufloesung
	#if MAP_RADIUS_FIELDS_GODEST > 1
		int8 radius = MAP_RADIUS_FIELDS_GODEST + roff;  // etwas weiter suchen
		int16 rquad = radius * radius;
		uint16 quad=0;                           // Quadratwert des Abstandes in der Schleife
		uint16 roffq = (roff + 4) * (roff + 4);  // Wert ist Experimentwert um nicht zu viele Felder zu checken 
	#else 
		int8 radius = 1;  // sonst bei geringer Aufloesung direkter Nachbar 
	#endif	// MAP_RADIUS_FIELDS_GODEST > 1
     
	//nur innerhalb des Radiuskreises bleiben
	for (i=-radius; i<=radius; i++) {//Zeilenzaehler
		yi = y + i;
		    
		for (j=-radius; j<=radius; j++) {//Spaltenzaehler
			xj = x + j;

			#if MAP_RADIUS_FIELDS_GODEST > 1  // Umkreisfelder nur bei hoeherer Aufloesung, sonst 8Nachbarschaft
				if (radius>1) quad=i*i + j*j ; 

				if ((!get_next_pathpoint && (quad == rquad || quad==rquad-1 || quad==rquad+1)) || // Potermittlung
			         (get_next_pathpoint && quad >= MAP_RADIUS_FIELDS_GODEST_HALF_QUAD + roffq)) { // Umkreissuche des Weges
			#endif	// MAP_RADIUS_FIELDS_GODEST > 1 
              
			//Potenzial ermitteln an Indexposition; nicht Mittelpos 0,0 selbst
			if (((j!=0)||(i!=0))/* && xj>=0 && yi>=0*/) {

				// nur innerhalb der Rechteckgrenzen
				if (xj>=map_min_x && xj<=map_max_x && yi>=map_min_y && yi<=map_max_y) {
					//wenn nicht betretbar laut Map gar nicht erst auswerten und gleich uebergehen	 		 
					mapval=map_get_field(xj,yi);

					// entweder Pfad erst ermitteln oder den naechsten Punkt des Pfades zurueckgeben
					if ((/*(xj!=x_parent||yi!=y_parent)&&*/(mapval>-MAPFIELD_IGNORE)&& !get_next_pathpoint)||
			           (get_next_pathpoint && mapval==PATHNODES && !map_in_dest(xj,yi,x_parent,y_parent) )) {	
						sumpotrep=9999;
						sumpot=9999;
						/* Pfadverfolgung; bereits gekennzeichnete Pfadpunkte werden gesucht im Botumkreis
						 * gibts mehrere, wird bei geringer Aufloesung derjenige mit kleinstem Abstand zum Ziel genommen 
						 */	
						sumpotrep=0;
						//koennte auch mehrere geben, daher den mit geringstem pot
						if (map_in_dest(xj,yi,dest_x_map,dest_y_map)) {
							sumpot = 0;
						}
	                    else {
							if (get_next_pathpoint) {
								sumpotrep=get_all_rep_pot(xj,yi,xdest,ydest);
								sumpot=sumpotrep + get_att_pot(xj,yi,xdest,ydest);
							}
	                       	else {
								sumpotrep=get_all_rep_pot(xj,yi,xdest,ydest);
								sumpot=sumpotrep + get_att_pot(xj,yi,xdest,ydest);
							}
						}
						//bei kleinerem Potenzial Werte merken, Abstossungspot muss auch > Ignore-Potenzial sein
						if (sumpot<minpot && (get_next_pathpoint||(sumpotrep < 255 && !get_next_pathpoint))) {
							//Node darf nicht existieren um Nachbarn einzufuegen, bei Gradabstieg aber auch wenn noch nicht existiert
							minpot=sumpot;  // Potenzial nur mit Kosten zum Parent
							x_minpos=xj;
							y_minpos=yi;
							*nextx=xj;    // Rueckgabewerte setzen
							*nexty=yi;	
			  	    	}//if sumpot<minpot
					}//if Mapfeld nicht betretbar
				} //ausserhalb der Indexgrenzen 	     
			}//if, Pos selbst
			#if MAP_RADIUS_FIELDS_GODEST > 1	  	// nur bei hoeherer Aufloesung
				} //Suche war ab halben bis ganzem Botradius 	
			#endif
		} //for Spalten x->j			  
	} //for Zeilen y->i
	// Botposition wird nun zur neuen Elternposition
	x_parent=XP_START;
	y_parent=YP_START; 
}



// =================== endlich das eigentliche Fahrverhalten ================================
  

/*! 
 * Verhalten, damit der bot der eingetragenen Map-Pfadlinie folgt; das Verhalten richtet den bot zuerst 
 * zum Endziel aus und faehrt in diese Richtung ohne Pfadplanung; erst wenn ein Hindernis voraus erkannt 
 * wird, erfolgt eine Pfadplanung wobei in der Map die Pfadpunkte in bestimmter Umkreisentfernung vermerkt
 * werden; waehrend der Fahrt wird in der Map im Botumkreis der naechste Pfadpunkt gesucht und dieser 
 * angefahren und bei erreichen des Punktes wiederum der Folgepunkt gesucht; erst wenn keiner gefunden werden 
 * kann (durch Eintragen der Hinderniswahrscheinlichkeit) erfolgt eine neue Pfadplanung an Hand der Potenzialsuche 
 * @param *data der Verhaltensdatensatz
 */
void bot_gotoxy_behaviour_map(Behaviour_t *data) {
	#define INITIAL_TURN 	0
	#define DRIVE_TO_NEXT	2
	#define POS_REACHED	    3
	#define NEXT_POS	    4
	#define INITIAL_GETPATH 5
	#define GO_PATH 	    6
	#define CLEAR_MAP 	    7
	#define CLEAR_MAP_FREEFIELDS 8
	#define DIRECTION_GOAL 	     9
	#define NEXT_IS_HAZARD 	    10
	
	static uint16 diffx_last=0;
	static uint16 diffy_last=0;
	
	static uint8 laststate = 0;  

	int16 X_pos;
	int16 Y_pos;

	int16 xDiff=0;
	int16 yDiff=0;
    
    // zur Erkennung, falls mehrmals dasselbe Ziel angesteuert wird, dies aber nicht geht
    static int16 last_xhaz=0;
    static int16 last_yhaz=0;

	// war Hindernis voraus, wird dessen Mappositionen bestimmt
	uint16 x_nexthaz=0;
    uint16 y_nexthaz=0;

	//Map-Mauspopsition selbst
	X_pos=map_world_to_map(x_pos);
	Y_pos=map_world_to_map(y_pos);
	
	//Jederzeit Schluss wenn Positionen Maus und Endziel uebereinstimmen
	if (map_in_dest (X_pos, Y_pos,dest_x_map, dest_y_map)) {
		gotoStatexy=POS_REACHED;
		speedWishLeft=BOT_SPEED_STOP;
		speedWishRight=BOT_SPEED_STOP;
	}
	else {	// Check ob naechster Punkt ueberhaupt befahrbar ist	
		if (map_get_field(next_x,next_y)<=-MAPFIELD_IGNORE && gotoStatexy!=DIRECTION_GOAL && gotoStatexy!=NEXT_IS_HAZARD) 	
			gotoStatexy=INITIAL_GETPATH;  // Neubestimmung notwendig
	} 

	// hier nun Aufruf der Funktionen nach den Verhaltenszustaenden
	switch(gotoStatexy) {
		case DIRECTION_GOAL:    // Ausrichten und Fahren zum Endziel, solange Weg frei
			next_x=dest_x_map;    // das naechste Ziel ist das Endziel
			next_y=dest_y_map;
			target_x = next_x;    // Zwischenziel identisch Endziel
			target_y = next_y; 
			laststate = DIRECTION_GOAL;  // Kennung setzen dass ich aus Zielfahrt komme
			gotoStatexy=INITIAL_TURN;

			// wenn Weg zum Ziel nicht frei ist oder Abgrund um Bot, dann gleich mit Pfadplaung weiter
			if (map_value_in_circle(X_pos,Y_pos,MAP_RADIUS_FIELDS_GODEST_HALF,-128)) {
	      		laststate = CLEAR_MAP;  // nicht mehr direction_goal
				gotoStatexy = CLEAR_MAP;
			}

			break;

		case CLEAR_MAP:
			// MAP von altem Pfad und kuenstlichen Hindernissen befreien
			map_clear(-MAP_ART_HAZARD, -MAPFIELD_IGNORE); 
			gotoStatexy=CLEAR_MAP_FREEFIELDS;	
			break;

		case CLEAR_MAP_FREEFIELDS:
			// MAP nun von Freifeldern befreien-Originalmap wieder da ohne Freiwahrscheinlichkeiten voraus
			map_clear(-HAZPOT,127); 
			gotoStatexy=INITIAL_GETPATH;	
			break;

		case INITIAL_GETPATH:
			
		    // naechsten Punkt nach Potenzialfeldmethode ermitteln, Wert steht dann in next_x, next_y
		    next_x=0;
		    next_y=0;
		    bot_path_bestfirst(data);   //Umschaltung Pfadsuche mit Deaktivierung dieses Verhaltens
		    laststate=INITIAL_GETPATH;  
		    gotoStatexy=GO_PATH;	   
			break;
			
		case GO_PATH:
			// kommt hierher nach Beendigung des Pfadplanungsverhaltens
		    // Pfadplanung muss Ergebnis liefern, sonst gehts eben nicht weiter
		    		
			// Pfadplanung hat naechsten anzufahrenden Punk um den Bot ermittelt		    
            if (next_x==0 && next_y==0) {  // nichts gefunden -> Ende
				gotoStatexy=POS_REACHED;  // Kein Ziel mehr gefunden go_path und Schluss 
				break;
			}	
            // Zwischenziel belegen 
		    target_x = next_x;
		    target_y = next_y; 	    	
			  
			gotoStatexy=INITIAL_TURN;      // als naechstes Drehen zum Zwischenziel
            
			break;
			
		case INITIAL_TURN:
		    // hier erfolgt Ausrichtung zum naechsten Zwischenzielpunkt durch Aufruf des bereits
		    // vorhandenen bot_turn-Verhaltens
		    xDiff = target_x-X_pos;
		    yDiff = target_y-Y_pos;
		    diffx_last=fabs(xDiff);
		    diffy_last=fabs(yDiff); 
			gotoStatexy=DRIVE_TO_NEXT;      // gedreht sind wir hiernach zum Zwischenziel, also fahren

            // Umschaltung zum Drehverhalten; Drehung zum Zwischenziel wenn noetig
			bot_turn(data,calc_angle_diff(xDiff,yDiff));  
			
			
			break;
  
		case DRIVE_TO_NEXT:
			// hier wird zum Zwischenziel gefahren mit Auswertung auf Haengenbleiben und Locherkennung
		    xDiff=target_x-X_pos;
	        yDiff=target_y-Y_pos;

			// Position erreicht? 
			if (map_in_dest (X_pos, Y_pos,target_x, target_y)) {
				//Zwischenziel erreicht, ist dies auch Endziel dann Schluss
				if (map_in_dest (X_pos, Y_pos,dest_x_map, dest_y_map)) {
					gotoStatexy=POS_REACHED;  // Endziel erreicht und Schluss
					break; 
				} else {
					// Zwischenziel wurde erreicht; bei hoeherer Aufloesung ist der bot nur in der Naehe
					gotoStatexy=NEXT_POS;  // zum Ermitteln der naechsten Position

					// ist der Bot auf gewollte Zwischenzielpos, dann die Pfadkennnung
					// wegnehmen, sonst kann es zu Endlosschleife im lok. Min. kommen
					if (map_get_field(X_pos,Y_pos)==PATHNODES) map_set_field(X_pos,Y_pos,0);
					
					// vorsichtige Weiterfahrt
					speedWishLeft  = BOT_SPEED_FOLLOW;
					speedWishRight = BOT_SPEED_FOLLOW;
					break;				  				
				}				
			}  //Ende Endzielauswertung
			         
			// Check auf zu nah oder nicht befahrbar
			if  (sensDistL<=OPTIMAL_DISTANCE || sensDistR<=OPTIMAL_DISTANCE ||   // < 144mm
			     map_get_field(target_x,target_y) <= -MAPFIELD_IGNORE ) {        // Mapposition nicht befahrbar
			     
				if (laststate==DIRECTION_GOAL) {   // Abbruch der Zielfahrt und Pfadsuche
					gotoStatexy=INITIAL_GETPATH;
					break;
				} else { 
					if (map_in_dest(target_x,target_y,dest_x_map,dest_y_map)) {  // Schluss bei Endziel 
						gotoStatexy=POS_REACHED;	
						break;
					} else {
						gotoStatexy=NEXT_IS_HAZARD;  // Ziel noch nicht erreicht, naechstes Ziel aber Hindernis	
						break;
					}
				}			     
			} 
			
			// wenn Map-Abgrundverhalten Loch erkennt, da ja bot gerade ueber Abgrund gehangen hatte, ist
			// neue Wegsuche mit Hindernis voraus aufzuruefen
			// vom Notfallverhalten wurde durch die registrierte Proc Abgrundkennung gesetzt; Notfallverhalten
			// jetzt auch durch das jetzt globale hang_on Verhalten aufgerufen und kommt hierher
			if (border_fired) {   
				border_fired=False;  // wieder ruecksetzen, erst durch Abgrundverhalten neu gesetzt
				gotoStatexy=NEXT_IS_HAZARD;	  // Hindernis voraus
				break;
			}
			
			//Abstand wird wieder groesser; neue Pfadsuche
			if (laststate!=DIRECTION_GOAL && ( diffx_last<fabs(xDiff)|| diffy_last<fabs(yDiff))) {
				gotoStatexy=NEXT_POS;
				speedWishLeft  = BOT_SPEED_FOLLOW;  // vorsichtige Weiterfahrt
				speedWishRight = BOT_SPEED_FOLLOW;
				break;
			}
   
		    // hier ist der eigentliche Antrieb fuer das Fahren
			#ifdef PC
				speedWishLeft=BOT_SPEED_FAST;
				speedWishRight=BOT_SPEED_FAST;
			#else
				speedWishLeft=BOT_SPEED_MEDIUM;
				speedWishRight=BOT_SPEED_MEDIUM;
			#endif	 // PC 
			
            // Abstandsdifferenz aus diesem Durchlauf merken
			diffx_last=fabs(xDiff);
		    diffy_last=fabs(yDiff); 	
			break;
 
		case NEXT_IS_HAZARD:
			/* das naechste Ziel kann nicht erreicht werden und wird hinderniswahrscheinlicher
			 * Bot ist hier schon rueckwaerts gefahren, steht also nicht mehr auf Abgrundfeld
			 * wenn mehrmals hintereinander dasselbe Ziel ermittelt wird aber Hindernis darstellt,
			 * dann auch auf Hind. setzen
			 */
			if   (last_xhaz==target_x && last_yhaz==target_y) {     // gleiches Ziel wie vorher erkannt 
				map_set_value_occupied(target_x,target_y,-MAP_ART_HAZARD);  // kuenstl. Hindernis
			} else {
				last_xhaz=target_x;  // Hindernisziel aus diesem Durchlauf merken
				last_yhaz=target_y;
			}
  
			// 10 cm voraus Hindernisposition laut Map bestimmen
			//get_mappos_dist(x_pos,y_pos,(heading * M_PI /180),100,&x_nexthaz,&y_nexthaz);
			float alpha = (heading * M_PI /180);
			x_nexthaz=map_get_posx_dist(x_pos,y_pos,alpha,100);
			y_nexthaz=map_get_posy_dist(x_pos,y_pos,alpha,100);

			//Abstand erhoehen auf 15cm falls Botpos ermittelt wurde
			if (x_nexthaz==X_pos && y_nexthaz==Y_pos) {
				//get_mappos_dist(x_pos,y_pos,(heading * M_PI /180),150,&x_nexthaz,&y_nexthaz);
				x_nexthaz=map_get_posx_dist(x_pos,y_pos,alpha,150);
				y_nexthaz=map_get_posy_dist(x_pos,y_pos,alpha,150);
			}

			// neue Pfadplanung wird erforderlich
			gotoStatexy=INITIAL_GETPATH;	
     	  
			// Hohe Hinderniswahrscheinlichkeit mit Umfeldaktualisierung der -50 in Map eintragen 
			map_set_value_occupied(x_nexthaz,y_nexthaz,-HAZPOT); 

			// ist naechstes Ziel nicht Endziel, dann Richtung Hindernis mit Umfeld druecken
			if (!map_in_dest(target_x,target_y,dest_x_map,dest_y_map))
				map_set_value_occupied(target_x,target_y,-HAZPOT); 	  
			break;

		case NEXT_POS:
			/* das naechste Zwischenziel wurde erreicht (geringe Aufloesung) oder bot befindet sich in gewissem
			 * Umkreis (hohe Aufloesung); hier ist der naechste in der Map vermerkte Pfadpunkt zu suchen oder die
			 * Pfadsuche neu anzustossen
			 * wichtig bei hoeherer Aufloesung: Der eigentliche Zielpunkt wird konkret kaum genau erreicht, daher
			 * diesen vom Pfad nehmen und im Umkreis suchen; Abstossungspot setzen damit Benachteiligung bei Neurechnung
			 */
			if (map_value_in_circle(X_pos,Y_pos,MAP_RADIUS_FIELDS_GODEST_HALF,PATHNODES))	    	
				map_set_value_occupied (target_x,target_y,-HAZPOT);
				                                            
			// Suche des naechsten Pfadpunktes in der Map zur Pfadverfolgung
			#if MAP_RADIUS_FIELDS_GODEST > 1
				// bei hoher Aufloesung mit Radiusumkreis entsprechend Aufloesungsfelder
				get_gradient(dest_x_map, dest_y_map, target_x,target_y,&next_x,&next_y,True, MAP_RADIUS_FIELDS_GODEST);
				if (next_x==0 && next_y==0) 
					get_gradient(dest_x_map, dest_y_map, X_pos,Y_pos,&next_x,&next_y,True, MAP_RADIUS_FIELDS_GODEST);
			#else
				// bei geringer MCU Aufloesung immer das naechste Mapfeld
				get_gradient(dest_x_map, dest_y_map, X_pos,Y_pos,&next_x,&next_y,True,0);
			#endif	// MAP_RADIUS_FIELDS_GODEST > 1

			// bei freier Fahrt zum Endepunkt Zielfahrt
			if (map_way_free_fields(X_pos,Y_pos,dest_x_map,dest_y_map)) {
				gotoStatexy=DIRECTION_GOAL;
				break;
			} 
 
			if (next_x==0 && next_y==0) {  // neue Pfadsuche erforderlich mit Maploeschung
				gotoStatexy=CLEAR_MAP;
				break;
			} 
                 
			// naechstes Zwischenziel ist der neu ermittelte Pfadpunkt laut Map
			target_x = next_x;
			target_y = next_y; 
	            		            
			// weiter zum Ausrichten auf neuen Pfadpunkt mit Drehung
			gotoStatexy=INITIAL_TURN;

			// langsam weiterfahren  
			#ifdef PC
				speedWishLeft=BOT_SPEED_NORMAL;
				speedWishRight=BOT_SPEED_NORMAL;
			#else
				speedWishLeft=BOT_SPEED_MEDIUM;
				speedWishRight=BOT_SPEED_MEDIUM;
			#endif	// PC 
			break;
              
		case POS_REACHED:
			// hier ist das gewuenschte Endziel erreicht
			gotoStatexy=DIRECTION_GOAL;
			return_from_behaviour(data);
			break;
	}
}




/*! 
 * Bot faehrt auf eine bestimmt Map-Position relativ zu den akt. Botkoordinaten; globales Ziel
 * wird mit den neuen Koordinaten belegt bei Uebergabe xy ungleich 0
 * @param *caller der Verhaltensdatensatz
 * @param x zu der x-Botkoordinate relative anzufahrende Map-Position
 * @param y zu der y-Botkoordinate relative anzufahrende Map-Position
 */
static void bot_gotoxy_map(Behaviour_t *caller, uint16 x, uint16 y) {
	//Mauskoords in  Mapkoordinaten 
	uint16 X_pos = map_world_to_map(x_pos);
	uint16 Y_pos = map_world_to_map(y_pos);
	
	//globale Zielkoordinaten setzen, die fuer Pfadplaner Verwendung finden
	if (x!=0 || y!=0) {       // fuer Fahrt relativ zu der Botposition
		dest_x_map=X_pos + x;
		dest_y_map=Y_pos + y;
	}

    /* Kollisions-Verhalten ausschalten bei hoeherer Aufloesung; hier kann mit groesserem 
     * Hindernis-Umkreis ein groesserer Abstand erzielt werden */
	#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
		deactivateBehaviour(bot_avoid_col_behaviour);
	#endif
  
	/* Einschalten sicherheitshalber des on_the_fly Verhaltens, wird bei Notaus sonst deaktiviert */
	#ifdef BEHAVIOUR_SCAN_AVAILABLE
		activateBehaviour(bot_scan_onthefly_behaviour);
	#endif

	/* Einschalten sicherheitshalber des Abgrund-Verhaltens */	
    #ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
		activateBehaviour(bot_avoid_border_behaviour);
	#endif	

    gotoStatexy=DIRECTION_GOAL;  // auf Zielfahrt init.
   	switch_to_behaviour(caller, bot_gotoxy_behaviour_map, OVERRIDE);  // Umschaltung zum Fahr-Verhalten
}


/*! 
 * Verhalten laesst den Bot zum Endziel fahren, welches vorher auf eine bestimmte Botposition festgelegt wurde;
 * ansonsten ist das Ziel der Bot-Startpunkt
 * @param *caller der Verhaltensdatensatz
 */
static void bot_gotodest_map(Behaviour_t *caller) {
	// zum bereits gesetztem Endziel gehen mit 0-Differenz
	bot_gotoxy_map(caller, 0,0);	  
}


/*!
 * Verhalten zur Generierung des Pfades vom Bot-Ausgangspunkt zum Ziel nach der Potenzialfeldmethode;
 * es wird der Weg ermittelt von der Botposition mit hohem Potenzial bis zum gewuenschten Ziel mit geringem
 * Potenzial; Um den Hindernissen herum existiert laut den Mapwerten ein abstossendes Potenzial und die 
 * Entfernung zum Ziel dient als heuristisches Mittel fuer das global anziehende Potenzial; die Wegpunkte werden
 * mit bestimmter Kennung in die Map eingetragen und diese Pfadpunkte im Fahrverhalten gesucht und nacheinander
 * angefahren 
 *   
 * @param *data der Verhaltensdatensatz
 */
void bot_path_bestfirst_behaviour(Behaviour_t *data) {
	#define GET_PATH_INITIAL  0
	#define GET_PATH_BACKWARD 1 
	#define REACHED_POS       9 
	
	static uint8 state=GET_PATH_INITIAL;
	static uint8 pathcounter=0;  // Abbruchzaehler fuer Pfadermittlung

    // ausgehend von diesem Punkt wird das Potenzial ermittelt
    static uint16 X=0;
    static uint16 Y=0;
    static uint16 x_first=0;       // 1. anzufahrender Punkt laut Pfadplanung ab Botposition
    static uint16 y_first=0;
    static uint8 counter_noway=0;

	switch (state){		
		case GET_PATH_INITIAL:
		
			// alle als frei markierten Mappositionen > 0 wieder auf 0 setzen
			map_clear(0,127);  // Loeschen der Freifelder, nicht der kuenstlichen Hindernisse

			// Bot-Map-Position ermitteln
			X=map_world_to_map(x_pos);
			Y=map_world_to_map(y_pos);

			// Eltern init auf Botpos.
			x_parent=X;
			y_parent=Y;

			// erste anzufahrende Botposition init.
			x_first=0;
			y_first=0;

			//  Pfadermittlung kann nun losgehen
			state=GET_PATH_BACKWARD;
			break;

		case GET_PATH_BACKWARD:
			// Gradientenabstieg-naechste Koord mit kleinstem Potenzial ab Punkt X,Y ermitteln
			get_gradient(dest_x_map, dest_y_map, X,Y,&next_x,&next_y,False,0);

            // Schluss falls kein Wert ermittelbar
			if (next_x==0 && next_y==0) {
				state = REACHED_POS;
				break;  
			}
            
            // am Endziel mit Pfadplanung angekommen und Schluss
            if (map_in_dest(next_x,next_y,dest_x_map,dest_y_map)) {
				map_set_field(dest_x_map,dest_y_map,PATHNODES);  // Endziel auch mit Pfadpunkt markieren   
				state = REACHED_POS;
				if (x_first==0) {    
					x_first=next_x;
					y_first=next_y;
				}
				break;	
			}
                       
			// lokales Minimum -Sackgasse- wird erkannt; versucht hier rauszukommen durch Auffuellen 
			if ((next_x==X && next_y==Y) || map_value_in_circle(next_x,next_y,MAP_RADIUS_FIELDS_GODEST_HALF,PATHNODES)) {
				// hier wird der bereits beschrittene Weg erreicht, kein anderer Zielpunkt hat kleineres Pot
            	// Pot dieses Punktes nun kuenstlich vergrossern Richtung Hindernis und neu suchen
            	
                // im halben Botradius auf kuenstl. Hindernis setzen 
				map_update_occupied(next_x,next_y);           // Richtung Hindernis druecken mit Umfeld
				map_set_field(next_x,next_y,-MAP_ART_HAZARD); // lok. Min. direkt auf Punkt unpassierbar
                 
				map_update_occupied(X,Y);  //aktuelle Pos unwahrscheinlicher
                  
				state=GET_PATH_INITIAL;                                // neue Pfadsuche 
				pathcounter++;                                         // Pfadsuchzaehler erhoeht
				if (pathcounter>MAXCOUNTER)                           // Schluss wenn Maximum erreicht
					state = REACHED_POS;

				next_x=0;                                              // nix gefunden
                next_y=0;
                
            	break;
            } else {                         
				// ersten Punkt um botpos merken, da dieser sonst ueberschrieben werden kann durch 
				// Mapaktualisierung der Botpos
				if (x_first==0) {
					x_first=next_x;
					y_first=next_y;
				}              
			}
                   
			// Abfrage ob Bot-Zielposition erreicht wurde und damit Pfad erfolgreich bestimmt
			if (state!=REACHED_POS && map_in_dest (map_world_to_map(x_pos),map_world_to_map(y_pos), dest_x_map,dest_y_map)) 
				state = REACHED_POS;
 
			// naechsten Zielpunkt in die Map eintragen
			map_set_field(next_x,next_y,PATHNODES);

            x_parent=X;  // als Parent den aktuellen Punkt merken
            y_parent=Y;
            
            X=next_x;    // der weiter zu untersuchende Punkt ist der Punkt aus Gradientenabstieg
            Y=next_y;

			break;	
			
		default:
			state=GET_PATH_INITIAL;
			// zuerst mehrere Versuche starten fuer Zielfindung, falls dieses nicht ermittelt
			// werden konnte
			counter_noway++;
			if (next_x==0 && next_y==0 && (counter_noway < 4)) {
				if (counter_noway==1) {
					state=GET_PATH_INITIAL;
					pathcounter=0;
					map_clear(-MAP_ART_HAZARD, -MAP_ART_HAZARD+10);  // Loeschen der kuenstlichen Hindernisse
					break;
				}
				if (counter_noway==2) {
					state=GET_PATH_INITIAL;
					pathcounter=0;
					map_clear(-MAP_ART_HAZARD, 127);  // Loeschen der kuenstlichen Hindernisse
					break;
				}
				if (counter_noway==3) {
					state=DIRECTION_GOAL;
					break;
			    }
			}
			counter_noway=0;
            next_x=x_first; // 1. Punkt des Weges ab Botposition zurueckgeben
            next_y=y_first;
            pathcounter=0;
			return_from_behaviour(data);
			break; 
	}
}

/*! 
 * Botenverhalten zum Aufruf des Pfadplanungs-Verhaltens
 * @param *caller der Verhaltensdatensatz
 */
void bot_path_bestfirst(Behaviour_t *caller) {
	// Botposition in Map-Koordinaten berechnen
	uint16 X = map_world_to_map(x_pos);
	uint16 Y = map_world_to_map(y_pos);
	
	if (!map_in_dest (X,Y, dest_x_map,dest_y_map)) {
		// Ziel bekommt freieste Feldwahrscheinlichkeit	
		map_set_field(dest_x_map,dest_y_map,GOAL);

		// Botpos selbst erhaelt hohes Abstossungspotenzial	
		map_set_field(X,Y,-90);	
		
		// Umschaltung zur Pfadplanung 	 
		switch_to_behaviour(caller, bot_path_bestfirst_behaviour,OVERRIDE);
	}
}





#ifdef DISPLAY_MAP_GO_DESTINATION
	static void mapgo_disp_key_handler(void) {
		/* Keyhandling um Map-Verhalten zu starten */
		switch (RC5_Code) {
			#ifdef PC
				case RC5_CODE_1:
					RC5_Code = 0;
					map_print();	
					break;
				#endif
			case RC5_CODE_3: 
				RC5_Code = 0;	
				bot_set_destination(map_world_to_map(x_pos),map_world_to_map(y_pos));
				break;
			case RC5_CODE_4: 
				RC5_Code = 0;	
				map_clear(-MAP_ART_HAZARD, -MAPFIELD_IGNORE);  // Loeschen der kuenstlichen Hindernisse und des alten Pfades fuer Neubeginn
				next_x=0;
				next_y=0;
				bot_gotodest_map(0);
				break;
			case RC5_CODE_7: 
				// Volldrehung mit Mapaktualisierung
				RC5_Code = 0;	
				bot_scan(0);
				break;	
		}	// switch
	}  // Ende Keyhandler


	/*! 
	 * @brief	Display zum Start der Map-Routinen
	 */
	void mapgo_display(void) {
		display_cursor(1,1);
		display_printf("MAP last %1d %1d",dest_x_map,dest_y_map);
		display_cursor(2,6);
		display_printf("Bot %1d %1d",map_world_to_map(x_pos),map_world_to_map(y_pos));
		display_cursor(3,1);
		display_printf("Pos Save/Goto: 3/4");
// 		display_cursor(3,1);
//		display_printf("fahre zu Pos : 4 ");
		#ifdef PC
			display_cursor(4,4);
			display_printf("Print/Scan: 1/7");
		#else
	   		display_cursor(4,10);
			display_printf("Scan: 7");
		#endif	// PC
		mapgo_disp_key_handler();		  // aufrufen des Key-Handlers
	}
#endif	// MAPGO_DISPLAY

#endif  // BEHAVIOUR_MAP_GO_DESTINATION_AVAILABLE
