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

#ifdef BEHAVIOUR_DRIVE_AREA_AVAILABLE

#include <math.h>
#include <stdlib.h>

#include "map.h"
#include "timer.h"
#include "math_utils.h"
#include "pos_stack.h"
#include "log.h"


//#define DEBUG_DRIVE_AREA	// Schalter fuer Debug-Code

#ifndef DEBUG_DRIVE_AREA
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif

/*! nur alle X-Millisekunden Mapzugriff der Observer-Verhalten */
#define	CORRECTION_DELAY	700

/*! Mapwert zur Erkennung einer bereits befahrenen Spur */
#define MAP_TRACKVAL   30

/*!  In diesem Abstand voraus [mm] ab Abstandssensoren wird auf Hindernis gecheckt */
#define DIST_AWARD_HAZ  150

/*! ca. 1 1/2 Botbreiten, Mindestabstand der Punkte fuer eine gueltige Bahn, welche es zu befahren gilt */
#define DIST_VALID_QUAD (uint32_t)(150 * 150)

/*! anzufahrender Punkt gilt als sehr nah zum Bot, wenn Abstand kleiner Botdurchmesser (Quadrat) */
#define DIST_VALID_LOW (uint32_t)(BOT_DIAMETER * BOT_DIAMETER)

/*! Defines zum festlegen, auf welcher Seite des Bots sich die Spur befindet */
#define TRACKLEFT  -1
#define TRACKRIGHT 1

/*! Verhalten-Define zum Verlassen */
#define TRACK_END 99

/*! zu erreichender Punkt laut Observer neben dem Bot auf Parallelbahn; Korrekturwert, um den der
 *  Seitenabstand (Botdurchmesser) subtrahiert wird
 */
#define SIDEDIST_MINUS  30


/*! Statusvariable des area-Verhaltens */
static uint8_t track_state = 0;

/*! Notfallkennung fuer Abgrund; wird gesetzt in registrierter Notfallroutine */
static uint8_t border_fired = False;

/*! Statusvars fuer die Observer-Verhalten links und rechts */
static uint8_t observe1_state = 0;
static uint8_t observe2_state = 0;

/*! Zeit-Merkvariable fuer die Observer-Verhalten fuer Mapzugriff erst nach Ablauf einer gewissen Zeit */
static uint32_t lastCorrectionTime = 0;
static uint32_t lastCorrectionTimeObserveLeft = 0;
static uint32_t lastCorrectionTimeObserveRight = 0;

/*! Strukturtyp fuer die beobachteten Nachbar-Bahnen; fuer Links und Rechts bestehend aus 2 Punkten */
typedef struct {
	int16_t point1x; /*!< Einzelne Punkte */
	int16_t point1y;
	int16_t point2x;
	int16_t point2y;
} trackpoint_t;

/*! Nachbarbahnen links und rechts */
static trackpoint_t observe_trackleft;
static trackpoint_t observe_trackright;

/*! nachste anzufahrende Bahn */
static trackpoint_t nextline;


/* ==================================================================================
 * ===== Start der allgemein fuer diese Verhalten notwendigen Routinen ==============
 * ==================================================================================*/

/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung und muss registriert werden
 */
void border_drive_area_handler(void) {
	// Routine setzt hier einfach nur die Borderkennung, die vom area-Verhalten ausgewertet wird
	  border_fired = True;
}

/*!
 * liefert den Wert eines Feldes als Durchschnittwert laut Map in bestimmtem Umkreis
 * @param x	X-Weltkoordinate
 * @param y	Y-Weltkoordinate
 * @return	Durchschnittswert des Feldes (>0 heisst frei, <0 heisst belegt)
 */
static int8_t map_get_field(int16_t x, int16_t y) {
#ifdef DEBUG_DRIVE_AREA
	uint8_t result;
	TIMER_MEASURE_TIME(result = map_get_average(x, y, 50));
	return result;
#else
	return map_get_average(x, y, 50);  // Mapwert Durchschnitt
#endif
}

/*!
 * Speichert eine Line auf dem Stack, d.h. den Start- und Endpunkt der Linie
 * @param x1	X-Koordinate des ersten Punktes der Linie
 * @param y1	Y-Koordinate des ersten Punktes der Linie
 * @param x2	X-Koordinate des zweiten Punktes der Linie
 * @param y2	Y-Koordinate des zweiten Punktes der Linie
 */
static void push_stack_pos_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

	if ((x1 == 0 && y1 == 0) || (x2 == 0 && y2 == 0))
		return; // ungueltige Werte werden nicht uebernommen

	// Push der Einzelpunkte
	pos_stack_push(x1, y1);
	pos_stack_push(x2, y2);
}

/*!
 * Holt eine Line vom Stack, d.h. die beiden Punkte der Linie werden zurueckgegeben
 * @param *x1	X-Koordinate des ersten Punktes der Linie
 * @param *y1	Y-Koordinate des ersten Punktes der Linie
 * @param *x2	X-Koordinate des zweiten Punktes der Linie
 * @param *y2	Y-Koordinate des zweiten Punktes der Linie
 * @return True wenn erfolgreich, False falls Stack leer ist
 */
