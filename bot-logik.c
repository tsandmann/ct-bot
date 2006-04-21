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
 * Jedes Verhalten kann entweder absolute Werte setzen, dann kommen niedrigerpriorisierte nicht mehr dran.
 * Alternativ dazu kann es Modifikatoren aufstellen, die bei niedriger priorisierten angewendet werden.
 * bot_behave_init() baut diese Liste auf.
 * Jede Verhaltensfunktion bekommt einen Verhaltensdatensatz uebergeben, in den Sie ihre Daten eintraegt
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author 	Christoph Grimmer (c.grimmer@futurio.de)
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
 * Alle Konstanten, die Verhaltensweisen benutzen, sind in bot-local.h ausgelagert. 
 * Dort kann man sie per .cvsignore vor updates schuetzen.
 * 
 * Alle Variablen mit Sensor-Werten findet man in sensor.h, Variablen fuer den Motor in motor.h.
 * 
 */	

uint8 col_zone_l=ZONE_CLEAR;			/*!< Kollisionszone, in der sich der linke Sensor befindet */
uint8 col_zone_r=ZONE_CLEAR;			/*!< Kollisionszone, in der sich der rechte Sensor befindet */

volatile int16 mot_l_goto=0;		/*!< Speichert wie weit der linke Motor drehen soll */
volatile int16 mot_r_goto=0;		/*!< Speichert wie weit der rechte Motor drehen soll */

int16 speedWishLeft;				/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit links*/
int16 speedWishRight;				/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit rechts*/

float faktorWishLeft;				/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor links*/
float faktorWishRight;				/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor rechts */

volatile int16 target_speed_l=BOT_SPEED_STOP;	/*!< Sollgeschwindigkeit linker Motor - darum kuemmert sich bot_base()*/
volatile int16 target_speed_r=BOT_SPEED_STOP;	/*!< Sollgeschwindigkeit rechter Motor - darum kuemmert sich bot_base() */

/* Parameter fuer das bot_explore_behaviour() */
int8 (*exploration_check_function)(void);	/*!< Die Funktion, mit der das bot_explore_behaviour() feststellt, ob es etwas gefunden hat.
											 * Die Funktion muss True (1) zurueck geben, wenn dem so ist, sonst False (0).
											 * Beispiele fuer eine solche Funktion sind check_for_light, is_good_pillar_ahead etc.*/

/* Parameter fuer das bot_drive_distance_behaviour() */
int16 drive_distance_target;	/*!< Zu fahrende Distanz bzw. angepeilter Stand der Radencoder sensEncL bzw. sensEncR */
int8 drive_distance_curve;		/*!< Kruemmung der zu fahrenden Strecke. */
int16 drive_distance_speed;		/*!< Angepeilte Geschwindigkeit. */

/* Parameter fuer das bot_turn_behaviour() */
int16 turn_targetR;				/*!< Zu drehender Winkel bzw. angepeilter Stand des Radencoders sensEncR */
int16 turn_targetL;				/*!< Zu drehender Winkel bzw. angepeilter Stand des Radencoders sensEncL */
int8 turn_direction;			/*!< Richtung der Drehung */

/* Parameter fuer das check_wall_behaviour() */
int8 wall_detected;				/*!< enthaelt True oder False, je nach Ergebnis des Verhaltens */
int8 check_direction;			/*!< enthaelt CHECK_WALL_LEFT oder CHECK_WALL_RIGHT */
int16 wall_distance;			/*!< enthaelt gemessene Entfernung */

/* Konstanten fuer check_wall_behaviour-Verhalten */
#define CHECK_WALL_RIGHT			0
#define CHECK_WALL_LEFT				1

/* Parameter fuer das measure_angle_behaviour() */
int8 measure_direction;			/*!< enthaelt MEASURE_RIGHT oder MEASURE_LEFT */
int16 measure_distance;			/*!< enthaelt maximale Messentfernung, enthaelt nach der Messung die Entfernung */
int16 measured_angle;			/*!< enthaelt gedrehten Winkel oder 0, falls nichts entdeckt */
int16 startEncL;				/*!< enthaelt Encoderstand zu Beginn der Messung */
int16 startEncR;				/*!< enthaelt Encoderstand zu Beginn der Messung */

/* Konstanten fuer measure_angle_behaviour-Verhalten */
#define MEASURE_LEFT				1
#define MEASURE_RIGHT				-1

/*! Liste mit allen Verhalten */
Behaviour_t *behaviour = NULL;


#define INACTIVE 0	/*!< Verhalten ist aus */
#define ACTIVE 1	/*!< Verhalten ist an */

#define OVERRIDE	1	/*!< Konstante, wenn Verhalten beim Aufruf alte Wuensche ueberschreiben sollen */
#define NOOVERRIDE 0	/*!< Konstanten, wenn Verhalten beim Aufruf alte Wuensche nicht ueberschreiben sollen */

#define SUBSUCCESS	1	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe erfolgreich abgeschlossen */
#define SUBFAIL	0	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe nicht abgeschlossen */
#define SUBRUNNING 2	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe wird noch beabeitet */

/*!
 * Aktiviert eine Regel mit gegebener Funktion
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void activateBehaviour(BehaviourFunc function){
	Behaviour_t *job;						// Zeiger auf ein Verhalten

	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == function) {
			job->active = ACTIVE;
			break;
		}
	}
}


/*!
 * Deaktiviert eine Regel mit gegebener Funktion
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void deactivateBehaviour(BehaviourFunc function){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
		
	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == function) {
			job->active = INACTIVE;
			break;
		}
	}
}

/*! 
 * Ruft ein anderes Verhalten auf und merkt sich den Ruecksprung 
 * return_from_behaviour() kehrt dann spaeter wieder zum aufrufenden Verhalten zurueck
 * @param from aufrufendes Verhalten
 * @param to aufgerufenes Verhalten
 * @param override Hier sind zwei Werte Moeglich:
 * 		1. OVERRIDE : Das Zielverhalten to wird aktiviert, auch wenn es noch aktiv ist. 
 * 					  Das Verhalten, das es zuletzt aufgerufen hat wird dadurch automatisch 
 * 					  wieder aktiv und muss selbst sein eigenes Feld subResult auswerten, um zu pruefen, ob das
 * 					  gewuenschte Ziel erreicht wurde, oder vorher ein Abbruch stattgefunden hat. 
 * 		2. NOOVERRIDE : Das Zielverhalten wird nur aktiviert, wenn es gerade nichts zu tun hat.
 * 						In diesem Fall kann der Aufrufer aus seinem eigenen subResult auslesen,
 * 						ob seibem Wunsch Folge geleistet wurde.
 */ 
void switch_to_behaviour(Behaviour_t * from, void *to, uint8 override ){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
	
	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == to) {
			break;		// Abbruch der Schleife, job zeigt nun auf die Datenstruktur des Zielverhaltens
		}
	}	

	if (job->caller){		// Ist das auzurufende Verhalten noch beschaeftigt?
		if (override==NOOVERRIDE){	// nicht ueberschreiben, sofortige Rueckkehr
			if (from)
				from->subResult=SUBFAIL;
			return;
		}
		// Wir wollen also ueberschreiben, aber nett zum alten Aufrufer sein und ihn darueber benachrichtigen
		job->caller->active=ACTIVE;	// alten Aufrufer reaktivieren
		job->caller->subResult=SUBFAIL;	// er bekam aber nicht das gewuenschte Resultat
	}

	if (from) {
		// laufendes Verhalten abschalten
		from->active=INACTIVE;
		from->subResult=SUBRUNNING;
	}
		
	// neues Verhalten aktivieren
	job->active=ACTIVE;
	// Aufrufer sichern
	job->caller =  from;
}

/*! 
 * Kehrt zum aufrufenden Verhalten zurueck
 * @param running laufendes Verhalten
 */ 
