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
 * @file 	behaviour_line_shortest_way.c
 * @brief 	Linienverfolger, der an Kreuzungen eine bestimmte Vorzugsrichtung einschlaegt (links) und diesen Weg weiterverfolgt, bis das Ziel
 *          (gruenes Feld an Wand) gefunden ist; Linien muessen immer an einem gruenen Feld ohne Hindernis enden, damit der Bot ein Ende erkennt und
 *          umdrehen kann.
 *          Die Kreuzungen und der eingeschlagene Weg werden auf dem Stack vermerkt, Wege die nicht zum Ziel fuehren vergessen; Am Ziel angekommen
 *          steht im Stack der kuerzeste Weg; Es kann nun via Display auf dem kuezesten Weg zum Ausgangspunkt zurueckgefahren werden oder der Bot wieder
 *          manuell an den Start gestellt werden und das Ziel auf kuerzestem Weg angefahren werden.
 * @author 	Frank Menzel (Menzelfr@gmx.de)
 * @date 	21.12.2008
 *
 * @todo	Unterstuetzung fuer Linienlabyrinthe mit Zyklen
 */

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE

#include "pos_store.h"
#include "rc5-codes.h"
#include "math_utils.h"

//#define DEBUG_BEHAVIOUR_LINE_SHORTEST_WAY // Schalter fuer Debug-Code

#ifndef LOG_AVAILABLE
#undef DEBUG_BEHAVIOUR_LINE_SHORTEST_WAY
#endif
#ifndef DEBUG_BEHAVIOUR_LINE_SHORTEST_WAY
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif

/*! Version des Linefolgers, die optimal fuer dieses Verhalten ist */
#define OPTIMAL_LINE_BEHAVIOUR_VERSION 3

/*! Check-Umkehrverhalten soll Drehrichtungswechsel erkennen (fuer Gruenfeld als Umkehrfeld auskommentieren)
 *  Gruenfeld nicht mehr Umkehrfeld sondern nur noch Zielfeld */
#define CHECK_REVERSE_BEHAVIOUR

#if FOLLOW_LINE_VERSION != OPTIMAL_LINE_BEHAVIOUR_VERSION
#warning "Optimal nur mit Version 3 des Linienfolgers"
#endif

/*! bevorzugte Richtung an Kreuzungen; 1 zuerst immer nach links, -1 rechts */
#define START_SIDEWISH	1

/*! Statusvariable des Verhaltens */
static int8_t lineState = 0;

/*! Farbe des Umkehr- und Zielfeldes, via Default auf gruen festgelegt */
#ifdef PC
#define GROUND_GOAL_DEF      GROUND_GOAL
#else
#define GROUND_GOAL_DEF      0x9AB  // damit es beim echten Bot nicht zu Fehlausloesungen kommt
#endif

/*! Positionsspeicher, den das Verhalten benutzt zum Speichern der Weginfos */
static pos_store_t * pos_store = NULL;

/*! Statischer Speicher fuer pos_store */
static position_t pos_store_data[POS_STORE_SIZE];

/*! Kennung links, welcher der Bordersensoren zugeschlagen hat zur Erkennung der Kreuzungen, notwendig
 *  weil sicht nicht immer beide gleichzeitig ueber Kreuzungslinie befinden */
static uint8_t border_side_l_fired = 0;

/*! Kennung rechts, welcher der Bordersensoren zugeschlagen hat zur Erkennung der Kreuzungen, notwendig
 *  weil sicht nicht immer beide gleichzeitig ueber Kreuzungslinie befinden */
static uint8_t border_side_r_fired = 0;

/*! Variable, die im Verhalten die Richtung an Kreuzung bestimmt; zum Beginn auf bevorzugte Startrichtung gesetzt */
static int8_t sidewish = START_SIDEWISH;

/*! Kennung ob bot sich von Kreuzung weg bewegt und einen Weg entdeckt (vorwaerts) oder sich auf dem Rueckweg befindet nach
 *  Erkennen der gruenen Umkehrfarbe */
static uint8_t way_back = 0;

/*! eingeschlagene Richtung an Kreuzung ausgehend von urspruenglichem Ankunftsweg */
static int8_t direction_counter = 0;

/*! Kreuzungstyp, hier nur die X-Kreuzungen erlaubt (4 Seiten); mitgefuehrt fuer moegliche Erweiterungen */
static int8_t crosstype = 0;

