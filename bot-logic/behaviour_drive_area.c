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
 * @file 	behaviour_drive_area.c
 * @brief 	Flaechendeckendes Fahren als Verhalten (Staubsauger)
 *          Der Bot faehrt geradeaus bis zu einem Hindernis, dort dreht er sich zu einer Seite und faehrt auf der Nebenspur wieder zurueck. Der
 *          andere moegliche Weg wird als Alternative auf den Stack gespeichert. Falls es irgendwo nicht weitergeht, so wird eine solche Strecke
 *          vom Stack geholt und angefahren. Leider gibt es noch kein Map-Planungsverhalten, welches einen anderen Punkt unter Umgehung von
 *          Hindernissen anfaehrt. In dieser Version wird etwas tricky gefahren und versucht, diese Strecke anzufahren. Im Falle
 *          aber des nicht moeglichen Anfahrens wird eben diese Strecke verworfen. Ein Planungsverhalten, welches moeglichst auch
 *          nur ueber befahrene Abschnitte plant, wuerde entscheidend helfen.
 *
 * @author 	Frank Menzel (Menzelfr@gmx.net)
 * @date 	16.07.2008
 */

#include "bot-logic/bot-logik.h"
#include "bot-logic/behaviour_drive_area.h"
#include "map.h"
#include "timer.h"
#include "math_utils.h"
#include "pos_store.h"
#include "log.h"
#include "command.h"
#include <math.h>
#include <stdlib.h>

#ifdef BEHAVIOUR_DRIVE_AREA_AVAILABLE

#define GO_WITH_PATHPLANING  		// auskommentieren, falls ohne Pfadplanung

//#define DEBUG_BEHAVIOUR_AREA		// Schalter fuer Debug-Code
//#define DEBUG_DRIVE_AREA_TIMES	// Schalter fuer Zeitmessungen


#ifndef LOG_AVAILABLE
#undef DEBUG_BEHAVIOUR_AREA
#endif
#ifndef DEBUG_BEHAVIOUR_AREA
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif

#ifndef BEHAVIOUR_PATHPLANING_AVAILABLE
#undef GO_WITH_PATHPLANING
#endif


#define STACK_SIZE	64 /*!< Groesse des verwendeten Positionsspeichers */

#if STACK_SIZE > POS_STORE_SIZE
#undef STACK_SIZE
#define STACK_SIZE POS_STORE_SIZE
#endif

/*! nur alle X-Millisekunden Mapzugriff der Observer-Verhalten */
#define	CORRECTION_DELAY	700

/*! Mapwert zur Erkennung einer bereits befahrenen Spur */
#define MAP_TRACKVAL   30

/*!  In diesem Abstand voraus [mm] ab Abstandssensoren wird auf Hindernis gecheckt */
#define DIST_AWARD_HAZ  OPTIMAL_DISTANCE

#define MAP_WAY_FREE_MARGIN	10	/*!< Toleranzbereich links und rechts vom Bot fuer map_way_free() [mm] */

/*! Mindestabstand der Punkte fuer eine gueltige Bahn, welche es zu befahren gilt */
#define DIST_VALID_QUAD (int32_t)(140 * 140)


/*! Defines zum festlegen, auf welcher Seite des Bots sich die Spur befindet */
#define TRACKLEFT  -1
#define TRACKRIGHT 1

/*! Verhalten-Define zum Verlassen */
#define TRACK_END 99

/*! zu erreichender Punkt laut Observer neben dem Bot auf Parallelbahn; Korrekturwert, um den der
 *  Seitenabstand (Botdurchmesser) subtrahiert wird
 */
#define SIDEDIST_MINUS  30

/*! Mapzugriff der Observer nach dieser gefahrenen Strecke in mm
 */
#define MAPACCESS_AFTER_DISTANCE 30
#define MAPACCESS_AFTER_DISTANCE_QUAD (MAPACCESS_AFTER_DISTANCE * MAPACCESS_AFTER_DISTANCE)  // Quadrat der gefahrenen Strecke

/*! Statusvariable des area-Verhaltens */
static uint8_t track_state = 0;
static uint8_t next_behavstate=0;


/*! Notfallkennung fuer Abgrund; wird gesetzt in registrierter Notfallroutine */
static uint8_t border_fired = False;

/*! Statusvars fuer die Observer-Verhalten links und rechts */
static uint8_t observe1_state = 0;
static uint8_t observe2_state = 0;

/*! Zeit-Merkvariable fuer die Observer-Verhalten fuer Mapzugriff erst nach Ablauf einer gewissen Zeit */
static uint32_t lastCorrectionTime = 0;

/*! Strukturtyp fuer die beobachteten Nachbar-Bahnen; fuer Links und Rechts bestehend aus 2 Punkten */
typedef struct {
	position_t point1;
	position_t point2;
} trackpoint_t;

/*! Nachbarbahnen links und rechts */
static trackpoint_t observe_trackleft;
static trackpoint_t observe_trackright;

/*! naechste anzufahrende Bahn */
static trackpoint_t nextline;

/*! gemerkte letzte Position des Bots, um nur aller xx gefahrender mm auf Map zuzugreifen */
static trackpoint_t observe_lastpos;

static pos_store_t * pos_store = NULL;			/*!< Positionsspeicher, den das Verhalten benutzt */
static position_t pos_store_data[STACK_SIZE];	/*!< Statischer Speicher fuer pos_store */

/* ==================================================================================
 * ===== Start der allgemein fuer diese Verhalten notwendigen Routinen ==============
 * ==================================================================================*/


#if defined MAP_2_SIM_AVAILABLE && defined DEBUG_BEHAVIOUR_AREA
/*!
 * Zeichnet eine Linie von Koordinate from nach to in der Farbe color in die Map ein;
 * dient zur Visualisierung der Arbeitsweise des Verhaltens
 * @param from	Koordinaten des ersten Punktes der Linie
 * @param to	Koordinaten des zweiten Punktes der Linie
 * @param color Farbe der Linie: 0=gruen, 1=rot, sonst schwarz
 */
static void draw_line_world(position_t from, position_t to, uint8_t color) {
	command_write(CMD_MAP, SUB_MAP_CLEAR_LINES, 2, 0, 0);
	map_draw_line_world(from, to, color);
}
#endif	// MAP_2_SIM_AVAILABLE

/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung und muss registriert werden
 */
void border_drive_area_handler(void) {
	// Routine setzt hier einfach nur die Borderkennung, die vom area-Verhalten ausgewertet wird
	  border_fired = True;
}

/*!
 * liefert den Wert eines Feldes als Durchschnittwert laut Map in bestimmtem Umkreis
 * @param point	XY-Weltkoordinate
 * @return	Durchschnittswert des Feldes (>0 heisst frei, <0 heisst belegt)
 */
static int8_t map_get_field(position_t point) {
#ifdef DEBUG_DRIVE_AREA_TIMES
	uint8_t result;
	TIMER_MEASURE_TIME(result = map_get_average(point.x, point.y, 50));
	return result;
#else
	return map_get_average(point.x, point.y, 50);  // Mapwert Durchschnitt
#endif
}

/*!
 * Speichert eine Line auf dem Stack, d.h. den Start- und Endpunkt der Linie
 * @param point1	Koordinaten des ersten Punktes der Linie
 * @param point2	Koordinaten des zweiten Punktes der Linie
 */
static void push_stack_pos_line(position_t point1, position_t point2) {
	if ((point1.x == 0 && point1.y == 0) || (point2.x == 0 && point2.y == 0))
		return; // ungueltige Werte werden nicht uebernommen

	// Push der Linien-Einzelpunkte
	pos_store_push(pos_store, point1);
	pos_store_push(pos_store, point2);

	LOG_DEBUG("PUSH x1y1: %1d %1d x2y2: %1d %1d", point1.x,point1.y,point2.x,point2.y);

#if defined MAP_2_SIM_AVAILABLE && defined DEBUG_BEHAVIOUR_AREA
	draw_line_world(point1, point2, 2);  // schwarze Linie
#endif
}

