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
 * @file 	bot-logik.c
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


#include "bot-logic/bot-logik.h"
#include "bot-logic/available_behaviours.h"

#ifdef BEHAVIOUR_AVAILABLE

#include "display.h"
#include "rc5.h"
#include "rc5-codes.h"
#include "ui/available_screens.h"
#include "timer.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>

//#define DEBUG_BOT_LOGIC		// Schalter um recht viel Debug-Code anzumachen

#ifndef DEBUG_BOT_LOGIC
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif

int16_t speedWishLeft;				/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit links */
int16_t speedWishRight;				/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit rechts */

#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
float factorWishLeft;				/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor links */
float factorWishRight;				/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor rechts */
#endif

int16_t target_speed_l = BOT_SPEED_STOP;	/*!< Sollgeschwindigkeit linker Motor - darum kuemmert sich bot_base() */
int16_t target_speed_r = BOT_SPEED_STOP;	/*!< Sollgeschwindigkeit rechter Motor - darum kuemmert sich bot_base() */


/*! Liste mit allen Verhalten */
static Behaviour_t * behaviour = NULL;

#define MAX_PROCS 3					/*!< Maximale Anzahl der registrierbaren Funktionen */
static int8_t count_arr_emerg = 0;	/*!< Anzahl der zurzeit registrierten Notfallfunktionen */
/*! hier liegen die Zeiger auf die auszufuehrenden Abgrund Notfall-Funktionen */
static void (* emerg_functions[MAX_PROCS])(void) = {NULL};

/*!
 * Routine zum Registrieren einer Notfallfunktion, die beim Ausloesen eines Abgrundsensors
 * aufgerufen wird; hierdurch kann ein Verhalten vom Abgrund benachrichtigt werden und
 * entsprechend dem Verhalten reagieren
 * @param *fkt	Die zu registrierende Routine, welche aufzurufen ist
 * @return 		Index, den die Routine im Array einnimmt, bei -1 ist alles voll
 */
static inline int8_t register_emergency_proc(void (* fkt)(void)) {
	if (count_arr_emerg == MAX_PROCS) {
		return -1;	// sorry, aber fuer dich ist kein Platz mehr da :(
	}
	int8_t proc_nr = count_arr_emerg++;	// neue Routine hinten anfuegen
	emerg_functions[proc_nr] = fkt;	// Pointer im Array speichern
	return proc_nr;
}

/*!
 * Beim Ausloesen eines Notfalls wird diese Routine angesprungen
 * und ruft alle registrierten Prozeduren der Reihe nach auf
 */
void start_registered_emergency_procs(void) {
	uint8_t i;
	for (i=0; i<MAX_PROCS; i++) {
		if (emerg_functions[i] != NULL) {
			emerg_functions[i]();
		}
	}
}

/*!
 * Das einfachste Grundverhalten
 * @param *data der Verhaltensdatensatz
 */
static void bot_base_behaviour(Behaviour_t * data) {
	data = data; // kein warning
	speedWishLeft = target_speed_l;
	speedWishRight = target_speed_r;
//	LOG_DEBUG("\tMaus:\t%d\tSpeed:\t%d", (int16)x_pos, target_speed_r);
//	LOG_DEBUG("\tDistL:\t%u\tDistR:\t%u", sensDistL, sensDistR);
//	LOG_DEBUG("\tTime:\t%lu", TIMER_GET_TICKCOUNT_32);
}

/*!
 * Erzeugt ein neues Verhalten
 * @param priority 	Die Prioritaet
 * @param *work 	Die Funktion, die sich drum kuemmert
 * @param active	Boolean, ob das Verhalten aktiv oder inaktiv erstellt wird
 */
