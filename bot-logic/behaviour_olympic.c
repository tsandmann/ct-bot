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

/*! @file 	behaviour_olympic.c
 * @brief 	Bot sucht saeulen und faehrt dann slalom
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
#include <stdlib.h>

/* Zustaende des bot_explore-Verhaltens */

#define EXPLORATION_STATE_GOTO_WALL 			1	/*!< Zustand: Bot sucht eine Wand o.ae. Hinderniss */
#define EXPLORATION_STATE_TURN_PARALLEL_LEFT 	2	/*!< Zustand: Bot dreht sich nach links, bis er parallel zur Wand blickt. */
#define EXPLORATION_STATE_TURN_PARALLEL_RIGHT 	3	/*!< Zustand: Bot dreht sich nach rechts, bis er parallel zur Wand blickt. */
#define EXPLORATION_STATE_DRIVE_PARALLEL_LEFT	4	/*!< Zustand: Bot faehrt parallel zur Wand links von sich. */
#define EXPLORATION_STATE_DRIVE_PARALLEL_RIGHT	5	/*!< Zustand: Bot faehrt parallel zur Wand rechts von sich. */
#define EXPLORATION_STATE_TURN_ORTHOGONAL_LEFT	6	/*!< Zustand: Bot dreht sich nach links, bis er senkrecht zur Wand steht. */
#define EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT	7	/*!< Zustand: Bot dreht sich nach rechts, bis er senkrecht zur Wand steht. */
#define EXPLORATION_STATE_DRIVE_ARC				8	/*!< Zustand: Bot faehrt einen Bogen. Der Winkel des Bogens sollte in einer 													 
													 *!< weiteren static Variablen (z.B. curve) gespeichert sein. */

/* Zustaende des bot_olympic_behaviour-Verhaltens */ 

#define CB_STATE_EXPLORATION		0	/*!< Zustand: Bot erforscht die Umgebung. */
#define CB_STATE_DOING_SLALOM 		1	/*!< Zustand: Bot ist dabei Slalom zu fahren. */

/* Zustaende des bot_do_slalom-Verhaltens */ 

#define SLALOM_STATE_START			0	/*!< Zustand: Bot startet eine Slalomlauf und positioniert sich vor der Saeule. */
#define SLALOM_STATE_TURN_1		1	/*!< Zustand: Bot dreht sich um 90�. */
#define SLALOM_STATE_DRIVE_ARC		2	/*!< Zustand: Bot faehrt den Bogen um die Saeule. */
#define SLALOM_STATE_TURN_2		3	/*!< Zustand: Bot dreht sich fuer den Sweep um 45�. */
#define SLALOM_STATE_SWEEP_RUNNING	4	/*!< Zustand: Bot macht den Sweep. */
#define SLALOM_STATE_SWEEP_DONE	5	/*!< Zustand: Bot ist fertig mit dem Sweep. */
#define SLALOM_STATE_CHECK_PILLAR	6	/*!< Zustand: Bot ueberprueft, ob er den Slalom fortsetzen kann. */

#define SLALOM_ORIENTATION_LEFT	0
#define SLALOM_ORIENTATION_RIGHT	1

/* Parameter fuer das bot_explore_behaviour() */
int8 (*exploration_check_function)(void);	/*!< Die Funktion, mit der das bot_explore_behaviour() feststellt, ob es etwas gefunden hat.
											 * Die Funktion muss True (1) zurueck geben, wenn dem so ist, sonst False (0).
											 * Beispiele fuer eine solche Funktion sind check_for_light, is_good_pillar_ahead etc.*/




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
			#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
				deactivateBehaviour(bot_avoid_col_behaviour);
			#endif
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
		#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
			activateBehaviour(bot_avoid_col_behaviour);
		#endif
		break;
	case EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT:
		// Drehe den Bot um 90 Grad nach rechts.
		/* Da der Bot sich immer ein bisschen zu weit von der Wand weg dreht, soll er sich
		 * hier nur um 85 Grad drehen. Nicht schoen, aber klappt.*/
		bot_turn(data,-85);
		state = EXPLORATION_STATE_DRIVE_ARC;
		#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
			activateBehaviour(bot_avoid_col_behaviour);
		#endif
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
			#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
				deactivateBehaviour(bot_avoid_col_behaviour);
			#endif
		} else {
			bot_drive(curve, BOT_SPEED_MAX);
		}
		break;
	default:
		state = EXPLORATION_STATE_GOTO_WALL;
		curve = 0;
		#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
			activateBehaviour(bot_avoid_col_behaviour);
		#endif
	}
	
}

/*!
 * Aktiviert bot_explore_behaviour. */
void bot_explore(Behaviour_t *caller, int8 (*check)(void)){
	exploration_check_function = check;
	switch_to_behaviour(caller,bot_explore_behaviour,NOOVERRIDE);
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
			#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
				activateBehaviour(bot_avoid_col_behaviour);
			#endif
			return_from_behaviour(data);
		}
		break;
	case SLALOM_STATE_START:
		// Hier ist Platz fuer weitere Vorbereitungen, falls noetig.
		#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
			deactivateBehaviour(bot_avoid_col_behaviour);
		#endif
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
 * Initialisiert das Olympische Verhalten
 * @param prio_main Prioritaet des Olympischen Verhalten (typ. 100)
 * @param prio_helper Prioritaet der Hilfsfunktionen (typ. 52)
 * @param active ACTIVE wenn es sofort starten soll, sonst INACTIVE
 */
void bot_olympic_init(int8 prio_main,int8 prio_helper, int8 active){
	// unwichtigere Hilfsverhalten
	insert_behaviour_to_list(&behaviour, new_behaviour(prio_helper--, bot_explore_behaviour,INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour( prio_helper, bot_do_slalom_behaviour,INACTIVE));
	// Demo-Verhalten fuer aufwendiges System, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(prio_main, bot_olympic_behaviour, active));
}

#endif