/*!
 * Holt eine Line vom Stack, d.h. die beiden Punkte der Linie werden zurueckgegeben
 * @param *point1	Koordinaten des ersten Punktes der Linie
 * @param *point2	Koordinaten des zweiten Punktes der Linie
 * @return True wenn erfolgreich, False falls Stack leer ist
 */
static uint8_t pop_stack_pos_line(position_t *point1, position_t *point2) {

	if (!pos_store_pop(pos_store, point1))
		return False;

	if (!pos_store_pop(pos_store, point2))
		return False;

	return True;
}

/*!
 * Hilfsroutine, um 2 Punkte in die Merkvariable zu speichern
 * @param *lastpoint XY-Merkvariable
 * @param mapx zu merkender X-Wert
 * @param mapy zu merkender Y-Wert
 */
static void set_point_to_lastpoint(position_t * lastpoint,int16_t mapx, int16_t mapy) {
	lastpoint->x = mapx;
	lastpoint->y = mapy;
}

/*!
 * Liefert True, wenn der Abstand zwischen den beiden Punkten gueltig ist, d.h. erreicht oder ueberschritten wurde
 * @param point_s World-XY-Koordinate des zu untersuchenden Punktes
 * @param point_d World-XY-Koordinate des Zielpunktes
 * @return True bei Erreichen des Abstandes zwischen den beiden Punkten sonst False
 */
static uint8_t dist_valid(position_t point_s, position_t point_d) {
	// falls ein Punktepaar noch 0 hat, so ist dies ungueltig
	if ((point_s.x == 0 && point_s.y == 0) || (point_d.x == 0 && point_d.y == 0))
		return False;

	/* Abstandsermittlung nach dem guten alten Pythagoras ohne Ziehen der Wurzel */
	return (uint8_t) (get_dist(point_s.x, point_s.y, point_d.x, point_d.y) > DIST_VALID_QUAD);
}

/*!
 * Berechnung des Seitenpunktes der Nebenbahn mit Rueckgabe des Punktes und des Map-Durchschnittswertes
 * @param sidedist	seitlicher Abstand vom Botmittelpunkt
 * @param dist 		Punkt im Abstand voraus
 * @param side 		Trackleft (-1) oder Trackright (+1) fuer links oder rechts vom bot gesehen
 * @param *point 	berechnete Map-Weltkoordinate
 * @return 			Mapwert an dem ermittelten Punkt in Blickrichtung zum Abstand dist
 */
static int8_t getpoint_side_dist(int16_t sidedist, int16_t dist, int8_t side, position_t * point) {
	// Berechnung je nach zu blickender Seite des Bots
	*point = (side == TRACKLEFT) ? calc_point_in_distance(heading, DISTSENSOR_POS_FW
			+ dist, sidedist) : calc_point_in_distance(
			heading, DISTSENSOR_POS_FW + dist, -sidedist);

	// Rueckgabe des Mapwertes
#ifdef DEBUG_DRIVE_AREA_TIMES
	uint8_t result;
	TIMER_MEASURE_TIME(result = map_get_field(*point));
	return result;
#else
	return map_get_field(*point);
#endif
}


/* ==========================================================================================
 * ===== Obserververhalten und der dafuer notwendigen Routinen; es gibt 2 Observer-Verhalten,
 * ===== wobei eines die Bahn links und das andere die Bahn rechts vom Bot beobachtet
 * =========================================================================================*/


/*! Prueft ob der Bot schon eine bestimmte Strecke gefahren ist seit dem letzten Observerdurchgang
 * @param  *last_xpoint   letzte gemerkte X-Koordinate
 * @param  *last_ypoint   letzte gemerkte Y-Koordinate
 * @return True, wenn Bot schon gewisse Strecke gefahren ist und Map zu checken ist sonst False
 */
static uint8_t check_map_after_distance(int16_t * last_xpoint,
		int16_t * last_ypoint) {

	// Abstand seit letztem Observerlauf ermitteln
	uint16_t diff = (uint16_t) get_dist(x_pos, y_pos, *last_xpoint, *last_ypoint);

	//erst nach gewissem Abstand oder gleich bei noch initialem Wert Mappruefung
	if ((diff >= MAPACCESS_AFTER_DISTANCE_QUAD) || (*last_xpoint == 0
			&& *last_ypoint == 0)) {
		*last_xpoint = x_pos;
		*last_ypoint = y_pos;
		return True;
	}
	return False;
}

/*!  Kennung, dass die Observer-Verhalten beendet werden sollen; bei True soll Verhalten sofort beendet werden mit
 *   vorherigem Gueltigkeitscheck des Endpunktes
 */
static uint8_t endrequest = False;

/*! Verhaltensroutine fuer beide Observer zur Ermittlung des Startpunktes der Nebenspur
 * Je nach zu checkender Botseite wird der Punkt in der Nachbarspur auf Anfahrbarkeit gecheckt
 * und hier der Startpunkt der nebenbahn ermittelt
 * @param checkside		zu checkende Seite (TRACKLEFT TRACKRIGHT)
 * @param *observe		Zeiger auf die Koordinaten der Bahn, hier Startpunkt zurueckgegeben
 * @param *behavstate	Zeiger auf Status
 * @return 				True falls Startpunkt anfahrbar ist und zurueckgegeben wurde sonst False
 */
static uint8_t observe_get_startpoint(int8_t checkside, trackpoint_t * observe, uint8_t * behavstate) {
	position_t map_pos;
	int8_t mapval;

	if (map_locked() == 1) {
		return False;
	}

    // nur wenn bot seit letztem Zugriff gewisse Strecke gefahren ist erfolgt Mapzugriff; Check erfolgt seitenabhaengig
    uint8_t mapaccess = (uint8_t) (checkside == TRACKLEFT ? check_map_after_distance(&observe_lastpos.point1.x,
    	&observe_lastpos.point1.y) : check_map_after_distance(&observe_lastpos.point2.x, &observe_lastpos.point2.y));

    // Schluss bei Endeanforderung oder noch nicht weit genug gefahren
	if (endrequest || ! mapaccess) {
		*behavstate = (uint8_t) (endrequest ? TRACK_END : 0);
		return False;
	}

	// hier seitenabhaengig auf die Spur nebenan sehen auf Hoehe Mittelpunkt und Mapwert checken
	mapval = getpoint_side_dist(BOT_DIAMETER - SIDEDIST_MINUS,
			-DISTSENSOR_POS_FW, checkside, &map_pos);

	// falls Punkt bereits als befahren gekennzeichnet ist oder Hinderniswert hat gehts weiter mit Startpunktsuche
	if (mapval >= MAP_TRACKVAL || mapval < 0) {
		return False;
	}

	// hier ist also der Trackpunkt frei, dann merken und zur Endpunktsuche im Observer
	// zuweisen der Initwerte in die richtigen Puntvars
	set_point_to_lastpoint(&observe->point1, map_pos.x, map_pos.y);
	set_point_to_lastpoint(&observe->point2, 0, 0);

    // Startpunkt gefunden
	return True;
}



/*! Statusvariable der Obserververhalten */
#define OBSERVE_SEARCH_STARTPOINT     0
#define OBSERVE_SEARCH_ENDPOINT       1

/*!
 * Verhaltensroutine fuer beide Observer zur Endepunktermittlung; liefert True wenn der Endepunkt gueltig ist und damit ermittelt werden konnte
 * bei einem Teilhindernis auf der Nebenbahn wird eine gueltige befahrbare Strecke automatisch auf den Stack gelegt zum spaeteren Anfahren
 * @param checkside			Seite der zu beobachtenden Nebenbahn (TRACKLEFT TRACKRIGHT)
 * @param *behavstate		Neuer Zustand des Obserververhalten
 * @param *observer			Zeiger auf Punkte der Nebenbahn, gueltiger Endepunkt wird hier vermerkt
 * @param *lastpoint		Zeiger auf letzten gueltigen Wert der Nebenbahn, verwendeter Wert bei Erkennung Hindernis
 * @return True falls Endpunkt auf der Seite ermittelt werden konnte sonst False
 */
