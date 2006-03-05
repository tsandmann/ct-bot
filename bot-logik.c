/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	bot-logik.c
 * @brief 	High-Level Routinen fuer die Steuerung des c't-Bots.
 * Diese Datei sollte der Einstiegspunkt fuer eigene Experimente sein, 
 * den Roboter zu steuern.
 * 
 * bot_behave() arbeitet eine Liste von Verhalten ab. 
 * Jedes Verhalten kann entweder absolute Werte setzen, dann kommen niedrigerpriorisierte nicht mehr dran
 * Alternativ dazu kann es Modifikatoren aufstellen, die bei niedriger priosierten angewendet werden.
 * bot_behave_init() baut diese Liste auf.
 * Jede Verhaltensfunktion bekommt einen Verhaltensdatensatz übergeben, in den Sie ihre Daten eintraegt
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/
#include <stdio.h>

#include "ct-Bot.h"
#include "motor.h"
#include "sensor.h"
#include "bot-logik.h"
#include "display.h"
#include "bot-local.h"

#include "rc5.h"
#include <stdlib.h>
#include <math.h>
/*
 * Alle Konstanten, die die Verhalten befinden sind in bot-local.h ausgelagert. 
 * Dort kann man sie per .cvsignore vor updates schützen 
 * 
 * Alle Variablen mit Sensor-Werten findet man in sensor.h
 */

/*! Verwaltungsstruktur für die Verhaltensroutinen */
typedef struct _Behaviour_t {
   void (*work) (struct _Behaviour_t *data); 	/*!< Zeiger auf die Funktion, die das Verhalten bearbeitet */
   
   uint8 priority;		/*!< Priorität */
   struct _Behaviour_t *caller ; /* aufrufendes verhalten */
   
   char active:1;				/*!< Ist das Verhalten aktiv */
   char subResult:2;			/*!< War das aufgerufene unterverhalten erfolgreich (==1)?*/
   struct _Behaviour_t *next;					/*!< Naechster Eintrag in der Liste */
#ifndef DOXYGEN
	}__attribute__ ((packed)) Behaviour_t;
#else
	} Behaviour_t;
#endif
	

char col_zone_l=ZONE_CLEAR;			/*!< Kollisionszone, in der sich der linke Sensor befindet */
char col_zone_r=ZONE_CLEAR;			/*!< Kollisionszone, in der sich der rechte Sensor befindet */

volatile int16 mot_l_goto=0;		/*!< Speichert, wie weit der linke Motor drehen soll */
volatile int16 mot_r_goto=0;		/*!< Speichert, wie weit der rechte Motor drehen soll */

int16 speedWishLeft;				/*!< Puffervariablen für die Verhaltensfunktionen absolut Geschwindigkeit links*/
int16 speedWishRight;				/*!< Puffervariablen für die Verhaltensfunktionen absolut Geschwindigkeit rechts*/

float faktorWishLeft;				/*!< Puffervariablen für die Verhaltensfunktionen Modifikationsfaktor links*/
float faktorWishRight;				/*!< Puffervariablen für die Verhaltensfunktionen Modifikationsfaktor rechts */

volatile int16 target_speed_l=0;	/*!< Sollgeschwindigkeit linker Motor - darum kuemmert sich bot_base()*/
volatile int16 target_speed_r=0;	/*!< Sollgeschwindigkeit rechter Motor - darum kuemmert sich bot_base() */

/*! Liste mit allen Verhalten */
Behaviour_t *behaviour = NULL;

#define OVERRIDE	1	/*!< Konstante, wenn Verhalten beim Aufruf alte Wuensche ueberschreiben sollen */
#define NOOVERRIDE 0	/*!< Konstanten, wenn Verhalten beim Aufruf alte Wuensche nicht ueberschreiben sollen */

#define SUBSUCCESS	1	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe erfolgreich abgeschlossen */
#define SUBFAIL	0	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe nicht abgeschlossen */
#define SUBRUNNING 2	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe wird noch beabeitet */

/*!
 * Aktiviert eine Regel mit gegebener Prioritaet
 * @param priority Die Prioritaet der zu aktivierenden Regel
 */
void activateBehaviour(void *function){
	Behaviour_t *job;						// Zeiger auf ein Verhalten

	// Einmal durch die Liste gehen, bis wir den gwuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == function) {
			job->active = 1;
			break;
		}
	}
}


/*!
 * Deaktiviert eine Regel mit gegebener Prioritaet
 * @param priority Die Prioritaet der zu deaktivierenden Regel
 */
void deactivateBehaviour(void *function){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
		
	// Einmal durch die Liste gehen, bis wir den gwuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == function) {
			job->active = 0;
			break;
		}
	}
}

/*! 
 * Ruft ein anderes Verhalten auf und merkt sich den Ruecksprung 
 * return_from_behaviour() kehrt dann spaeter wieder zum aufrufenden Verhalten zurueck
 * @param from aufrufendes Verhalten
 * @param to aufgerufenes Verhalten
 * @param override Steht hier ein 1, so führt das aufgerufene Verhalten den Befehl aus, 
 * auch wenn es gerade etwas tut. 
 * Achtung das bedeutet jedoch, dass der ursprüngliche Caller
 * wieder aktiviert wird. er muss selbst prüfen, ob er mit dem Zustand zufrieden ist!
 */ 
void switch_to_behaviour(Behaviour_t * from, void *to, uint8 override ){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
	
	// Einmal durch die Liste gehen, bis wir den gwuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == to) {
			break;
		}
	}	

	if (job->caller){		// Ist das auzurufende Verhalten noch beschaeftigt?
		if (override==NOOVERRIDE){	// nicht ueberschreiben, sofortige rueckkehr
			if (from)
				from->subResult=SUBFAIL;
			return;
		}
		// Wir wollen also ueberschreiben, aber aber nett zum alten Aufrufer sein und ihn darueber benachrichtigen
		job->caller->active=1;	// alten aufrufer reaktivieren
		job->caller->subResult=SUBFAIL;	// er bekam aber nicht das gewuenschte resultat
	}

	if (from) {
		// laufendes verhalten abschalten
		from->active=0;
		from->subResult=SUBRUNNING;
	}
		
	// neues Verhalten aktivieren
	job->active=1;
	// aufrufer sichern
	job->caller =  from;
}

