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
 * @file 	behaviour_olympic.c
 * @brief 	Bot sucht Saeulen und faehrt dann Slalom
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.2006
 */

#include "bot-logic/bot-logic.h"

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
#define SLALOM_STATE_TURN_1			1	/*!< Zustand: Bot dreht sich um 90 Grad. */
#define SLALOM_STATE_DRIVE_ARC		2	/*!< Zustand: Bot faehrt den Bogen um die Saeule. */
#define SLALOM_STATE_TURN_2			3	/*!< Zustand: Bot dreht sich fuer den Sweep um 45 Grad. */
#define SLALOM_STATE_SWEEP_RUNNING	4	/*!< Zustand: Bot macht den Sweep. */
#define SLALOM_STATE_SWEEP_DONE		5	/*!< Zustand: Bot ist fertig mit dem Sweep. */
#define SLALOM_STATE_CHECK_PILLAR	6	/*!< Zustand: Bot ueberprueft, ob er den Slalom fortsetzen kann. */

#define SLALOM_ORIENTATION_LEFT		0
#define SLALOM_ORIENTATION_RIGHT	1

#define SWEEP_STATE_TURN			0	/*!< Zustand: Drehung im Sweep. */
#define SWEEP_STATE_CHECK			1	/*!< Zustand: Ueberpruefe Objekt vor dem Bot. */

/* Parameter fuer das bot_explore_behaviour() */
static int8_t (* exploration_check_function)(void);	/*!< Die Funktion, mit der das bot_explore_behaviour() feststellt, ob es etwas gefunden hat.
											 * Die Funktion muss True (1) zurueck geben, wenn dem so ist, sonst False (0).
											 * Beispiele fuer eine solche Funktion sind check_for_light, is_good_pillar_ahead etc.*/



/*!
 * Das Verhalten dreht den Bot so, dass er auf eine Lichtquelle zufaehrt.
 */
static void bot_goto_light(void) {
	int16_t speed, curve = (int16_t) ((sensLDRL - sensLDRR) / 1.5f);

	if (curve < -127)
		curve = -127;
	if (curve > 127)
		curve = 127;

	if (abs(sensLDRL - sensLDRR) < 20) {
		speed = BOT_SPEED_MAX;
	} else if (abs(sensLDRL - sensLDRR) < 150) {
		speed = BOT_SPEED_FAST;
	} else {
		speed = BOT_SPEED_NORMAL;
	}

	bot_drive((int8_t) curve, speed);
}

/*!
 * Gibt aus, ob der Bot Licht sehen kann.
 * @return True, wenn der Bot Licht sieht, sonst False.
 */
static int8_t check_for_light(void) {
	// Im Simulator kann man den Bot gut auf den kleinsten Lichtschein
	// reagieren lassen, in der Realitaet gibt es immer Streulicht, so dass
	// hier ein hoeherer Schwellwert besser erscheint.
	// Simulator:
	if (sensLDRL >= 1023 && sensLDRR >= 1023)
		return False;
	// Beim echten Bot eher:
	// if(sensLDRL >= 100 && sensLDRR >= 100) return False;

	else
		return True;
}


/*!
 * Gibt aus, ob der Bot eine fuer sein Slalomverhalten geeignete Saeule vor sich hat.
 * @return True, wenn er eine solche Saeule vor sich hat, sonst False.
 */
static int8_t is_good_pillar_ahead(void) {
	if(is_obstacle_ahead(COL_NEAR) != False && sensLDRL < 600 && sensLDRR < 600) return True;
	else return False;
}

/*!
 * Das Verhalten verhindert, dass dem Bot boese Dinge wie Kollisionen oder Abstuerze widerfahren.
 * Moegliche Erweiterung: Parameter einfuegen, der dem Verhalten vorschlaegt, wie zu reagieren ist.
 * @return Bestand Handlungsbedarf? True, wenn das Verhalten ausweichen musste, sonst False.
 */
static int8_t bot_avoid_harm(void) {
	if(is_obstacle_ahead(COL_CLOSEST) != False || sensBorderL > BORDER_DANGEROUS || sensBorderR > BORDER_DANGEROUS) {
		speedWishLeft = -BOT_SPEED_NORMAL;
		speedWishRight = -BOT_SPEED_NORMAL;
		return True;
	} else return False;
}