void return_from_behaviour(Behaviour_t * data){
	data->active=INACTIVE; 				// Unterverhalten deaktivieren
	if (data->caller){			
		data->caller->active=ACTIVE; 	// aufrufendes Verhalten aktivieren
		data->caller->subResult=SUBSUCCESS;	// Unterverhalten war erfolgreich
	}
	data->caller=NULL;				// Job erledigt, Verweis loeschen
}

/*!
 * Deaktiviert alle Verhalten bis auf Grundverhalten. Bei Verhaltensauswahl werden die Aktivitaeten vorher
 * in die Verhaltens-Auswahlvariable gesichert.
 */
void deactivateAllBehaviours(void){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
	
	#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
	  // bei Verhaltensanzeige in Aktivitaets-Auswahl-Variable sichern
	  set_behaviours_equal();
	#endif
		
	// Einmal durch die Liste gehen und (fast) alle deaktivieren, Grundverhalten nicht 
	for (job = behaviour; job; job = job->next) {
		if ((job->priority > 2) &&(job->priority <= 200)) {
            // Verhalten deaktivieren 
			job->active = INACTIVE;	
		}
	}	
}

/*! 
 * Ein ganz einfaches Verhalten, es hat maximale Prioritaet
 * Hier kann man auf ganz einfache Weise die ersten Schritte wagen. 
 * Wer die Moeglichkeiten des ganzen Verhaltensframeworks ausschoepfen will, kann diese Funktion getrost auskommentieren
 * und findet dann in bot_behave_init() und bot_behave() weitere Hinweise fuer elegante Bot-Programmierung....
 * @param *data der Verhaltensdatensatz
 */
void bot_simple_behaviour(Behaviour_t *data){
	static uint8 state=0;
	
	switch (state){
		case 0:
			bot_drive_distance(data ,0 , BOT_SPEED_MAX, 14);
			state++;
			break;
		case 1:
			bot_turn(data , 90);
			state=0;
			break;
		default:
				return_from_behaviour(data);
				break;
	}
}

/*!
 * Laesst den Roboter ein Quadrat abfahren.
 * Einfaches Beispiel fuer ein Verhalten, das einen Zustand besitzt.
 * Es greift auf andere Behaviours zurueck und setzt daher 
 * selbst keine speedWishes.
 * @param *data der Verhaltensdatensatz
 */