/*! 
 * Kehrt zum aufrufenden Verhalten zurück
 * @param running laufendes Verhalten
 */ 
void return_from_behaviour(Behaviour_t * data){
	data->active=0; 				// Unterverhalten deaktivieren
	if (data->caller){			
		data->caller->active=1; 	// aufrufendes Verhalten aktivieren
		data->caller->subResult=SUBSUCCESS;	// Unterverhalten war erfolgreich
	}
	data->caller=NULL;				// Job erledigt, verweis loeschen
}


/*!
 * Drehe die Raeder um die gegebene Zahl an Encoder-Schritten weiter
 * @param left Schritte links
 * @param right Schritte rechts
 */
void bot_goto(int16 left, int16 right, Behaviour_t * caller);


/*! 
 * Ein ganz einfaches Verhalten 
 * Es hat maximale Prioritaet
 * Hier kann man ganz einfach spielen. Eleganter ist es jedoch, dieses Nicht zu verwenden!!!
 * Wer Lust hat sich mit dem ganzen Verhaltensframework zu beschäfigen, kann diese Funktion getrost auskommentieren
 * und findet dann in bot_behave_init() und bot_behave() weiter Hinweise ....
 * @param *data der Verhaltensdatensatz
 */
void bot_simple(Behaviour_t *data){
/* Diese Routine unterscheidet sich minimal von der in c't 04/06
 * Das Verhaltensframework hat in der Zwischenzeit eine 
 * Weiterentwicklung durchlaufen */

/*
  int16 speed_l_col, speed_r_col;

  speedWishLeft=BOT_SPEED_MAX;
  speedWishRight=BOT_SPEED_MAX;

  if (sensDistL < COL_NEAR)
    speed_r_col=-speed_r-BOT_SPEED_NORMAL;
  else speed_r_col=0;
  
  if (sensDistR < COL_NEAR)
    speed_l_col=-speed_l-BOT_SPEED_FAST;
  else speed_l_col=0;

  speedWishLeft+=speed_l_col;
  speedWishRight+=speed_r_col;  
  */
}

/*!
 * Beispiel fuer ein Verhalten, das einen Zustand besitzt
 * es greift auf andere Verhalten zurueck und setzt daher 
 * selbst keine speedWishes
 * Laesst den Roboter ein Quadrat abfahren
 * @param *data der Verhaltensdatensatz
 */
void bot_drive_square(Behaviour_t *data){
	#define STATE_TURN 1
	#define STATE_FORWARD 0
	#define STATE_INTERRUPTED 2
	
	static uint8 state = STATE_FORWARD;

   if (data->subResult == SUBFAIL) // letzter Auftrag schlug fehl?
   		state= STATE_INTERRUPTED;
	
	switch (state) {
		case STATE_FORWARD: // Vorwaerts
		   bot_goto(100,100,data);
		   state = STATE_TURN;
		   break;
		case STATE_TURN: // Drehen
		   bot_goto(22,-22,data);
		   state=STATE_FORWARD;
		   break;		
		case STATE_INTERRUPTED:
			return_from_behaviour(data);	// Beleidigt sein und sich selbst deaktiviern			
			break;   
		   
		default:		/* Sind wir fertig, dann Kontrolle zurueck an Aufrufer */
			return_from_behaviour(data);
			break;
	}
}


/*! Uebergabevariable fuer den Dummy */
static int16 dummy_light=0; 


/*!
 * Beispiel fuer ein Hilfsverhalten, 
 * das selbst SpeedWishes aussert und 
 * nach getaner Arbeit die Aufrufende Funktion wieder aktiviert
 * @param *data der Verhaltensdatensatz
 * @see bot_drive()
 */
void bot_dummy(Behaviour_t *data){
	#define STATE_DUMMY_INIT 0
	#define STATE_DUMMY_WORK 1
	#define STATE_DUMMY_DONE 2
	static uint8 state = 0;

	switch	(state) {
		case STATE_DUMMY_INIT: 
			// Nichts zu tun
			state=STATE_DUMMY_WORK;
			break;
		case STATE_DUMMY_WORK: 
			speedWishLeft = BOT_SPEED_FAST;
			speedWishRight = BOT_SPEED_FAST; 
			if (sensLDRL< dummy_light)	// Beispielbedingung
				state=STATE_DUMMY_DONE;		
			break;
			
		case STATE_DUMMY_DONE:		/* Sind wir fertig, dann Kontrolle zurueck an Aufrufer */
			return_from_behaviour(data);
	}
}

/*!
 * Rufe das Dummy-Verhalten auf und uebergebe light
 * @param light Uebergabeparameter
 */
void bot_call_dummy(int16 light, Behaviour_t * caller){
	dummy_light=light;

	// Zielwerte speichern
	switch_to_behaviour(caller,bot_dummy,OVERRIDE);	
}







/*! 
 * Das basisverhalten Grundverhalten 
 * @param *data der Verhaltensdatensatz
 */
void bot_base(Behaviour_t *data){
	speedWishLeft=target_speed_l;
	speedWishRight=target_speed_r;
}

/*!
 * Verhindert, dass der Bot an der Wand hängenbleibt.
 * Der Bot ändert periodisch seine Richtung nach links und rechts
 * um den Erfassungsbereich der Sensoren zu vergrößern.
 * (Er fährt also einen leichten Schlangenlinienkurs)
 * @param *data der Verhaltensdatensatz
 */
void bot_glance(Behaviour_t *data){
	static int16 glance_counter = 0;  // Zähler für die periodischen Bewegungsimpulse

	glance_counter++;
	glance_counter %= (GLANCE_STRAIGHT + 4*GLANCE_SIDE);

	if (glance_counter >= GLANCE_STRAIGHT){						/* wir fangen erst an, wenn GLANCE_STRAIGHT erreicht ist */
		if 	(glance_counter < GLANCE_STRAIGHT+GLANCE_SIDE){		//GLANCE_SIDE Zyklen nach links
			faktorWishLeft = GLANCE_FACTOR; 
			faktorWishRight = 1.0;			
		} else if (glance_counter < GLANCE_STRAIGHT+ 3*GLANCE_SIDE){	//2*GLANCE_SIDE Zyklen nach rechts
			faktorWishLeft = 1.0; // glance right
			faktorWishRight = GLANCE_FACTOR;
		} else{														//GLANCE_SIDE Zyklen nach links
			faktorWishLeft = GLANCE_FACTOR; 
			faktorWishRight = 1.0;			
		}
	}
}


