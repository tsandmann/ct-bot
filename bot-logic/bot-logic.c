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

/**
 * \file 	bot-logic.c
 * \brief 	High-Level Routinen fuer die Steuerung des c't-Bots.
 *
 * bot_behave() arbeitet eine Liste von Verhalten ab.
 * Jedes Verhalten kann entweder absolute Werte setzen, dann kommen niedrigerpriorisierte nicht mehr dran.
 * Alternativ dazu kann es Modifikatoren aufstellen, die bei niedriger priorisierten angewendet werden.
 * bot_behave_init() baut diese Liste auf.
 * Jede Verhaltensfunktion bekommt einen Verhaltensdatensatz uebergeben, in den sie ihre Daten eintraegt
 *
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author 	Christoph Grimmer (c.grimmer@futurio.de)
 * \date 	01.12.2005
 */


#include "bot-logic.h"

#ifdef BEHAVIOUR_AVAILABLE
#include "display.h"
#include "rc5.h"
#include "rc5-codes.h"
#include "ui/available_screens.h"
#include "timer.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>


//#define DEBUG_BOT_LOGIC	// Schalter um recht viel Debug-Code anzumachen

#ifndef DEBUG_BOT_LOGIC
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif

/* Verhaltenssteuerung, Verhalten mit Prioritaet zwischen MIN und MAX werden mit Notaus-Taste deaktiviert */
#define BEHAVIOUR_PRIO_MIN	3	/**< Prioritaet, die ein Verhalten mindestens haben muss, um deaktiviert zu werden */
#define BEHAVIOUR_PRIO_MAX	200	/**< Prioritaet, die ein Verhalten hoechstens haben darf, um deaktiviert zu werden */

static Behaviour_t * behaviour = NULL; /**< Liste mit allen Verhalten */
int16_t target_speed_l = BOT_SPEED_STOP; /**< Sollgeschwindigkeit linker Motor - darum kuemmert sich bot_base() */
int16_t target_speed_r = BOT_SPEED_STOP; /**< Sollgeschwindigkeit rechter Motor - darum kuemmert sich bot_base() */
int16_t speedWishLeft;	/**< Puffervariable fuer die Verhaltensfunktionen absolute Geschwindigkeit links */
int16_t speedWishRight;	/**< Puffervariable fuer die Verhaltensfunktionen absolute Geschwindigkeit rechts */
#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
float factorWishLeft;	/**< Puffervariable fuer die Verhaltensfunktionen Modifikationsfaktor links */
float factorWishRight;	/**< Puffervariable fuer die Verhaltensfunktionen Modifikationsfaktor rechts */
#endif // BEHAVIOUR_FACTOR_WISH_AVAILABLE

/* Forward-Deklarationen */
static void insert_behaviour_to_list(Behaviour_t * * list, Behaviour_t * behave);
static Behaviour_t * new_behaviour(uint8_t priority, void (* work) (struct _Behaviour_t * data), uint8_t active);
static void bot_base_behaviour(Behaviour_t * data);
int8_t register_emergency_proc(void (* fkt)(void));


/**
 * Initialisiert alle Verhalten
 * \todo Doku (Kommantare innerhalb der Funktion) ueberarbeiten!
 */