/*! Zusatnd des Verhaltens, ob Linienfolger Weg erst suchen soll oder den bereits im Stack befindlichen kuerzesten Weg abfahren soll;
 *  zum Abfahren des kuerzesten Weges aus Stack ist Wert True sonst False */
static uint8_t go_stack_way = 0;

/*! an der 1. Kreuzung wird Kennung gesetzt und erst ab dann ein Umkehrfeld ausgewertet */
static uint8_t crossing_reached = 0;

/*! zaehlt gruene Felder */
static uint8_t greencounter = 0;

/*! Merkkoordinaten fuer bereits gefahrene Strecke */
static int16_t lastpos_x = 0;
static int16_t lastpos_y = 0;

/*! Pruefung auf gefahrene Strecke notwendig fuer nicht optimalen Linienfolger */
#if FOLLOW_LINE_VERSION != OPTIMAL_LINE_BEHAVIOUR_VERSION
  #define DISTCHECK_NEEDED
#endif

/*!Check-Umkehrverhalten benoetigt ebenfalls Pruefung auf gefahrene Strecke; Define setzen */
#ifdef CHECK_REVERSE_BEHAVIOUR
  #define DISTCHECK_NEEDED
#endif

/*! Schwellwert fuer Pruefung der gefahrenen Strecke in mm
 */
#define CHECK_DISTANCE 60
#define CHECK_DISTANCE_QUAD (CHECK_DISTANCE * CHECK_DISTANCE)  // Quadrat der gefahrenen Strecke

/* Zustaende des Verhaltens */
#define GO_TO_LINE	              0
#define CHECK_LINE	              1
#define GO_FORWARD                2
#define TURN_SIDEWISH_ON_CROSSING 3
#define GO_FORWARD_AFTER_TURN     4
#define CHECK_BORDER              5
#define TURN_ON_GREEN             6
#define GOAL_FOUND                7
#define END					      99



/*     *************************************************************************
 *     ** Verhalten zum Erkennung einer 180 Grad Drehung **
 *     *************************************************************************
*/

#ifdef DISTCHECK_NEEDED

/*! Prueft ob der Bot schon eine bestimmte Strecke gefahren ist seit dem letzten Durchgang
 * @param  *last_xpoint   letzte gemerkte X-Koordinate
 * @param  *last_ypoint   letzte gemerkte Y-Koordinate
 * @return True, wenn Bot schon gewisse Strecke gefahren ist und Map zu checken ist sonst False
 */
static uint8_t distance_reached(int16_t * last_xpoint,int16_t * last_ypoint) {

	// Abstand seit letztem Observerlauf ermitteln
	uint16_t diff = get_dist(x_pos, y_pos, *last_xpoint, *last_ypoint);

	//erst nach gewissem Abstand oder gleich bei noch initialem Wert Mappruefung
	if (diff >= CHECK_DISTANCE_QUAD) {

		*last_xpoint = x_pos;
		*last_ypoint = y_pos;
		return True;
	}
	return False;
}

#endif  // nur wenn gefordert


#ifdef CHECK_REVERSE_BEHAVIOUR  // nur wenn Verhalten anstatt Gruenfeld verwendet werden soll

static uint8_t reverse_state=0;
static int16_t last_heading = 0;		/*!< letzte gemerkte Botausrichtung */
#define CHECK_ANGLE_REV 170           /*!< nach Erreichen dieses Drehwinkels [Grad] wird Richtungsumkehr erkannt */

static uint8_t bot_reverse=0;          /*!<  Kennung dass Bot die Umgedrehte Richtung eingenommen hat nach Richtungsumkehr */

int16_t angle_t=0;

/*!
 * Verhalten zum Checken auf Einnehmen der entgegengesetzten Richtung (Linienende)
 * @param *data	eigener Verhaltensdatensatz
 */