/*!
 * Kuemmert sich intern um die Ausfuehrung der goto-Kommandos,
 * @param *data der Verhaltensdatensatz
 * @see bot_goto()
 */
void bot_goto_system(Behaviour_t *data){
	static int16 mot_goto_l=0;	/*!< Muss der linke Motor noch drehen?  */
	static int16 mot_goto_r=0;	/*!< Muss der rechte Motor noch drehen?  */

  	int diff_l;	/* Restdistanz links */
	int diff_r; /* Restdistanz rechts */

	/* Sind beide Zähler Null und die Funktion active 
	 * -- sonst wären wir nicht hier -- 
	 * so ist es der erste Aufruf ==> initialisieren */	
	if (( mot_goto_l ==0) && ( mot_goto_r ==0)){
		/* Zähler einstellen */
		if (mot_l_goto !=0) 
			mot_goto_l= MOT_GOTO_MAX; 
		if (mot_r_goto !=0) 
			mot_goto_r=MOT_GOTO_MAX;

		/* Encoder zuruecksetzen */
		sensEncL=0;
		sensEncR=0;			
	}		
	
	/* Motor L hat noch keine MOT_GOTO_MAX Nulldurchgaenge gehabt */
	if (mot_goto_l >0){
		diff_l = sensEncL - mot_l_goto;		/* Restdistanz links */

		if (abs(diff_l) <= GOTO_REACHED){				/* 2 Encoderstaende Genauigkeit reicht */
			speedWishLeft = BOT_SPEED_STOP;	/* Stop */
			mot_goto_l--;					/* wie Nulldurchgang behandeln */
		}else if (abs(diff_l) < GOTO_SLOW)
			speedWishLeft= BOT_SPEED_SLOW;
		else if (abs(diff_l) < GOTO_NORMAL)
			speedWishLeft= BOT_SPEED_NORMAL;
		else if (abs(diff_l) < GOTO_FAST)
			speedWishLeft= BOT_SPEED_FAST;
		else speedWishLeft= BOT_SPEED_MAX;

		// Richtung	
		if (diff_l>0) {		// Wenn uebersteuert,
			speedWishLeft= -speedWishLeft;	//Richtung umkehren
		}
		
		// Wenn neue Richtung ungleich alter Richtung
		if (((speedWishLeft<0)&& (direction.left == DIRECTION_FORWARD))|| ( (speedWishLeft>0) && (direction.left == DIRECTION_BACKWARD) ) ) 
			mot_goto_l--;		// Nulldurchgang merken
	} 
	
	// Motor R hat noch keine MOT_GOTO_MAX Nulldurchgaenge gehabt
	if (mot_goto_r >0){
		diff_r = sensEncR - mot_r_goto;	/* Restdistanz rechts */

		if (abs(diff_r) <= GOTO_REACHED){			// 2 Encoderstaende Genauigkeit reicht
			speedWishRight = BOT_SPEED_STOP;	//Stop
			mot_goto_r--;			// wie Nulldurchgang behandeln
		}else if (abs(diff_r) < GOTO_SLOW)
			speedWishRight= BOT_SPEED_SLOW;
		else if (abs(diff_r) < GOTO_NORMAL)
			speedWishRight= BOT_SPEED_NORMAL;
		else if (abs(diff_r) < GOTO_FAST)
			speedWishRight= BOT_SPEED_FAST;
		else speedWishRight= BOT_SPEED_MAX;

		// Richtung	
		if (diff_r>0) {		// Wenn uebersteurt,
			speedWishRight= -speedWishRight;	//Richtung umkehren
		}

		// Wenn neue Richtung ungleich alter Richtung
		if (((speedWishRight<0)&& (direction.right == DIRECTION_FORWARD))|| ( (speedWishRight>0) && (direction.right == DIRECTION_BACKWARD) ) ) 
			mot_goto_r--;		// Nulldurchgang merken
	} 
	
	/* Sind wir fertig, dann Regel deaktivieren */
	if ((mot_goto_l == 0) && (mot_goto_r == 0)){
		return_from_behaviour(data);
	}
}

/*!
 * TODO: Diese Funktion ist nur ein Dummy-Beispiel, wie eine Kollisionsvermeidung aussehen
 * koennte. Hier ist ein guter Einstiegspunkt fuer eigene Experimente und Algorithmen!
 * Passt auf, dass keine Kollision mit Hindernissen an der Front des Roboters  
 * geschieht.
 * @param *data der Verhaltensdatensatz
 */ 