void bot_drive_square_behaviour(Behaviour_t *data){
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
 * nach getaner Arbeit die aufrufende Funktion wieder aktiviert
 * @param *data der Verhaltensdatensatz
 * @see bot_drive()
 */
void bot_dummy_behaviour(Behaviour_t *data){
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
void bot_dummy(int16 light, Behaviour_t * caller){
	dummy_light=light;

	// Zielwerte speichern
	switch_to_behaviour(caller,bot_dummy_behaviour,OVERRIDE);	
}

/*! 
 * Das einfachste Grundverhalten 
 * @param *data der Verhaltensdatensatz
 */
void bot_base_behaviour(Behaviour_t *data){
	speedWishLeft=target_speed_l;
	speedWishRight=target_speed_r;
}

/*!
 * Verhindert, dass der Bot an der Wand haengenbleibt.
 * Der Bot aendert periodisch seine Richtung nach links und rechts,
 * um den Erfassungsbereich der Sensoren zu vergroessern
 * (er faehrt also einen leichten Schlangenlinienkurs).
 * @param *data der Verhaltensdatensatz
 */
void bot_glance_behaviour(Behaviour_t *data){
	static int16 glance_counter = 0;  // Zaehler fuer die periodischen Bewegungsimpulse

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
void bot_goto_behaviour(Behaviour_t *data){
	static int16 mot_goto_l=0;	/*!< Muss der linke Motor noch drehen?  */
	static int16 mot_goto_r=0;	/*!< Muss der rechte Motor noch drehen?  */

  	int16 diff_l;	/* Restdistanz links */
	int16 diff_r; /* Restdistanz rechts */

	/* Sind beide Zaehler Null und ist die Funktion active 
	 * -- sonst waeren wir nicht hier -- 
	 * so ist dies der erste Aufruf ==> initialisieren */	
	if (( mot_goto_l ==0) && ( mot_goto_r ==0)){
		/* Zaehler einstellen */
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
 * Passt auf, dass keine Kollision mit Hindernissen an der Front des Roboters  
 * geschieht.
 * TODO: Diese Funktion ist nur ein Dummy-Beispiel, wie eine Kollisionsvermeidung aussehen
 * koennte. Hier ist ein guter Einstiegspunkt fuer eigene Experimente und Algorithmen!
 * @param *data der Verhaltensdatensatz
 */ 
void bot_avoid_col_behaviour(Behaviour_t *data){		
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

	// Nur reagieren, wenn der Bot vorwaerts faehrt
	if ((speed_l > 0) && (speed_r >0)) {
		// wenn auf beiden Seiten in der Kollisionszone
		if ((col_zone_r == ZONE_CLOSEST)&&(col_zone_l == ZONE_CLOSEST)){
			// Drehe Dich zur Seite, wo mehr Platz ist
			if (sensDistR < sensDistL)
				bot_turn(data,20);	
			else
				bot_turn(data,-20);	
			return;
		}
			
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
	}
}

/*!
 * Verhindert, dass der Bot in Graeben faellt
 * @param *data der Verhaltensdatensatz
 */
void bot_avoid_border_behaviour(Behaviour_t *data){
	if (sensBorderL > BORDER_DANGEROUS)
		speedWishLeft=-BOT_SPEED_NORMAL;
	
	if (sensBorderR > BORDER_DANGEROUS)
		speedWishRight=-BOT_SPEED_NORMAL;
}


/*!
 * Gibt aus, ob der Bot Licht sehen kann.
 * @return True, wenn der Bot Licht sieht, sonst False. */
int8 check_for_light(void){
	// Im Simulator kann man den Bot gut auf den kleinsten Lichtschein
	// reagieren lassen, in der Realitaet gibt es immer Streulicht, so dass
	// hier ein hoeherer Schwellwert besser erscheint.
	// Simulator:
	if(sensLDRL >= 1023 && sensLDRR >= 1023) return False;
	// Beim echten Bot eher:
	// if(sensLDRL >= 100 && sensLDRR >= 100) return False;
	
	else return True;	
}

/*!
 * Die Funktion gibt aus, ob sich innerhalb einer gewissen Entfernung ein Objekt-Hindernis befindet.
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

/*!
 * Gibt aus, ob der Bot eine fuer sein Slalomverhalten geeignete Saeule vor sich hat. 
 * @return True, wenn er eine solche Saeule vor sich hat, sonst False.*/
int8 is_good_pillar_ahead(void){
	if(is_obstacle_ahead(COL_NEAR) != False && sensLDRL < 600 && sensLDRR < 600) return True;
	else return False;	
}

/*!
 * Das Verhalten verhindert, dass dem Bot boese Dinge wie Kollisionen oder Abstuerze widerfahren.
 * @return Bestand Handlungsbedarf? True, wenn das Verhalten ausweichen musste, sonst False.
 * TODO: Parameter einfuegen, der dem Verhalten vorschlaegt, wie zu reagieren ist.
 * */
int8 bot_avoid_harm(void){
	if(is_obstacle_ahead(COL_CLOSEST) != False || sensBorderL > BORDER_DANGEROUS || sensBorderR > BORDER_DANGEROUS){
		speedWishLeft = -BOT_SPEED_NORMAL;
		speedWishRight = -BOT_SPEED_NORMAL;
		return True;
	} else return False;
}

/*!
 * Das Verhalten laesst den Bot in eine Richtung fahren. Dabei stellt es sicher, dass der Bot nicht gegen ein Hinderniss prallt oder abstuerzt.
 * Es handelt sich hierbei nicht im eigentlichen Sinn um ein Verhalten, sondern ist nur eine Abstraktion der Motorkontrollen.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll. */
void bot_drive(int8 curve, int16 speed){
	// Wenn etwas ausgewichen wurde, bricht das Verhalten hier ab, sonst wuerde es evtl. die Handlungsanweisungen von bot_avoid_harm() stoeren.
	//if(bot_avoid_harm()) return;
	if(curve < 0) {
		speedWishLeft = speed * (1.0 + 2.0*((float)curve/127));
		speedWishRight = speed;
	} else if (curve > 0) {
		speedWishRight = speed * (1.0 - 2.0*((float)curve/127));
		speedWishLeft = speed;
	} else {
		speedWishLeft = speed;
		speedWishRight = speed;	
	}
}

/*!
 * Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren.
 * @see bot_drive_distance() 
 * */

void bot_drive_distance_behaviour(Behaviour_t* data){
	int16 *encoder;
	int16 to_drive;

	if (drive_distance_curve > 0){
		// Es handelt sich um eine Rechtskurve, daher wird mit dem linken Encoder gerechnet
		encoder = (int16*)&sensEncL;
	} else {
		encoder = (int16*)&sensEncR;
	}
	
	to_drive = drive_distance_target - *encoder;
	if(drive_distance_speed < 0) to_drive = -to_drive;
	
	if(to_drive <= 0){
		return_from_behaviour(data);
	} else {
		if((drive_distance_speed > BOT_SPEED_SLOW || drive_distance_speed < -BOT_SPEED_SLOW) && to_drive < (0.1 * ENCODER_MARKS)) bot_drive(drive_distance_curve, drive_distance_speed / 2);
		else bot_drive(drive_distance_curve, drive_distance_speed);
	}	 
}



/*! 
 * Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren. Dabei legt die Geschwindigkeit fest, ob der Bot vorwaerts oder rueckwaerts fahren soll.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll. Negative Werte lassen den Bot rueckwaerts fahren.
 * @param cm Gibt an, wie weit der Bot fahren soll. In cm :-) Die Strecke muss positiv sein, die Fahrtrichtung wird ueber speed geregelt.
 * */

void bot_drive_distance(Behaviour_t* caller,int8 curve, int16 speed, int16 cm){
	int16 marks_to_drive = cm * 10 * ENCODER_MARKS / WHEEL_PERIMETER;
	int16 *encoder;
	drive_distance_curve = curve;
	drive_distance_speed = speed;

	if (curve > 0){
		// Es handelt sich um eine Rechtskurve, daher wird mit dem linken Encoder gerechnet
		encoder = (int16*)&sensEncL;
	} else {
		encoder = (int16*)&sensEncR;
	}
	if(speed < 0){
		// Es soll rueckwaerts gefahren werden. Der Zielwert ist also kleiner als der aktuelle Encoder-Stand.
		drive_distance_target = *encoder - marks_to_drive;
	} else {
		drive_distance_target = *encoder + marks_to_drive;	
	}
	switch_to_behaviour(caller, bot_drive_distance_behaviour,NOOVERRIDE);
}


/*!
 * Das Verhalten laesst den Bot eine Punktdrehung durchfuehren. 
 * @see bot_turn()
 * */
void bot_turn_behaviour(Behaviour_t* data){
	/* Drehen findet in vier Schritten statt. Die Drehung wird dabei
	 * bei Winkeln > 90 Grad zunaechst mit maximaler Geschwindigkeit ausgefuehrt. Bei kleineren
	 * Winkeln oder wenn nur noch 90 Grad zu drehen sind, nur noch mit normaler Geschwindigkeit
	 */
	 /* Zustaende fuer das bot_turn_behaviour-Verhalten */
	#define NORMAL_TURN					0
	#define SHORT_REVERSE				1
	#define CORRECT_POSITION			2
	#define FULL_STOP					3
	static int8 turnState=NORMAL_TURN;
	/* zu drehende Schritte in die korrekte Drehrichtung korrigieren */
	int16 to_turnR = turn_direction*(turn_targetR - sensEncR);
	int16 to_turnL = -turn_direction*(turn_targetL - sensEncL);

	switch(turnState) {
		case NORMAL_TURN:
			/* Solange drehen, bis beide Encoder nur noch zwei oder weniger Schritte zu fahren haben */

			if (to_turnL <= 2 && to_turnR<=2)
			{
				/* nur noch 2 Schritte oder weniger, abbremsen einleiten */
				turnState=SHORT_REVERSE;
				break;	
			}
			/* Bis 90 Grad kann mit maximaler Geschwindigkeit gefahren werden, danach auf Normal reduzieren */
			/* Geschwindigkeit fuer beide Raeder getrennt ermitteln */
			if(abs(to_turnL) < ANGLE_CONSTANT*0.25) {
				speedWishLeft = (turn_direction > 0) ? -BOT_SPEED_NORMAL : BOT_SPEED_NORMAL;
			} else {
				speedWishLeft = (turn_direction > 0) ? -BOT_SPEED_FAST : BOT_SPEED_FAST;
			}
			if(abs(to_turnR) < ANGLE_CONSTANT*0.25) {
				speedWishRight = (turn_direction > 0) ? BOT_SPEED_NORMAL : -BOT_SPEED_NORMAL;
			} else {	
				speedWishRight = (turn_direction > 0) ? BOT_SPEED_FAST : -BOT_SPEED_FAST;
			}	
			break;
			
		case SHORT_REVERSE:
			/* Ganz kurz durch umpolen anbremsen */ 
			speedWishLeft = (turn_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			speedWishRight = (turn_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			turnState=CORRECT_POSITION;
			break;
		
		case CORRECT_POSITION:
			/* Evtl. etwas zuruecksetzen, falls wir zu weit gefahren sind */
			if (to_turnR<0) {
				/* rechts zu weit gefahren..langsam zurueck */
				speedWishRight = (turn_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			} else if (to_turnR>0) {
				/* rechts noch nicht weit genug...langsam vor */
				speedWishRight = (turn_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, rechtes Rad anhalten */
				speedWishRight = BOT_SPEED_STOP;
			}		
			if (to_turnL<0) {
				/* links zu weit gefahren..langsam zurueck */
				speedWishLeft = (turn_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;				
			} else if (to_turnL>0) {
				/* links noch nicht weit genug...langsam vor */
				speedWishLeft = (turn_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, linkes Rad anhalten */
				speedWishLeft = BOT_SPEED_STOP;
			}
			if (speedWishLeft == BOT_SPEED_STOP && speedWishRight == BOT_SPEED_STOP) {
				/* beide Raeder haben nun wirklich die Endposition erreicht, daher anhalten */
				turnState=FULL_STOP;
			}
			break;
		
			
		default: 
			/* ist gleichzeitig FULL_STOP, da gleiche Aktion 
			 * Stoppen, State zuruecksetzen und Verhalten beenden */
			speedWishLeft = BOT_SPEED_STOP;
			speedWishRight = BOT_SPEED_STOP;
			turnState=NORMAL_TURN;
			return_from_behaviour(data);
			break;			
	}
}

/*! 
 * Dreht den Bot im mathematisch positiven Sinn. 
 * @param degrees Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * */
void bot_turn(Behaviour_t* caller,int16 degrees){
	/* Umrechnung von Grad in Encoder-Markierungen.
	 * Hinweis: Eigentlich muessten der Umfang von Bot und Rad verwendet werden. Die Rechnung wird
	 * allerdings viel einfacher, wenn man Pi auskuerzt.
	 * Ist degrees negativ, ist die Drehung negativ und der rechte Encoder muss kleiner werden.
	 */
	
	if(degrees < 0) turn_direction = -1;
	else turn_direction = 1;
	/* Anzahl zu fahrender Encoderschritte berechnen */
	turn_targetR=(degrees*ANGLE_CONSTANT)/360;
	/* linkes Rad dreht entgegengesetzt, daher negativer Wert */
 	turn_targetL=-turn_targetR;

 	/* aktuellen Sensorwert zu zu drehenden Encoderschritten addieren */
 	turn_targetR+=sensEncR;
 	turn_targetL+=sensEncL;
	switch_to_behaviour(caller, bot_turn_behaviour,NOOVERRIDE);
}

/*!
 * Das Verhalten laesst den Roboter den Raum durchsuchen. 
 * Das Verhalten hat mehrere unterschiedlich Zustaende:
 * 1. Zu einer Wand oder einem anderen Hindernis fahren.
 * 2. Zu einer Seite drehen, bis der Bot parallel zur Wand ist. 
 * Es macht vielleicht Sinn, den Maussensor auszulesen, um eine Drehung um 
 * einen bestimmten Winkel zu realisieren. Allerdings muesste dafuer auch der
 * Winkel des Bots zur Wand bekannt sein.
 * 3. Eine feste Strecke parallel zur Wand vorwaerts fahren.
 * Da bot_glance_behaviour abwechselnd zu beiden Seiten schaut, ist es fuer die Aufgabe, 
 * einer Wand auf einer Seite des Bots zu folgen, nur bedingt gewachsen und muss
 * evtl. erweitert werden.
 * 4. Senkrecht zur Wand drehen.
 * Siehe 2.
 * 5. Einen Bogen fahren, bis der Bot wieder auf ein Hindernis stoesst. 
 * Dann das Ganze von vorne beginnen, nur in die andere Richtung und mit einem
 * weiteren Bogen. So erforscht der Bot einigermassen systematisch den Raum.
 * 
 * Da das Verhalten jeweils nach 10ms neu aufgerufen wird, muss der Bot sich
 * 'merken', in welchem Zustand er sich gerade befindet.
 * */
void bot_explore_behaviour(Behaviour_t *data){
	static int8 curve = 0,state = EXPLORATION_STATE_GOTO_WALL, running_curve = False;
	
	if((*exploration_check_function)()) return_from_behaviour(data);
	
	switch(state){
	// Volle Fahrt voraus, bis ein Hindernis erreicht ist.
	case EXPLORATION_STATE_GOTO_WALL:
		// Der Bot steht jetzt vor einem Hindernis und soll sich nach rechts drehen
		if(bot_avoid_harm()) {
			state = EXPLORATION_STATE_TURN_PARALLEL_RIGHT;
			deactivateBehaviour(bot_avoid_col_behaviour);
		}
		// Es befindet sich kein Hindernis direkt vor dem Bot.
		else {
			if(sensDistL < COL_NEAR || sensDistR < COL_NEAR){
				bot_drive(0,BOT_SPEED_FAST);
			} else {
				bot_drive(0,BOT_SPEED_MAX);
			}
		}
		break;
	// Nach links drehen, bis der Bot parallel zum Hindernis auf der rechten Seite steht.
	/* TODO: Aufgabe: Entwickle ein Verhalten, dass auch bei Loechern funktioniert. 
	 * Tipp dazu: Drehe den Roboter auf das Loch zu, bis beide Bodensensoren das Loch 'sehen'. Anschliessend drehe den Bot um 90 Grad.
	 * Es ist noetig, neue Zustaende zu definieren, die diese Zwischenschritte beschreiben. 
	 * TODO: Drehung mit dem Maussensor ueberwachen. */
	case EXPLORATION_STATE_TURN_PARALLEL_LEFT:
		if(sensDistR < COL_FAR){
			// Volle Drehung nach links mit ca. 3Grad/10ms
			bot_drive(-127,BOT_SPEED_FAST);
		} else {
			// Nachdem das Hindernis nicht mehr in Sicht ist, dreht der Bot noch ca. 3 Grad weiter.
			// Im Zweifelsfall dreht das den Bot zu weit, aber das ist besser, als ihn zu kurz zu drehen.
			bot_drive(-127,BOT_SPEED_FAST);
			state = EXPLORATION_STATE_DRIVE_PARALLEL_RIGHT;
		}
		break;
	// Nach rechts drehen, bis der Bot parallel zum Hindernis auf der linken Seite steht.
	/* Aufgabe: siehe EXPLORATION_STATE_TURN_PARALLEL_LEFT */
	case EXPLORATION_STATE_TURN_PARALLEL_RIGHT:
		if(sensDistL < COL_FAR){
			// Volle Drehung nach rechts mit ca. 3Grad/10ms
			bot_drive(127,BOT_SPEED_FAST);
		} else {
			/* Nachdem das Hindernis nicht mehr in Sicht ist, dreht der Bot noch ca. 3 Grad weiter.
			 * Im Zweifelsfall dreht das den Bot zu weit, aber das ist besser, als ihn zu kurz zu drehen. */
			bot_drive(127,BOT_SPEED_FAST);
			state = EXPLORATION_STATE_DRIVE_PARALLEL_LEFT;
		}
		break;
	case EXPLORATION_STATE_DRIVE_PARALLEL_LEFT:
		bot_drive_distance(data,0,BOT_SPEED_FAST,15);
		state = EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT;
		break;
	case EXPLORATION_STATE_DRIVE_PARALLEL_RIGHT:
		bot_drive_distance(data,0,BOT_SPEED_FAST,15);
		state = EXPLORATION_STATE_TURN_ORTHOGONAL_LEFT;
		break;
	case EXPLORATION_STATE_TURN_ORTHOGONAL_LEFT:
		// Drehe den Bot um 90 Grad nach links.
		/* Da der Bot sich immer ein bisschen zu weit von der Wand weg dreht, soll er sich
		 * hier nur um 85 Grad drehen. Nicht schoen, aber klappt.*/
		bot_turn(data,85);
		state = EXPLORATION_STATE_DRIVE_ARC;
		activateBehaviour(bot_avoid_col_behaviour);
		break;
	case EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT:
		// Drehe den Bot um 90 Grad nach rechts.
		/* Da der Bot sich immer ein bisschen zu weit von der Wand weg dreht, soll er sich
		 * hier nur um 85 Grad drehen. Nicht schoen, aber klappt.*/
		bot_turn(data,-85);
		state = EXPLORATION_STATE_DRIVE_ARC;
		activateBehaviour(bot_avoid_col_behaviour);
		break;
	case EXPLORATION_STATE_DRIVE_ARC:
		/* Fahre einen Bogen.
		 * Der Bot soll im Wechsel Links- und Rechtsboegen fahren. Daher muss das Vorzeichen von curve wechseln.
		 * Ausserdem soll der Bogen zunehmend weiter werden, so dass der absolute Wert von curve abnehmen muss.
		 * Ist der Wert 0, wird er auf den engsten Bogen initialisiert. Da der Bot am Anfang nach rechts abbiegt,
		 * muss der Wert positiv sein. 
		 * Aufgabe: Manchmal kann es passieren, dass der Bot bei einer kleinen Kurve zu weit weg von der Wand
		 * startet und dann nur noch im Kreis faehrt. Unterbinde dieses Verhalten.
		 */
		if(curve == 0){
			curve = 25;
			running_curve = True;
		} else if (running_curve == False){
			curve *= -0.9;
			running_curve = True;
		}
		/* Sobald der Bot auf ein Hindernis stoesst, wird der naechste Zyklus eingeleitet.
		 * Auf einen Rechtsbogen (curve > 0) folgt eine Linksdrehung und auf einen Linksbogen eine Rechtsdrehung.
		 * Wenn der Wert von curve (durch Rundungsfehler bei int) auf 0 faellt, beginnt das Suchverhalten erneut.*/
		if(bot_avoid_harm()) {
			state = (curve > 0) ? EXPLORATION_STATE_TURN_PARALLEL_LEFT : EXPLORATION_STATE_TURN_PARALLEL_RIGHT;
			running_curve = False;
			deactivateBehaviour(bot_avoid_col_behaviour);
		} else {
			bot_drive(curve, BOT_SPEED_MAX);
		}
		break;
	default:
		state = EXPLORATION_STATE_GOTO_WALL;
		curve = 0;
		activateBehaviour(bot_avoid_col_behaviour);
	}
	
}

/*!
 * Aktiviert bot_explore_behaviour. */
void bot_explore(Behaviour_t *caller, int8 (*check)(void)){
	exploration_check_function = check;
	switch_to_behaviour(caller,bot_explore_behaviour,NOOVERRIDE);
}

/*!
 * Das Verhalten dreht den Bot so, dass er auf eine Lichtquelle zufaehrt. */
void bot_goto_light(void){
	int16 speed, curve = (sensLDRL - sensLDRR)/1.5;

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

/*!
 * Das Verhalten laesst den Bot einen Slalom fahren.
 * @see bot_do_slalom()
 * */
void bot_do_slalom_behaviour(Behaviour_t *data){
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
			// Wenn der Bot direkt vor der Saeule steht, kann der Slalom anfangen, sonst zum Licht fahren
			if(bot_avoid_harm()){
				state = SLALOM_STATE_START;
			} else bot_goto_light();
		} else {// ... sonst muss er den Slalom-Kurs neu suchen. 
			activateBehaviour(bot_avoid_col_behaviour);
			return_from_behaviour(data);
		}
		break;
	case SLALOM_STATE_START:
		// Hier ist Platz fuer weitere Vorbereitungen, falls noetig.
		deactivateBehaviour(bot_avoid_col_behaviour);
		state = SLALOM_STATE_TURN_1;
		// break;
	case SLALOM_STATE_TURN_1:
		turn = (orientation == SLALOM_ORIENTATION_LEFT) ? 90 : -90;
		bot_turn(data,turn);
		state = SLALOM_STATE_DRIVE_ARC;
		break;
	case SLALOM_STATE_DRIVE_ARC:
		// Nicht wundern: Bei einem Links-Slalom faehrt der Bot eine Rechtskurve.
		curve = (orientation == SLALOM_ORIENTATION_LEFT) ? 25 : -25;
		bot_drive_distance(data,curve,BOT_SPEED_FAST,20);
		state = SLALOM_STATE_TURN_2;
		break;
	case SLALOM_STATE_TURN_2:
		turn = (orientation == SLALOM_ORIENTATION_LEFT) ? 45 : -45;
		bot_turn(data,turn);
		state = SLALOM_STATE_SWEEP_RUNNING;
		break;
	case SLALOM_STATE_SWEEP_RUNNING:
		if(sweep_steps == 0){
			sweep_state = SWEEP_STATE_CHECK;	
		}
		// Insgesamt 6 Schritte drehen
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
			// Phase 2: Bot um 15 Grad drehen
				turn = (orientation == SLALOM_ORIENTATION_LEFT) ? 15 : -15;
				bot_turn(data,turn);
				sweep_state = SWEEP_STATE_CHECK;
				sweep_steps++;
			}
		} else {
			turn = (orientation == SLALOM_ORIENTATION_LEFT) ? -90 : 90;
			bot_turn(data,turn);
			state = SLALOM_STATE_SWEEP_DONE;
			sweep_steps = 0;
		}
		break;
	case SLALOM_STATE_SWEEP_DONE:
		turn = (orientation == SLALOM_ORIENTATION_LEFT) ? -135 : 135;
		bot_turn(data,turn);
		state = SLALOM_STATE_CHECK_PILLAR;
		break;
	default:
		state = SLALOM_STATE_CHECK_PILLAR;
	}
	
}

/*!
 * Das Verhalten laesst den Bot zwischen einer Reihe beleuchteter Saeulen Slalom fahren. 
 * Das Verhalten ist wie bot_explore() in eine Anzahl von Teilschritten unterteilt.
 * 1. Vor die aktuelle Saeule stellen, so dass sie zentral vor dem Bot und ungefaehr 
 * COL_CLOSEST (100 mm) entfernt ist.
 * 2. 90 Grad nach rechts drehen.
 * 3. In einem relativ engen Bogen 20 cm weit fahren.
 * 4. Auf der rechten Seite des Bot nach einem Objekt suchen, dass
 * 	a) im rechten Sektor des Bot liegt, also zwischen -45 Grad und -135 Grad zur Fahrtrichtung liegt,
 * 	b) beleuchtet und 
 * 	c) nicht zu weit entfernt ist.
 * Wenn es dieses Objekt gibt, wird es zur aktuellen Saeule und der Bot faehrt jetzt Slalom links.
 * 5. Sonst zurueck drehen, 90 Grad drehen und Slalom rechts fahren.
 * In diesem Schritt kann der Bot das Verhalten auch abbrechen, falls er gar kein Objekt mehr findet.
 */
void bot_do_slalom(Behaviour_t *caller){
	switch_to_behaviour(caller, bot_do_slalom_behaviour,NOOVERRIDE);
}

/*!
 * Das Verhalten setzt sich aus 3 Teilverhalten zusammen: 
 * Nach Licht suchen, auf das Licht zufahren, im Licht Slalom fahren. */
void bot_olympic_behaviour(Behaviour_t *data){
	if(check_for_light()){
		/* Sobald der Bot auf ein Objekt-Hinderniss stoesst, versucht er, Slalom zu fahren.
		 * Aufgabe: Wenn der Bot vor einem Loch steht, hinter welchem sich die Lichtquelle 
		 * befindet, wird er daran haengen bleiben. Schreibe ein Verhalten, dass das verhindert. */
		if(bot_avoid_harm() && is_obstacle_ahead(COL_NEAR)){
			bot_do_slalom(data);
		} else bot_goto_light();
	} else bot_explore(data,check_for_light);
}

/*!
 * Das Verhalten dreht sich um 45� in die angegebene Richtung (0=rechts, 1=links)
 * und prueft, ob auf dem Sensor auf der Seite der angegebenen Richtung eine Wand
 * im Abstand von 12cm zu sehen ist. Wenn dem so ist, wird die Variable wall_present
 * auf True gesetzt, sonst False */
void bot_check_wall_behaviour(Behaviour_t *data) {
	/* Konstantenfuer check_wall_behaviour-Verhalten */
	#define CORRECT_NEAR				0
	#define CORRECT_NONE				1
	#define CORRECT_FAR					2
	/* Zustaende fuer check_wall_behaviour-Verhalten */
	#define CHECK_WALL_TURN				0
	#define CHECK_WALL_SENS				1
	#define CHECK_WALL_TURN_BACK		2
	#define CHECK_WALL_FINISHED			3
	#define CHECK_WALL_CORRECT			4
	
	static int8 checkState=CHECK_WALL_TURN;
	/* wenn die Wand noch da ist aber aus dem Blickfeld rueckt, Entfernung und Winkel korrigieren */
	static int8 correctDistance=CORRECT_NONE;	
	/* letzte, gueltige Distanz fuer Abweichungsberechnung */
	static int16 lastDistance=0;
	
	int16 sensor;	/* fuer temporaer benutzte Senorwerte */

	switch(checkState) {
		case CHECK_WALL_TURN:
			/* haben wir links oder rechts eine Wand? */
			if (check_direction==CHECK_WALL_LEFT) {
				checkState=CHECK_WALL_SENS;
				bot_turn(data,45);
				break;	
			} else {
				checkState=CHECK_WALL_SENS;
				bot_turn(data,-45);
				break;
			}
			
		case CHECK_WALL_SENS:
			if (check_direction==CHECK_WALL_LEFT) {
				sensor=sensDistL;
			} else {
				sensor=sensDistR;
			}
			
			/* keine wand in eingestellter Maximalentfernung? */
			if (sensor>IGNORE_DISTANCE) {
				correctDistance=CORRECT_NONE; /* dann auch keine Korrektur */
				lastDistance=0;				  /* auch kein Vergleichswert */
				wall_detected=False;		  /* keine Wand da */
			} else if (sensor<OPTIMAL_DISTANCE-ADJUST_DISTANCE) {
				/* bot hat den falschen Abstand zur Wand, zu nah dran */
				wall_detected=True;
				correctDistance=CORRECT_NEAR;
			} else if (sensor>OPTIMAL_DISTANCE+ADJUST_DISTANCE) {
				/* bot hat den falschen Abstand zur Wand, zu weit weg */
				wall_detected=True;
				correctDistance=CORRECT_FAR;
			} else {
				/* perfekter Abstand */
				correctDistance=CORRECT_NONE;
				wall_detected=True;
			}
			wall_distance=sensor;
			/* Wenn Korrektur noetig, dann durchfuehren, sonst gleich zurueckdrehen */
			if (correctDistance==CORRECT_NONE) checkState=CHECK_WALL_TURN_BACK;
			else checkState=CHECK_WALL_CORRECT;
			break;

			
		case CHECK_WALL_TURN_BACK:
			checkState=CHECK_WALL_FINISHED;
			int16 turnAngle=45;
			
			/* welcher Sensorwert wird gebraucht? */
			if (check_direction==CHECK_WALL_LEFT) {
				sensor=sensDistL;
			} else {
				sensor=sensDistR;
			}
			/* wenn vorhanden, aus letztem Abstand und aktuellem Abstand
			 * Korrekturwinkel berechnen, falls vorheriger Abstand da.
			 * wird durch Abstand>IGNORE_DISTANCE zurueckgesetzt */
			if (lastDistance!=0) {
				turnAngle=turnAngle+(lastDistance-wall_distance)/5;
			}
			if (sensor<IGNORE_DISTANCE) lastDistance=sensor;
			if (check_direction==CHECK_WALL_LEFT) {
				bot_turn(data,-turnAngle);
			} else {
				bot_turn(data,turnAngle);
			}
			break;
		
		case CHECK_WALL_FINISHED:
			/* ok, check beendet -> Verhalten verlassen */
			checkState=CHECK_WALL_TURN;
			return_from_behaviour(data);
			break; 
			
		case CHECK_WALL_CORRECT:
			/* Abhaengig von der Seite, auf der die Wand ist, Entfernung korrigieren */
			if (check_direction==CHECK_WALL_LEFT) {
				sensor=sensDistL;
			} else {
				sensor=sensDistR;
			}
			if (sensor-OPTIMAL_DISTANCE<0) {
				speedWishLeft=-BOT_SPEED_SLOW;
				speedWishRight=-BOT_SPEED_SLOW;
			} else if (sensor-OPTIMAL_DISTANCE>0) {
				speedWishLeft=BOT_SPEED_SLOW;
				speedWishRight=BOT_SPEED_SLOW;
			}
			else {
				checkState=CHECK_WALL_TURN_BACK;
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
			}
			break;
			
			
		default:
			checkState=CHECK_WALL_TURN;
			return_from_behaviour(data);
			break;
	}
}
 

/*!
 * Das Verhalten dreht sich um 45� in die angegebene Richtung (0=rechts, 1=links)
 * und prueft, ob auf dem Sensor auf der Seite der angegebenen Richtung eine Wand
 * im Abstand von 12-22cm zu sehen ist. Wenn dem so ist, wird die Variable wall_present
 * auf True gesetzt, sonst False */
void bot_check_wall(Behaviour_t *caller,int8 direction) {
	check_direction=direction;
	wall_detected=False;
	switch_to_behaviour(caller, bot_check_wall_behaviour,NOOVERRIDE);
}


/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 */
 
void bot_measure_angle_behaviour(Behaviour_t *caller) {
	/* Zustaende measure_angle_behaviour-Verhalten */
	#define MEASURE_TURN				0
	#define FOUND_OBSTACLE				1
	#define TURN_COMPLETED				2
	#define TURN_BACK					3
	#define CORRECT_ANGLE				4
	#define MEASUREMENT_DONE			5
	
	static int8 measureState=MEASURE_TURN;

	/* bereits gedrehte Strecke errechnen */
	int16 turnedLeft=(measure_direction>0)?-(sensEncL-startEncL):(sensEncL-startEncL);
	int16 turnedRight=(measure_direction>0)?(sensEncR-startEncR):-(sensEncR-startEncR);
	int16 turnedSteps=(abs(turnedLeft)+abs(turnedRight))/2;
	
	/* sensorwert abhaengig von der Drehrichtung abnehmen */
	int16 sensor=(measure_direction==MEASURE_LEFT)?sensDistL:sensDistR;
	/* solange drehen, bis Hindernis innerhalb Messstrecke oder 360� komplett */
	switch(measureState){
		case MEASURE_TURN:
			/* nicht mehr als eine komplette Drehung machen! */
			if (turnedSteps>=ANGLE_CONSTANT) {
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				measureState=TURN_COMPLETED;
				measured_angle=0;		/* kein Hindernis gefunden */
				break;
			}
			/* Ist ein Objekt in Reichweite? */
			if (sensor<=measure_distance) {
				measure_distance=sensor;
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				measureState=FOUND_OBSTACLE;
				break;
			}
			/* Beginnen, zurueckzudrehen */
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_NORMAL : BOT_SPEED_NORMAL;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_NORMAL : -BOT_SPEED_NORMAL;
			break;
			
		case FOUND_OBSTACLE:
			/* Hindernis gefunden, nun Bot wieder in Ausgangsstellung drehen */
			measure_direction=-measure_direction;
			measured_angle=(int16)((long)(turnedSteps*360)/ANGLE_CONSTANT);
			measureState=TURN_BACK;
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_NORMAL : BOT_SPEED_NORMAL;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_NORMAL : -BOT_SPEED_NORMAL;
			break;
			
		case TURN_COMPLETED:
			/* bot steht wieder in Ausgangsrichtung, Verhalten beenden */
			speedWishLeft=BOT_SPEED_STOP;
			speedWishRight=BOT_SPEED_STOP;
			measureState=CORRECT_ANGLE;
			break;
			
		case TURN_BACK:
			/* Bot in Ausgangsposition drehen */
			if ((turnedLeft+turnedRight)/2>=0) {
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				measureState=TURN_COMPLETED;
				break;
			}
			speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_NORMAL : BOT_SPEED_NORMAL;
			speedWishRight = (measure_direction > 0) ? BOT_SPEED_NORMAL : -BOT_SPEED_NORMAL;
			break;
			
		case CORRECT_ANGLE:
			/* Evtl. etwas zuruecksetzen, falls wir zu weit gefahren sind */
			if (turnedRight>0) {
				/* rechts zu weit gefahren..langsam zurueck */
				speedWishRight = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			} else if (turnedRight<0) {
				/* rechts noch nicht weit genug...langsam vor */
				speedWishRight = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, rechtes Rad anhalten */
				speedWishRight = BOT_SPEED_STOP;
			}		
			if (turnedLeft<0) {
				/* links zu weit gefahren..langsam zurueck */
				speedWishLeft = (measure_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;				
			} else if (turnedLeft>0) {
				/* links noch nicht weit genug...langsam vor */
				speedWishLeft = (measure_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, linkes Rad anhalten */
				speedWishLeft = BOT_SPEED_STOP;
			}
			if (speedWishLeft == BOT_SPEED_STOP && speedWishRight == BOT_SPEED_STOP) {
				/* beide Raeder haben nun wirklich die Endposition erreicht, daher anhalten */
				measureState=MEASUREMENT_DONE;
			}
			break;
			
		case MEASUREMENT_DONE:
			measureState=MEASURE_TURN;
			return_from_behaviour(caller);
			break;
	}	
}

/*!
 * Das Verhalten dreht den Bot in die angegebene Richtung bis ein Hindernis
 * im Sichtbereich erscheint, das eine Entfernung bis max. zur angegebenen
 * Distanz zum Bot hat.
 */

void bot_measure_angle(Behaviour_t *caller, int8 direction, int16 distance) {
	/* maximale Messentfernung und Richtung setzen */
	measure_direction=direction;
	measure_distance=distance;
	/* Encoderwerte zu Anfang des Verhaltens merken */
	startEncL=sensEncL;
	startEncR=sensEncR;
	switch_to_behaviour(caller, bot_measure_angle_behaviour,NOOVERRIDE);	
}

/*!
 * Das Verhalten findet seinen Weg durch ein Labyrinth, das nach gewissen Grundregeln gebaut ist
 * in nicht immer optimaler Weise aber in jedem Fall. Es arbeitet nach dem Hoehlenforscher-Algorithmus.
 * Einschraenkung: Objekte im Labyrinth, die Endlossschleifen verursachen koennen, z.b. ein einzeln
 * stehender Pfeiler im Labyrinth um den der Bot dann immer wieder herum fahren wuerde. 
 */
void bot_solve_maze_behaviour(Behaviour_t *data){
	/* Zustaende fuer das bot_solve_maze_behaviour-Verhalten */
	#define CHECK_FOR_STARTPAD			0
	#define CHECK_FOR_WALL_RIGHT		1
	#define CHECK_FOR_WALL_LEFT			2
	#define CHECK_WALL_PRESENT			3
	#define SOLVE_MAZE_LOOP				4
	#define SOLVE_TURN_WALL				5
	#define CHECK_CONDITION				6
	#define TURN_TO_BRANCH				7
	#define DETECTED_CROSS_BRANCH		8
	#define APPROACH_CORNER				9
	#define AVOID_ABYSS					10
	#define REACHED_GOAL				11
	static int8 mazeState=CHECK_FOR_STARTPAD;
	static int8 followWall=-1;
	static int8 checkedWalls=0;
	
	int16 distance;
	double x;
	printf("mazeState= %d\n",mazeState);
	switch(mazeState) {
		case CHECK_FOR_STARTPAD:
			/* Wo beginnen wir, nach einer Wand zu suchen? 
			 * Abgrund- und Kollisions-Verhalten ausschalten */
			deactivateBehaviour(bot_avoid_col_behaviour);
			deactivateBehaviour(bot_avoid_border_behaviour);
			/* bot_glance() stoert bot_turn() */
			deactivateBehaviour(bot_glance_behaviour);
			/* sieht nach, ob der Bot auf einem definierten Startpad steht und
			 * beginnt dann mit der Suche gleich an der richtigen Wand */
			/* Zuserst bei nach Startpad1 gucken */
			checkedWalls=0;
			if ((sensLineL>=STARTPAD1-10 && sensLineL<=STARTPAD1+10) ||
				(sensLineR>=STARTPAD1-10 && sensLineR<=STARTPAD1+10)) {
				mazeState=CHECK_FOR_WALL_LEFT;
				break;
			}
			mazeState=CHECK_FOR_WALL_RIGHT;
			break;
			
		case CHECK_FOR_WALL_RIGHT:

			mazeState=CHECK_WALL_PRESENT;
			followWall=CHECK_WALL_RIGHT;
			bot_check_wall(data,followWall);
			break;
			
		case CHECK_FOR_WALL_LEFT:
		
			followWall=CHECK_WALL_LEFT;
			bot_check_wall(data,followWall);
			mazeState=CHECK_WALL_PRESENT;
			break;
			
		
		case CHECK_WALL_PRESENT:
			/* wenn keine Wand gefunden aber links noch nicht nachgesehen, andere 
			 * Richtung checken, sonst vorfahren */
			checkedWalls++;
			if (wall_detected==False){
				/* Wand noch nicht entdeckt...haben wir schon beide gecheckt? */
				if (checkedWalls<2) {
					if (followWall==CHECK_WALL_RIGHT) {
						mazeState=CHECK_FOR_WALL_LEFT;
						break;
					} else {
						mazeState=CHECK_FOR_WALL_RIGHT;
						break;
					}
				} else {
					/* keine wand? dann vorfahren und selbes prozedere nochmal */
					bot_drive_distance(data,0,BOT_SPEED_NORMAL,BOT_DIAMETER);
					mazeState=CHECK_FOR_WALL_RIGHT;
					checkedWalls=0;
					break;
				}
			}
			/* ok, wir haben unsere richtung im labyrinth gefunden jetzt dieser 
			 * nur noch folgen bis Ziel, Abzweig oder Abgrund */
			mazeState=SOLVE_MAZE_LOOP;
			break;
			
		case SOLVE_MAZE_LOOP:
			/* Einen Schritt (=halbe BOT-Groesse) vorwaerts */
			mazeState=SOLVE_TURN_WALL;
			bot_drive_distance(data,0,BOT_SPEED_NORMAL,BOT_DIAMETER);
			break;
			
		case SOLVE_TURN_WALL:
			/* Ziel erreicht? */
			if ((sensLineL>GROUND_GOAL-20 && sensLineL<GROUND_GOAL+20) || 
			    (sensLineR>GROUND_GOAL-20 && sensLineR<GROUND_GOAL+20)) {
				/* Bot hat Ziel erreicht...aus Freude einmal um die Achse drehen */
				bot_turn(data,360);
				mazeState=REACHED_GOAL;
				break;
			}
			/* checken, ob wand vor uns (Abstand 2.5*Bot-Durchmesser in mm) */
			distance=(sensDistL+sensDistR)/2;
			if (distance<=25*BOT_DIAMETER) { // umrechnen 10*BOT_DIAMETER, da letzteres in cm angegeben wird
				/* berechnete Entfernung zur Wand abzueglich optimale Distanz fahren */
				mazeState=DETECTED_CROSS_BRANCH;
				bot_drive_distance(data,0,BOT_SPEED_NORMAL,(distance-OPTIMAL_DISTANCE)/10);
				break;
			}
			/* Zur Wand drehen....ist die Wand noch da? */
			mazeState=CHECK_CONDITION;
			bot_check_wall(data,followWall);
			break;
			
		case CHECK_CONDITION:
			/* Solange weiter, wie die Wand zu sehen ist */
			if (wall_detected==True) {
				mazeState=SOLVE_MAZE_LOOP;
				break;
			}
			/* messen, wo genau die Ecke ist */
			mazeState=APPROACH_CORNER;
			if (followWall==CHECK_WALL_LEFT){
				bot_measure_angle(data,MEASURE_LEFT,300);
			} else {
				bot_measure_angle(data,MEASURE_RIGHT,300);
			} 
			break;

		case TURN_TO_BRANCH:
			/* nun in Richtung Abzweig drehen , dann mit Hauptschleife weiter*/
			mazeState=SOLVE_MAZE_LOOP;
			if (followWall==CHECK_WALL_RIGHT) {
				bot_turn(data,-90);
			} else {
				bot_turn(data,90);
			}
			break;
			
		case DETECTED_CROSS_BRANCH:
			/* Bot faehrt auf eine Ecke zu, in Richtung Gang drehen */
			if (followWall==CHECK_WALL_LEFT) {
				mazeState=SOLVE_MAZE_LOOP;
				bot_turn(data,-90);
			} else {
				mazeState=SOLVE_MAZE_LOOP;
				bot_turn(data,90);
			}
			break;
			
		case APPROACH_CORNER:
			/* ok, nun strecke bis zur Kante berechnen */
			x=measure_distance*cos(measured_angle*3.1416/180)/10+BOT_DIAMETER;
			mazeState=TURN_TO_BRANCH;
			bot_drive_distance(data,0,BOT_SPEED_NORMAL,(int16)x);
			break;
			
		case REACHED_GOAL:
			mazeState=CHECK_WALL_RIGHT;
			speedWishLeft=BOT_SPEED_STOP;
			speedWishRight=BOT_SPEED_STOP;
			return_from_behaviour(data);
			break;
	}
}


/*!
 * Das Verhalten findet seinen Weg durch ein Labyrinth, das nach gewissen Grundregeln gebaut ist
 * in nicht immer optimaler Weise aber in jedem Fall. Es arbeitet nach dem Hoehlenforscher-Algorithmus.
 * Einschraenkung: Objekte im Labyrinth, die Endlossschleifen verursachen koennen, z.b. ein einzeln
 * stehender Pfeiler im Labyrinth um den der Bot dann immer wieder herum fahren wuerde. 
 */
 
void bot_solve_maze(Behaviour_t *caller){
	switch_to_behaviour(caller, bot_solve_maze_behaviour,NOOVERRIDE);
}


/*! 
 * Zentrale Verhaltens-Routine, wird regelmaessig aufgerufen. 
 * Dies ist der richtige Platz fuer eigene Routinen, um den Bot zu steuern.
 */
void bot_behave(void){	
	Behaviour_t *job;						// Zeiger auf ein Verhalten
	
	float faktorLeft = 1.0;					// Puffer fuer Modifkatoren
	float faktorRight = 1.0;				// Puffer fuer Modifkatoren
	
	#ifdef RC5_AVAILABLE
		rc5_control();						// Abfrage der IR-Fernbedienung
	#endif

	/* Solange noch Verhalten in der Liste sind...
	   (Achtung: Wir werten die Jobs sortiert nach Prioritaet aus. Wichtige zuerst einsortieren!!!) */
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
Behaviour_t *new_behaviour(uint8 priority, void (*work) (struct _Behaviour_t *data), int8 active){
	Behaviour_t *newbehaviour = (Behaviour_t *) malloc(sizeof(Behaviour_t)); 
	
	if (newbehaviour == NULL) 
		return NULL;
	
	newbehaviour->priority = priority;
	newbehaviour->active=active;
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
	// Hoechste Prioritate haben die Notfall Verhalten

	// Verhalten zum Schutz des Bots, hohe Prioritaet, Aktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(250, bot_avoid_border_behaviour,ACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(249, bot_avoid_col_behaviour,ACTIVE));


	// Verhalten, um Hidnernisse besser zu erkennen, relativ hoe Prioritaet, modifiziert nur
	insert_behaviour_to_list(&behaviour, new_behaviour(200, bot_glance_behaviour,ACTIVE));

	// Verhalten, um ein Labyrinth nach der Hoehlenforscher-Methode loesen 
	insert_behaviour_to_list(&behaviour, new_behaviour(150, bot_solve_maze_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(43, bot_measure_angle_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(42, bot_check_wall_behaviour,INACTIVE));


	// Alle Hilfsroutinen sind relativ wichtig, da sie auch von den Notverhalten her genutzt werden
	// Hilfsverhalten, die Befehle von Boten-Funktionen ausfuehren, erst inaktiv, werden von Boten aktiviert	
	insert_behaviour_to_list(&behaviour, new_behaviour(150, bot_turn_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(149, bot_drive_distance_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(148, bot_goto_behaviour,INACTIVE));

	// unwichtigere Hilfsverhalten
	insert_behaviour_to_list(&behaviour, new_behaviour(100, bot_explore_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour( 99, bot_do_slalom_behaviour,INACTIVE));

	// Demo-Verhalten, ganz einfach, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(50, bot_simple_behaviour,INACTIVE));
	// Demo-Verhalten, etwas komplexer, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(51, bot_drive_square_behaviour,INACTIVE));
	// Demo-Verhalten für aufwendiges System, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(52, bot_olympic_behaviour,INACTIVE));


	// Grundverhalten, setzt aeltere FB-Befehle um, aktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(2, bot_base_behaviour, ACTIVE));


//	activateBehaviour(bot_simple_behaviour);


/*

	// Verhalten zum Schutz des Bots, hohe Prioritaet, Aktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(200, bot_avoid_border,ACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(100, bot_avoid_col,ACTIVE));
	
	// Verhalten, um Hidnernisse besser zu erkennen, relativ hoe Prioritaet, modifiziert nur
	insert_behaviour_to_list(&behaviour, new_behaviour(60, bot_glance,INACTIVE));

	// Demo-Verhalten, ganz einfach, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(200, bot_simple_behaviour,INACTIVE));

	// Demo-Verhalten, etwas komplexer, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(50, bot_drive_square_behaviour,INACTIVE));

	// Demo-Verhalten für aufwendiges System, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(55, bot_olympic_behaviour,INACTIVE));

	// Hilfsverhalten, die Befehle von Boten-Funktionen ausfuehren, erst inaktiv, werden von Boten aktiviert	
	insert_behaviour_to_list(&behaviour, new_behaviour(41, bot_drive_distance_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(40, bot_turn_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(30, bot_goto_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(51, bot_explore_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(50, bot_do_slalom_behaviour,INACTIVE));

	// Grundverhalten, setzt aeltere FB-Befehle um, aktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(2, bot_base, ACTIVE));

*/
//	activateBehaviour(bot_simple_behaviour);


	#ifdef PC
		#ifdef DISPLAY_AVAILABLE
			/* Anzeigen der geladenen Verhalten  */
				Behaviour_t	*ptr	= behaviour;
	
				display_cursor(5,1);
				display_printf("Verhaltensstack:\n");
				while(ptr != NULL)	{
					display_printf("Prioritaet: %d.\n", ptr->priority);
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

	switch_to_behaviour(caller,bot_goto_behaviour,OVERRIDE);	
}

#ifdef DISPLAY_BEHAVIOUR_AVAILABLE

/*!
 * ermittelt ob noch eine weitere Verhaltensseite existiert 	
 */
 int8  another_behaviour_page(void) {
  int16 max_behaviours ;
  Behaviour_t	*ptr	;
  
  /* dazu muss ich auch gueltige Screenseite sein */
  #ifdef DISPLAY_SCREENS_AVAILABLE
   if (display_screen != 2)
     return 0;
  #endif 
  
  ptr = behaviour;
  max_behaviours = 0;
  
// zuerst alle Verhalten ermitteln ausser Grundverhalten
  while(ptr != NULL)	{			 
	if  ((ptr->priority > 2) &&(ptr->priority <= 200)) 
    			max_behaviours++;		  
						  
	ptr = ptr->next;
   }  

   return (behaviour_page  * 6) < max_behaviours;
}


/*! 
 * toggled ein Verhalten der Verhaltensliste an Position pos,
 * die Aenderung erfolgt nur auf die Puffervariable  
 * @param pos Listenposition, entspricht der Taste 1-6 der gewaehlten Verhaltensseite
 */
void toggleNewBehaviourPos(int8 pos){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
    int8 i;
    
    // nur aendern, wenn ich richtige Screenseite bin 
     if (display_screen != 2)
       return ;
     
    // richtigen Index je nach Seite ermitteln 
    pos = (behaviour_page - 1) * 6 + pos;
    i   = 0;

	// durch die Liste gehen, bis wir den gewuenschten Index erreicht haben 
	for (job = behaviour; job; job = job->next) {
	    if ((job->priority > 2) &&(job->priority <= 200)) {		
		  i++;
		  if (i == pos) {
		  	  job->active_new = !job->active_new;
			  			      
			  break;
		  }
	    }
	}
}


/*! 
 * Startschuss, die gewaehlten neuen Verhaltensaktivitaeten werden in die
 * Verhaltensliste geschrieben und die Verhalten damit scharf geschaltet 
 */
void set_behaviours_active_to_new(void) {
 Behaviour_t *job;	
   for (job = behaviour; job; job = job->next) 	{
					 
	if  ((job->priority > 2) &&(job->priority <= 200)) 
            job->active = job->active_new;            				 
	 
   }
}

/*!
 * Die Aktivitaeten der Verhalten werden in die Puffervariable geschrieben, 
 * welche zur Anzeige und Auswahl verwendet wird
 */
void set_behaviours_equal(void) {
 Behaviour_t *job;	
   for (job = behaviour; job; job = job->next) 	{
					 
	if  ((job->priority > 2) &&(job->priority <= 200)) 
            job->active_new = job->active;            				 
	 
   }
}


#endif