void bot_behave_init(void) {

#ifdef BEHAVIOUR_PROTOTYPE_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(102, bot_prototype_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
	// Dieses Verhalten kann andere Verhalten starten
	insert_behaviour_to_list(&behaviour, new_behaviour(254, bot_remotecall_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_SERVO_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(253, bot_servo_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
	/* Demo-Verhalten, ganz einfach, bot_simple wird unten sofort aktiv geschaltet.
	 * Achtung, im Moment hat es eine hoehere Prioritaet als die Gefahrenerkenner! */
	insert_behaviour_to_list(&behaviour, new_behaviour(252, bot_simple_behaviour, BEHAVIOUR_INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(251, bot_simple2_behaviour, BEHAVIOUR_INACTIVE));

	// Um das Simple2-Behaviour zu nutzen, die Kommentarzeichen der folgenden beiden Zeilen tauschen
	activateBehaviour(NULL, bot_simple_behaviour);
	//activateBehaviour(NULL, bot_simple2_behaviour);
#endif

#ifdef BEHAVIOUR_SCAN_AVAILABLE
	// Verhalten, das die Umgebung des Bots on-the-fly beim Fahren scannt
	insert_behaviour_to_list(&behaviour, new_behaviour(250, bot_scan_onthefly_behaviour, BEHAVIOUR_ACTIVE));
#endif

	/* Notfall Verhalten zum Schutz des Bots, hohe Prioritaet, sofort aktiv */
#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(249, bot_avoid_border_behaviour, BEHAVIOUR_ACTIVE));
#endif
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(248, bot_avoid_col_behaviour, BEHAVIOUR_ACTIVE));
#endif
#ifdef BEHAVIOUR_HANG_ON_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(245, bot_hang_on_behaviour, BEHAVIOUR_ACTIVE));
	// Registrierung des Handlers zur Behandlung des Haengenbleibens
	register_emergency_proc(&hang_on_handler);
#endif

#ifdef BEHAVIOUR_GET_UTILIZATION_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(244, bot_get_utilization_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_DELAY_AVAILABLE
	// Delay-Routine als Verhalten
	insert_behaviour_to_list(&behaviour, new_behaviour(200, bot_delay_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(190, bot_save_waypos_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_CANCEL_BEHAVIOUR_AVAILABLE
	// Verhalten, das andere Verhalten abbricht, sobald eine Bedingung erfuellt ist
	insert_behaviour_to_list(&behaviour, new_behaviour(154, bot_behaviour_cancel_behaviour, BEHAVIOUR_INACTIVE));
#endif

	// Alle Hilfsroutinen sind relativ wichtig, da sie auch von den Notverhalten her genutzt werden
	// Hilfsverhalten, die Befehle von Boten-Funktionen ausfuehren, erstmal inaktiv, werden von Boten aktiviert
#ifdef BEHAVIOUR_TURN_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(150, bot_turn_behaviour, BEHAVIOUR_INACTIVE));
#endif
#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(149, bot_drive_distance_behaviour, BEHAVIOUR_INACTIVE));
#endif
#ifdef BEHAVIOUR_GOTO_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(148, bot_goto_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
	// Hilfsverhalten zum Anfahren von Positionen
	insert_behaviour_to_list(&behaviour, new_behaviour(147, bot_gotoxy_behaviour, BEHAVIOUR_INACTIVE));
#endif
#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(146, bot_goto_pos_behaviour, BEHAVIOUR_INACTIVE));
#endif
#ifdef BEHAVIOUR_GOTO_OBSTACLE_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(145, bot_goto_obstacle_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_MEASURE_DISTANCE_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(140, bot_measure_distance_behaviour, BEHAVIOUR_INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(139, bot_check_distance_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
	// Fahrverhalten fuer das neuronale Netz; setzt dieses natuerlich voraus
	insert_behaviour_to_list(&behaviour, new_behaviour(135, bot_drive_neuralnet_behaviour, BEHAVIOUR_INACTIVE));
	// Unterverhalten zum Checken der Sektoren links Mitte und rechts vom Bot
	insert_behaviour_to_list(&behaviour, new_behaviour(134, bot_check_sector_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_NEURALNET_AVAILABLE
	// Training des neuronalen Netzes sofort Losstarten bei Vorhandensein, da initiale Lernpattern vorhanden sind
	insert_behaviour_to_list(&behaviour, new_behaviour(102, bot_neuralnet_behaviour, BEHAVIOUR_ACTIVE));
#endif

#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
	// Verhalten, um ein Labyrinth nach der Hoehlenforscher-Methode loesen
	insert_behaviour_to_list(&behaviour, new_behaviour(100, bot_solve_maze_behaviour, BEHAVIOUR_INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(90, bot_measure_angle_behaviour, BEHAVIOUR_INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(89, bot_check_wall_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_DRIVE_AREA_AVAILABLE
	// Registrierung des Handlers zur Behandlung des Abgrundes im Area-Verhalten
	register_emergency_proc(&border_drive_area_handler);
	// Observer bekomemn hohe Prio um noch vor goto_pos zum Zuge zu kommen;
	// links hoeher als rechts, damit rechts zuletzt Werte in den Stack schreibt
	insert_behaviour_to_list(&behaviour, new_behaviour(171, bot_observe_left_behaviour, BEHAVIOUR_INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(170, bot_observe_right_behaviour, BEHAVIOUR_INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(72, bot_drive_area_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_PATHPLANING_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(71, bot_calc_wave_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE
	// erweiterter Linienfolge, der mit Unterbrechungen und Hindernissen klarkommt
	insert_behaviour_to_list(&behaviour, new_behaviour(71, bot_follow_line_enh_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
	// Verhalten um einer Linie zu folgen
	insert_behaviour_to_list(&behaviour, new_behaviour(70, bot_follow_line_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
	// Linie folgen ueber Kreuzungen hinweg zum Ziel, kuerzester Weg befindet sich danach im Stack
	insert_behaviour_to_list(&behaviour, new_behaviour(69, bot_line_shortest_way_behaviour, BEHAVIOUR_INACTIVE));
	// Ueberwacherverhalten auf Fahren in entgegengesetzte Richtung bekommt hohe Prio, um vor bot_turn zu kommen
	insert_behaviour_to_list(&behaviour, new_behaviour(169, bot_check_reverse_direction_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
	// unwichtigere Hilfsverhalten
	insert_behaviour_to_list(&behaviour, new_behaviour(55, bot_explore_behaviour, BEHAVIOUR_INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(54, bot_do_slalom_behaviour, BEHAVIOUR_INACTIVE));
	// Demo-Verhalten fuer aufwendiges System, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(60, bot_olympic_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
	// Demo-Verhalten, etwas komplexer, inaktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(51, bot_drive_square_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_FOLLOW_WALL_AVAILABLE
	// Explorer-Verhalten um einer Wand zu folgen
	insert_behaviour_to_list(&behaviour, new_behaviour(48, bot_follow_wall_behaviour, BEHAVIOUR_INACTIVE));
	// Registrierung zur Behandlung des Notfallverhaltens zum R ueckwaertsfahren
	register_emergency_proc(&border_follow_wall_handler);
#endif

#ifdef BEHAVIOUR_CLASSIFY_OBJECTS_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(45, bot_classify_objects_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(44, bot_catch_pillar_behaviour, BEHAVIOUR_INACTIVE));
	insert_behaviour_to_list(&behaviour, new_behaviour(43, bot_unload_pillar_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(41, bot_transport_pillar_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_FOLLOW_OBJECT_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(40, bot_follow_object_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_DRIVE_CHESS_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(39, bot_drive_chess_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_ABL_AVAILABLE
	// Verhalten, das ABL interpretiert und per Remote-Call andere Verhalten starten kann
	insert_behaviour_to_list(&behaviour, new_behaviour(35, bot_abl_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(34, bot_ubasic_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_DRIVE_STACK_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(33, bot_drive_stack_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_SCAN_BEACONS_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(32, bot_scan_beacons_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(30, bot_calibrate_pid_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_CALIBRATE_SHARPS_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(29, bot_calibrate_sharps_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_TURN_TEST_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(28, bot_turn_test_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_TEST_ENCODER_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(20, bot_test_encoder_behaviour, BEHAVIOUR_INACTIVE));
#endif

#ifdef BEHAVIOUR_HW_TEST_AVAILABLE
	insert_behaviour_to_list(&behaviour, new_behaviour(10, bot_hw_test_behaviour, BEHAVIOUR_ACTIVE));
#endif

	// Grundverhalten, setzt aeltere FB-Befehle um, aktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(2, bot_base_behaviour, BEHAVIOUR_ACTIVE));
}


/**
 * Das einfachste Grundverhalten
 * \param *data der Verhaltensdatensatz
 */
static void bot_base_behaviour(Behaviour_t * data) {
	(void) data; // kein warning
	speedWishLeft = target_speed_l;
	speedWishRight = target_speed_r;
}

/**
 * Erzeugt ein neues Verhalten
 * \param priority 	Die Prioritaet
 * \param *work 	Die Funktion, die sich drum kuemmert
 * \param active	Boolean, ob das Verhalten aktiv oder inaktiv erstellt wird
 * \return			Zeiger auf erzeugten Verhaltensdatensatz, oder NULL im Fehlerfall
 */
static Behaviour_t * new_behaviour(uint8_t priority, void (* work) (struct _Behaviour_t * data), uint8_t active) {
	Behaviour_t * newbehaviour = malloc(sizeof(Behaviour_t));
	if (newbehaviour == NULL) {
		return NULL;
	}

	newbehaviour->priority = priority;

	bit_t tmp = { active };
	newbehaviour->active = tmp.bit;

	newbehaviour->next = NULL;
	newbehaviour->work = work;
	newbehaviour->caller = NULL;
	newbehaviour->subResult = BEHAVIOUR_SUBSUCCESS;
	return newbehaviour;
}

/**
 * Fuegt ein Verhalten der Verhaltenliste anhand der Prioritaet ein.
 * \param **list	Die Speicherstelle an der die globale Verhaltensliste anfaengt
 * \param *behave	Zeiger auf einzufuegendes Verhalten
 */
static void insert_behaviour_to_list(Behaviour_t * * list, Behaviour_t * behave) {
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

/**
 * Liefert das Verhalten zurueck, welches durch function implementiert ist
 * \param function	Die Funktion, die das Verhalten realisiert
 * \return			Zeiger auf Verhaltensdatensatz oder NULL
 */
Behaviour_t * get_behaviour(BehaviourFunc_t function) {
	Behaviour_t * job; // Zeiger auf ein Verhalten

	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben
	for (job = behaviour; job; job = job->next) {
		if (job->work == function) {
			return job;
		}
	}
	return NULL;
}

/**
 * Zu ein Verhalten mit der gegebenen Prioritaet
 * \param prio	Prioritaet des gesuchten Verhaltens
 * \return		Zeiger auf Verhaltensdatensatz oder NULL
 */
Behaviour_t * get_behaviour_from_prio(uint8_t prio) {
	Behaviour_t * job; // Zeiger auf ein Verhalten

	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben
	for (job = behaviour; job; job = job->next) {
		if (job->priority == prio) {
			return job;
		}
	}
	return NULL;
}

/**
 * Deaktiviert ein Verhalten
 * \param beh Das zu deaktivierende Verhalten
 */
void deactivate_behaviour(Behaviour_t * beh) {
	if (beh == NULL) {
		return;
	}
#ifdef DEBUG_BOT_LOGIC
	if (beh->active == BEHAVIOUR_ACTIVE) {
		LOG_DEBUG("Verhalten %u wird deaktiviert", beh->priority);
	}
#endif
	beh->active = BEHAVIOUR_INACTIVE;
	beh->caller = NULL;	// Caller loeschen, damit Verhalten auch ohne BEHAVIOUR_OVERRIDE neu gestartet werden koennen
}

/**
 * Rueckgabe von True, wenn das Verhalten gerade laeuft (aktiv ist), sonst False
 * \param function Die Funktion, die das Verhalten realisiert.
 * \return True wenn Verhalten aktiv, sonst False
 */
uint8_t behaviour_is_activated(BehaviourFunc_t function) {
	Behaviour_t * job = get_behaviour(function);
	if (job == NULL) {
		return False;
	}

	return job->active;
}

/**
 * liefert !=0 zurueck, wenn function ueber eine beliebige Kette (job->caller->caller ....) von anderen Verhalten job aufgerufen hat
 * \param *job			Zeiger auf den Datensatz des aufgerufenen Verhaltens
 * \param *caller_beh	Das Verhalten, das urspruenglich aufgerufen hat
 * \return 				0 wenn keine Call-Abhaengigkeit besteht, ansonsten die Anzahl der Stufen
 */
static uint8_t isInCallHierarchy(Behaviour_t * job, Behaviour_t * caller_beh) {
	uint8_t level = 0;

	if (job == NULL) {
		LOG_DEBUG("kein Verhaltensdatensatz gegeben");
		return 0; // Liste ist leer
	}

	for (; job->caller; job = job->caller) {
		if (job == job->caller) {
			/* Verhalten ist selbst als Caller eingetragen -> Abbruch */
			return 0;
		}
		LOG_DEBUG("  ueberpruefe Verhalten %u mit caller %u...", job->priority, job->caller->priority);
		level++;
		if (job->caller == caller_beh) {
			LOG_DEBUG("   Verhalten %u wurde direkt von %u aufgerufen", job->priority, job->caller->priority);
			return level; // Direkter Aufrufer in Tiefe level gefunden
		}
	}
	return 0; // function kommt in Caller-Liste von job nicht vor
} // O(n), n:=|Caller-Liste|

/**
 * Deaktiviert alle von diesem Verhalten aufgerufenen Verhalten.
 * Das Verhalten selbst bleibt aktiv und bekommt ein BEHAVIOUR_SUBCANCEL in seine Datanestruktur eingetragen.
 * \param *caller Zeiger auf den Aufrufer
 */
void deactivate_called_behaviours(Behaviour_t * caller) {
	if (! caller) {
		LOG_DEBUG("Parameter caller invalid");
		return;
	}

	LOG_DEBUG(""); // new line
	LOG_DEBUG("Callees von Verhalten %u sollen abgeschaltet werden.", caller->priority);
	LOG_DEBUG("Beginne mit dem Durchsuchen der Liste");
	// Einmal durch die Liste gehen, und alle aktiven Funktionen pruefen, ob sie von dem uebergebenen Verhalten aktiviert wurden
	uint8_t i = 0;
	Behaviour_t * job; // Zeiger auf ein Verhalten
	Behaviour_t * beh_of_function = NULL;
	for (job = behaviour; job; job = job->next) { // n mal
		if (job->active == BEHAVIOUR_ACTIVE) {
			i++;
			LOG_DEBUG("Verhalten mit Prio = %u ist ACTIVE, Durchlauf %u", job->priority, i);
			uint8_t level = isInCallHierarchy(job, caller); // O(n)
			LOG_DEBUG(" und hat Level %u Call-Abhaengigkeit", level);
			/* die komplette Caller-Liste (aber auch nur die) abschalten */
			Behaviour_t * ptr = job;
			for (; level > 0; --level) { // n mal
				Behaviour_t * beh;
				for (beh = behaviour; beh; beh = beh->next) { // n mal
					/* Falls das Verhalten Caller eines anderen Verhaltens ist, duerfen wir es (noch) nicht deaktivieren! */
					if (beh->caller == ptr) {
						LOG_DEBUG("  Verhalten %u ist Caller eines anderen Verhaltens", ptr->priority);
						break;
					}
				} // O(n)
				Behaviour_t * tmp = ptr;
				ptr = ptr->caller; // zur naechsten Ebene
				if (beh == NULL) {
					LOG_DEBUG("  Verhalten %u wird in Tiefe %u abgeschaltet", tmp->priority, level);
					tmp->active = BEHAVIOUR_INACTIVE; // callee abschalten
					tmp->subResult = BEHAVIOUR_SUBCANCEL;
					tmp->caller = NULL; // Caller loeschen, damit Verhalten auch ohne BEHAVIOUR_OVERRIDE neu gestartet werden koennen
				}
			}	// O(n^2)
			LOG_DEBUG(""); // new line
		}
		if (job == caller) {
			/* Verhalten fuer spaeter merken, wenn wir hier eh schon die ganze Liste absuchen */
			beh_of_function = job;
		}
	} // O(n^3)
	/* Verhaltenseintrag zu function benachrichtigen und wieder aktiv schalten */
	if (beh_of_function != NULL) {
		LOG_DEBUG("Verhalten %u wird aktiviert", beh_of_function->priority);
		beh_of_function->subResult = BEHAVIOUR_SUBCANCEL; // externer Abbruch
		beh_of_function->active = BEHAVIOUR_ACTIVE;
	}
} // O(n^3)

/**
 * Ruft ein anderes Verhalten auf und merkt sich den Ruecksprung
 * return_from_behaviour() kehrt dann spaeter wieder zum aufrufenden Verhalten zurueck
 * \param *from		aufrufendes Verhalten
 * \param *to		aufgerufenes Verhalten
 * \param mode		Hier sind vier Werte moeglich:
 * 		1. BEHAVIOUR_OVERRIDE:	Das Zielverhalten to wird aktiviert, auch wenn es noch aktiv ist.
 *						Das Verhalten, das es zuletzt aufgerufen hat wird dadurch automatisch
 *						wieder aktiv und muss selbst sein eigenes Feld subResult auswerten, um zu pruefen, ob das
 *						gewuenschte Ziel erreicht wurde, oder vorher ein Abbruch stattgefunden hat.
 * 		2. BEHAVIOUR_NOOVERRIDE:	Das Zielverhalten wird nur aktiviert, wenn es gerade nichts zu tun hat.
 *						In diesem Fall kann der Aufrufer aus seinem eigenen subResult auslesen,
 *						ob seinem Wunsch Folge geleistet wurde.
 *		3. BEHAVIOUR_FOREGROUND	Das Verhalten laeuft im Fordergrund (Aufrufer wird solange deaktiviert)
 *		4. BEHAVIOUR_BACKGROUND	Das Verhalten laeuft im Hintergrund (Aufrufer bleibt aktiv)
 * \return			Zeiger auf Verhaltensdatensatz des aufgerufenen Verhaltens, oder NULL im Fehlerfall
 */
Behaviour_t * switch_to_behaviour(Behaviour_t * from, void (* to)(Behaviour_t *), uint8_t mode) {
	LOG_DEBUG("switch_to_behaviour(0x%lx, 0x%lx, %u)", (size_t) from, (size_t) to, mode);
	Behaviour_t * job = get_behaviour(to);
	if (job == NULL) {
		/* Zielverhalten existiert gar nicht */
		if (from) {
			from->subResult = BEHAVIOUR_SUBFAIL;
		}
		return NULL;
	}

	bit_t tmp;
	behaviour_mode_t beh_mode;
	tmp.byte = (uint8_t) (mode & 1);
	beh_mode.override = tmp.bit;
	tmp.byte = (uint8_t) ((mode & 2) >> 1);
	beh_mode.background = tmp.bit;

	if (job->caller || job->active || job->subResult == BEHAVIOUR_SUBRUNNING) { // Ist das auzurufende Verhalten noch beschaeftigt?
		if (beh_mode.override == BEHAVIOUR_NOOVERRIDE) { // nicht ueberschreiben, sofortige Rueckkehr
			if (from) {
				from->subResult = BEHAVIOUR_SUBFAIL;
			}
			return NULL;
		}
		if (job->caller) {
			// Wir wollenalso ueberschreiben, aber nett zum alten Aufrufer sein und ihn darueber benachrichtigen
			job->caller->active = BEHAVIOUR_ACTIVE;	// alten Aufrufer reaktivieren
			job->caller->subResult = BEHAVIOUR_SUBFAIL;	// er bekam aber nicht das gewuenschte Resultat
		}
	}

	if (from) {
		if (beh_mode.background == 0) {
			// laufendes Verhalten abschalten
			from->active = BEHAVIOUR_INACTIVE;
			from->subResult = BEHAVIOUR_SUBRUNNING;
		} else {
			from->subResult = BEHAVIOUR_SUBBACKGR;
		}
	}

	// neues Verhalten aktivieren
	job->active = BEHAVIOUR_ACTIVE;
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
#endif // DEBUG_BOT_LOGIC

	return job;
}

/**
 * Kehrt zum aufrufenden Verhalten zurueck und setzt den Status auf Erfolg oder Misserfolg
 * \param *data	laufendes Verhalten
 * \param state	Abschlussstatus des Verhaltens (BEHAVIOUR_SUBSUCCESS oder BEHAVIOUR_SUBFAIL)
 */
void exit_behaviour(Behaviour_t * data, uint8_t state) {
	LOG_DEBUG("exit_behaviour(0x%lx (Prio %u), %u)", (size_t) data, data->priority, state);
	data->active = BEHAVIOUR_INACTIVE; // Unterverhalten deaktivieren
	LOG_DEBUG("Verhalten %u wurde beendet", data->priority);
	if (data->caller) {
		data->caller->active = BEHAVIOUR_ACTIVE; // aufrufendes Verhalten aktivieren

		union {
			uint8_t byte;
			unsigned bits:3;
		} tmp = {state};
		data->caller->subResult = tmp.bits;	// Status beim Aufrufer speichern

		LOG_DEBUG("Caller %u wurde wieder aktiviert", data->caller->priority);
	}
	data->caller = NULL; // Job erledigt, Verweis loeschen
}

/**
 * Deaktiviert alle Verhalten bis auf Grundverhalten.
 */
void deactivateAllBehaviours(void) {
	Behaviour_t * job; // Zeiger auf ein Verhalten
	// Einmal durch die Liste gehen und (fast) alle deaktivieren, Grundverhalten nicht
	for (job = behaviour; job; job = job->next) {
		if ((job->priority >= BEHAVIOUR_PRIO_MIN) && (job->priority <= BEHAVIOUR_PRIO_MAX)) {
            // Verhalten deaktivieren
			LOG_DEBUG("Verhalten %u wird deaktiviert", job->priority);
			job->active = BEHAVIOUR_INACTIVE;
			job->subResult = BEHAVIOUR_SUBCANCEL;
			job->caller = NULL; // Caller loeschen, damit Verhalten auch ohne BEHAVIOUR_OVERRIDE neu gestartet werden koennen
		}
	}
}

/**
 * Zentrale Verhaltens-Routine, wird regelmaessig aufgerufen.
 */
void bot_behave(void) {
	Behaviour_t * job; // Zeiger auf ein Verhalten

#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
	float factorLeft = 1.0f; // Puffer fuer Modifikatoren
	float factorRight = 1.0f; // Puffer fuer Modifikatoren
#endif // BEHAVIOUR_FACTOR_WISH_AVAILABLE

	/* Solange noch Verhalten in der Liste sind...
	   (Achtung: Wir werten die Jobs sortiert nach Prioritaet aus. Wichtige zuerst einsortieren!!!) */
	for (job = behaviour; job; job = job->next) {
		if (job->active) {
			/* WunschVariablen initialisieren */
			speedWishLeft = BOT_SPEED_IGNORE;
			speedWishRight = BOT_SPEED_IGNORE;

#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
			factorWishLeft = 1.0f;
			factorWishRight = 1.0f;
#endif // BEHAVIOUR_FACTOR_WISH_AVAILABLE

			if (job->work) { // hat das Verhalten eine Work-Routine
				job->work(job); // Verhalten ausfuehren
			} else { // wenn nicht: Verhalten deaktivieren, da es nicht sinnvoll arbeiten kann
				job->active = BEHAVIOUR_INACTIVE;
			}

#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
			/* Modifikatoren sammeln  */
			factorLeft  *= factorWishLeft;
			factorRight *= factorWishRight;
#endif // BEHAVIOUR_FACTOR_WISH_AVAILABLE

           /* Geschwindigkeit aendern? */
			if ((speedWishLeft != BOT_SPEED_IGNORE) || (speedWishRight != BOT_SPEED_IGNORE)) {
#ifdef BEHAVIOUR_FACTOR_WISH_AVAILABLE
				if (speedWishLeft != BOT_SPEED_IGNORE) {
					speedWishLeft = (int16_t) (speedWishLeft * factorLeft);
				}
				if (speedWishRight != BOT_SPEED_IGNORE) {
					speedWishRight = (int16_t) (speedWishRight * factorRight);
				}
#endif // BEHAVIOUR_FACTOR_WISH_AVAILABLE

				motor_set(speedWishLeft, speedWishRight);
				break; // Wenn ein Verhalten Werte direkt setzen will, nicht weitermachen
			}
		}
		/* Dieser Punkt wird nur erreicht, wenn keine Regel im System die Motoren beeinflusen will */
		if (job->next == NULL) {
			motor_set(BOT_SPEED_IGNORE, BOT_SPEED_IGNORE);
		}
	}
}

/**
 * Gibt das naechste Verhalten der Liste zurueck
 * \param *beh	Zeiger auf Verhalten, dessen Nachfolger gewuenscht ist, NULL fuer Listenanfang
 * \return		Zeiger auf Nachfolger von beh
 */
Behaviour_t * get_next_behaviour(Behaviour_t * beh) {
	if (beh == NULL) {
		return behaviour;
	} else {
		return beh->next;
	}
}


#define MAX_EMERG_PROCS 3			/**< Maximale Anzahl der registrierbaren Funktionen */
static int8_t count_arr_emerg = 0;	/**< Anzahl der zurzeit registrierten Notfallfunktionen */
/** hier liegen die Zeiger auf die auszufuehrenden Notfall-Funktionen */
void (* bot_logic_emerg_functions[MAX_EMERG_PROCS])(void) = { NULL };

/**
 * Routine zum Registrieren einer Notfallfunktion, die beim Ausloesen eines Abgrundsensors
 * aufgerufen wird; hierdurch kann ein Verhalten vom Abgrund benachrichtigt werden und
 * entsprechend dem Verhalten reagieren
 * \param *fkt	Die zu registrierende Routine, welche aufzurufen ist
 * \return 		Index, den die Routine im Array einnimmt, bei -1 ist alles voll
 */
int8_t register_emergency_proc(void (* fkt)(void)) {
	if (count_arr_emerg == MAX_EMERG_PROCS) {
		return -1; // sorry, aber fuer dich ist kein Platz mehr da :(
	}
	int8_t proc_nr = count_arr_emerg++;	// neue Routine hinten anfuegen
	bot_logic_emerg_functions[proc_nr] = fkt; // Pointer im Array speichern
	return proc_nr;
}

/**
 * Beim Ausloesen eines Notfalls wird diese Routine angesprungen
 * und ruft alle registrierten Prozeduren der Reihe nach auf
 */
void start_registered_emergency_procs(void) {
	uint8_t i;
	for (i = 0; i < MAX_EMERG_PROCS; ++i) {
		if (bot_logic_emerg_functions[i] != NULL) {
			bot_logic_emerg_functions[i]();
		}
	}
}
#endif // BEHAVIOUR_AVAILABLE