void bot_avoid_col(Behaviour_t *data){	
	if (sensDistR < COL_CLOSEST)	/* sehr nah */
		col_zone_r=ZONE_CLOSEST;	/* dann auf jeden Fall CLOSEST Zone */
	else 
	/* sind wir naeher als NEAR und nicht in der inneren Zone gewesen */
	if ((sensDistR < COL_NEAR) && (col_zone_r > ZONE_CLOSEST))
		col_zone_r=ZONE_NEAR;	/* dann auf in die NEAR-Zone */
	else
	/* sind wir naeher als FAR und nicht in der NEAR-Zone gewesen */
	if ((sensDistR < COL_FAR) && (col_zone_r > ZONE_NEAR))
		col_zone_r=ZONE_FAR;	/* dann auf in die FAR-Zone */
	else
	/* wir waren in einer engeren Zone und verlassen sie in Richtung NEAR */
	if (sensDistR < (COL_NEAR * 0.50))
		col_zone_r=ZONE_NEAR;	/* dann auf in die NEAR-Zone */
	else
	if (sensDistR < (COL_FAR * 0.50))
		col_zone_r=ZONE_FAR;	/* dann auf in die NEAR-Zone */
	else
		col_zone_r=ZONE_CLEAR;	/* dann auf in die CLEAR-Zone */
	
	if (sensDistL < COL_CLOSEST)	/* sehr nah */
		col_zone_l=ZONE_CLOSEST;	/* dann auf jeden Fall CLOSEST-Zone */
	else 
	/* sind wir naeher als NEAR und nicht in der inneren Zone gewesen */
	if ((sensDistL < COL_NEAR) && (col_zone_l > ZONE_CLOSEST))
		col_zone_l=ZONE_NEAR;	/* dann auf in die NEAR-Zone */
	else
	/* sind wir naeher als FAR und nicht in der NEAR-Zone gewesen */
	if ((sensDistL < COL_FAR) && (col_zone_l > ZONE_NEAR))
		col_zone_l=ZONE_FAR;	/* dann auf in die FAR-Zone */
	else
	/* wir waren in einer engeren Zone und verlassen sie in Richtung NEAR */
	if (sensDistL < (COL_NEAR * 0.50))
		col_zone_l=ZONE_NEAR;	/* dann auf in die NEAR-Zone */
	else
	if (sensDistL < (COL_FAR * 0.50))
		col_zone_l=ZONE_FAR;	/* dann auf in die NEAR-Zone */
	else
		col_zone_l=ZONE_CLEAR;	/* dann auf in die CLEAR-Zone */
		
	switch (col_zone_l){
		case ZONE_CLOSEST:
			faktorWishRight = BRAKE_CLOSEST;
			break;
		case ZONE_NEAR:
			faktorWishRight =  BRAKE_NEAR;
			break;
		case ZONE_FAR:
			faktorWishRight =  BRAKE_FAR;
			break;
		case ZONE_CLEAR:
			faktorWishRight = 1;
			break;
		default:
			col_zone_l = ZONE_CLEAR;
			break;
	}
		
	switch (col_zone_r){
		case ZONE_CLOSEST:
			faktorWishLeft = BRAKE_CLOSEST;
			break;
		case ZONE_NEAR:
			faktorWishLeft = BRAKE_NEAR;
			break;
		case ZONE_FAR:
			faktorWishLeft = BRAKE_FAR;
			break;
		case ZONE_CLEAR:
			faktorWishLeft = 1;
			break;
		default:
			col_zone_r = ZONE_CLEAR;
			break;
	}	
	
	if ((col_zone_r == ZONE_CLOSEST)&&(col_zone_l == ZONE_CLOSEST)){
		speedWishLeft = -target_speed_l + BOT_SPEED_MAX;
		speedWishRight = -target_speed_r - BOT_SPEED_MAX;
	}
}

/*!
 * Verhindert, dass der Bot in Graeben faellt
 * @param *data der Verhaltensdatensatz
 */
void bot_avoid_border(Behaviour_t *data){
	if (sensBorderL > BORDER_DANGEROUS)
		speedWishLeft=-BOT_SPEED_NORMAL;
	
	if (sensBorderR > BORDER_DANGEROUS)
		speedWishRight=-BOT_SPEED_NORMAL;
}



int8 check_for_light(void){
	if(sensLDRL >= 1023 && sensLDRR >= 1023) return False;
	else return True;	
}

/* @brief Die Funktion gibt aus, ob sich innerhalb einer gewissen Entfernung ein Objekt-Hinderniss befindet.
 * @param distance Entfernung in mm, bis zu welcher ein Objekt gesichtet wird. 
 * @return Gibt False (0) zurueck, wenn kein Objekt innerhalb von distance gesichtet wird. Ansonsten die Differenz 
 * zwischen dem linken und rechten Sensor. Negative Werte besagen, dass das Objekt naeher am linken, positive, dass 
 * es naeher am rechten Sensor ist. Sollten beide Sensoren den gleichen Wert haben, gibt die Funktion 1 zurueck, um
 * von False unterscheiden zu koennen. */
int16 is_obstacle_ahead(int16 distance){
	if(sensDistL > distance && sensDistR > distance) return False;
	if(sensDistL - sensDistR == 0) return 1;
	else return (sensDistL - sensDistR);
}

int8 is_good_pillar_ahead(void){
	if(is_obstacle_ahead(COL_NEAR) != False && sensLDRL < 600 && sensLDRR < 600) return True;
	else return False;	
}

/*
 * Das Verhalten verhindert, dass dem Bot boese Dinge wie Kollisionen oder Abstuerze widerfahren.
 * @return Bestand Handlungsbedarf? True, wenn das Verhalten etwas ausweichen musste, sonst False.
 * TODO: Parameter einfuegen, der dem Verhalten vorschlaegt, wie zu reagieren ist.
 * */
int bot_avoid_harm(void){
	if(is_obstacle_ahead(COL_CLOSEST) != False || sensBorderL > BORDER_DANGEROUS || sensBorderR > BORDER_DANGEROUS){
		speedWishLeft = -BOT_SPEED_NORMAL;
		speedWishRight = -BOT_SPEED_NORMAL;
		return True;
	} else return False;
}

/*
 * Das Verhalten laesst den Bot in eine Richtung fahren. Dabei stellt es sicher, dass der Bot nicht gegen ein Hinderniss prallt oder abstuerzt.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll. */
void bot_drive(int curve, int speed){
	// Wenn etwas ausgewichen wurde, bricht das Verhalten hier ab, sonst wuerde es evtl. die Handlungsanweisungen von bot_avoid_harm() stoeren.
	if(bot_avoid_harm()) return;
	if(curve < 0 && curve >= -127) {
		speedWishLeft = speed * (1.0 + 2.0*((float)curve/127));
		speedWishRight = speed;
	} else if (curve > 0 && curve <= 127) {
		speedWishRight = speed * (1.0 - 2.0*((float)curve/127));
		speedWishLeft = speed;
	} else {
		speedWishLeft = speed;
		speedWishRight = speed;	
	}
}




/*
 * @brief Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll.
 * @param cm Gibt an, wie weit der Bot fahren soll. In cm :-)
 * */