void bot_check_reverse_direction_behaviour(Behaviour_t * data) {
	switch (reverse_state) {
	case 0:
		last_heading = heading;
		reverse_state = 1;
		bot_reverse = False;
		//LOG_DEBUG("-Start Check reverse mit ang %1d", last_heading);
		/* no break */
	case 1:
		// bei Drehwinkelaenderung und Uberschreitung einer gewissen Groesse (um 180 Grad) feuern
		angle_t = turned_angle(last_heading);
		if (angle_t > 180)
			angle_t = 360 - angle_t;

		if (angle_t > CHECK_ANGLE_REV) {
			last_heading = heading;
			LOG_DEBUG("-um Winkel gedreht %1d", angle_t);
			bot_reverse = True; // Kennung muss vom Auswerteverhalten nach Erkennung rueckgesetzt werden
			return_from_behaviour(data); // gleich Deaktivierung nach Erkennung entgegengesetzte Richtung
		}

		break;

	default:
		// kommt eigentlich nie hierher, da es solange aktiv ist bis Deaktivierung eintritt (Notaus oder Steuerung vom Verhalten selbst)
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Botenfunktion: Verhalten zum Checken der Botrichtung auf Einnehmen der entgegengesetzten Richtung (Linienende)
 * @param *caller	Der Verhaltensdatensatz des Aufrufers
 */
static void bot_check_reverse_direction(Behaviour_t * caller) {
	// via Override umschalten; Aufruf erfolgt aller x mm gefahrene Strecke, falls Kurve gefahren wurde zum Setzen des neuen Pruefwinkels
	switch_to_behaviour(caller, bot_check_reverse_direction_behaviour, OVERRIDE);
	reverse_state = 0;
}
#else
/*!
 * Dummy fuer bot-logic.c
 * @param *data	Der Verhaltensdatensatz des Aufrufers
 */
void bot_check_reverse_direction_behaviour(Behaviour_t * data) {
}
#endif	// CHECK_REVERSE_BEHAVIOUR

/*!
 * Push der Kreuzungsinformationen, etwas tricky dazu der eigentliche Positionsspeicher benutzt
 * @param crosstype	Typ der Kreuzung (jetzt nur 1 Typ, gedacht fuer spaetere Erweiterungen und weil Postyp sowieso 2 Params hat)
 * @param direction	eingeschlagener Weg an der Kreuzung
 */
static void push_stack_crossing(int8_t crosstype, int8_t direction) {
	position_t p_temp; // Stack erlaubt nur Speicherung von Positionstypen
	p_temp.x = crosstype;
	p_temp.y = direction;

	// der eigentliche Push der Kreuzungsinformation
	pos_store_push(pos_store, p_temp);
}

/*!
 * Holt eine Kreuzungsinformation vom Stack (Bot auf Rueckweg an Kreuzung oder Stackabfahren vom Ziel zum Start)
 * @param *crosstype Kreuzungstyp
 * @param *direction zuletzt eingeschlagene Richtung
 * @return True wenn erfolgreich, False falls Stack leer ist
 */
static uint8_t pop_stack_crossing(int8_t * crosstype, int8_t * direction) {
	position_t p_temp; // Stack erlaubt nur Speicherung von Positionstypen
	if (!pos_store_pop(pos_store, &p_temp))
		return False;
	*crosstype = p_temp.x;
	*direction = p_temp.y;

	return True;
}

/*!
 * Holt eine Kreuzung von vorn aus dem Stack (in dem Fall Queue) zum Abfahren von Start- zum Zielpunkt
 * @param *crosstype Kreuzungstyp
 * @param *direction zuletzt eingeschlagene Richtung
 * @return True wenn erfolgreich, False falls Stack leer ist
 */
static uint8_t dequeue_stack_crossing(int8_t * crosstype, int8_t * direction) {
	position_t p_temp; // Stack erlaubt nur Speicherung von Positionstypen
	if (!pos_store_dequeue(pos_store, &p_temp))
		return False;
	*crosstype = p_temp.x;
	*direction = p_temp.y;

	return True;
}





/*!
 * Prueft ob sich der Bot auf dem definierten Umkehr- Zielfeld befindet (via Default gruen; gut im Simulator verwendbar)
 * @param goalcheck True falls Aufruf von Zielerkennung kam, False sonst fuer Pruefung auf Umkehrfeld
 * @return True falls Bot sich auf dem Farbfeld befindet sonst False
 */
static uint8_t green_field(uint8_t goalcheck) {
	if (crossing_reached && ((sensLineL > GROUND_GOAL_DEF - 5 && sensLineL
			< GROUND_GOAL_DEF + 5) || (sensLineR > GROUND_GOAL_DEF - 5
			&& sensLineR < GROUND_GOAL_DEF + 5))) {
#ifdef CHECK_REVERSE_BEHAVIOUR
        if (goalcheck) {  // nur fuer Zielcheck
          return True;
        }
#else
        if (goalcheck) {  // bei Zielpruefung sofort True
        	return True;
        }
        else {
		  greencounter++;
		  LOG_DEBUG("Gruen erkannt counter %1d, l/r %1d %1d", greencounter, sensLineL, sensLineR);
		  if (greencounter > 5) {  // muss mehrmals hintereinander Gruenfeld erkennen um Fehlausloesungen besser zu vermeiden
			  greencounter = 0;
  		      return True;
		  }
        }
#endif	// CHECK_REVERSE_BEHAVIOUR
	}
	return False;
}

/*!
 * Prueft ob der Bot sich auf dem Ziel befindet, also wenn er auf dem definierten Farbfeld steht und Hindernis dahinter
 * @return True falls bot sich auf dem Zielfarbfeld befindet sonst False
 */
static uint8_t goal_reached(void) {
	if (green_field(True) && sensDistL < 300 && sensDistR < 300)
		return True;

	return False;
}


#if FOLLOW_LINE_VERSION != OPTIMAL_LINE_BEHAVIOUR_VERSION

/*!
 * Prueft ob der Bot sich auf einer Kreuzung befindet; weil Abgrundsensoren nicht gleichzeitig ueber Kreuzungslinie erscheinen, werden hier Kennungen fuer
 * die beiden Abgrundsensoren verwendet falls Linie erkannt wurde;Wird innerhalb der naechsten 3cm die andere Seite auch erkannt, so steht Bot auf Kreuzung
 * @return True falls bot sich auf Kreuzung befindet sonst False
 */
uint8_t check_crossing(void) {
	if (goal_reached()) {
		LOG_DEBUG("Ziel erreicht und Ende");
		lineState = GOAL_FOUND; // Verhalten Ende
		return True;
	}

#ifdef CHECK_REVERSE_BEHAVIOUR
	if (bot_reverse ) { // entgegengesetzte Richtung wurde eingenommen
		way_back = True;
		bot_reverse=False;               // Kennung Richtungswechsel wegsetzen
		LOG_DEBUG("Richtungswechsel-Umkehr");
		lineState = TURN_ON_GREEN;       // weiter mit Eintritt nach Gruenerkennung
		//deactivateBehaviour(bot_check_reverse_direction_behaviour);  //Verhaltensueberwachung Richtungsumkehr beenden
		return True;
	}
#else
	if (green_field(False)) { // Umkehrfeld Gruenfeld erkannt
		way_back = True;
		LOG_DEBUG("auf Gruen Umkehr");
		lineState = TURN_ON_GREEN;       // weiter mit Eintritt nach Gruenerkennung
		return True;
	}
#endif



	if (sensBorderL > BORDER_DANGEROUS || sensBorderR > BORDER_DANGEROUS) {
		// Kennungen setzen auf Querlinie erkannt links oder rechts voraus, also wenn Abgrundsensor Linie (vorausgesetzt Abgrund gibt es nicht) erkennt
		if (sensBorderR > BORDER_DANGEROUS) {
			border_side_r_fired = True;
		} else {
			border_side_l_fired = True;
		}

		// Beide erkennen Querlinie
		if (sensBorderL > BORDER_DANGEROUS && sensBorderR > BORDER_DANGEROUS) {
			border_side_l_fired = True;
			border_side_r_fired = True;
		}

		if (border_side_l_fired && border_side_r_fired) {
			LOG_DEBUG("beide zugeschlagen l/r %1d %1d!!!", sensBorderL, sensBorderR);

			return True;
		}
		if (border_side_l_fired || border_side_r_fired) {
			LOG_DEBUG("l oder r: %1d %1d", border_side_l_fired, border_side_r_fired);
		}

	}

	if (distance_reached(&lastpos_x, &lastpos_y)) {
		border_side_l_fired = False;
		border_side_r_fired = False;

#ifdef CHECK_REVERSE_BEHAVIOUR
		// Ueberwachungsverhalten zum Check auf entgegengesetzte Richtung
		bot_check_reverse_direction(0);
#endif
	}


	return False;
}
#else	// optimale Version

/*!
 * Prueft ob der Bot sich auf Umkehr- oder Zielfeld befindet; hier wurde der optimal Linienfolger gestartet, der sich beendet bei Kreuzungen oder Abgruenden
 * @return True falls bot sich auf Ziel- oder Umkehrfeld befindet sonst False
 */
static uint8_t check_crossing(void) {
	if (goal_reached()) {
		LOG_DEBUG("Ziel erreicht und Ende");
		lineState = 99; // Verhalten Ende
		return True;
	}

#ifdef CHECK_REVERSE_BEHAVIOUR
	if (bot_reverse ) { // entgegengesetzte Richtung wurde eingenommen
		way_back = True;
		bot_reverse=False;               // Kennung Richtungswechsel wegsetzen
		LOG_DEBUG("Richtungswechsel-Umkehr");
		lineState = TURN_ON_GREEN;       // weiter mit Eintritt nach Gruenerkennung
		deactivateBehaviour(bot_check_reverse_direction_behaviour);  //Verhaltensueberwachung Richtungsumkehr beenden
		return True;
	}

    if (distance_reached(&lastpos_x, &lastpos_y))
  	    // Ueberwachungsverhalten zum Check auf entgegengesetzte Richtung starten oder mit neuer Ausrichtung weiterpruefen
	    bot_check_reverse_direction(0);

#else
  if (green_field(False)) { // Gruenfeld erkannt
		way_back = True;
		LOG_DEBUG("auf Gruen Umkehr");
		lineState = TURN_ON_GREEN;       // weiter mit Eintritt nach Gruenerkennung
		return True;
	}

#endif // Drehrichtungsumkehr mit Verhalten ueberwachen

	return False;
}
#endif	// OPTIMAL_VERSION

/* Check-Routine zum Erkennen ob sich bot schon auf der Linie befindet
 * @return True wenn mindestens ein Liniensensor die Linie erkennt
 */
static uint8_t check_line_sensors(void) {
	if (sensLineL >= LINE_SENSE || sensLineR >= LINE_SENSE)
		return True;

	return False;
}

/*!
 * Das eigentliche Verhalten, welches den bot einer Linie folgen laesst, X-Kreuzungen erkennt und
 * dann in bestimmter Reihenfolge die Abzweigungen entlangfaehrt bis zu seinem Ziel (gruenes Feld an Hindernis); die
 * Kreuzungen werden enweder neu im Stack angelegt oder von dort geholt und die Wegeinfos dort wieder vermerkt; eine Kreuzung
 * wird vergessen, wenn kein Weg von dort zum Ziel gefuehrt hatte; Verhalten laesst den bot ebenefalls den bereits gemerkten Weg
 * zum Ziel oder von dort rueckwaerts direkt auf kuerzestem Weg zum Ausgangspunkt fahren
 * @param *data	Verhaltensdatensatz
 */
void bot_line_shortest_way_behaviour(Behaviour_t * data) {
	switch (lineState) {
	case GO_TO_LINE: // bot faehrt gewisse Strecke vorwaerts bis zur Linie
		LOG_DEBUG("Start Linienfolger l/r %1d %1d, Version %1d", sensLineL, sensLineR, FOLLOW_LINE_VERSION);

		// naechster Verhaltenszustand
		lineState = CHECK_LINE;

		// falls vom Ziel rueckwaerts gefahren werden soll und bot ist noch auf Zielendposition, dann erst einmal drehen
		if (goal_reached() && way_back) {
			(bot_turn(data, 180));
			break; // im selben Verhaltensstatus weiter mit Liniensuche Strecke voraus fahren
		} else {
			bot_goto_dist(data, 600, 1); // maximal 60cm vorwaerts bis Linie voraus
			bot_cancel_behaviour(data, bot_goto_pos_behaviour,
					check_line_sensors); // Ende der Vorwaertsfahrt wenn Linie erkannt wurde
		}
		break;

	case CHECK_LINE: // laesst hier den bot eine Linie folgen

		// Linienfolger-Verhalten starten
		bot_follow_line(data);
		LOG_DEBUG("Linienfolger CheckLine l/r %1d %1d, Version %1d", sensLineL, sensLineR, FOLLOW_LINE_VERSION);

		// cancel nur fuer die Linienfolger, die nicht Kreuzungen oder Abgrund erkennen
		// der optimale Linienfolger 3 macht dies von sich aus
#if FOLLOW_LINE_VERSION != OPTIMAL_LINE_BEHAVIOUR_VERSION
		// ebenfalls Kennungen initialisieren fuer Endeerkennung der Kreuzungen
		border_side_l_fired = False;
		border_side_r_fired = False;
#endif
        // fuer Cancel-Check_Verhalten letzten Positionen, also Botpos, belegen
		lastpos_x = x_pos;
		lastpos_y = y_pos;

		bot_cancel_behaviour(data, bot_follow_line_behaviour, check_crossing);

		lineState = CHECK_BORDER; // naechster Zustand
		break;

	case CHECK_BORDER: // kleines Stueck vorfahren fuer Abgrundcheck
		lineState = GO_FORWARD; // naechster Verhaltenszustand
		bot_goto_dist(data, 20, 1); // 2cm vorfahren, Liniensensoren sind dann runter von Linie, falls nicht Abgrund
		deactivateBehaviour(bot_cancel_behaviour_behaviour); // Cancelverhalten fuer Linienfolger beenden
		crossing_reached = True; // Kennung setzen, ab jetzt auch Gruenfelder fuer Umkehr ausgewertet
		break;

	case GO_FORWARD: // Erkennung ob Abgrund und Ende, falls nicht weiter bis Liniensensoren auf Krezung
/*! @todo schlaegt auch bei Kreuzungen zu, muss ueberarbeitet werden! */
//		if (sensBorderL > BORDER_DANGEROUS || sensBorderR > BORDER_DANGEROUS) {
//			LOG_DEBUG("Abgrund und Ende %1d %1d", sensBorderL, sensBorderR);
//			lineState = GOAL_FOUND; // Verhalten Ende
//			break;
//		}
		lineState = TURN_SIDEWISH_ON_CROSSING; // naechster Verhaltenszustand
		bot_goto_dist(data, 30, 1); // vorfahren bis Liniensensoren ideal auf Kreuzung stehen zur Drehung
		break;

	case TURN_SIDEWISH_ON_CROSSING: // Festlegen und Ausfuehren der Drehung je nach Richtung und Fahrlogik (Stackfahren vor oder zurueck)
		// beim Rueckweg way_back==True vom Stack hier die Kreuzung holen; bei Hinweg neu auf den
		// Stack legen

		LOG_DEBUG("vor turn 90 l/r %1d %1d,way_back  %1d", sensLineL, sensLineR, way_back);

		lineState = GO_FORWARD_AFTER_TURN; // naechster Verhaltenszustand

		if (!go_stack_way) { // Logik Zielsuchen, kein Stackfahren

			if (way_back) { // bot auf Rueckweg nach Wegende mit Gruenfeld

				if (!pop_stack_crossing(&crosstype, &direction_counter)) {
					LOG_DEBUG("kein Pop moeglich");
					lineState = GOAL_FOUND; // Verhalten Ende
					break;
				}
				direction_counter++;

				LOG_DEBUG("---X vom Stack nehmen wg. back %1d counter %1d", way_back, direction_counter);
				if (direction_counter <= 3) {
					push_stack_crossing(1, direction_counter);
					way_back = False; // wird wieder zu vorwaerts
					LOG_DEBUG("wieder push, back %1d counter %1d", way_back, direction_counter);
				} else
				LOG_DEBUG("X wird vergessen-irrelevant");

			} else { // Bot kommt auf Vorwaertsfahrt an Kreuzung an

				direction_counter = 1; // ersten Weg nehmen
				push_stack_crossing(1, direction_counter); // Kreuzung und Weg auf Stack legen
				LOG_DEBUG(">> X ist neu vorw. und Push, counter %1d", direction_counter);
				way_back = False; // jedenfalls auf vorwaerts
			}
		} else { // hier soll der im Stack liegende kuerzeste Weg abgefahren werden
			LOG_DEBUG("Stack abfahren und Richtung aus Stack");

			// Unterscheidung fuer Vorwaerts und Rueckwaerts,
			// bei vorwaerts wurde Bot wieder an Ausgangspunkt gesetzt und faehrt zum Ziel
			// bei rueckwaerts faehrt bot vom Ziel zurueck zum Anfang der Linie bis Stack leer ist an Ankunft 1.Kreuzung	ab Start
			// gespeicherter Richtungswert muss je nach vorwaerts oder rueckwaerts anders interpretiert werden
			if (way_back) {
				// Weg rueckwaerts heisst Weg vom Ziel zurueckzufahren, also via Pop von Kreuzung zu Kreuzung hangeln
				if (!pop_stack_crossing(&crosstype, &direction_counter)) {
					LOG_DEBUG("kein Pop moeglich");
					lineState = GOAL_FOUND;
					break;
				}

				// je nach Start-Ausgangs-Richtungswahl Richtungswert interpretieren
#if START_SIDEWISH == 1 // links bevorzugt
				if (direction_counter == 1) {
					LOG_DEBUG("nach rechts");
					sidewish = -1;
				}
				if (direction_counter == 3) {
					LOG_DEBUG("nach links");
					sidewish = 1;
				}
#else // rechts bevorzugt
				LOG_DEBUG("neg wg. rechts zuerst");
				if (direction_counter == 1) {
					LOG_DEBUG("nach links");
					sidewish = 1;
				}
				if (direction_counter == 3) {
					LOG_DEBUG("nach rechts");
					sidewish = -1;
				}}
#endif // START_SIDEWISH == 1

			} // Ende zurueck vom Ziel
			else {
				// bot soll hier den kuerzesten Weg vom Start- zum Zielpunkt abfahren, d.h. bot befindet sich wieder
				// am Startpunkt (manuell hingesetzt/ gefahren) und muss sich den jeweils 1. Stackwert holen und abfahren
				if (!dequeue_stack_crossing(&crosstype, &direction_counter)) {
					LOG_DEBUG("kein dequeue moeglich");
					lineState = GOAL_FOUND;
					break;
				}

				// auch hier je nach Wunsch-Start-Richtungswahl die Richtung interpretieren
#if START_SIDEWISH == 1 // links bevorzugt
				if (direction_counter == 1) { // links bevorzugt
					LOG_DEBUG("nach links");
					sidewish = 1;
				}
				if (direction_counter == 3) {
					LOG_DEBUG("nach rechts");
					sidewish = -1;
				}
#else // rechts bevorzugt
				LOG_DEBUG("neg wegen rechts zuerst");
				if (direction_counter == 1) {
					LOG_DEBUG("nach rechts");
					sidewish = -1;
				}
				if (direction_counter == 3) {
					LOG_DEBUG("nach links");
					sidewish = 1;
				}
#endif // START_SIDEWISH
			}

			LOG_DEBUG("X Richtg. %1d", direction_counter);

			if (direction_counter == 2) {
				LOG_DEBUG("geradeaus");
				break; // ohne Drehung weiter
			}
		}
		// Drehung 90 Grad ausfuehren
		bot_turn(data, sidewish * 90);
		break;

	case GO_FORWARD_AFTER_TURN: // hierher nach 90 Grad Drehung in gewuenschte Richtung
		BLOCK_BEHAVIOUR(data, 500); // etwas warten
		LOG_DEBUG("etwas vor nach Dreh l/r %1d %1d", sensLineL, sensLineR);

		// geht dann wieder mit Linienfolger weiter
		lineState = GO_TO_LINE; // dort vorfahren bis Cancelverhalten Linie erkennt
		bot_goto_dist(data, 20, 1); // vorfahren, Linesensoren sind dann auf Kreuzung
		border_side_l_fired = False; // Kennungen ruecksetzen
		border_side_r_fired = False;
		//bot_cancel_behaviour(data, bot_goto_pos_behaviour, check_line_sensors);  // Ende der Vorwaertsfahrt wenn Linie erkannt wurde
		break;

	case TURN_ON_GREEN: // hierher nach Erkennung des Gruenfeldes mit Richtungsumkehr
		deactivateBehaviour(bot_cancel_behaviour_behaviour); // Cancelverhalten fuer Linienfolger beenden
		BLOCK_BEHAVIOUR(data, 500); // evtl. etwas warten
		lineState = CHECK_LINE;
#ifndef CHECK_REVERSE_BEHAVIOUR		// hat ja bereits die entgegengesetzte Richtung eingenommen und damit wieder in Ausgangs-Zielrichtung
		bot_turn(data, 180);
		LOG_DEBUG("Umkehr erkannt und bot_turn");
#endif
		break;

	case GOAL_FOUND:
		lineState = END;
#ifdef BEHAVIOUR_SERVO_AVAILABLE
		bot_servo(data, SERVO1, DOOR_OPEN); // Ziel gefunden und Klappe auf
#endif
		break;

	case END: // Verhaltensende
		LOG_DEBUG("Ende Behav. l/r %1d %1d", sensLineL, sensLineR);
#ifdef BEHAVIOUR_SERVO_AVAILABLE
		bot_servo(data, SERVO1, DOOR_CLOSE); // Ziel gefunden und Klappe wieder zu
#endif
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Startet das Verhalten
 * @param	*caller Verhaltensdatensatz des Aufrufers
 */
void bot_line_shortest_way(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_line_shortest_way_behaviour, NOOVERRIDE);
	lineState = 0;
	border_side_l_fired = 0;
	border_side_r_fired = 0;
	direction_counter = 0;
	crossing_reached = 0;
	greencounter = 0;
	sidewish = START_SIDEWISH;
	way_back = False;
	go_stack_way = False;
	pos_store = pos_store_create(get_behaviour(bot_line_shortest_way_behaviour),
			pos_store_data);

	/* stoerende Notfallverhalten aus */
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
	deactivateBehaviour(bot_avoid_col_behaviour);
#endif
#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
	deactivateBehaviour(bot_avoid_border_behaviour);
#endif
}

/*!
 * Falls Linienfolger Linie nicht findet kann hier weitergefuehrt werden nach manuellem richtigen wiederausrichten
 * @param	*caller Verhaltensdatensatz des Aufrufers
 */
void bot_line_shortest_way_continue(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_line_shortest_way_behaviour, NOOVERRIDE);
	lineState = 0;
	border_side_l_fired = 0;
	border_side_r_fired = 0;
}

/*!
 * Abfahren des gefundenen Weges vorwaerts
 * @param	*caller Verhaltensdatensatz des Aufrufers
 */
void bot_line_shortest_way_forward(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_line_shortest_way_behaviour, NOOVERRIDE);
	lineState = 0;
	border_side_l_fired = 0;
	border_side_r_fired = 0;
	crossing_reached = 0;
	way_back = False;
	go_stack_way = True;
}