static uint8_t pop_stack_pos_line(int16_t * x1, int16_t * y1, int16_t * x2,
		int16_t * y2) {
	int16_t x_i;
	int16_t y_i;
	if (!pos_stack_pop(&x_i, &y_i))
		return False;
	*x1 = x_i;
	*y1 = y_i;

	if (!pos_stack_pop(&x_i, &y_i))
		return False;
	*x2 = x_i;
	*y2 = y_i;
	return True;
}

/*!
 * Hilfsroutine, um 2 Punkte in die Merkvariable zu speichern
 * @param *lastpointx X-Merkvariable
 * @param *lastpointy Y-Merkvariable
 * @param mapx zu merkender X-Wert
 * @param mapy zu merkender Y-Wert
 */
static void set_point_to_lastpoint(int16_t * lastpointx, int16_t * lastpointy,
		int16_t mapx, int16_t mapy) {
	*lastpointx = mapx;
	*lastpointy = mapy;
}

/*!
 * Liefert True, wenn der Abstand zwischen den beiden Punkten gueltig ist, d.h. erreicht oder ueberschritten wurde
 * @param xs World-Koordinate des zu untersuchenden Punktes
 * @param ys World-Koordinate des zu untersuchenden Punktes
 * @param xd World-Koordinate des Zielpunktes
 * @param yd World-Koordinate des Zielpunktes
 * @return True bei Erreichen des Abstandes zwischen den beiden Punkten sonst False
 */
static uint8_t dist_valid(int16_t xs, int16_t ys, int16_t xd, int16_t yd) {
	// falls ein Punktepaar noch 0 hat, so ist dies ungueltig
	if ((xs == 0 && ys == 0) || (xd == 0 && yd == 0))
		return False;

	/* Abstandsermittlung nach dem guten alten Pythagoras ohne Ziehen der Wurzel */
	return (get_dist(xs, ys, xd, yd) > DIST_VALID_QUAD);
}

/*!
 * Liefert True, wenn der Abstand zwischen den beiden Punkten sehr klein ist
 * @param xs World-Koordinate des zu untersuchenden Punktes
 * @param ys World-Koordinate des zu untersuchenden Punktes
 * @param xd World-Koordinate des Zielpunktes
 * @param yd World-Koordinate des Zielpunktes
 * @return True bei geringem Abstand zwischen den beiden Punkten sonst False
 */
static uint8_t pointdist_low(int16_t xs, int16_t ys, int16_t xd, int16_t yd) {

	/* Abstandsermittlung nach dem guten alten Pythagoras ohne Ziehen der Wurzel */
	return (get_dist(xs, ys, xd, yd) < DIST_VALID_LOW);
}

/*!
 * Berechnung des Seitenpunktes der Nebenbahn mit Rueckgabe des Punktes und des Map-Durchschnittswertes
 * @param sidedist	seitlicher Abstand vom Botmittelpunkt
 * @param dist 		Punkt im Abstand voraus
 * @param side 		Trackleft (-1) oder Trackright (+1) fuer links oder rechts vom bot gesehen
 * @param *point 	berechnete Map-Weltkoordinate
 * @return 			Mapwert an dem ermittelten Punkt in Blickrichtung zum Abstand dist
 */
static int8_t getpoint_side_dist(uint16_t sidedist, int16_t dist, int8_t side,
		position_t * point) {
	// Berechnung je nach zu blickender Seite des Bots
	*point = (side == TRACKLEFT) ? calc_point_in_distance(heading, DISTSENSOR_POS_FW
			+ dist, sidedist) : calc_point_in_distance(
			heading, DISTSENSOR_POS_FW + dist, -sidedist);

	// Rueckgabe des Mapwertes
#ifdef DEBUG_DRIVE_AREA
	uint8_t result;
	TIMER_MEASURE_TIME(result = map_get_average(point->x, point->y, 20));
	return result;
#else
	return map_get_average(point->x, point->y, 20); // Mapwert Durchschnitt ueber 2 cm
#endif
}


/* ==========================================================================================
 * ===== Obserververhalten und der dafuer notwendigen Routinen; es gibt 2 Observer-Verhalten,
 * ===== wobei eines die Bahn links und das andere die Bahn rechts vom Bot beobachtet
 * =========================================================================================*/


/*! Verhaltensroutine fuer beide Observer zur Ermittlung des Startpunktes der Nebenspur
 * Je nach zu checkender Botseite wird der Punkt in der Nachbarspur auf Anfahrbarkeit gecheckt
 * und hier der Startpunkt der nebenbahn ermittelt
 * @param checkside  zu checkende Seite (TRACKLEFT TRACKRIGHT)
 * @param *observe Zeiger auf die Koordinaten der Bahn, hier Startpunkt zurueckgegeben
 * @param *lastCorrectTime Zeiger auf Timervariable; erst nach Ablauf einer Zeitspanne erfolgt Mapzugriff
 * @return True falls Startpunkt anfahrbar ist und zurueckgegeben wurde sonst False
 */