int bot_drive_distance(int curve, int speed, int cm){
	static int to_drive = 0;
	static int running = False;
	
	// Initialisierung
	if(running == False && cm > 0){
		// Umrechnung von cm in Encoder-Markierungen.
		to_drive = cm * 10 * ENCODER_MARKS / WHEEL_PERIMETER;
		sensEncL = 0;
		sensEncR = 0;
		running = True;
	}
	/* Abzug der gefahrenen Markierungen. Es wird die jeweils groessere Encoder abgezogen.
	 * Alternativ kann auch geprueft werden, in welche Richtung die Fahrt geht und welches Rad dementsprechend
	 * die laengere Strecke faehrt.
	 * TODO: Eine Funktion schreiben, die die gefahrene Strecke auch vom Maussensor abliest. 
	 * */
	if(speed > 0) to_drive -= (sensEncL > sensEncR) ? sensEncL : sensEncR;
	else to_drive -= (sensEncL < sensEncR) ? abs(sensEncL) : abs(sensEncR);
	sensEncL = 0;
	sensEncR = 0;
	
	if(to_drive <= 0) {
		running = False;
		return BOT_BEHAVIOUR_DONE;
	}	
	/* Abschaetzen, wie schnell der Bot fahren darf, damit er nicht weit ueber das Ziel hinaus schiesst.
	 * Bei einer maximalen Geschwindigkeit von 151 U/min dreht sich das Rad in 10ms um 0.025 Umdrehungen.
	 * Das bedeutet, dass der Bot bis kurz vor sein Ziel mit voller Geschwindigkeit fahren kann.
	 */
	 
	 if((speed > BOT_SPEED_SLOW || speed < -BOT_SPEED_SLOW) && to_drive < (0.1 * ENCODER_MARKS)) bot_drive(curve, speed / 2);
	 else bot_drive(curve, speed);
	 
	 return BOT_BEHAVIOUR_RUNNING;
}

/* @brief Dreht den Bot im mathematisch positiven Sinn. 
 * @param degrees Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * @return BEHAVIOUR_RUNNING, wenn der Verhalten weiterlaufen soll, sonst BEHAVIOUR_DONE
 * */
int bot_turn(int degrees){
	static int to_turn = 0;
	static int running = False;
	
	// Initialisierung
	if(running == False && degrees != 0){
		/* Umrechnung von Grad in Encoder-Markierungen.
		 * Hinweis: Eigentlich muessten der Umgang von Bot und Rad verwendet werden. Die Rechnung wird
		 * allerdings viel einfacher, wenn man Pi auskuerzt. 
		 */
		to_turn = abs((WHEEL_TO_WHEEL_DIAMETER * degrees * ENCODER_MARKS) / (360 * WHEEL_DIAMETER));
		sensEncL = 0;
		sensEncR = 0;
		running = True;
	}
	/* Abzug der gefahrenen Markierungen. Dreht sich der Bot mathematisch positiv, dreht das linke Rad rueckwaerts und das rechte vorwaerts.
	 * Da to_turn absolute Werte enthaelt, muss immer abgezogen werden.
	 * */
	if(degrees > 0) to_turn -= sensEncR;
	else to_turn -= sensEncL;
	sensEncL = 0;
	sensEncR = 0;
	
	if(to_turn <= 0){
		running = False;
		return BOT_BEHAVIOUR_DONE;
	}
	
	/* Abschaetzen, wie schnell der Bot drehen darf, damit er nicht weit ueber das Ziel hinaus schiesst.
	 * Bei einer maximalen Geschwindigkeit von 151 U/min dreht sich das Rad in 10ms um 0.025 Umdrehungen.
	 * Das bedeutet, dass der Bot bis kurz vor sein Ziel mit voller Geschwindigkeit drehen kann.*/
	
	if(to_turn < (0.1 * ENCODER_MARKS)) {
		speedWishLeft = (degrees > 0) ? -BOT_SPEED_NORMAL : BOT_SPEED_NORMAL;
		speedWishRight = (degrees > 0) ? BOT_SPEED_NORMAL : -BOT_SPEED_NORMAL;
	} else {
		speedWishLeft = (degrees > 0) ? -BOT_SPEED_FAST : BOT_SPEED_FAST;
		speedWishRight = (degrees > 0) ? BOT_SPEED_FAST : -BOT_SPEED_FAST;
	}
	return BOT_BEHAVIOUR_RUNNING;
}


/*
 * @brief Das Verhalten laesst den Roboter den Raum durchsuchen. 
 * Das Verhalten hat mehrere unterschiedlich Zustaende:
 * 1. Zu einer Wand oder einem anderen Hinderniss fahren.
 * 2. Zu einer Seite drehen, bis der Bot parallel zur Wand ist. 
 * Es macht vielleicht Sinn, den Maussensor auszulesen, um eine Drehung um 
 * einen bestimmten Winkel zu realisieren. Allerdings muesste dafuer auch der
 * Winkel des Bots zur Wand bekannt sein.
 * 3. Eine feste Strecke parallel zur Wand vorwaerts fahren.
 * Da bot_glance abwechselnd zu beiden Seiten schaut, ist es fuer die Aufgabe, 
 * einer Wand auf einer Seite des Bots zu folgen, nur bedingt gewachsen und muss
 * evtl. erweitert werden.
 * 4. Senkrecht zur Wand drehen.
 * Siehe 2.
 * 5. Einen Bogen fahren, bis der Bot wieder auf ein Hinderniss st��t. 
 * Dann das Ganze von vorne beginnen nur in die andere Richtung und mit einem
 * weiteren Bogen. So erforscht der Bot einigerma�en systematisch den Raum.
 * 
 * Da das Verhalten jeweils nach 10ms neu aufgerufen wird, muss der Bot sich
 * 'merken', in welchem Zustand er sich gerade befindet.
 * */