static uint8_t observe_get_endpoint(int8_t checkside, uint8_t * behavstate,
		trackpoint_t * observer, position_t * lastpoint) {
	position_t map_pos;
	int8_t mapval = 0;

	if (map_locked() == 1) {
		return False;
	}

	// nur wenn bot seit letztem Zugriff gewisse Strecke gefahren ist erfolgt Mapzugriff
    uint8_t mapaccess = (uint8_t) (checkside == TRACKLEFT ? check_map_after_distance(&observe_lastpos.point1.x,
   		&observe_lastpos.point1.y) : check_map_after_distance(&observe_lastpos.point2.x, &observe_lastpos.point2.y));

	// weitere Checks nur wenn gewisse Strecke gefahren oder Endeanforderung besteht
    if (! endrequest && ! mapaccess) {
		return False;  // Endepunktsuche ungueltig
    }

	// hier seitenabhaengig auf die jeweilige Spur sehen, Mapwert holen und die Koordinaten des Punktes der Nebenbahn
	mapval = getpoint_side_dist(BOT_DIAMETER - SIDEDIST_MINUS, 10, checkside, //etwas voraus
			&map_pos);

	// Endepunktsuche nicht weiter verfolgen falls noch kein Startpunkt vorliegt
	if ((observer->point1.x == 0 && observer->point1.y == 0)) {
		*behavstate = (uint8_t) (endrequest ? TRACK_END : 0); // weiter mit Startpunktsuche oder Ende
		return False; // Endepunktsuche ungueltig
	}

    // Hindernis oder schon befahren erkannt aber noch kein gueltiger Zwischenendpunkt ermittelt; Ende falls
    // Endeanforderung kam sonst weiter mit Startpunktsuche
    if ((mapval < 0 || mapval >= MAP_TRACKVAL) && &lastpoint->x == 0 && &lastpoint->y == 0) {
		set_point_to_lastpoint(&observer->point1, 0, 0);
		*behavstate = (uint8_t) (endrequest ? TRACK_END : 0); // weiter mit Startpunktsuche
		set_point_to_lastpoint(lastpoint, 0, 0);
		return False; // Endepunktsuche ungueltig
	}

    // Abstand zum Startpunkt muss gueltig sein sonst ist Schluss
	if (! dist_valid(observer->point1, map_pos)) {
		*behavstate = (uint8_t) (endrequest ? TRACK_END : 1); // bleibt in Endepunktsuche wenn nicht Endeanforderung kam
		if (endrequest) {
			LOG_DEBUG("Nebenpunkt zum Startpunkt dist nicht valid %1.0f",
				sqrtf(get_dist(observer->point1.x, observer->point1.y, map_pos.x, map_pos.y)));
		}
		return False; // Endepunktsuche ungueltig
	}

    // hier wird nun unterschieden ob Nebenspur schon befahren ist oder sich dort Hindernis befindet; in diesem Fall wird sich
    // die Strecke als gueltig bis zu dem Punkt vor dem Hindernis auf dem Stack gemerkt; weiter dann mit Startpunktsuche
	if (mapval < 0 || mapval >= MAP_TRACKVAL) { //Punkt auf Nebenbahn hat Hinderniswahrscheinlichkeit oder ist schon befahren

		// bei Hindernis auf aktuellem Punkt wird der letzte gemerkte Nicht-Hindernispunkt auf den Stack gespeichert, der
		// vorher auf Gueltigkeit gecheckt wird, d.h. muss bestimmten Mindestabstand zum Startpunkt haben und kein Hindernis sein
		set_point_to_lastpoint(&observer->point2, lastpoint->x, lastpoint->y);

		// zuletzt gueltigen Endpunkt auf Stack sichern
		push_stack_pos_line(observer->point1, *lastpoint);

		if (lastpoint->x != 0 || lastpoint->y != 0) {
			LOG_DEBUG("Autopush wg. Hind oder befahren xy: %1d %1d mapval: %1d", lastpoint->x, lastpoint->y, mapval);
		}

		// letzten gemerkten Punkt wieder als ungueltig kennzeichnen
	    set_point_to_lastpoint(lastpoint, 0, 0);

	} else {
		// Punkt auf Nebenbahn hat hier keine Hinderniswahrscheinlichkeit und ist noch nicht befahren

		// aktuellen gueltigen Zwischenpunkt der Nebenbahn merken; wird als Endepunkt genommen bei Hindernis auf Nebenbahn
		set_point_to_lastpoint(lastpoint, map_pos.x, map_pos.y);

		// falls sich bei diesem gueltigen Endpunkt herausstellt, dass der vorhin ermittelte Startpunkt nun Hinderniswert besitzt, so wird der Endpunkt zum Startpunkt
		// und weiter mit Endpunktsuche
		if (map_get_field(observer->point1) < 0) {
			set_point_to_lastpoint(&observer->point1,lastpoint->x, lastpoint->y); // Startpunkt ueberschreiben
		    *behavstate = (uint8_t) (endrequest ? TRACK_END : 1); // bleibt in Endepunktsuche wenn nicht Endeanforderung kam

			set_point_to_lastpoint(lastpoint, 0, 0);
			set_point_to_lastpoint(&observer->point2, 0, 0);
			return True; // gueltiger Endepunkt ermittelt
		}

		// bei Nicht Ende geht Endesuche weiter;bei Endeanforderung ist dies der aktuelle Endpunkt fuer den Stack
		if (!endrequest) { // weiter nur falls kein Ende kommen soll sonst ist dies nicht der echte Endpunkt
			return False;
		}

		// gueltigen Endepunkt in Observervar vermerken
		set_point_to_lastpoint(&observer->point2, lastpoint->x, lastpoint->y);

		// Merken der Strecke bei Endanforderung auf dem Stack
		push_stack_pos_line(observer->point1,*lastpoint);

	}

    // weiter mit Startpunktsuche wenn nicht Ende gefordert
	*behavstate = (uint8_t) (endrequest ? TRACK_END : 0);

	// solange die Startpunktsuche keinen neuen Startpunkt ermittelt hat, ist dieser initial
	set_point_to_lastpoint(&observer->point1, 0, 0);

	return True; // Streckenpunkt ist hier gueltig
}

/*! Merkvariable des letzten gueltigen Endpunktes fuer Obserververhalten linke Spur */
static position_t lastpointleft = { 0, 0 };

/*!
 * Observer links; jeweils ein selbstaendiges Verhalten, welches die Nachbarbahn beobachtet und eine befahrbare Strecke bis zu einem Hindernis
 * auf den Stack legt fuer spaeteres Anfahren; ebenfalls wird eine Alternativroute auf dem Stack gemerkt
 * Verhalten wurde so geschrieben, dass es zwar 2 Verhalten sind, der Code jedoch identisch ist und daher in Subroutinen ausgelagert ist
 * @param *data	Verhaltensdatensatz
 */