static int8_t explore_curve = 0;	/*!< Kurve des explore-Verhaltens */
static int8_t explore_state = 0;	/*!< Status des explore-Verhaltens */
static int8_t explore_running_curve = 0;	/*!< Kurvenfahrt bei explore-Verhalten */

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
 *
 * @param *data	Verhaltensdatensatz
 */
void bot_explore_behaviour(Behaviour_t * data) {

	if ((*exploration_check_function)())
		return_from_behaviour(data);

	switch (explore_state) {
	// Volle Fahrt voraus, bis ein Hindernis erreicht ist.
	case EXPLORATION_STATE_GOTO_WALL:
		// Der Bot steht jetzt vor einem Hindernis und soll sich nach rechts drehen
		if (bot_avoid_harm()) {
			explore_state = EXPLORATION_STATE_TURN_PARALLEL_RIGHT;
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
			deactivateBehaviour(bot_avoid_col_behaviour);
#endif
		}
		// Es befindet sich kein Hindernis direkt vor dem Bot.
		else {
			if (sensDistL < COL_NEAR || sensDistR < COL_NEAR) {
				bot_drive(0, BOT_SPEED_FAST);
			} else {
				bot_drive(0, BOT_SPEED_MAX);
			}
		}
		break;
		// Nach links drehen, bis der Bot parallel zum Hindernis auf der rechten Seite steht.
		/* Moegliche Erweiterung: Entwickle ein Verhalten, das auch bei Loechern funktioniert.
		 * Tipp dazu: Drehe den Roboter auf das Loch zu, bis beide Bodensensoren das Loch 'sehen'. Anschliessend drehe den Bot um 90 Grad.
		 * Es ist noetig, neue Zustaende zu definieren, die diese Zwischenschritte beschreiben. */
	case EXPLORATION_STATE_TURN_PARALLEL_LEFT:
		if (sensDistR < COL_FAR) {
			// Volle Drehung nach links mit ca. 3Grad/10ms
			bot_drive(-127, BOT_SPEED_FAST);
		} else {
			// Nachdem das Hindernis nicht mehr in Sicht ist, dreht der Bot noch ca. 3 Grad weiter.
			// Im Zweifelsfall dreht das den Bot zu weit, aber das ist besser, als ihn zu kurz zu drehen.
			bot_drive(-127, BOT_SPEED_FAST);
			explore_state = EXPLORATION_STATE_DRIVE_PARALLEL_RIGHT;
		}
		break;
		// Nach rechts drehen, bis der Bot parallel zum Hindernis auf der linken Seite steht.
		/* Moegliche Erweiterung: siehe EXPLORATION_STATE_TURN_PARALLEL_LEFT */
	case EXPLORATION_STATE_TURN_PARALLEL_RIGHT:
		if (sensDistL < COL_FAR) {
			// Volle Drehung nach rechts mit ca. 3Grad/10ms
			bot_drive(127, BOT_SPEED_FAST);
		} else {
			/* Nachdem das Hindernis nicht mehr in Sicht ist, dreht der Bot noch ca. 3 Grad weiter.
			 * Im Zweifelsfall dreht das den Bot zu weit, aber das ist besser, als ihn zu kurz zu drehen. */
			bot_drive(127, BOT_SPEED_FAST);
			explore_state = EXPLORATION_STATE_DRIVE_PARALLEL_LEFT;
		}
		break;
	case EXPLORATION_STATE_DRIVE_PARALLEL_LEFT:
		bot_drive_distance(data,0,BOT_SPEED_FAST,15);
		explore_state = EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT;
		break;
	case EXPLORATION_STATE_DRIVE_PARALLEL_RIGHT:
		bot_drive_distance(data,0,BOT_SPEED_FAST,15);
		explore_state = EXPLORATION_STATE_TURN_ORTHOGONAL_LEFT;
		break;
	case EXPLORATION_STATE_TURN_ORTHOGONAL_LEFT:
		// Drehe den Bot um 90 Grad nach links.
		/* Da der Bot sich immer ein bisschen zu weit von der Wand weg dreht, soll er sich
		 * hier nur um 85 Grad drehen. Nicht schoen, aber klappt.*/
		bot_turn(data, 85);
		explore_state = EXPLORATION_STATE_DRIVE_ARC;
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
		activateBehaviour(NULL, bot_avoid_col_behaviour);
#endif
		break;
	case EXPLORATION_STATE_TURN_ORTHOGONAL_RIGHT:
		// Drehe den Bot um 90 Grad nach rechts.
		/* Da der Bot sich immer ein bisschen zu weit von der Wand weg dreht, soll er sich
		 * hier nur um 85 Grad drehen. Nicht schoen, aber klappt.*/
		bot_turn(data, -85);
		explore_state = EXPLORATION_STATE_DRIVE_ARC;
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
		activateBehaviour(NULL, bot_avoid_col_behaviour);
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
		if (explore_curve == 0) {
			explore_curve = 25;
			explore_running_curve = True;
		} else if (explore_running_curve == False) {
			explore_curve = (int8_t) (explore_curve * -0.9f);
			explore_running_curve = True;
		}
		/* Sobald der Bot auf ein Hindernis stoesst, wird der naechste Zyklus eingeleitet.
		 * Auf einen Rechtsbogen (curve > 0) folgt eine Linksdrehung und auf einen Linksbogen eine Rechtsdrehung.
		 * Wenn der Wert von curve (durch Rundungsfehler bei int) auf 0 faellt, beginnt das Suchverhalten erneut.*/
		if (bot_avoid_harm()) {
			explore_state
					= (int8_t) (explore_curve > 0 ? EXPLORATION_STATE_TURN_PARALLEL_LEFT
							: EXPLORATION_STATE_TURN_PARALLEL_RIGHT);
			explore_running_curve = False;
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
			deactivateBehaviour(bot_avoid_col_behaviour);
#endif
		} else {
			bot_drive(explore_curve, BOT_SPEED_MAX);
		}
		break;
	default:
		explore_state = EXPLORATION_STATE_GOTO_WALL;
		explore_curve = 0;
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
		activateBehaviour(NULL, bot_avoid_col_behaviour);
#endif
	}
}