static Behaviour_t * new_behaviour(uint8_t priority, void (* work) (struct _Behaviour_t * data), uint8_t active) {
	Behaviour_t * newbehaviour = (Behaviour_t *) malloc(sizeof(Behaviour_t));
	if (newbehaviour == NULL) {
		return NULL;
	}

	newbehaviour->priority = priority;

	bit_t tmp = {active};
	newbehaviour->active = tmp.bit;

	newbehaviour->next = NULL;
	newbehaviour->work = work;
	newbehaviour->caller = NULL;
	newbehaviour->subResult = SUBSUCCESS;
	return newbehaviour;
}

/*!
 * Fuegt ein Verhalten der Verhaltenliste anhand der Prioritaet ein.
 * @param list Die Speicherstelle an der die globale Verhaltensliste anfaengt
 * @param behave Einzufuegendes Verhalten
 */
static void insert_behaviour_to_list(Behaviour_t ** list, Behaviour_t * behave) {
	Behaviour_t	* ptr	= * list;
	Behaviour_t * temp;

	/* Kein Eintrag dabei? */
	if (behave == NULL) {
		return;
	}

	/* Erster Eintrag in der Liste? */
	if (ptr == NULL) {
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
			while (NULL != ptr->next) {
				if (ptr->next->priority < behave->priority) {
					break;
				}

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
void bot_behave_init(void) {
	#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
		// Dieses Verhalten kann andere starten
		insert_behaviour_to_list(&behaviour, new_behaviour(254, bot_remotecall_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_SERVO_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(253, bot_servo_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
		// Demo-Verhalten, ganz einfach, inaktiv
		// Achtung, im Moment hat es eine hoehere Prioritaet als die Gefahrenerkenner!!!
		insert_behaviour_to_list(&behaviour, new_behaviour(252, bot_simple_behaviour, INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(251, bot_simple2_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_SCAN_AVAILABLE
		// Verhalten, das die Umgebung des Bots on-the-fly beim Fahren scannt
	    insert_behaviour_to_list(&behaviour, new_behaviour(250, bot_scan_onthefly_behaviour, ACTIVE));
	#endif

	// Sehr hohe Prioritaet haben die Notfall Verhalten

	// Verhalten zum Schutz des Bots, hohe Prioritaet, aktiv
	#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(249, bot_avoid_border_behaviour, ACTIVE));
	#endif
	#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(248, bot_avoid_col_behaviour, ACTIVE));
	#endif
    #ifdef BEHAVIOUR_HANG_ON_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(245, bot_hang_on_behaviour, ACTIVE));
		// Registrierung des Handlers zur Behandlung des Haengenbleibens
 	    register_emergency_proc(&hang_on_handler);
	#endif

	#ifdef BEHAVIOUR_GET_UTILIZATION_AVAILABLE
 	    insert_behaviour_to_list(&behaviour, new_behaviour(244, bot_get_utilization_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_DELAY_AVAILABLE
		// Delay-Routine als Verhalten
		insert_behaviour_to_list(&behaviour, new_behaviour(200, bot_delay_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(190, bot_save_waypositions_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
		// Verhalten, das andere Verhalten abbricht, sobald eine Bedingung erfuellt ist
		insert_behaviour_to_list(&behaviour, new_behaviour(154, bot_cancel_behaviour_behaviour, INACTIVE));
	#endif

	// Alle Hilfsroutinen sind relativ wichtig, da sie auch von den Notverhalten her genutzt werden
	// Hilfsverhalten, die Befehle von Boten-Funktionen ausfuehren, erst inaktiv, werden von Boten aktiviert
	#ifdef BEHAVIOUR_TURN_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(150, bot_turn_behaviour, INACTIVE));
	#endif
	#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(149, bot_drive_distance_behaviour, INACTIVE));
	#endif
	#ifdef BEHAVIOUR_GOTO_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(148, bot_goto_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
		// Hilfsverhalten zum Anfahren von Positionen
		insert_behaviour_to_list(&behaviour, new_behaviour(147, bot_gotoxy_behaviour, INACTIVE));
	#endif
	#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(146, bot_goto_pos_behaviour, INACTIVE));
	#endif
	#ifdef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(145, bot_goto_obstacle_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(140, bot_measure_distance_behaviour, INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(139, bot_check_distance_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
		// Verhalten, um ein Labyrinth nach der Hoehlenforscher-Methode loesen
		insert_behaviour_to_list(&behaviour, new_behaviour(100, bot_solve_maze_behaviour, INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(90, bot_measure_angle_behaviour, INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(89, bot_check_wall_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_DRIVE_AREA_AVAILABLE
		// Registrierung des Handlers zur Behandlung des Abgrundes im Area-Verhalten
		register_emergency_proc(&border_drive_area_handler);
		// Observer bekomemn hohe Prio um noch vor goto_pos zum Zuge zu kommen;
		// links hoeher als rechts, damit rechts zuletzt Werte in den Stack schreibt
		insert_behaviour_to_list(&behaviour, new_behaviour(171, bot_observe_left_behaviour, INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(170, bot_observe_right_behaviour, INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(72, bot_drive_area_behaviour, INACTIVE));
    #endif

    #ifdef BEHAVIOUR_PATHPLANING_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(71, bot_calc_wave_behaviour, INACTIVE));
    #endif

	#ifdef BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE
	  	// erweiterter Linienfolge, der mit Unterbrechungen und Hindernissen klarkommt
		insert_behaviour_to_list(&behaviour, new_behaviour(71, bot_follow_line_enh_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
		// Verhalten um einer Linie zu folgen
		insert_behaviour_to_list(&behaviour, new_behaviour(70, bot_follow_line_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
		// Linie folgen ueber Kreuzungen hinweg zum Ziel, kuerzester Weg befindet sich danach im Stack
		insert_behaviour_to_list(&behaviour, new_behaviour(69, bot_line_shortest_way_behaviour, INACTIVE));
		// Ueberwacherverhalten auf Fahren in entgegengesetzte Richtung bekommt hohe Prio, um vor bot_turn zu kommen
		insert_behaviour_to_list(&behaviour, new_behaviour(169, bot_check_reverse_direction_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
		// unwichtigere Hilfsverhalten
		insert_behaviour_to_list(&behaviour, new_behaviour(55, bot_explore_behaviour, INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(54, bot_do_slalom_behaviour, INACTIVE));
		// Demo-Verhalten fuer aufwendiges System, inaktiv
		insert_behaviour_to_list(&behaviour, new_behaviour(60, bot_olympic_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
		// Demo-Verhalten, etwas komplexer, inaktiv
		insert_behaviour_to_list(&behaviour, new_behaviour(51, bot_drive_square_behaviour, INACTIVE));
	#endif

    #ifdef BEHAVIOUR_FOLLOW_WALL_AVAILABLE
 	    // Explorer-Verhalten um einer Wand zu folgen
 	    insert_behaviour_to_list(&behaviour, new_behaviour(48, bot_follow_wall_behaviour, INACTIVE));
 	    // Registrierung zur Behandlung des Notfallverhaltens zum R ueckwaertsfahren
 	    register_emergency_proc(&border_follow_wall_handler);
	#endif

	#ifdef BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE
 	    insert_behaviour_to_list(&behaviour, new_behaviour(45, bot_classify_objects_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
 	    insert_behaviour_to_list(&behaviour, new_behaviour(44, bot_catch_pillar_behaviour, INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(43, bot_unload_pillar_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(41, bot_transport_pillar_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(40, bot_follow_object_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(33, bot_drive_stack_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_SCAN_BEACONS_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(32, bot_scan_beacons_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(30, bot_calibrate_pid_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(29, bot_calibrate_sharps_behaviour, INACTIVE));
	#endif

	#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(28, bot_turn_test_behaviour, INACTIVE));
	#endif

	// Grundverhalten, setzt aeltere FB-Befehle um, aktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(2, bot_base_behaviour, ACTIVE));

	#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
		// Um das Simple2-Behaviour zu nutzen, die Kommentarzeichen der folgenden beiden Zeilen tauschen
		activateBehaviour(NULL, bot_simple_behaviour);
		//activateBehaviour(bot_simple2_behaviour);
	#endif
}

/*!
 * Liefert das Verhalten zurueck, welches durch function implementiert ist
 * @param function	Die Funktion, die das Verhalten realisiert
 * @return			Zeiger auf Verhaltensdatensatz oder NULL
 */
Behaviour_t * get_behaviour(BehaviourFunc function) {
	Behaviour_t * job;	// Zeiger auf ein Verhalten

	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben
	for (job = behaviour; job; job = job->next) {
		if (job->work == function) {
			return job;
		}
	}
	return NULL;
}

/*!
 * Zu ein Verhalten mit der gegebenen Prioritaet
 * @param prio	Prioritaet des gesuchten Verhaltens
 * @return		Zeiger auf Verhaltensdatensatz oder NULL
 */
Behaviour_t * get_behaviour_from_prio(uint8_t prio) {
	Behaviour_t * job;	// Zeiger auf ein Verhalten

	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben
	for (job = behaviour; job; job = job->next) {
		if (job->priority == prio) {
			return job;
		}
	}
	return NULL;
}

/*!
 * Deaktiviert eine Regel mit gegebener Funktion
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void deactivateBehaviour(BehaviourFunc function) {
	Behaviour_t * job = get_behaviour(function);
	if (job == NULL) {
		return;
	}
	job->active = INACTIVE;
	job->caller = NULL;	// Caller loeschen, damit Verhalten auch ohne OVERRIDE neu gestartet werden koennen
#ifdef DEBUG_BOT_LOGIC
	if (behaviour_is_activated(function) == 1) {
		LOG_DEBUG("Verhalten %u wird deaktiviert", job->priority);
	}
#endif
}

/*!
 * Rueckgabe von True, wenn das Verhalten gerade laeuft (aktiv ist) sonst False
 * @param function Die Funktion, die das Verhalten realisiert.
 * @return True wenn Verhalten aktiv sonst False
 */
uint8_t behaviour_is_activated(BehaviourFunc function) {
	Behaviour_t * job = get_behaviour(function);
	if (job == NULL) {
		return False;
	}

	return job->active;
}


/*!
 * liefert !=0 zurueck, wenn function ueber eine beliebige Kette (job->caller->caller ....) von anderen Verhalten job aufgerufen hat
 * @param *job		Zeiger auf den Datensatz des aufgerufenen Verhaltens
 * @param function	Das Verhalten, das urspruenglich aufgerufen hat
 * @return 			0 wenn keine Call-Abhaengigkeit besteht, ansonsten die Anzahl der Stufen
 */
static uint8_t isInCallHierarchy(Behaviour_t * job, BehaviourFunc function) {
	uint8_t level = 0;

	if (job == NULL) {
		LOG_DEBUG("kein Verhaltensdatensatz gegeben");
		return 0;	// Liste ist leer
	}

	for (; job->caller; job=job->caller) {
		LOG_DEBUG("  ueberpruefe Verhalten %u mit caller %u...", job->priority, job->caller->priority);
		level++;
		if (job->caller->work == function) {
			LOG_DEBUG("   Verhalten %u wurde direkt von %u aufgerufen", job->priority, job->caller->priority);
			return level;	// Direkter Aufrufer in Tiefe level gefunden
		}
	}
	return 0;	// function kommt in Caller-Liste von job nicht vor
}	// O(n), n:=|Caller-Liste|

/*!
 * Deaktiviert alle von diesem Verhalten aufgerufenen Verhalten.
 * Das Verhalten selbst bleibt aktiv und bekommt ein SUBCANCEL in seine Datanestruktur eingetragen.
 * @param function	Die Funktion, die das Verhalten realisiert.
 */
void deactivateCalledBehaviours(BehaviourFunc function) {
	Behaviour_t * job;	// Zeiger auf ein Verhalten

	LOG_DEBUG("");	// new line
	LOG_DEBUG("Callees von Verhalten %u sollen abgeschaltet werden.", get_behaviour(function)->priority);
	LOG_DEBUG("Beginne mit dem Durchsuchen der Liste");
	// Einmal durch die Liste gehen, und alle aktiven Funktionen pruefen, ob sie von dem uebergebenen Verhalten aktiviert wurden
	uint8_t i = 0;
	Behaviour_t * beh_of_function = NULL;
	for (job=behaviour; job; job=job->next) {	// n mal
		if (job->active == ACTIVE) {
			i++;
			LOG_DEBUG("Verhalten mit Prio = %u ist ACTIVE, Durchlauf %u", job->priority, i);
			uint8_t level = isInCallHierarchy(job, function);	// O(n)
			LOG_DEBUG(" und hat Level %u Call-Abhaengigkeit", level);
			/* die komplette Caller-Liste (aber auch nur die) abschalten */
			Behaviour_t * ptr = job;
			for (; level>0; level--) {	// n mal
				Behaviour_t * beh;
				for (beh=behaviour; beh; beh=beh->next) {	// n mal
					/* Falls das Verhalten Caller eines anderen Verhaltens ist, duerfen wir es (noch) nicht deaktivieren! */
					if (beh->caller == ptr) {
						LOG_DEBUG("  Verhalten %u ist Caller eines anderen Verhaltens", ptr->priority);
						break;
					}
				}	// O(n)
				Behaviour_t * tmp = ptr;
				ptr = ptr->caller;	// zur naechsten Ebene
				if (beh == NULL) {
					LOG_DEBUG("  Verhalten %u wird in Tiefe %u abgeschaltet", tmp->priority, level);
					tmp->active = INACTIVE;	// callee abschalten
					tmp->caller = NULL;	// Caller loeschen, damit Verhalten auch ohne OVERRIDE neu gestartet werden koennen
				}
			}	// O(n^2)
			LOG_DEBUG("");	// new line
		}
		if (job->work == function) {
			/* Verhalten von function fuer spaeter merken, wenn wir hier eh schon die ganze Liste absuchen */
			beh_of_function = job;
		}
	}	// O(n^3)
	/* Verhaltenseintrag zu function benachrichtigen und wieder aktiv schalten */
	if (beh_of_function != NULL) {
		LOG_DEBUG("Verhalten %u wird aktiviert", beh_of_function->priority);
		beh_of_function->subResult = SUBCANCEL;	// externer Abbruch
		beh_of_function->active = ACTIVE;
	}
}	// O(n^3)

/*!
 * Ruft ein anderes Verhalten auf und merkt sich den Ruecksprung
 * return_from_behaviour() kehrt dann spaeter wieder zum aufrufenden Verhalten zurueck
 * @param *from		aufrufendes Verhalten
 * @param *to		aufgerufenes Verhalten
 * @param mode		Hier sind vier Werte moeglich:
 * 		1. OVERRIDE:	Das Zielverhalten to wird aktiviert, auch wenn es noch aktiv ist.
 *						Das Verhalten, das es zuletzt aufgerufen hat wird dadurch automatisch
 *						wieder aktiv und muss selbst sein eigenes Feld subResult auswerten, um zu pruefen, ob das
 *						gewuenschte Ziel erreicht wurde, oder vorher ein Abbruch stattgefunden hat.
 * 		2. NOOVERRIDE:	Das Zielverhalten wird nur aktiviert, wenn es gerade nichts zu tun hat.
 *						In diesem Fall kann der Aufrufer aus seinem eigenen subResult auslesen,
 *						ob seinem Wunsch Folge geleistet wurde.
 *		3. FOREGROUND	Das Verhalten laeuft im Fordergrund (Aufrufer wird solange deaktiviert)
 *		4. BACKGROUND	Das Verhalten laeuft im Hintergrund (Aufrufer bleibt aktiv)
 */
void switch_to_behaviour(Behaviour_t * from, void (*to)(Behaviour_t *), uint8_t mode) {
	LOG_DEBUG("switch_to_behaviour(0x%lx, 0x%lx, %u)", (size_t) from, (size_t) to, mode);
	Behaviour_t * job = get_behaviour(to);
	if (job == NULL) {
		/* Zielverhalten existiert gar nicht */
		if (from) {
			from->subResult = SUBFAIL;
		}
		return;
	}

	bit_t tmp;
	behaviour_mode_t beh_mode;
	tmp.byte = (uint8_t) (mode & 1);
	beh_mode.override = tmp.bit;
	tmp.byte = (uint8_t) ((mode & 2) >> 1);
	beh_mode.background = tmp.bit;

	if (job->caller) {		// Ist das auzurufende Verhalten noch beschaeftigt?
		if (beh_mode.override == NOOVERRIDE) {	// nicht ueberschreiben, sofortige Rueckkehr
			if (from) {
				from->subResult = SUBFAIL;
			}
			return;
		}
		// Wir wollen also ueberschreiben, aber nett zum alten Aufrufer sein und ihn darueber benachrichtigen
		job->caller->active = ACTIVE;	// alten Aufrufer reaktivieren
		job->caller->subResult = SUBFAIL;	// er bekam aber nicht das gewuenschte Resultat
	}

	if (from) {
		if (beh_mode.background == 0) {
			// laufendes Verhalten abschalten
			from->active = INACTIVE;
			from->subResult = SUBRUNNING;
		} else {
			from->subResult = SUBBACKGR;
		}
	}

	// neues Verhalten aktivieren
	job->active = ACTIVE;
	// Aufrufer sichern
	job->caller = from;

#ifdef DEBUG_BOT_LOGIC
	if (from) {
		LOG_DEBUG("Verhaltenscall: %u wurde von %u aufgerufen", job->priority, from->priority);
	} else {
		LOG_DEBUG("Verhaltenscall: %u wurde direkt aufgerufen", job->priority);
	}
	if (beh_mode.background == 1) {
		LOG_DEBUG(" Verhalten laeuft im Hintergrund");
	}
#endif
}

/*!
 * Kehrt zum aufrufenden Verhalten zurueck und setzt den Status auf Erfolg oder Misserfolg
 * @param *data	laufendes Verhalten
 * @param state	Abschlussstatus des Verhaltens (SUBSUCCESS oder SUBFAIL)
 */
void exit_behaviour(Behaviour_t * data, uint8_t state) {
	LOG_DEBUG("exit_behaviour(0x%lx, %u)", (size_t)data, state);
	data->active = INACTIVE;	 				// Unterverhalten deaktivieren
	LOG_DEBUG("Verhalten %u wurde beendet", data->priority);
	if (data->caller) {
		data->caller->active = ACTIVE; 		// aufrufendes Verhalten aktivieren

		union {
			uint8_t byte;
			unsigned bits:3;
		} tmp = {state};
		data->caller->subResult = tmp.bits;	// Status beim Aufrufer speichern

		LOG_DEBUG("Caller %u wurde wieder aktiviert", data->caller->priority);
	}
	data->caller = NULL;	// Job erledigt, Verweis loeschen
}

/*!
 * Deaktiviert alle Verhalten bis auf Grundverhalten.
 */
void deactivateAllBehaviours(void) {
	Behaviour_t * job;	// Zeiger auf ein Verhalten
	// Einmal durch die Liste gehen und (fast) alle deaktivieren, Grundverhalten nicht
	for (job = behaviour; job; job = job->next) {
		if ((job->priority >= PRIO_VISIBLE_MIN) && (job->priority <= PRIO_VISIBLE_MAX)) {
            // Verhalten deaktivieren
			LOG_DEBUG("Verhalten %u wird deaktiviert", job->priority);
			job->active = INACTIVE;
			job->caller = NULL;	// Caller loeschen, damit Verhalten auch ohne OVERRIDE neu gestartet werden koennen
		}
	}
}

/*!
 * Zentrale Verhaltens-Routine, wird regelmaessig aufgerufen.
 */
void bot_behave(void) {
	Behaviour_t * job;	// Zeiger auf ein Verhalten

#ifdef RC5_AVAILABLE
	rc5_control();	// Abfrage der IR-Fernbedienung
#endif

#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
	float factorLeft = 1.0;		// Puffer fuer Modifkatoren
	float factorRight = 1.0;	// Puffer fuer Modifkatoren
#endif

	/* Solange noch Verhalten in der Liste sind...
	   (Achtung: Wir werten die Jobs sortiert nach Prioritaet aus. Wichtige zuerst einsortieren!!!) */
	for (job = behaviour; job; job = job->next) {
		if (job->active) {
			/* WunschVariablen initialisieren */
			speedWishLeft = BOT_SPEED_IGNORE;
			speedWishRight = BOT_SPEED_IGNORE;

#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
			factorWishLeft = 1.0;
			factorWishRight = 1.0;
#endif

			if (job->work) {	// hat das Verhalten eine Work-Routine
				job->work(job);	// Verhalten ausfuehren
			} else {	// wenn nicht: Verhalten deaktivieren, da es nicht sinnvoll arbeiten kann
				job->active = INACTIVE;
			}

#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
			/* Modifikatoren sammeln  */
			factorLeft  *= factorWishLeft;
			factorRight *= factorWishRight;
#endif

           /* Geschwindigkeit aendern? */
			if ((speedWishLeft != BOT_SPEED_IGNORE) || (speedWishRight != BOT_SPEED_IGNORE)) {
#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
				if (speedWishLeft != BOT_SPEED_IGNORE)
					speedWishLeft = (int16_t) (speedWishLeft * factorLeft);
				if (speedWishRight != BOT_SPEED_IGNORE)
					speedWishRight = (int16_t) (speedWishRight * factorRight);
#endif

				motor_set(speedWishLeft, speedWishRight);
				break;	// Wenn ein Verhalten Werte direkt setzen will, nicht weitermachen
			}
		}
		/* Dieser Punkt wird nur erreicht, wenn keine Regel im System die Motoren beeinflusen will */
		if (job->next == NULL) {
			motor_set(BOT_SPEED_IGNORE, BOT_SPEED_IGNORE);
		}
	}
}

/*!
 * Gibt das naechste Verhalten der Liste zurueck
 * @param *beh	Zeiger auf Verhalten, dessen Nachfolger gewuenscht ist, NULL fuer Listenanfang
 * @return		Zeiger auf Nachfolger von beh
 */
Behaviour_t * get_next_behaviour(Behaviour_t * beh) {
	if (beh == NULL) {
		return behaviour;
	} else {
		return beh->next;
	}
}

#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
/*!
 * @brief		Behandelt die Tasten fuer die Verhaltensanezeige, die das jeweilige Verhalten aktivieren oder deaktivieren.
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		14.02.2007
 * @param data	Zeiger auf ein Array mit Verhaltensdatensatzzeigern
 */
static void beh_disp_key_handler(Behaviour_t ** data){
	Behaviour_t * callee = NULL;
	/* Keyhandling um Verhalten ein- / auszuschalten */
	switch (RC5_Code) {
		case RC5_CODE_1: callee = data[0]; break;
		case RC5_CODE_2: callee = data[1]; break;
		case RC5_CODE_3: callee = data[2]; break;
		case RC5_CODE_4: callee = data[3]; break;
		case RC5_CODE_5: callee = data[4]; break;
		case RC5_CODE_6: callee = data[5]; break;
		case RC5_CODE_7: callee = data[6]; break;
		case RC5_CODE_8: callee = data[7]; break;
	}
	/* Verhaltensstatus toggeln */
	if (callee != NULL) {
		RC5_Code = 0;
		callee->active ^= 1;
	}
}

/*!
 * @brief	Zeigt Informationen ueber Verhalten an, 'A' fuer Verhalten aktiv, 'I' fuer Verhalten inaktiv.
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007
 * Es werden zwei Spalten mit jeweils 4 Verhalten angezeigt. Gibt es mehr Verhalten in der Liste, kommt man
 * mit der Taste DOWN auf eine weitere Seite (die aber kein extra Screen ist). Mit der Taste UP geht's bei Bedarf
 * wieder zurueck. Vor den Prioritaeten steht eine Nummer von 1 bis 8, drueckt man die entsprechende Zifferntaste
 * auf der Fernbedienung, so wird das Verhalten aktiv oder inaktiv geschaltet, komplementaer zum aktuellen Status.
 * Den Keyhandler dazu stellt beh_disp_key_handler() dar.
 */
void behaviour_display(void) {
	static uint8 behaviour_page = 0;	/*!< zuletzt angezeigte Verhaltensseite */
	if (RC5_Code == RC5_CODE_DOWN) {
		/* naechste Seite */
		behaviour_page++;
		display_clear();	// Screen-Nr. wechselt nicht => Screen selbst loeschen
		RC5_Code = 0;	// Taste behandelt
	} else if (RC5_Code == RC5_CODE_UP) {
		/* vorherige Seite */
		if (behaviour_page > 0) behaviour_page--;
		display_clear();
		RC5_Code = 0;
	}
	Behaviour_t* behaviours[8] = {NULL};	/*!< speichert Zeiger auf die Verhalten fuer den Keyhandler zwischen */
	uint8 i,j,k=0;
	Behaviour_t* ptr = behaviour;
	while (ptr != NULL && ptr->priority > PRIO_VISIBLE_MAX)
		ptr = ptr->next;	// alles ausserhalb der Sichtbarkeit ueberspringen
	/* Verhalten auf vorherigen Seiten ueberspringen */
	if (behaviour_page > 0) {
		for (i=1; i<=(behaviour_page<<3); i++) {	// 8 Verhalten pro Seite
			ptr = ptr->next;
			while (ptr != NULL && ptr->priority > PRIO_VISIBLE_MAX)
				ptr = ptr->next;	// alles ausserhalb der Sichtbarkeit ueberspringen
			if (ptr == NULL) {
				behaviour_page--;	// kein Verhalten mehr da => beim naechsten Aufruf stimmt's so aber wieder
				return;
			}
		}
	}
	char status[2] = "IA";	// I: inactive, A: active
	/* max. 4 Zeilen mit jeweils 2 Verhalten (= 8 Verhalten) anzeigbar */
	for (i=1; i<=20; i+=11) {	// Spalten
		for (j=1; j<=4; j++) {	// Zeilen
			while (ptr != NULL && ptr->priority > PRIO_VISIBLE_MAX)
				ptr = ptr->next;	// alles ausserhalb der Sichtbarkeit ueberspringen
			if (ptr == NULL || ptr->priority < PRIO_VISIBLE_MIN) {
				if (i==1 && j==1 && behaviour_page > 0) behaviour_page--;	// keine unnoetige leere Seite anzeigen
				if (RC5_Code !=0) beh_disp_key_handler(behaviours);	// Tasten auswerten
				return; // fertig, da ptr == NULL oder Prioritaet bereits zu klein
			}
			/* Ausgabe */
			display_cursor(j, i);
			display_printf("%u: %3d=%c ", k+1, ptr->priority, status[ptr->active]);
			behaviours[k++] = ptr;	// speichern fuer Tastenhandler
			ptr = ptr->next;
		}
	}
	if (RC5_Code !=0) beh_disp_key_handler(behaviours);	// Tasten auswerten
}
#endif	// DISPLAY_BEHAVIOUR_AVAILABLE
#endif	// BEHAVIOUR_AVAILABLE