/*!
 * Abfahren des gefundenen Weges rueckwaerts
 * @param	*caller Verhaltensdatensatz des Aufrufers
 */
void bot_line_shortest_way_backward(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_line_shortest_way_behaviour, NOOVERRIDE);
	lineState = 0;
	border_side_l_fired = 0;
	border_side_r_fired = 0;
	go_stack_way = True;
	way_back = True;
	crossing_reached = True; // damit bei Start auf gruenem Zielfeld dies auch erkannt wird, anders als vorwaerts
}


/*!
 * Keyhandler zur Verwendung via Fernbedienung auf dem Display zum Stackanfahren
 */
#ifdef DISPLAY_LINE_SHORTEST_WAY_AVAILABLE
static void driveline_disp_key_handler(void) {
	switch (RC5_Code) {

	case RC5_CODE_5:
		/* Verhalten starten zum Anfahren der Stackpunkte */
		RC5_Code = 0;
		bot_line_shortest_way(NULL);
		break;

	case RC5_CODE_6:
		/* Verhalten zum Speichern relevanter Wegepopsitionen zum Spaeteren Zurueckfahren */
		RC5_Code = 0;
		bot_line_shortest_way_continue(NULL);
		break;

	case RC5_CODE_8:
		/* Zueckfahren vom Ziel zum Ausgangspunkt auf kuerzestem gespeicherten Weg */
		RC5_Code = 0;
		bot_line_shortest_way_forward(NULL);
		break;

	case RC5_CODE_9:
		/* Zueckfahren vom Ziel zum Ausgangspunkt auf kuerzestem gespeicherten Weg */
		RC5_Code = 0;
		bot_line_shortest_way_backward(NULL);
		break;

	}
}

/*!
 * Display zum Verhalten
 */
void bot_line_shortest_way_display(void) {
	display_cursor(1, 1);
	display_printf("DRIVE_LINE_S_WAY");
	display_cursor(2, 1);
	display_printf("GoLine/Continue:5/6");
	display_cursor(4, 1);
	display_printf("GoWayForw/Back:8/9");

	driveline_disp_key_handler(); // aufrufen des Key-Handlers
}
#endif	// DISPLAY_LINE_SHORTEST_WAY_AVAILABLE

#endif	// BEHAVIOUR_LINE_SHORTEST_WAY_AVAILABLE