void bot_explore(void){
	static int8 curve = 0,state = EXPLORATION_STATE_GOTO_WALL, running_curve = False;
	
	switch(state){
	// Volle Fahrt voraus, bis ein Hinderniss erreicht ist.
	case EXPLORATION_STATE_GOTO_WALL:
		// Der Bot steht jetzt vor einem Hinderniss und soll sich nach rechts drehen
		if(bot_avoid_harm()) {
			state = EXPLORATION_STATE_TURN_PARALLEL_RIGHT;
		}
		// Es ist kein Hinderniss direkt vor dem Bot.
		else {
			if(sensDistL < COL_NEAR || sensDistR < COL_NEAR){
				bot_drive(0,BOT_SPEED_FAST);
			} else {
				bot_drive(0,BOT_SPEED_MAX);
			}
		}
		break;
	// Nach links drehen, bis der Bot parallel zum Hinderniss auf der rechten Seite steht.
	/* Aufgabe: Entwickle ein Verhalten, dass auch bei Loechern funktioniert. 
	 * Tipp dazu: Drehe den Roboter auf das Loch zu, bis beide Bodensensoren das Loch 'sehen'. Anschlie�end drehe den Bot um 90�.
	 * Es ist noetig, neue Zustaende zu definieren, die diese Zwischenschritte beschreiben. 
	 * TODO: Drehung mit dem Maussensor ueberwachen. */
	case EXPLORATION_STATE_TURN_PARALLEL_LEFT:
		if(sensDistR < COL_FAR){
			// Volle Drehung nach Links mit ca. 3�/10ms
			bot_drive(-127,BOT_SPEED_FAST);
		} else {
			//Nachdem das Hinderniss nicht mehr in Sicht ist, dreht der Bot noch ca. 3� weiter.
			// Im Zweifelsfall dreht das den Bot zu weit, aber das ist besser, als ihn zu kurz zu drehen.
			bot_drive(-127,BOT_SPEED_FAST);
			state = EXPLORATION_STATE_DRIVE_PARALLEL_RIGHT;
		}
		break;
	// Nach rechts drehen, bis der Bot parallel zum Hinderniss auf der linken Seite steht.
	/* Aufgabe: siehe EXPLORATION_STATE_TURN_PARALLEL_LEFT */
	case EXPLORATION_STATE_TURN_PARALLEL_RIGHT:
		if(sensDistL < COL_FAR){
			// Volle Drehung nach Rechts mit ca. 3�/10ms
			bot_drive(127,BOT_SPEED_FAST);
		} else {
			/* Nachdem das Hinderniss nicht mehr in Sicht ist, dreht der Bot noch ca. 3� weiter.
			 * Im Zweifelsfall dreht das den Bot zu weit, aber das ist besser, als ihn zu kurz zu drehen. */
			bot_drive(127,BOT_SPEED_FAST);
			state = EXPLORATION_STATE_DRIVE_PARALLEL_LEFT;
		}
		break;
	case EXPLORATION_STATE_DRIVE_PARALLEL_LEFT:
		if(bot_drive_distance(0,BOT_SPEED_FAST,15) == BOT_BEHAVIOUR_DONE ){
			 state = EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT;
		}
		break;
	case EXPLORATION_STATE_DRIVE_PARALLEL_RIGHT:
		if(bot_drive_distance(0,BOT_SPEED_FAST,15) == BOT_BEHAVIOUR_DONE){
			 state = EXPLORATION_STATE_TURN_ORTHOGONAL_LEFT;
		}
		break;
	case EXPLORATION_STATE_TURN_ORTHOGONAL_LEFT:
		// drehe den Bot um 90� nach links
		/* Da der Bot sich immer ein bisschen zu weit von der Wand weg dreht, soll er sich
		 * hier nur um 85� drehen. Nicht schoen, aber klappt.*/
		if(bot_turn(85) == BOT_BEHAVIOUR_DONE) {
			state = EXPLORATION_STATE_DRIVE_ARC;
		}
		break;
	case EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT:
		// drehe den Bot um 90� nach links
		/* Da der Bot sich immer ein bisschen zu weit von der Wand weg dreht, soll er sich
		 * hier nur um 85� drehen. Nicht schoen, aber klappt.*/
		if(bot_turn(-85) == BOT_BEHAVIOUR_DONE) {
			state = EXPLORATION_STATE_DRIVE_ARC;
		}
		break;
	case EXPLORATION_STATE_DRIVE_ARC:
		/* Fahre einen Bogen
		 * Der Bot soll im Wechsel Links und Rechtsboegen fahren. Daher muss das Vorzeichen von curve wechseln.
		 * Ausserdem soll der Bogen zunehmend weiter werden, so dass der absolute Wert von curve abnehmen muss.
		 * Ist der Wert 0 wird er auf den engsten Bogen initialisiert. Da der Bot am Anfang nach Rechts abbiegt,
		 * muss der Wert positiv sein. 
		 * Aufgabe: Manchmal kann es passieren, dass der Bot bei einer kleinen Kurve zu weit weg von der Wand
		 * startet und dann nurnoch im Kreis faehrt. Unterbinde dieses Verhalten.
		 * */
		if(curve == 0){
			curve = 25;
			running_curve = True;
		} else if (running_curve == False){
			curve *= -0.9;
			running_curve = True;
		}
		/* Sobald der Bot auf ein Hinderniss stoesst, wird der naechste Zyklus eingeleitet.
		 * Auf einen Rechtsbogen (curve > 0) folgt eine Linksdrehung und auf einen Linksbogen eine Rechtsdrehung.
		 * Wenn der Wert von curve (durch Rundungsfehler bei int) auf 0 faellt, beginnt das Suchverhalten erneut.*/
		if(bot_avoid_harm()) {
			state = (curve > 0) ? EXPLORATION_STATE_TURN_PARALLEL_LEFT : EXPLORATION_STATE_TURN_PARALLEL_RIGHT;
			running_curve = False;
		} else {
			bot_drive(curve, BOT_SPEED_MAX);
		}
		break;
	default:
		state = EXPLORATION_STATE_GOTO_WALL;
		curve = 0;
	}
	
}

/*
 * @brief Das Verhalten dreht den Bot so, dass er auf eine Lichtquelle zufaehrt. */
void bot_goto_light(void){
	int16 speed, curve = (sensLDRL - sensLDRR)/2;

	if(curve < -127) curve = -127;
	if(curve > 127) curve = 127;

	if(abs(sensLDRL - sensLDRR) < 20){
		speed = BOT_SPEED_MAX;
	}else if(abs(sensLDRL - sensLDRR) < 150) {
		speed = BOT_SPEED_FAST;
	}else {
		speed = BOT_SPEED_NORMAL;
	}
	
	bot_drive(curve, speed);
}