/*!
 * Aktiviert bot_explore_behaviour.
 * @param *caller	Verhaltensdatensatz des Aufrufers
 * @param *check	Bewertungsfunktion fuer Suche
 */
static void bot_explore(Behaviour_t * caller, int8_t (* check)(void)) {
	switch_to_behaviour(caller, bot_explore_behaviour, BEHAVIOUR_NOOVERRIDE);
	exploration_check_function = check;
	explore_state = EXPLORATION_STATE_GOTO_WALL;
	explore_curve = 0;
	explore_running_curve = False;
}

static int8_t slalom_state = 0;
static int8_t slalom_orientation = 0;
static int8_t slalom_sweep_state = 0;
static int8_t slalom_sweep_steps = 0;

/*!
 * Das Verhalten laesst den Bot einen Slalom fahren.
 * @see bot_do_slalom()
 * @param *data	Verhaltensdatensatz
 */
void bot_do_slalom_behaviour(Behaviour_t * data) {
	int16_t turn;
	int8_t curve;

	switch (slalom_state) {
	case SLALOM_STATE_CHECK_PILLAR:
		// Der Bot sollte jetzt Licht sehen koennen...
		if (check_for_light()) {
			// Wenn der Bot direkt vor der Saeule steht, kann der Slalom anfangen, sonst zum Licht fahren
			if (bot_avoid_harm()) {
				slalom_state = SLALOM_STATE_START;
			} else
				bot_goto_light();
		} else {// ... sonst muss er den Slalom-Kurs neu suchen.
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
			activateBehaviour(NULL, bot_avoid_col_behaviour);
#endif
			return_from_behaviour(data);
		}
		break;
	case SLALOM_STATE_START:
		// Hier ist Platz fuer weitere Vorbereitungen, falls noetig.
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
		deactivateBehaviour(bot_avoid_col_behaviour);
#endif
		slalom_state = SLALOM_STATE_TURN_1;
		CASE_NO_BREAK;
	case SLALOM_STATE_TURN_1:
		turn = (slalom_orientation == SLALOM_ORIENTATION_LEFT) ? 90 : -90;
		bot_turn(data, turn);
		slalom_state = SLALOM_STATE_DRIVE_ARC;
		break;
	case SLALOM_STATE_DRIVE_ARC:
		// Nicht wundern: Bei einem Links-Slalom faehrt der Bot eine Rechtskurve.
		curve = (int8_t) (slalom_orientation == SLALOM_ORIENTATION_LEFT ? 25 : -25);
		bot_drive_distance(data,curve,BOT_SPEED_FAST,20);
		slalom_state = SLALOM_STATE_TURN_2;
		break;
	case SLALOM_STATE_TURN_2:
		turn = (slalom_orientation == SLALOM_ORIENTATION_LEFT) ? 45 : -45;
		bot_turn(data, turn);
		slalom_state = SLALOM_STATE_SWEEP_RUNNING;
		break;
	case SLALOM_STATE_SWEEP_RUNNING:
		if (slalom_sweep_steps == 0) {
			slalom_sweep_state = SWEEP_STATE_CHECK;
		}
		// Insgesamt 6 Schritte drehen
		if (slalom_sweep_steps < 6) {
			if (slalom_sweep_state == SWEEP_STATE_CHECK) {
				// Phase 1: Pruefen, ob vor dem Bot eine gute Saeule ist
				if (is_good_pillar_ahead() == True) {
					// Wenn die Saeule gut ist, drauf zu und Slalom anders rum fahren.
					slalom_state = SLALOM_STATE_CHECK_PILLAR;
					slalom_orientation
							= (int8_t) (slalom_orientation == SLALOM_ORIENTATION_LEFT ? SLALOM_ORIENTATION_RIGHT
									: SLALOM_ORIENTATION_LEFT);
					slalom_sweep_steps = 0;
				} else {
					// Sonst drehen.
					slalom_sweep_state = SWEEP_STATE_TURN;
				}
			}
			if (slalom_sweep_state == SWEEP_STATE_TURN) {
				// Phase 2: Bot um 15 Grad drehen
				turn = (slalom_orientation == SLALOM_ORIENTATION_LEFT) ? 15
						: -15;
				bot_turn(data, turn);
				slalom_sweep_state = SWEEP_STATE_CHECK;
				slalom_sweep_steps++;
			}
		} else {
			turn = (slalom_orientation == SLALOM_ORIENTATION_LEFT) ? -90 : 90;
			bot_turn(data, turn);
			slalom_state = SLALOM_STATE_SWEEP_DONE;
			slalom_sweep_steps = 0;
		}
		break;
	case SLALOM_STATE_SWEEP_DONE:
		turn = (slalom_orientation == SLALOM_ORIENTATION_LEFT) ? -135 : 135;
		bot_turn(data, turn);
		slalom_state = SLALOM_STATE_CHECK_PILLAR;
		break;
	default:
		slalom_state = SLALOM_STATE_CHECK_PILLAR;
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
 *
 * @param *caller	Verhaltensdatensatz des Aufrufers
 */
void bot_do_slalom(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_do_slalom_behaviour, BEHAVIOUR_NOOVERRIDE);
	slalom_state = SLALOM_STATE_CHECK_PILLAR;
	slalom_orientation = SLALOM_ORIENTATION_RIGHT;
	slalom_sweep_state = 0;
	slalom_sweep_steps = 0;
}

/*!
 * Das Verhalten setzt sich aus 3 Teilverhalten zusammen:
 * Nach Licht suchen, auf das Licht zufahren, im Licht Slalom fahren.
 * @param *data	Verhaltensdatensatz
 */
void bot_olympic_behaviour(Behaviour_t * data) {
	if (check_for_light()) {
		/* Sobald der Bot auf ein Objekt-Hinderniss stoesst, versucht er, Slalom zu fahren.
		 * Aufgabe: Wenn der Bot vor einem Loch steht, hinter welchem sich die Lichtquelle
		 * befindet, wird er daran haengen bleiben. Schreibe ein Verhalten, dass das verhindert. */
		if (bot_avoid_harm() && is_obstacle_ahead(COL_NEAR)) {
			bot_do_slalom(data);
		} else
			bot_goto_light();
	} else
		bot_explore(data, check_for_light);
}

#endif // BEHAVIOUR_OLYMPIC_AVAILABLE