static uint8_t observe_get_startpoint(int8_t checkside, trackpoint_t * observe,
		uint32_t * lastCorrectTime) {
	position_t map_pos;
	int8_t mapval;

	if (map_locked() == 1) {
		return False;
	}

	// nur alle x ms Mapzugriff des Observers
	if (!timer_ms_passed(lastCorrectTime, CORRECTION_DELAY))
		return False;

	// hier seitenabhaengig auf die Spur nebenan sehen auf Hoehe Mittelpunkt und Mapwert checken
	mapval = getpoint_side_dist(BOT_DIAMETER - SIDEDIST_MINUS,
			-DISTSENSOR_POS_FW, checkside, &map_pos);

	// falls Punkt bereits als befahren gekennzeichnet ist oder Hinderniswert hat gehts weiter mit Startpunktsuche
	if (mapval >= MAP_TRACKVAL || mapval < 0)
		return False;

	// hier ist also der Trackpunkt frei, dann merken und zur Endpunktsuche im Observer
	// zuweisen der Initwerte in die richtigen Puntvars
	set_point_to_lastpoint(&observe->point1x, &observe->point1y, map_pos.x, map_pos.y);
	set_point_to_lastpoint(&observe->point2x, &observe->point2y, 0, 0);

	return True;
}

/*!  Kennung, dass die Observer-Verhalten beendet werden sollen; bei True soll Verhalten sofort beendet werden mit
 *   vorherigem Gueltigkeitscheck des Endpunktes
 */
static uint8_t endrequest = False;

/*! Statusvariable der Obserververhalten */
#define OBSERVE_SEARCH_STARTPOINT     0
#define OBSERVE_SEARCH_ENDPOINT       1

/*!
 * Verhaltensroutine fuer beide Observer zur Endepunktermittlung; liefert True wenn der Endepunkt gueltig ist und damit ermittelt werden konnte
 * bei einem Teilhindernis auf der Nebenbahn wird eine gueltige befahrbare Strecke automatisch auf den Stack gelegt zum spaeteren Anfahren
 * @param checkside			Seite der zu beobachtenden Nebenbahn (TRACKLEFT TRACKRIGHT)
 * @param *behavstate		Neuer Zustand des Obserververhalten
 * @param *observer			Zeiger auf Punkte der Nebenbahn, gueltiger Endepunkt wird hier vermerkt
 * @param *lastpointx		letzter gueltiger X-Wert der Nebenbahn, verwendeter Wert bei Erkennung Hindernis
 * @param *lastpointy		letzter gueltiger Y-Wert der Nebenbahn, verwendeter Wert bei Erkennung Hindernis
 * @param *lastCorrectTime	Merkvariable der abgelaufenen Zeit; erst nach Ablauf gewisser Zeit erfolgt Mapzugriff
 * @return True falls Endpunkt auf der Seite ermittelt werden konnte sonst False
 */
static uint8_t observe_get_endpoint(int8_t checkside, uint8_t * behavstate,
		trackpoint_t * observer, int16_t * lastpointx, int16_t * lastpointy,
		uint32_t * lastCorrectTime) {
	position_t map_pos;
	int8_t mapval = 0;

	if (map_locked() == 1) {
		return False;
	}

	// wenn Zeit noch nicht um dann sofort beenden ohne weitere Checks
	if (!endrequest && !timer_ms_passed(lastCorrectTime, CORRECTION_DELAY))
		return False;

	// hier seitenabhaengig auf die jeweilige Spur sehen und Mapwert holen
	mapval = getpoint_side_dist(BOT_DIAMETER - SIDEDIST_MINUS, 0, checkside,
			&map_pos);

	//Endepunktsuche nicht weiter verfolgen falls noch kein Startpunkt vorliegt
	if ((observer->point1x == 0 && observer->point1y == 0)) {
		*behavstate = (endrequest) ? TRACK_END : 0; // weiter mit Startpunktsuche oder Ende
		return False;
	}

	if (mapval < 0) { //Punkt auf Nebenbahn hat Hinderniswahrscheinlichkeit

		if (*lastpointx == 0 && *lastpointy == 0) {
			// noch kein gueltiger Zwischenendpunkt ermittelt; Ende falls Endeanforderung kam sonst weiter mit Endepunktsuche
			if (endrequest)
				*behavstate = TRACK_END;

			return False;
		}

		// bei Hindernis auf aktuellem Punkt wird der letzte gemerkte Nicht-Hindernispunkt auf den Stack gespeichert, der
		// vorher auf Gueltigkeit gecheckt wird, d.h. muss bestimmten Mindestabstand zum Startpunkt haben und kein Hindernis sein
		if (dist_valid(observer->point1x, observer->point1y, map_pos.x, map_pos.y)
				&& map_get_field(*lastpointx, *lastpointy) >= 0 && !endrequest) {
			set_point_to_lastpoint(&observer->point2x, &observer->point2y,
					*lastpointx, *lastpointy);

			//zuletzt gueltigen Endpunkt auf Stack sichern
			push_stack_pos_line(observer->point1x, observer->point1y,
					*lastpointx, *lastpointy);

			set_point_to_lastpoint(lastpointx, lastpointy, 0, 0);
			set_point_to_lastpoint(&observer->point1x, &observer->point1y, 0, 0);
			*behavstate = (endrequest) ? TRACK_END : 0; // weiter mit Startpunktsuche
			return True;
		}

		if (endrequest) // Verhaltensende nach Anforderung
			*behavstate = TRACK_END;

		return False; //Verhalten erst mal beendet


	} else { //Punkt auf Nebenbahn hat keine Hinderniswahrscheinlichkeit

		// Abstand zum Startpunkt muss gueltig sein
		if (!(dist_valid(observer->point1x, observer->point1y, map_pos.x, map_pos.y))) {
			if (endrequest)
				*behavstate = TRACK_END;

			return False;
		}

		// gueltigen Zwischenpunkt merken
		set_point_to_lastpoint(lastpointx, lastpointy, map_pos.x, map_pos.y);

		// falls sich bei diesem gueltigen Endpunkt herausstellt, dass der vorhin ermittelte Startpunkt nun Hinderniswert besitzt, so wird der Endpunkt zum Startpunkt
		// und weiter mit Endpunktsuche
		if (map_get_field(observer->point1x, observer->point1y) < 0) {
			set_point_to_lastpoint(&observer->point1x, &observer->point1y,
					*lastpointx, *lastpointy); // Startpunkt ueberschreiben
			if (endrequest)
				*behavstate = TRACK_END;
			set_point_to_lastpoint(lastpointx, lastpointy, 0, 0);
			set_point_to_lastpoint(&observer->point2x, &observer->point2y, 0, 0);
			return True;
		}

		// freien befahrbaren Punkt merken; bei Endeanforderung ist dies der aktuelle Endpunkt
		if (!endrequest) // weiter nur falls kein Ende kommen soll
			return False;

		// gueltigen Endepunkt merken
		set_point_to_lastpoint(&observer->point2x, &observer->point2y,
				*lastpointx, *lastpointy);
	}

	*behavstate = (endrequest) ? TRACK_END : 0;

	return True; // Streckenpunkt ist hier gueltig

}