/* @brief Das Verhalten laesst den Bot zwischen einer Reihe beleuchteter Saeulen Slalom fahren. 
 * Das Verhalten ist wie bot_explore() in eine Anzahl von Teilschritten unterteilt.
 * 1. Vor die aktuelle Saeule stellen, so dass sie zentral vor dem Bot und ungefaehr 
 * COL_CLOSEST (100mm) entfernt ist.
 * 2. 90� nach rechts drehen.
 * 3. In einem relativ engen Bogen 20 cm weit fahren.
 * 4. Auf der rechten Seite des Bot nach einem Objekt suchen, dass
 * 	a) im rechten Sektor des Bot liegt, also zwischen -45� und -135� zur Fahrtrichtung liegt,
 * 	b) beleuchtet und 
 * 	c) nicht zu weit entfernt ist.
 * Wenn es dieses Objekt gibt, wird es zur aktuellen Saeule und der Bot faehrt jetzt Slalom links.
 * 5. Sonst zurueck drehen, 90� drehen und Slalom rechts fahren.
 * In diesem Schritt kann der Bot das Verhalten auch abbrechen, falls er gar kein Objekt mehr findet.
 * */
void bot_do_slalom(int8 *cb_state){
	static int8 state = SLALOM_STATE_CHECK_PILLAR;
	static int8 orientation = SLALOM_ORIENTATION_RIGHT;
	static int8 sweep_state;
	static int8 sweep_steps = 0;
	int16 turn;
	int8 curve;
	
	switch(state){
	case SLALOM_STATE_CHECK_PILLAR:
		// Der Bot sollte jetzt Licht sehen koennen...
		if(check_for_light()){
			// Wenn der Bot direkt vor der Saeule steht, kann er anfangen, sonst zum Licht fahren
			if(bot_avoid_harm()){
				state = SLALOM_STATE_START;
			} else bot_goto_light();
		} // ... sonst muss er den Slalom-Kurs neu suchen. 
		else *cb_state = CB_STATE_EXPLORATION;
		break;
	case SLALOM_STATE_START:
		// Hier ist Platz fuer weitere Vorbereitungen, falls noetig.
		state = SLALOM_STATE_TURN_1;
		// break;
	case SLALOM_STATE_TURN_1:
		turn = (orientation == SLALOM_ORIENTATION_LEFT) ? 90 : -90;
		if(bot_turn(turn) == BOT_BEHAVIOUR_DONE) {
			state = SLALOM_STATE_DRIVE_ARC;
		}
		break;
	case SLALOM_STATE_DRIVE_ARC:
		// Nicht wundern: Bei einem Links-Slalom faehrt der Bot eine Rechtskurve.
		curve = (orientation == SLALOM_ORIENTATION_LEFT) ? 25 : -25;
		if(bot_drive_distance(curve,BOT_SPEED_FAST,20) == BOT_BEHAVIOUR_DONE){
			state = SLALOM_STATE_TURN_2;
		}
		break;
	case SLALOM_STATE_TURN_2:
		turn = (orientation == SLALOM_ORIENTATION_LEFT) ? 45 : -45;
		if(bot_turn(turn) == BOT_BEHAVIOUR_DONE) {
			state = SLALOM_STATE_SWEEP_RUNNING;
		}
		break;
	case SLALOM_STATE_SWEEP_RUNNING:
		if(sweep_steps == 0){
			sweep_state = SWEEP_STATE_CHECK;	
		}
		// Insgesamt 3 Schritte drehen
		if(sweep_steps < 6) {
			if(sweep_state == SWEEP_STATE_CHECK){
			// Phase 1: Pruefen, ob vor dem Bot eine gute Saeule ist
				if(is_good_pillar_ahead() == True){
				// Wenn die Saeule gut ist, drauf zu und Slalom anders rum fahren.
					state = SLALOM_STATE_CHECK_PILLAR;
					orientation = (orientation == SLALOM_ORIENTATION_LEFT) ? SLALOM_ORIENTATION_RIGHT : SLALOM_ORIENTATION_LEFT;
					sweep_steps = 0;
				} else {
					// Sonst drehen.
					sweep_state = SWEEP_STATE_TURN;	
				}
			}
			if(sweep_state == SWEEP_STATE_TURN) {
			// Phase 2: Bot um 10� drehen
				turn = (orientation == SLALOM_ORIENTATION_LEFT) ? 15 : -15;
				if(bot_turn(turn) == BOT_BEHAVIOUR_DONE){
					sweep_state = SWEEP_STATE_CHECK;
					sweep_steps++;
				}
			}
		} else {
			turn = (orientation == SLALOM_ORIENTATION_LEFT) ? -90 : 90;
			if(bot_turn(turn) == BOT_BEHAVIOUR_DONE) {
				state = SLALOM_STATE_SWEEP_DONE;
				sweep_steps = 0;
			}
		}
		break;
	case SLALOM_STATE_SWEEP_DONE:
		turn = (orientation == SLALOM_ORIENTATION_LEFT) ? -135 : 135;
		if(bot_turn(turn) == BOT_BEHAVIOUR_DONE) {
			state = SLALOM_STATE_CHECK_PILLAR;
		}
		break;
	default:
		state = SLALOM_STATE_CHECK_PILLAR;
	}
}

/*
 * Das Verhalten setzt sich aus 3 Teilverhalten zusammen: 
 * Nach Licht suchen, auf das Licht zufahren, im Licht Slalom fahren. */
void bot_complex_behaviour(Behaviour_t *data){
	static int8 state = CB_STATE_EXPLORATION;
	switch(state){
	case CB_STATE_EXPLORATION:
		/* Sobald der Bot Licht sieht, faehrt er darauf zu. 
		 * Sonst sucht er die Umgebung ab.*/
		if(check_for_light()){
			/* Sobald der Bot auf ein Objekt-Hinderniss stoesst, versucht er, Slalom zu fahren.
			 * Aufgabe: Wenn der Bot vor einem Loch steht, hinter welchem sich die Lichtquelle 
			 * befindet, wird er daran haengen bleiben. Schreibe ein Verhalten, dass das verhindert. */
			if(bot_avoid_harm() && is_obstacle_ahead(COL_NEAR)){
				state = CB_STATE_DOING_SLALOM;
			} else bot_goto_light();
		} else bot_explore();
		break;
	case CB_STATE_DOING_SLALOM:
		bot_do_slalom(&state);
		break;
	default:
		state = CB_STATE_EXPLORATION;	
	}
}