void bot_observe_left_behaviour(Behaviour_t * data) {
	// je zu beobachtende Seite gibt es einen codeidentischen Observer; hier die Seite festgelegt
	static int8_t CHECK_SIDE = TRACKLEFT;

	switch (observe1_state) {
	case OBSERVE_SEARCH_STARTPOINT:
		// Eingang fuer Startpunktsuche
		if (!observe_get_startpoint(CHECK_SIDE, &observe_trackleft, &observe1_state)) {
			break;
		}

		observe1_state = OBSERVE_SEARCH_ENDPOINT;
		break;

	case OBSERVE_SEARCH_ENDPOINT:
		// Anfangspunkt wurde bestimmt und ist hier auf der Suche nach dem Endpunkt; ein Endpunkt wird als Ende der Strecke
		// vom bereits vorhandenen Startpunkt erkannt, wenn der Botbereich neben dem Bot nicht mehr befahren werden kann; es
		// wird dann der zuletzt gemerkte befahrbare Punkt als Endpunkt genommen
		if (!observe_get_endpoint(CHECK_SIDE, &observe1_state,
				&observe_trackleft, &lastpointleft)) {
			break;
		}

		break;
	default:
		if (observe_trackleft.point2.x == 0 && observe_trackleft.point2.y == 0) {
			set_point_to_lastpoint(&observe_trackleft.point1, 0, 0);
		}

		return_from_behaviour(data);
		break;
	}
}

/*!
 * Rufe das Observer-Verhalten links auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
static void bot_observe_left(Behaviour_t * caller) {
	if (!behaviour_is_activated(bot_observe_left_behaviour)) {
		activateBehaviour(caller, bot_observe_left_behaviour);
		observe1_state = 0;
		endrequest = False;
		set_point_to_lastpoint(&observe_trackleft.point1, 0, 0);
		set_point_to_lastpoint(&observe_trackleft.point2, 0, 0);
		set_point_to_lastpoint(&lastpointleft, 0, 0);
		set_point_to_lastpoint(&observe_lastpos.point1, 0, 0);
	}
}

/*! Merkvariable des letzten gueltigen Endpunktes fuer Obserververhalten rechte Spur */
static position_t lastpointright = { 0, 0 };

/*!
 * Observer rechts; jeweils ein selbstaendiges Verhalten, welches die Nachbarbahn beobachtet und eine befahrbare Strecke bis zu einem Hindernis
 * auf den Stack legt fuer spaeteres Anfahren; ebenfalls wird eine Alternativroute auf dem Stack gemerkt
 * Verhalten wurde so geschrieben, dass es zwar 2 Verhalten sind, der Code jedoch identisch ist und daher in Subroutinen ausgelagert ist
 * @param *data Verhaltensdatensatz
 */
void bot_observe_right_behaviour(Behaviour_t * data) {
	// je zu beobachtende Seite gibt es einen codeidentischen Observer; hier die Seite festgelegt
	static int8_t CHECK_SIDE = TRACKRIGHT;

	switch (observe2_state) {
	case OBSERVE_SEARCH_STARTPOINT:
		// Eingang fuer Startpunktsuche
		if (!observe_get_startpoint(CHECK_SIDE, &observe_trackright, &observe2_state)) {
			break;
		}

		observe2_state = OBSERVE_SEARCH_ENDPOINT;

		break;

	case OBSERVE_SEARCH_ENDPOINT:
		// Anfangspunkt wurde bestimmt und ist hier auf der Suche nach dem Endpunkt; ein Endpunkt wird als Ende der Strecke
		// vom bereits vorhandenen Startpunkt erkannt, wenn der Botbereich neben dem Bot nicht mehr befahren werden kann; es
		// wird dann der zuletzt gemerkte befahrbare Punkt als Endpunkt genommen
		if (!observe_get_endpoint(CHECK_SIDE, &observe2_state,
				&observe_trackright, &lastpointright)) {
			break;
		}

		break;
	default:
		if (observe_trackright.point2.x == 0 && observe_trackright.point2.y == 0) {
			set_point_to_lastpoint(&observe_trackright.point1, 0, 0);
		}

		return_from_behaviour(data);
		break;
	}
}

/*!
 * Rufe das Rechts-Observer-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
static void bot_observe_right(Behaviour_t * caller) {
	if (!behaviour_is_activated(bot_observe_right_behaviour)) {
		activateBehaviour(caller, bot_observe_right_behaviour);
		observe2_state = 0;
		set_point_to_lastpoint(&observe_trackright.point1, 0, 0);
		set_point_to_lastpoint(&observe_trackright.point2, 0, 0);
		set_point_to_lastpoint(&lastpointright, 0, 0);
		set_point_to_lastpoint(&observe_lastpos.point2, 0, 0);
	}
}

/*!
 * Startet die Observer
 */
static void start_observe_left_right(Behaviour_t * caller) {
	bot_observe_right(caller);
	bot_observe_left(caller);
	endrequest = False;
}

/*!
 * Stopanforderung der beiden Observer-Verhalten links und rechts
 */
static void bot_stop_observe(void) {
	// Verhalten soll beendet werden
	endrequest = True;
}

/* ==================================================================================
 * ===== Ende der Obserververhalten und der dafuer notwendigen Routinen =============
 * ==================================================================================*/



/* ==================================================================================
 * ===== Hier alles fuer das flaechendeckende Fahrverhalten  ========================
 * ==================================================================================*/

/* Check-Bedingung zum Triggern des cancel-Verhaltens
 * @return True bei Abgrund sonst False
 */
static uint8_t check_border_fired(void) {
	return border_fired;
}

/* Check-Routine zum Erkennen eines Hindernisses nach den Abgrund- als auch Abstandssensoren
 * @return True wenn Hindernis voraus sonst False
 */
static uint8_t check_haz_sensDist(void) {
	if (border_fired) {
		return True;
	}

	if (sensDistL <= DIST_AWARD_HAZ || sensDistR <= DIST_AWARD_HAZ) {
		return True;
	}

	return False;
}



/*!
 * Hilfsroutine, welche den Fahrtrack nextline mit den Koordinaten belegt; die Koordinaten
 * koennen hierbei vertauscht werden oder in point1x/y die zum Bot nahesten Werte gespeichert
 * @param point1 Koordinaten des Punktes 1
 * @param point2 Koordinaten des Punktes 2
 * @param change_points Punkte1 und 2 in nextline werden bei True vertauscht sonst belegt mit den Koordinaten
 */
static void set_nextline(position_t point1, position_t point2, uint8_t change_points) {
	if (change_points) {
		int16_t xtemp = nextline.point1.x;
		int16_t ytemp = nextline.point1.y;
		nextline.point1.x = nextline.point2.x;
		nextline.point1.y = nextline.point2.y;
		nextline.point2.x = xtemp;
		nextline.point2.y = ytemp;
	} else {
		// der Punkt in der Naehe wird richtig nach nextline uebernommen; der naheste nach Punkt1
		if (get_dist(point1.x, point1.y, x_pos, y_pos) < get_dist(point2.x, point2.y, x_pos, y_pos)) {
			set_point_to_lastpoint(&nextline.point1, point1.x, point1.y);
			set_point_to_lastpoint(&nextline.point2, point2.x, point2.y);

		} else {
			// der Punkt in der Naehe wird richtig nach nextline uebernommen; der naheste nach Punkt1
			set_point_to_lastpoint(&nextline.point1, point2.x, point2.y);
			set_point_to_lastpoint(&nextline.point2, point1.x, point1.y);
		}
	}
}



/* ==================================================================================
 * ===== Nun das Area-Fahrverhalten selbst                             ==============
 * ==================================================================================*/

/* Zustaende des Verhaltens */
#define CHECK_TRACKSIDE		0
#define GO_FORWARD          1
#define AFTER_FORWARD		2
#define ONWALL_TO_TRACK		3
#define DELAY_AFTER_FORWARD 4
#define TURN_TO_DESTINATION 5
#define GET_LINE_FROM_STACK 6
#define TURN_TO_NEAREST		7
#define GOTO_FIRST_DEST		8
#define CHECK_DIST	        9

/*!
 * Das Fahrverhalten selbst; Fahren bis zu einem Hindernis, drehen zu einer Seite und merken des anderen Weges auf den Stack; waehrend der
 * Fahrt werden die Nebenspuren beobachtet und bei Hindernissen in der Nebenspur automatisch Teilstrecken auf den Stack gelegt
 * @param *data der Verhaltensdatensatz
 */