/*! Merkvariable des letzten gueltigen Endpunktes fuer Obserververhalten linke Spur */
static int16_t lastpointleft_x = 0;
static int16_t lastpointleft_y = 0;

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
		if (!observe_get_startpoint(CHECK_SIDE, &observe_trackleft,
				&lastCorrectionTimeObserveLeft))
			break;

		observe1_state = OBSERVE_SEARCH_ENDPOINT;
		break;

	case OBSERVE_SEARCH_ENDPOINT:
		// Anfangspunkt wurde bestimmt und ist hier auf der Suche nach dem Endpunkt; ein Endpunkt wird als Ende der Strecke
		// vom bereits vorhandenen Startpunkt erkannt, wenn der Botbereich neben dem Bot nicht mehr befahren werden kann; es
		// wird dann der zuletzt gemerkte befahrbare Punkt als Endpunkt genommen
		// nur alle x ms Mapvergleich

		if (!observe_get_endpoint(CHECK_SIDE, &observe1_state,
				&observe_trackleft, &lastpointleft_x, &lastpointleft_y,
				&lastCorrectionTimeObserveLeft))
			break;

		break;
	default:
		if (observe_trackleft.point2x == 0 && observe_trackleft.point2y == 0)
			set_point_to_lastpoint(&observe_trackleft.point1x,
					&observe_trackleft.point1y, 0, 0);
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Rufe das Observer-Verhalten Links auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_observe_left(Behaviour_t * caller) {
	if (!behaviour_is_activated(bot_observe_left_behaviour)) {
		switch_to_behaviour(caller, bot_observe_left_behaviour, OVERRIDE);
		observe1_state = 0;
		endrequest = False;
		set_point_to_lastpoint(&observe_trackleft.point1x,
				&observe_trackleft.point1y, 0, 0);
		set_point_to_lastpoint(&observe_trackleft.point2x,
				&observe_trackleft.point2y, 0, 0);
		set_point_to_lastpoint(&lastpointleft_x, &lastpointleft_y, 0, 0);
		lastCorrectionTimeObserveLeft = 9999;
	}
}