/*! 
 * Zentrale Verhaltens-Routine, 
 * wird regelmaessig aufgerufen. 
 * Dies ist der richtige Platz fuer eigene Routinen, 
 * um den Bot zu steuern
 */
void bot_behave(void){	
	Behaviour_t *job;						// Zeiger auf ein Verhalten
	
	float faktorLeft = 1.0;					// Puffer für Modifkatoren
	float faktorRight = 1.0;				// Puffer für Modifkatoren
	
	#ifdef RC5_AVAILABLE
		rc5_control();						// Abfrage der IR-Fernbedienung
	#endif

	/* Solange noch Verhalten in der Liste sind
	   Achtung wir werten die Jobs sortiert nach Prioritaet. Wichtige zuerst!!! */
	for (job = behaviour; job; job = job->next) {
		if (job->active) {
			/* WunschVariablen initialisieren */
			speedWishLeft = BOT_SPEED_IGNORE;
			speedWishRight = BOT_SPEED_IGNORE;
			
			faktorWishLeft = 1.0;
			faktorWishRight = 1.0;
			
			job->work(job);	/* Verhalten ausfuehren */

			/* Modifikatoren sammeln  */
			faktorLeft  *= faktorWishLeft;
			faktorRight *= faktorWishRight;

			/* Geschwindigkeit aendern? */
			if ((speedWishLeft != BOT_SPEED_IGNORE) || (speedWishRight != BOT_SPEED_IGNORE)){
				if (speedWishLeft != BOT_SPEED_IGNORE)
					speedWishLeft *= faktorLeft;
				if (speedWishRight != BOT_SPEED_IGNORE)
					speedWishRight *= faktorRight;
					
				motor_set(speedWishLeft, speedWishRight);
				break;						/* Wenn ein Verhalten Werte direkt setzen will, nicht weitermachen */
			}
			
		}
		/* Dieser Punkt wird nur erreicht, wenn keine Regel im System die Motoren beeinflusen will */
		if (job->next == NULL) {
				motor_set(BOT_SPEED_IGNORE, BOT_SPEED_IGNORE);
		}
	}
}

/*! 
 * Erzeugt ein neues Verhalten 
 * @param priority Die Prioritaet
 * @param *work Den Namen der Funktion, die sich drum kuemmert
 */
Behaviour_t *new_behaviour(char priority, void (*work) (struct _Behaviour_t *data)){
	Behaviour_t *newbehaviour = (Behaviour_t *) malloc(sizeof(Behaviour_t)); 
	
	if (newbehaviour == NULL) 
		return NULL;
	
	newbehaviour->priority = priority;
	newbehaviour->active=1;
//	newbehaviour->ignore=0;
	newbehaviour->next= NULL;
	newbehaviour->work=work;
	newbehaviour->caller=NULL;
	newbehaviour->subResult=SUBSUCCESS;
	return newbehaviour;
}

/*!
 * Fuegt ein Verhalten der Verhaltenliste anhand der Prioritaet ein.
 * @param list Die Speicherstelle an der die globale Verhaltensliste anfaengt
 * @param behave Einzufuegendes Verhalten
 */
static void insert_behaviour_to_list(Behaviour_t **list, Behaviour_t *behave){
	Behaviour_t	*ptr	= *list;
	Behaviour_t *temp	= NULL;
	
	/* Kein Eintrag dabei? */
	if (behave == NULL)
		return;
	
	/* Erster Eintrag in der Liste? */
	if (ptr == NULL){
		ptr = behave;
		*list = ptr;
	} else {
		/* Gleich mit erstem Eintrag tauschen? */
		if (ptr->priority < behave->priority) {
			behave->next = ptr;
			ptr = behave;
			*list = ptr;
		} else {
			/* Mit dem naechsten Eintrag vergleichen */
			while(NULL != ptr->next) {
				if (ptr->next->priority < behave->priority)	
					break;
				
				/* Naechster Eintrag */
				ptr = ptr->next;
			}
			
			temp = ptr->next;
			ptr->next = behave;
			behave->next = temp;
		}
	}
}

/*!
 * Initialisert das ganze Verhalten
 */
void bot_behave_init(void){

	/* Einfache Verhaltensroutine, die alles andere uebersteuert */
	insert_behaviour_to_list(&behaviour, new_behaviour(200, bot_avoid_border));

	//insert_behaviour_to_list(&behaviour, new_behaviour(100, bot_avoid_col));
	//insert_behaviour_to_list(&behaviour, new_behaviour(60, bot_glance));
	//insert_behaviour_to_list(&behaviour, new_behaviour(55, bot_complex_behaviour));

	insert_behaviour_to_list(&behaviour, new_behaviour(55, bot_drive_square));


	insert_behaviour_to_list(&behaviour, new_behaviour(50, bot_goto_system));
	deactivateBehaviour(bot_goto_system);
	
	insert_behaviour_to_list(&behaviour, new_behaviour(0, bot_base));

	#ifdef PC
		#ifdef DISPLAY_AVAILABLE
			/* Annzeigen der geladenen Verhalten  */
				Behaviour_t	*ptr	= behaviour;
	
				display_cursor(5,1);
				sprintf(display_buf,"Verhaltensstack:\n");			
				display_buffer();
				while(ptr != NULL)	{
					sprintf(display_buf,"Prioritaet: %d.\n", ptr->priority);
					display_buffer();
					ptr = ptr->next;
				}
		#endif
	#endif

	return;
}


/*!
 * Drehe die Raeder um die gegebene Zahl an Encoder-Schritten weiter
 * @param left Schritte links
 * @param right Schritte rechts
 */
void bot_goto(int16 left, int16 right, Behaviour_t * caller){
	// Zielwerte speichern
	mot_l_goto=left; 
	mot_r_goto=right;

	switch_to_behaviour(caller,bot_goto_system,OVERRIDE);	
}