void bot_drive_area_behaviour(Behaviour_t * data) {
	static uint8_t go_long_distance = 0; // Kennung ob nach Stackholen weit gefahren wurde
	static uint32_t lastCheckTime = 0;
	static int16_t disttodrive = 0; // Abstand fuer das Vorwaertsfahren
	static trackpoint_t p_temp; // zum Merken beim Vertauschen

	position_t pos;
	int8_t mapval;

	deactivateBehaviour(bot_cancel_behaviour_behaviour); // Cancel-Verhalten abbrechen, damit es nicht ewig weiterlaeuft

	switch (track_state) {

	case CHECK_TRACKSIDE: // Vorwaeertsfahren mit Start der Observer

		BLOCK_BEHAVIOUR(data, 500); // etwas warten

		if (check_haz_sensDist() || (sensDistL <= 200 || sensDistR <= 200)) { // gar nicht erst fahren bei <20cm Hindernis
			track_state = GET_LINE_FROM_STACK;
			LOG_DEBUG("zu Stackholen wg. Abstand %1d %1d", sensDistL, sensDistR);

#if defined MAP_2_SIM_AVAILABLE && defined DEBUG_BEHAVIOUR_AREA
			if (nextline.point2.x != 0 || nextline.point2.y != 0) {
				draw_line_world(nextline.point1, nextline.point2, 1); // Strecke wird verworfen, nicht anfahrbar und rot darstellen
			}
#endif	// MAP_2_SIM_AVAILABLE

			break;
		}

		//wird jetzt zum Startzeitpunkt des Vorausfahrens bereits was gesehen, dann in Abhaengigkeit davon
		//einen Punkt in der Bahn voraus berechnen um Wegfreiheit laut Map zu bestimmen
		disttodrive = 900; // hoher init. Wert
		if (sensDistL < 400 || sensDistR < 400) {

            //notwendig, weil bei Map-Simexport Lampen als genausogrosses Hindernis gekennzeichnet sind wie Wandelemente, d.h. gesehener Abstand groesser
            //als in Map eingetragen und gesehener Anfahrpunkt waere im Hindernis und Strecke nicht anfahrbar; daher so weit kleinere Strecke
            //als gesehene nehmen, damit Punkt auf jeden Fall vor Hindernisfeld liegt
			disttodrive = sensDistL < sensDistR ? sensDistL - 80 : sensDistR - 80; // in kurzer Entfernung Hind. gesehen
			getpoint_side_dist(0, disttodrive, TRACKLEFT, &pos); // Punkt etwas vor Hindernis berechnen
			LOG_DEBUG("Mappunkt fuer %1d voraus berechnet l/r: %1d %1d", disttodrive, sensDistL, sensDistR);
		} else {
            // falls bei Weitsicht voraus berechneter Streckenpunkt schon befahren ist, dann nur bis dorthin fahren und nicht via obstacle; danach geht's wieder von
            // vorn  mit geradeausfahren weiter und saemtlichen Pruefungen; Beispielparcour hierfuer ist testbots_home
 			mapval=getpoint_side_dist(0,800, TRACKLEFT, &pos); //nur etwas voraus Punkt berechnen wenn weiter gesehen
			LOG_DEBUG("Map-Punkt %1d 80cm voraus ",mapval);

			if (mapval>MAP_TRACKVAL) {
			  LOG_DEBUG("Map-Punkt %1d 80cm voraus befahren l/r: %1d %1d",mapval,sensDistL, sensDistR);
			  disttodrive=400;  // bei befahren voraus stueckchenweise ranfahren
			}
			else {
				mapval=getpoint_side_dist(0,250, TRACKLEFT, &pos); //nur etwas voraus Punkt berechnen wenn weiter gesehen
				LOG_DEBUG("MapPunkt %1d fuer 25cm voraus: %1d",mapval);
			}
			// wenn Zielpunkt bekannt ist, dann nur Strecke bis Zielpunkt fahren und danach alle Plausis neu fuer moegliche Weiterfahrt
			if (nextline.point2.x!=0 || nextline.point2.y!=0) {
			 position_t bot_pos;
		     bot_pos.x=x_pos;
		     bot_pos.y=y_pos;
		     disttodrive = (int16_t) sqrtf((float) get_dist(bot_pos.x, bot_pos.y, pos.x, pos.y));
		     LOG_DEBUG("Weitsicht und P2 bekannt-nur bestimmte Laenge fahren %1d", disttodrive);
			}
		}

		if (nextline.point2.x!=0 || nextline.point2.y!=0) {
		  position_t botpos;
		    botpos.x=x_pos;
		    botpos.y=y_pos;
		   // Wenn Tracklaenge kleiner als Laenge voraus laut Abstandssensoren, dann Tracklaenge nehmen
		   if (get_dist(nextline.point1.x, nextline.point1.y,
					nextline.point2.x, nextline.point2.y) < get_dist(botpos.x,
					botpos.y, pos.x, pos.y)) {
				getpoint_side_dist(0, (int16_t) sqrtf(
						(float) get_dist(nextline.point1.x, nextline.point1.y,
								nextline.point2.x, nextline.point2.y)),
						TRACKLEFT, &pos); // nur etwas voraus Punkt berechnen wenn weiter gesehen
				LOG_DEBUG("Mappunkt korrigiert fuer kleinere Tracklaenge %1.0f",
						sqrt(get_dist(nextline.point1.x, nextline.point1.y, nextline.point2.x, nextline.point2.y)));
			}
		}

		// Wegfreiheit bis zum Punkt auf Strecke voraus bestimmen; manchmal kein Hind gesehen aber schon in Map
		uint8_t free1 = map_way_free(x_pos, y_pos, pos.x, pos.y, MAP_WAY_FREE_MARGIN);
		LOG_DEBUG("Wegfreiheit fuer Mappunkt voraus: %1d", free1);

		// ist der Weg voraus nicht frei laut Map, dann gar nicht erst fahren und verwerfen
		if (!free1) {
/*! @todo	Diese Strecke nicht gleich verwerfen sondern evtl untersuchen und Teilstrecken bilden; evtl. mit Pfadplanung andere Teilstrecke anfahren
			oder erst einmal mit anderem Weg tauschen, der gerade frei ist.
			da jetzt auch eigener Linienstack verwendet wird, koennte man einen Zaehler mitfuehren wie oft eine Strecke schon zurueckgestellt wurde und erst ab
			einem Schwellwert wirklich verwerfen */
			track_state = GET_LINE_FROM_STACK;
			LOG_DEBUG("Weg versperrt zu Zwischenpunkt %1d %1d, Endpunkt %1d %1d", pos.x, pos.y, nextline.point2.x, nextline.point2.y);
#if defined MAP_2_SIM_AVAILABLE && defined DEBUG_BEHAVIOUR_AREA
		    position_t aktpos;
		    aktpos.x=x_pos;
		    aktpos.y=y_pos;
		    draw_line_world(aktpos, pos, 1); // Linie wird rot, so weit voraus ist Weg nicht frei
#endif	// MAP_2_SIM_AVAILABLE

			break;
		}

		//naechster Verhaltenszustand nach diesen Pruefungen; notwendig damit Observer bereits
		//laufen vor direktem Losfahren
		track_state = GO_FORWARD;

		// anzufahrende naechste Strecke initialisieren
		set_point_to_lastpoint(&nextline.point1, 0, 0);
		//set_point_to_lastpoint(&nextline.point2, 0, 0);

		// Observer starten zum Beobachten der Nebenspuren
		start_observe_left_right(data);

		break;

	case GO_FORWARD: //nach bestandenen Pruefungen fuer Bahn voraus nun vorwaertsfahren
		track_state = DELAY_AFTER_FORWARD;    //naechster Verhaltenszustand
		next_behavstate = GET_LINE_FROM_STACK;  //Verhaltenszustand nach AFTER_FORWARD

		// wenn bereits Hindernis voraus gesehen wurde, dann nur Fahren bis kurz vor dem Hindernis falls
		// naemlich Hindernis wieder aus Blick der Sensoren verschwindet und nicht daran kleben bleibt
		if (disttodrive < 900) {
			bot_goto_dist(data, disttodrive, 1); // bei kleinem Abstand mit goto_dist fahren
			LOG_DEBUG("fahre disttodrive %1d", disttodrive);
			track_state = DELAY_AFTER_FORWARD;  // um Observer zu stoppen und Nebenbahnen zu pushen
			next_behavstate = CHECK_TRACKSIDE;  // mit voraus weiter nach AFTER_FORWARD und saemtlichen Checks voraus
			set_point_to_lastpoint(&nextline.point2, 0, 0);  // Zielpunkt ist dann bereits angefahren und einfach vorwaerts weiter
		} else {
			bot_goto_obstacle(data, DIST_AWARD_HAZ, 0); // mit hohem Abstand voraus via obstacle
			LOG_DEBUG("fahre obstacle ");
		}

		// damit auch Abgrund erkannt wird beim Vorfahren; obige Verhalten benutzen beide goto_pos
		bot_cancel_behaviour(data, bot_goto_pos_behaviour, check_haz_sensDist);
		break;

	case DELAY_AFTER_FORWARD:
		// kommt hierher, wenn das Fahrverhalten (cancel-Verhalten) Hindernis oder Abgrund voraus erkannt hat; hier wird etwas gewartet

		//bei Abgrund etwas rueckwaerts fahren
		if (border_fired) {
			bot_drive_distance(data, 0, -BOT_SPEED_NORMAL, OPTIMAL_DISTANCE / 10); // rueckwaerts bei Abgrund
		}

		lastCheckTime = TIMER_GET_TICKCOUNT_16; // Var auf Systemzeit setzen, damit etwas zeit vergehen kann im naechsten Zustand
		track_state = AFTER_FORWARD; //naechster Verhaltenszustand

		break;

	case AFTER_FORWARD:
		// kommt hierher, wenn das Verhalten Vorwaertsfahren an Wand angekommen ist und etwas Zeit abgewartet wurde; Bot blickt noch geradeaus zum Hindernis
		// Entscheidung treffen, wohin sich bot nun bewegen soll

		//Observer spaetestens hier anhalten, falls noch nicht geschehen, und gueltige Endpunkte in die Streckenvars vermerken
		if (!endrequest) {
			bot_stop_observe();
			break;
		}
		//Abgrundvar init.
		border_fired = False; //Abgrundvar init.

		//etwas hier verbleiben, damit Observer sicher gestoppt sind
		if (!timer_ms_passed_32(&lastCheckTime, CORRECTION_DELAY + 300))
			break;

		// naechster Verhaltenszustand abhaengig von gesetzter naechster Zustandsvariablen
		track_state = next_behavstate;

		// Falls es mit Weiterfahrt weitergehen soll, dann erst pruefen auf Befahren voraus
		if (track_state == CHECK_TRACKSIDE) {
			mapval=getpoint_side_dist(0, 250, TRACKLEFT, &pos);
			LOG_DEBUG("AFTER_FORWARD Spur voraus %1d", mapval);
			if (mapval > MAP_TRACKVAL) {
			      track_state = GET_LINE_FROM_STACK;
			      LOG_DEBUG("Spur knapp voraus befahren->zu Stackholen");
		     }
		}

		break;

	case TURN_TO_NEAREST:
		// in nextline befindet sich nun der naechste Fahrweg mit Start- und Endpunkt; hier wird der bot nun zum naechsten Zielpunkt ausgerichtet; das Ausrichten
		// haette auch Entfallen koennen weil ja sowieso direkt mit goto_pos gefahren wird, aber so wird erst in die Richtung des Fahrens gesehen und es kann schon
		// die Map aktualisiert und Hindernis gecheckt werden vor Start des Fahrverhaltens selbst
		// kommt nach dem Stackholen hierher

		// naechster Verhaltenszustand
		track_state = ONWALL_TO_TRACK;

		// Drehen des Bots zum nahesten Punkt bei Gueltigkeit desselben
		if (nextline.point1.x != 0 || nextline.point1.y != 0) {
			bot_turn(data, (int16_t) calc_angle_diff(nextline.point1.x - x_pos,
				nextline.point1.y - y_pos));
		}

		break;

	case ONWALL_TO_TRACK:
		// Bot ist hier zum naechsten Zielpunkt ausgerichtet und faehrt nun dahin
		// 2stufig zu einem weiten Zielpunkt fahren, damit Map aktualisiert werden kann ; zuerst bis 30cm ranfahren und dann je nach Mapwert weiter
		// das Fahrverhalten greift hier nur fuer nicht ganz nahe Distanzen
		BLOCK_BEHAVIOUR(data, 500); // etwas warten

		// Nach Drehung links und Abstand rechts ist zu klein (oder umgedreht) rueckwaerts fahren; der Punkt nebenan ist etwas vor seitlich und der bot dreht sich
		// somit zum Punkt aber etwas schraeg und der rechte Sensor ist etwas weiter weg als der linke (oder umgedreht), der bei zu nah unendlich liefert; also der von Wand weitere Sensor
		// wird zur Pruefung herangezogen, da bei zu nah der andere keinen gueltigen sinnvollen Wert liefert
		if (sensDistR <= DIST_AWARD_HAZ || sensDistL <= DIST_AWARD_HAZ) {
			// etwas zurueck um Map zu checken
			bot_drive_distance(data, 0, -BOT_SPEED_NORMAL, 2); // rueckwaerts bei Abgrund
			LOG_DEBUG("zu Nah->Back L: %1d R: %1d, ist < %1d", sensDistL, sensDistR, DIST_AWARD_HAZ);
			break; // in aktuellem Eingang drin bleiben
		}

		// Nach Ausrichtung zum Ziel darf kein Hind. in geringem Abstand voraus zu sehen sein
		if (check_haz_sensDist()) {
			track_state = GET_LINE_FROM_STACK;
			LOG_DEBUG("Hind voraus L: %1d R: %1d", sensDistL, sensDistR);
			break;
		}

		// naechster Verhaltenszustand
		track_state = GOTO_FIRST_DEST;

		// Abstand des bots zum ersten Zielpunkt bestimmen
		int16_t dist_to_point = (int16_t) sqrtf((float) get_dist(nextline.point1.x,
			nextline.point1.y, x_pos, y_pos));

		// Kennung fuer grossen Abstand zum Zielpunkt init.
		go_long_distance = (uint8_t) (dist_to_point > 150 ? True : False);

		// fuer grossen Abstand zum Zielpunkt wird hier gefahren bis Hindernis etwas voraus gesehen wird; dadurch kann die Map
		// aktualisiert werden und dann vor dem weiteren Ranfahren Entscheidungen getroffen werden
		if (dist_to_point > 400) {
			// es wird eine weite Distanz zurueckgelegt

			track_state = CHECK_DIST; // Restweg ermitteln nach Zuschlagen von cancel

			bot_goto_dist(data, dist_to_point - 400, 1); // bis 400mm vor Ziel

			// damit Bot nicht auf seinem weiten Weg zum Zielpunkt in den Abgrund faellt
			bot_cancel_behaviour(data, bot_goto_pos_behaviour,
					check_haz_sensDist);
		} else {
			// in Testparcours2 bleibt bot oft an unterer rechter Ecke haengen, weil der Weg als Frei erkannt wird obwohl auch laut
			// Map schon die Ecke Grau-Schwarz eingezeichnet ist (bei nicht uebergebener Karte mit on_the_fly Hindernis-Aktualisierung)
			// aber selbst mit uebergebener clean Karte aus sim-Export bleibt er haengen, da wap_way_free Weg frei liefert
			uint8_t free1 = map_way_free(x_pos, y_pos, nextline.point1.x, nextline.point1.y, MAP_WAY_FREE_MARGIN);
			LOG_DEBUG("kurze Wegefreiheit zu P1: %1d, l/r: %1d %1d, Zielabstand %1d", free1, sensDistL, sensDistR, dist_to_point);

			// Falls Abstand des Zielpunktes kleiner als ein gesehener Hindernisabstand ist, dann auch nicht anfahren
			if (free1 && (dist_to_point >= sensDistL + 20 || dist_to_point
					>= sensDistR + 20)) {
				LOG_DEBUG("Abstand zu fahren groesser als Hindabstand l/r: %1d %1d", sensDistL, sensDistR);
				free1 = False;
			}

			if (! free1) { // Visualisierung der Nicht Anfahrbarkeit
#if defined MAP_2_SIM_AVAILABLE && defined DEBUG_BEHAVIOUR_AREA  // zur Visualisierung Weg zu P1 Rot einfaerben
				position_t akt_pos;
				akt_pos.x = x_pos;
				akt_pos.y = y_pos;
				draw_line_world(akt_pos, nextline.point1, 1);//Linie von Botpos zu P1 wird rot
#endif	// MAP_2_SIM_AVAILABLE
			}

			// evtl erst den Punkt 2 anfahren wenn P1 nicht anfahrbar war, d.h. Punkte vertauschen
			if (!free1 /*&& (get_dist(nextline.point1.x, nextline.point1.y,nextline.point2.x, nextline.point2.y)>300*300)*/) {
				uint8_t free2 = map_way_free(x_pos, y_pos, nextline.point2.x,
						nextline.point2.y, MAP_WAY_FREE_MARGIN);
				LOG_DEBUG("Wegfreiheit P2: %1d, Abstand P1: %1.0f P2: %1.0f", free2, sqrtf(get_dist(x_pos, y_pos, nextline.point1.x, nextline.point1.y)),
						sqrtf(get_dist(x_pos, y_pos, nextline.point2.x, nextline.point2.y)));
				if (free2) {
					set_nextline(nextline.point1, nextline.point2, True); // Werte egal da nextline nur getauscht
					next_behavstate = TURN_TO_NEAREST;
					track_state = AFTER_FORWARD; // muss sich hiernach zum Zielpunkt ausrichten
					LOG_DEBUG("P2 anfahren, da dieser frei");
					break;
				}
			}

			// ist der Weg voraus nicht frei laut Map in geringerem Abstand, dann gar nicht erst fahren und verwerfen
			if (! free1) {
				track_state = GET_LINE_FROM_STACK;
				LOG_DEBUG("P1 nicht anfahrbar %1d %1d", nextline.point1.x, nextline.point1.y);

#if defined MAP_2_SIM_AVAILABLE && defined DEBUG_BEHAVIOUR_AREA // zur Visualisierung Weg zu P1 Rot einfaerben
				position_t aktpos;
				aktpos.x = x_pos;
				aktpos.y = y_pos;
				draw_line_world(nextline.point1, nextline.point2, 1); // eigentliche Fahrspur wird verworfen und auch Rot einfaerben
#endif	// MAP_2_SIM_AVAILABLE
				break;
			}
		}
		break;

	case CHECK_DIST:
		// Bot kommt nach langer Fahrt zum Startpunkt der naechsten Strecke hier an, bei Abbruch,
		// also Abbruchbed erfuellt kann es sein, dass ein Hindernis den Weg versperrt und es noch recht weit
		// zum Ziel ist; damit diese Strecke nicht verloren geht, Weg auf Stack nehmen

		// Bot hat sein Ziel erreicht ohne wegen Hindernis voraus Fahrt abgebrochen zu haben
		if (!check_haz_sensDist()) {
			track_state = GOTO_FIRST_DEST; // naechster Zustand ohne Abbruch
			break;
		}

		// hier wurde Fahrt unterbrochen auf Weg zu P1 wegen Hindernis und anzufahrende Strecke wird auf den Stack gesichert nach
		// Tausch mit letztem Eintrag, der gleich der erste sein wird; es besteht immerhin damit die Chance, dass in
		// einem spaeteren Anfahrversuch der Weg dorthin frei ist; besser waere vorn in den Stack zu schieben aber erst mal so
		// gibt es noch einen Stackeintrag, so wird die aktuell anzufahrende Strecke (nextline) auf den Stack gelegt und darueber
		// als gueltige naechste Strecke wieder den letzten Stackeintrag
		if (pop_stack_pos_line(&p_temp.point1, &p_temp.point2)) {
			push_stack_pos_line(nextline.point1, nextline.point2);
			push_stack_pos_line(p_temp.point1, p_temp.point2);
		}
		track_state = GET_LINE_FROM_STACK; // weiterfahren mit der naechsten gueltigen Strecke

		break;

	case GOTO_FIRST_DEST:
		// Bot ist hier nicht mehr weit von dem Start-Zielpunkt der naechsten Bahn entfernt und kann nun naeher ranfahren; Mapwerte sollten hier voraus vorliegen
		BLOCK_BEHAVIOUR(data, 500); // etwas warten
		// naechster Verhaltenszustand
		track_state = TURN_TO_DESTINATION;

		// falls weit gefahren wurde und Hindernis zum naechsten Zielpunkt erkannt wird aber der andere Streckenpunkt freie Fahrt bietet, dann einfach
		// Punkte tauschen und den anderen Punkt zuerst anfahren um sich diesem von der anderen Seite zu naehern
		if (go_long_distance && ((check_haz_sensDist()) || map_get_field(
				nextline.point1) < 0) && map_way_free(x_pos, y_pos,
				nextline.point2.x, nextline.point2.y, MAP_WAY_FREE_MARGIN)) {
			set_nextline(nextline.point1, nextline.point2, True); // Werte egal da nextline nur getauscht
		}

		// Ersten Streckenpunkt nun anfahren
		bot_goto_pos(data, nextline.point1.x, nextline.point1.y, 999);

		lastCorrectionTime = 0; // Ablaufzeit init.


		// bei kleinem Abstand nur Abgrund ueberwachen sonst ebenfalls auch Hindernisse mit Sensoren
		if (go_long_distance) {
			bot_cancel_behaviour(data, bot_goto_pos_behaviour,
				check_haz_sensDist); // Abgrund und Abstandsensorcheck bei langer Entfernung (nach Stackholen oder rueckwaerts wg. zu nah)
		} else {
			bot_cancel_behaviour(data, bot_goto_pos_behaviour,
				check_border_fired); // kurzer Abstand nur Abgrundcheck oder sehr nah
		}

		break;

	case TURN_TO_DESTINATION:
		// Bot ist hier beim ersten Zielpunkt angekommen oder Cancelverhalten hat das Fahren dorthin unterbrochen wegen Hindernis oder Abgrund

		// nach Abgrund etwas rueckwaerts
		if (border_fired) {
			bot_drive_distance(data, 0, -BOT_SPEED_NORMAL, OPTIMAL_DISTANCE / 10);
			border_fired = False;
			break;
		}
		BLOCK_BEHAVIOUR(data, 500); // etwas warten

		// Ausrichten auf den Endpunkt der Fahrstrecke; ebenfalls wieder zum Aktualisieren der Map vor dem Anfahren selbst
		bot_turn(data, (int16_t) calc_angle_diff(nextline.point2.x - x_pos,
				nextline.point2.y - y_pos));

		// naechster Verhaltenszustand, welches wiederum der Starteintritt des Verhaltens selbst ist und geht hiermit von vorn los
		track_state = CHECK_TRACKSIDE;

		LOG_DEBUG("Ausrichten auf %1d %1d", nextline.point2.x, nextline.point2.y);
		break;

	case GET_LINE_FROM_STACK:
		// kommt hierher, wenn  die naechste Fahrstrecke zu holen ist; kann sowohl die Bahn daneben sein als auch eine weit
		// entfernte Strecke, die ehemals eine Wegalternative war

		// etwas hier verbleiben, damit Observer sicher gestoppt sind
		if (! timer_ms_passed_32(&lastCheckTime, CORRECTION_DELAY + 300)) {
			break;
		}

		// auf jeden Fall hier Stop der Obserververhalten
		if (! endrequest) {
			bot_stop_observe();
		}

		// Weg vom Stack holen und Ende falls leer
		if (! pop_stack_pos_line(&nextline.point1, &nextline.point2)) {
			LOG_DEBUG("Stack leer");
			track_state = TRACK_END;
			break;
		}

		// der naheste Punkt zum Bot laut Luftdistance wird nextline
		set_nextline(nextline.point1, nextline.point2, 0);

#if defined MAP_2_SIM_AVAILABLE && defined DEBUG_BEHAVIOUR_AREA
		draw_line_world(nextline.point1, nextline.point2, 0); // naechste anzufahrende Linie gruen
#endif
		LOG_DEBUG("Stackholen P1: %1d %1d P2: %1d %1d", nextline.point1.x, nextline.point1.y, nextline.point2.x, nextline.point2.y);

		if (map_get_field(nextline.point1) > MAP_TRACKVAL || map_get_field(nextline.point2) > MAP_TRACKVAL) {
			LOG_DEBUG("Start- oder EndPunkt schon befahren->Track verwerfen");
#if defined MAP_2_SIM_AVAILABLE && defined DEBUG_BEHAVIOUR_AREA
		map_draw_line_world(nextline.point1, nextline.point2, 1); // verworfene Linie rot
#endif
		  break;
		}

		// pruefen, ob Wege zu den Punkten frei sind und Freikennungen speichern
		free1 = map_way_free(x_pos, y_pos, nextline.point1.x, nextline.point1.y, MAP_WAY_FREE_MARGIN);
	    LOG_DEBUG("Wegfreiheit zu P1: %1d, Abstand P1: %1.0f P2: %1.0f", free1,
	    	sqrtf(get_dist(x_pos,y_pos, nextline.point1.x, nextline.point1.y)),
	    	sqrtf(get_dist(x_pos,y_pos, nextline.point2.x, nextline.point2.y)));

       // wenn Weg zu P1 nicht frei ist aber Weg zu P2 und Abstand nicht zu kurz ist, dann zuerst P2 anfahren
       if (! free1) {
			uint8_t free2 = map_way_free(x_pos, y_pos, nextline.point2.x,
					nextline.point2.y, MAP_WAY_FREE_MARGIN);
			LOG_DEBUG("Wegfreiheit zu P2: %1d, Abstand P1: %1.0f P2: %1.0f",
				free2, sqrtf(get_dist(x_pos, y_pos, nextline.point1.x, nextline.point1.y)),
				sqrtf(get_dist(x_pos, y_pos, nextline.point2.x, nextline.point2.y)));
			if (free2 && (get_dist(nextline.point2.x, nextline.point2.y, x_pos,
					y_pos) >= 300L * 300L)) {
				set_nextline(nextline.point1, nextline.point2, True); // Werte egal da nextline nur getauscht
				track_state = AFTER_FORWARD; // muss sich hiernach zum Zielpunkt ausrichten
				next_behavstate = TURN_TO_NEAREST; // Verhaltenszustand nach AFTER_FORWARD; muss sich nach Observerstopp zum 1. Zielpunkt ausrichten
				LOG_DEBUG("P2 anfahren und dann zu P1, da P2 frei");
				break;
			}
		}

		if (! free1) {
			uint8_t free2 = map_way_free(x_pos, y_pos, nextline.point2.x,
					nextline.point2.y, MAP_WAY_FREE_MARGIN);
			LOG_DEBUG("Wegfreiheit zu P2 ohne Pfadplanung: %1d, Mapwert: %1d", free2, map_get_field(nextline.point2));

			// falls Weg zu keinem Punkt frei, dann mit dem naechsten Stackweg tauschen; ist dieser auch nicht befahrbar,
			// wird dieser verworfen und der Weg
			if (! free2) {
				LOG_DEBUG("Stackweg nicht frei");

#ifdef GO_WITH_PATHPLANING
				bot_stop_observe(); // auf jeden Fall erst mal stoppen
				// Fuer Pfadplanung nur Strecken anfahren mit gewisser Laenge
				if (get_dist(nextline.point1.x, nextline.point1.y,
						nextline.point2.x, nextline.point2.y) > 200L * 200L) {
					if (get_dist(nextline.point1.x, nextline.point1.y, x_pos,
							y_pos) >= 300L * 300L) {
						LOG_DEBUG("Weg nicht frei, >> Pfadplanung zu P1 << %1d %1d", nextline.point1.x, nextline.point1.y);
					} else {
						set_nextline(nextline.point1, nextline.point2, True); // Werte egal da nextline nur getauscht
						LOG_DEBUG("Weg nicht frei, >> Pfadplanung zu P2 nach Tausch wg. zu geringem Abstand << %1d %1d",
							nextline.point1.x, nextline.point1.y);
					}
					bot_calc_wave(data, nextline.point1.x, nextline.point1.y, /*MAP_DRIVEN_THRESHOLD*/0);
/*! todo Pfadplanung ueber befahrenes Gebiet wehnn es denn mal geht */
					track_state = TURN_TO_DESTINATION; // muss sich hiernach zum Zielpunkt ausrichten
					break;
				} else {
					LOG_DEBUG("Strecke zu kurz fuer Pfadplanung, evtl. Vertauschen");
				}
#else

				static trackpoint_t pt; // zum Merken beim Vertauschen

				if (pop_stack_pos_line(&pt.point1, &pt.point2)) {

					free1 = map_way_free(x_pos, y_pos, pt.point1.x, pt.point1.y, MAP_WAY_FREE_MARGIN);
					free2 = map_way_free(x_pos, y_pos, pt.point2.x, pt.point2.y, MAP_WAY_FREE_MARGIN);

					if (!free1 && !free2) {
						LOG_DEBUG("Folgeweg auch nicht frei"); // Weg verworfen wenn nicht anfahrbar->naechsten Stackweg holen
						break;
					}

					// falls naechster Stackweg auch nicht frei, dann weiter Stackwege holen bis zum Ende
					// hier ist aber der Folgeweg anfahrbar und wird getauscht mit vorher geholtem, noch nicht anfahrbarem Weg
					push_stack_pos_line(nextline.point1, nextline.point2); // wieder auf Stack
					push_stack_pos_line(pt.point1, pt.point2); // wieder auf Stack
					LOG_DEBUG("Weg mit Folgeweg getauscht");
					break;
				}
#endif	// GO_WITH_PATHPLANING
			}
		}

		// naechster Verhaltenszustand zum Anfahren der naechsten Bahn mit Ausrichten auf den naeheren Punkt laut nextline
		track_state = TURN_TO_NEAREST;
		break;

	default:
		// am Ende des Verhaltens Observer stoppen
		bot_stop_observe();
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Startet das Verhalten bot_drive_area_behaviour
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_drive_area(Behaviour_t * caller) {
	/* ein paar Initialisierungen sind notwendig */
	lastCorrectionTime = 0;
	switch_to_behaviour(caller, bot_drive_area_behaviour, OVERRIDE);
	track_state = CHECK_TRACKSIDE;
	border_fired = False;
	pos_store = pos_store_create_size(get_behaviour(bot_drive_area_behaviour), pos_store_data, STACK_SIZE);

	/* Kollisions-Verhalten ausschalten  */
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
	deactivateBehaviour(bot_avoid_col_behaviour);
#endif

	/* Einschalten sicherheitshalber des Scan-on_the_fly Verhaltens */
#ifdef BEHAVIOUR_SCAN_AVAILABLE
	activateBehaviour(get_behaviour(bot_drive_area_behaviour), bot_scan_onthefly_behaviour);
#endif
}

#endif	// BEHAVIOUR_DRIVE_AREA_AVAILABLE