/*! Merkvariable des letzten gueltigen Endpunktes fuer Obserververhalten rechte Spur */
static int16_t lastpointright_x = 0;
static int16_t lastpointright_y = 0;

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
		if (!observe_get_startpoint(CHECK_SIDE, &observe_trackright,
				&lastCorrectionTimeObserveRight))
			break;

		observe2_state = OBSERVE_SEARCH_ENDPOINT;

		break;

	case OBSERVE_SEARCH_ENDPOINT:
		// Anfangspunkt wurde bestimmt und ist hier auf der Suche nach dem Endpunkt; ein Endpunkt wird als Ende der Strecke
		// vom bereits vorhandenen Startpunkt erkannt, wenn der Botbereich neben dem Bot nicht mehr befahren werden kann; es
		// wird dann der zuletzt gemerkte befahrbare Punkt als Endpunkt genommen
		// nur alle x ms Mapvergleich

		if (!observe_get_endpoint(CHECK_SIDE, &observe2_state,
				&observe_trackright, &lastpointright_x, &lastpointright_y,
				&lastCorrectionTimeObserveRight))
			break;

		break;
	default:
		if (observe_trackright.point2x == 0 && observe_trackright.point2y == 0)
			set_point_to_lastpoint(&observe_trackright.point1x,
					&observe_trackright.point1y, 0, 0);
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Rufe das Rechts-Observer-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_observe_right(Behaviour_t * caller) {
	if (!behaviour_is_activated(bot_observe_right_behaviour)) {
		switch_to_behaviour(caller, bot_observe_right_behaviour, OVERRIDE);
		observe2_state = 0;
		set_point_to_lastpoint(&observe_trackright.point1x,
				&observe_trackright.point1y, 0, 0);
		set_point_to_lastpoint(&observe_trackright.point2x,
				&observe_trackright.point2y, 0, 0);
		set_point_to_lastpoint(&lastpointright_x, &lastpointright_y, 0, 0);
		lastCorrectionTimeObserveRight = 9999;
	}

}

/*!
 * Startet die Observer
 */
static void start_observe_left_right(void) {
	endrequest = False;
	bot_observe_right(NULL);
	bot_observe_left(NULL);
}

/*!
 * Stopanforderung der beiden Observer-Verhalten links und rechts
 */
void bot_stop_observe(void) {
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
static uint8_t check_haz_in_map(void) {
	if (border_fired)
		return True;

	if (sensDistL <= DIST_AWARD_HAZ || sensDistR <= DIST_AWARD_HAZ)
		return True;

	return False;
}



/*!
 * Hilfsroutine, welche den Fahrtrack nextline mit den Koordinaten belegt; die Koordinaten
 * koennen hierbei vertauscht werden oder in point1x/y die zum Bot nahesten Werte gespeichert
 * @param x1 X-Koordinate des Punktes 1
 * @param y1 Y-Koordinate des Punktes 1
 * @param x2 X-Koordinate des Punktes 2
 * @param y2 Y-Koordinate des Punktes 2
 * @param change_points Punkte1 und 2 in nextline werden bei True vertauscht sonst belegt mit den Koordinaten
 */
static void set_nextline(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
		uint8_t change_points) {
	if (change_points) {
		int16 xtemp = nextline.point1x;
		int16 ytemp = nextline.point1y;
		nextline.point1x = nextline.point2x;
		nextline.point1y = nextline.point2y;
		nextline.point2x = xtemp;
		nextline.point2y = ytemp;
	} else {
		// der Punkt in der Naehe wird richtig nach nextline uebernommen; der naheste nach Punkt1
		if (get_dist(x1, y1, x_pos, y_pos) < get_dist(x2, y2, x_pos, y_pos)) {

			set_point_to_lastpoint(&nextline.point1x, &nextline.point1y, x1, y1);
			set_point_to_lastpoint(&nextline.point2x, &nextline.point2y, x2, y2);

		} else {
			// der Punkt in der Naehe wird richtig nach nextline uebernommen; der naheste nach Punkt1
			set_point_to_lastpoint(&nextline.point1x, &nextline.point1y, x2, y2);
			set_point_to_lastpoint(&nextline.point2x, &nextline.point2y, x1, y1);
		}
	}
}

 /*!
 * Routine zur Entscheidungsfindung, welche Richtung (links oder rechts) der bot nach Ankunft vor Hindernis nehmen soll; Routine
 * wird seitenabhaengig aufgerufen und gibt nach Gueltigkeitspruefung des Punktes in der Map die Gueltigkeit zurueck
 * koennen hierbei vertauscht werden oder in point1x/y die zum Bot nahesten Werte gespeichert
 * @param observe_track zu pruefende Seitenbahn mit Start- und Endpunkt aus Ermittlung des Observers
 * @param observe_track_to_push Koordinaten der anderen Seitenbahn, wird auf Stack gelegt als Alternativweg zum spaeteren Anfahren
 * @param checkside zu pruefende Seite der Bahn (TRACKLEFT TRACKRIGHT)
 * @param *side_to_go Rueckgabe der Seite, zu der der bot sich bewegen soll; 0 falls nicht gueltig
 * @return True falls die zu pruefende Seite gueltig ist, bot kann sich zu dieser Seite hinbewegen, sonst false
 */
 static uint8_t check_side_for_decision(trackpoint_t observe_track,
		trackpoint_t observe_track_to_push, int8_t checkside,
		int8_t * side_to_go) {
	position_t map_pos;
	int8_t mapval = 0;

	*side_to_go = 0;

	// Strecke muss vom Observer als gueltig gesetzt worden sein, nur dann gehts weiter
	if (observe_track.point1x == 0 && observe_track.point1y == 0)
		return False;

	// Punkt laut Map holen
	mapval = getpoint_side_dist(BOT_DIAMETER - SIDEDIST_MINUS, 0, checkside,
			&map_pos);

	//Mapwert darf keine Hinderniswahrscheinlichkeit haben und noch nicht befahren worden sein
	if (!(mapval < 0 || mapval >= MAP_TRACKVAL)) {

		//Observer-Track init.
		set_nextline(observe_track.point1x, observe_track.point1y,
				observe_track.point2x, observe_track.point2y, 0);

		//gueltige Seite zurueckgeben
		*side_to_go = checkside;

		// Alternative auf Stack speichern; d.h. die vom anderen Observer ermittelte gueltige Strecke auf Stack merken
		push_stack_pos_line(observe_track_to_push.point1x,
				observe_track_to_push.point1y, observe_track_to_push.point2x,
				observe_track_to_push.point2y);

		return True; // Seite ist gueltig und raus
	}
	return False; // Seite ungueltig
}


 /*!
 * Entscheidungsfindung, welche Richtung (links oder rechts) der bot nach Ankunft vor Hindernis nehmen soll; Routine
 * checkt die Gueltigkeit zuerst der rechten Seite (damit bevorzugter Weg) und erst dann die linke Seite
 * @param observe_trackleft  linke Seitenbahn mit Start- und Endpunkt aus Ermittlung des Observers
 * @param observe_trackright rechte Seitenbahn mit Start- und Endpunkt aus Ermittlung des Observers
 * @param *side_to_go Rueckgabe der Seite, zu der der bot sich bewegen soll; 0 falls nicht gueltig
 * @return True falls eine gueltige Seite links oder rechts ermittelt werden konnte
 */
 uint8_t decision_for_one_direction(trackpoint_t observe_trackleft,
		trackpoint_t observe_trackright, int8_t * side_to_go) {

	*side_to_go = 0; // erst mal init. mit kein Weg gueltig

	// zuerst nextline auf 0, ist keiner nah so ist nextline 0
	set_point_to_lastpoint(&nextline.point1x, &nextline.point1y, 0, 0);
	set_point_to_lastpoint(&nextline.point2x, &nextline.point2y, 0, 0);

	// Rechts ist ja die zuerst bevorzugte Richtung, also nach rechts gucken; dazu muss eine
	if (check_side_for_decision(observe_trackright, observe_trackleft,
			TRACKRIGHT, side_to_go))
		return True;

	// bei rechts ungueltig wird nun die linke Seite ueberprueft
	if (check_side_for_decision(observe_trackleft, observe_trackright,
			TRACKLEFT, side_to_go))
		return True;

	return False;

}


/* ==================================================================================
 * ===== Nun das Area-Fahrverhalten selbst                             ==============
 * ==================================================================================*/

/* Zustaende des Verhaltens */
#define CHECK_TRACKSIDE		0
//#define GO_ON_TRACK		1
#define AFTER_FORWARD		2
#define ONWALL_TO_TRACK		3
#define DELAY_AFTER_FORWARD 4
#define TURN_TO_DESTINATION 5
//#define GO_ON_WALL		6
#define GET_LINE_FROM_STACK 7
#define TURN_TO_NEAREST		8
//#define GO_BACK			9
#define GOTO_FIRST_DEST		10

/*!
 * Das Fahrverhalten selbst; Fahren bis zu einem Hindernis, drehen zu einer Seite und merken des anderen Weges auf den Stack; waehrend der
 * Fahrt werden die Nebenspuren beobachtet und bei Hindernissen in der Nebenspur automatisch Teilstrecken auf den Stack gelegt
 * @param *data der Verhaltensdatensatz
 */
void bot_drive_area_behaviour(Behaviour_t * data) {
	static int8_t direct_choice = 0; // Entscheidungsrichtung

	static uint8_t go_long_distance = 0; //Kennung ob nach Stackholen weit gefahren wurde

	static uint32_t lastCheckTime = 0;

	position_t pos;

	deactivateBehaviour(bot_cancel_behaviour_behaviour);	// Cancel-Verhalten abbrechen, damit es nicht ewig weiterlaeuft

	switch (track_state) {
	case CHECK_TRACKSIDE: //Vorwaeertsfahren mit Start der Observer

		// anzufahrende naechste Strecke erst mal initialisieren
		set_point_to_lastpoint(&nextline.point1x, &nextline.point1y, 0, 0);
		set_point_to_lastpoint(&nextline.point2x, &nextline.point2y, 0, 0);

		//Naechste anzufahrende Bahn vom Stack holen wenn die Bahnen nebenan schon befahren wurden oder Hindernis voraus
		if (getpoint_side_dist(BOT_DIAMETER - SIDEDIST_MINUS, 0, TRACKLEFT, &pos)
				>= MAP_TRACKVAL && getpoint_side_dist(BOT_DIAMETER
				- SIDEDIST_MINUS, 0, TRACKRIGHT, &pos) >= MAP_TRACKVAL) {
			track_state = GET_LINE_FROM_STACK;
			break;
		}

		if (check_haz_in_map()) { //Hindernis voraus erkannt
			track_state = GET_LINE_FROM_STACK;
			break;
		}

		//naechster Verhaltenszustand
		track_state = DELAY_AFTER_FORWARD;

		//Observer starten zum Beobachten der Nebenspuren
		start_observe_left_right();

		//Verhalten zum Vorwaertsfahren bis Hindernis voraus
		bot_goto_obstacle(data, DIST_AWARD_HAZ, 0);

		bot_cancel_behaviour(data, bot_goto_pos_behaviour, check_haz_in_map); //damit auch Abgrund erkannt wird beim Vorfahren

		break;

	case DELAY_AFTER_FORWARD:
		// kommt hierher, wenn das Fahrverhalten (cancel-Verhalten) Hindernis oder Abgrund voraus erkannt hat; hier wird etwas gewartet

		//bei Abgrund etwas rueckwaerts fahren
		if (border_fired) {
			bot_drive_distance(data, 0, -BOT_SPEED_NORMAL, OPTIMAL_DISTANCE/10); // rueckwaerts bei Abgrund
		}
		lastCheckTime = TIMER_GET_TICKCOUNT_16; // Var auf Systemzeit setzen, damit etwas zeit vergehen kann im naechsten Zustand
		track_state = AFTER_FORWARD; //naechster Verhaltenszustand
		BLOCK_BEHAVIOUR(data, 500); // etwas warten

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
		if (!timer_ms_passed(&lastCheckTime, CORRECTION_DELAY + 300))
			break;

		//naechster Verhaltenszustand
		track_state = TURN_TO_NEAREST;

		// Entscheidungslogik welche Richtung genommen wird; Alternative wird auf Stack gespeichert und der anzufahrende Weg in nextline
		if (decision_for_one_direction(observe_trackleft, observe_trackright,
				&direct_choice))
			break;

		// hier war keine Richtungsentscheidung moeglich, daher naechsten Weg from Stack holen
		track_state = GET_LINE_FROM_STACK;

		break;

	case TURN_TO_NEAREST:
		// in nextline befindet sich nun der naechste Fahrweg mit Start- und Endpunkt; hier wird der bot nun zum naechsten Zielpunkt ausgerichtet; das Ausrichten
		// haette auch Entfallen koennen weil ja sowieso direkt mit goto_pos gefahren wird, aber so wird erst in die Richtung des Fahrens gesehen und es kann schon
		// die Map aktualisiert und Hindernis gecheckt werden vor Start des Fahrverhaltens selbst

		//naechster Verhaltenszustand
		track_state = ONWALL_TO_TRACK;

		//Drehen des Bots zum nahesten Punkt bei Gueltigkeit desselben
		if (nextline.point1x != 0 || nextline.point1y != 0)
			bot_turn(data, calc_angle_diff(nextline.point1x - x_pos,
					nextline.point1y - y_pos));

		break;

	case ONWALL_TO_TRACK:
		//bot ist hier zum naechsten Zielpunkt ausgerichtet und faehrt nun dahin
		// 2stufig zu einem weiten Zielpunkt fahren, damit Map aktualisiert werden kann ; zuerst bis 30cm ranfahren und dann je nach Mapwert weiter
		// das Fahrverhalten greift hier nur fuer weite Distanzen (nach Stackholen)

		//naechster Verhaltenszustand
		track_state = GOTO_FIRST_DEST;

		//Mapwert des Zielpunktes ermitteln; hat dieser Hinderniswert, dann zuerst den anderen Punkt anfahren um sich diesem hier von der
		//anderen Seite zu naehern
		if (map_get_field(nextline.point1x, nextline.point1y) < 0) {
			track_state = TURN_TO_DESTINATION; //Zustand zum Anfahren des anderen Punktes
			break;

		}

		//Abstand des bots zum ersten Zielpunkt bestimmen (hier mal mit Ziehen der Wurzel-spaeter vielleicht mal mit Quadratwert)
		int16_t dist_to_point = sqrt(get_dist(nextline.point1x,
				nextline.point1y, x_pos, y_pos));

		go_long_distance = False; //Kennung fuer grossen Abstand zum Zielpunkt init.


		//fuer grossen Abstand zum Zielpunkt (nach Stackholen) wird hier gefahren bis Hindernis etwas voraus gesehen wird; dadurch kann die Map
		// aktualisiert werden und dann vor dem weiteren Ranfahren Entscheidungen getroffen werden
		if (dist_to_point > 300) {

			//es wird eine lange Distanz zurueckgelegt
			go_long_distance = True;

			//track_state = TRACK_END;
			bot_goto_dist(data, dist_to_point - 300, 1); // bis 300mm vor Ziel

			//damit bot nicht auf seinem weiten Weg zum Zielpunkt in den Abgrund faellt
			bot_cancel_behaviour(data, bot_goto_pos_behaviour, check_haz_in_map);
		}
		break;

	case GOTO_FIRST_DEST:
		//bot ist hier nicht mehr weit von dem Zielpunkt entfernt und kann nun naeher ranfahren; Mapwerte sollten hier voraus vorliegen

		//naechster Verhaltenszustand
		track_state = TURN_TO_DESTINATION;

		//falls weit gefahren wurde und Hindernis zum naechsten Zielpunkt erkannt wird aber der andere Streckenpunkt freie Fahrt bietet, dann einfach
		//Punkte tauschen und den anderen Punkt zuerst anfahren um sich diesem von der anderen Seite zu naehern
		if (((go_long_distance && check_haz_in_map()) || map_get_field(
				nextline.point1x, nextline.point1y) < 0) && map_way_free(x_pos,
				y_pos, nextline.point2x, nextline.point2y))
			set_nextline(0, 0, 0, 0, True); //Werte egal da nextline nur getauscht

		//Ersten Streckenpunkt nun anfahren
		bot_goto_pos(data, nextline.point1x, nextline.point1y, 999);

		lastCorrectionTime = 0; //Ablaufzeit init.

		//bei geringem Abstand nur Abgrund ueberwachen sonst ebenfalls auch Hindernisse mit Sensoren
		if (pointdist_low(nextline.point1x, nextline.point1y, x_pos, y_pos)) {
			bot_cancel_behaviour(data, bot_goto_pos_behaviour,
					check_border_fired); // kurzer Abstand nur Abgrundcheck
		} else
			bot_cancel_behaviour(data, bot_goto_pos_behaviour, check_haz_in_map); // Abgrund und Abstandsensorcheck

		break;

	case TURN_TO_DESTINATION:
		//bot ist hier beim ersten Zielpunkt angekommen oder Cancelverhalten hat das Fahren dorthin unterbrochen wegen Hindernis oder Abgrund

		//nach Abgrund etwas rueckwaerts und weiter mit Stackholen
		if (border_fired) {
			bot_drive_distance(data,0,-BOT_SPEED_NORMAL,OPTIMAL_DISTANCE/10);
			border_fired = False;
			track_state = GET_LINE_FROM_STACK;
			break;
		}

		//Ausrichten auf den Endpunkt der Fahrstrecke; ebenfalls wieder zum Aktualisieren der Map vor dem Anfahren selbst
		bot_turn(data, calc_angle_diff(nextline.point2x - x_pos,
				nextline.point2y - y_pos));

		//naechster Verhaltenszustand, welches wiederum der Starteintritt des Verhaltens selbst ist und geht hiermit von vorn los
		track_state = CHECK_TRACKSIDE;
		break;

	case GET_LINE_FROM_STACK:
		//kommt hierher, wenn Bot weder nach linsk noch rechts fahren kann und deswegen wird hier eine gemerkte Fahrstrecke
		//vom Stack geholt und angefahren

		// auf jeden Fall hier Stop der Obserververhalten
		bot_stop_observe();

		// Weg vom Stack holen und Ende falls leer
		if (!pop_stack_pos_line(&nextline.point1x, &nextline.point1y,
				&nextline.point2x, &nextline.point2y)) {
			track_state = TRACK_END;
			break;
		}

		// der naheste Punkt zum Bot laut Luftdistance wird nextline.x1y1
		set_nextline(nextline.point1x, nextline.point1y, nextline.point2x,
				nextline.point2y, 0);

		//Mapwerte des Start- und Endepunktes holen
		int8_t mapval1 = map_get_field(nextline.point1x, nextline.point1y);
		int8_t mapval2 = map_get_field(nextline.point2x, nextline.point2y);

		//pruefen, ob Wege zu den Punkten frei sind und Freikennungen speichern
		uint8_t free1;
		uint8_t free2;
#ifdef DEBUG_DRIVE_AREA
		TIMER_MEASURE_TIME(free1 = map_way_free(x_pos, y_pos, nextline.point1x, nextline.point1y); \
		free2 = map_way_free(x_pos, y_pos, nextline.point2x, nextline.point2y));
#else
		free1 = map_way_free(x_pos, y_pos, nextline.point1x, nextline.point1y);
		free2 = map_way_free(x_pos, y_pos, nextline.point2x, nextline.point2y);
#endif

		//falls nur der weiter liegende Punkt freie Fahrt voraus bietet, dann Reihenfolge vertauschen
		//sind beide eigentlich nicht anfahrbar laut Map aber der weiter liegende hat hoehere Freiwahrscheinlichkeit,
		//dann ebenfalls tauschen
		if ((free2 && !free1) || (!free1 && !free2 && (mapval2 > mapval1)))
			set_nextline(0, 0, 0, 0, True); //Werte egal da nextline nur Werte vertauscht

		//naechster Verhaltenszustand mit Ausrichten auf den naeheren Punkt laut nextline
		track_state = TURN_TO_NEAREST;
		break;

	default:
		// am Ende des Verhaltens Observer stoppen
		bot_stop_observe();
		pos_stack_clear(); // Stack bereinigen
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
	pos_stack_clear();

	/* Kollisions-Verhalten ausschalten  */
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
	deactivateBehaviour(bot_avoid_col_behaviour);
#endif

	/* Einschalten sicherheitshalber des Scan-on_the_fly Verhaltens */
#ifdef BEHAVIOUR_SCAN_AVAILABLE
	activateBehaviour(bot_scan_onthefly_behaviour);
#endif
}

#endif	// BEHAVIOUR_DRIVE_AREA_AVAILABLE
