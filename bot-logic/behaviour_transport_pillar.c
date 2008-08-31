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
 * @file 	behaviour_transport_pillar.c
 * @brief 	Bot startet von einem Farb-Startpad und entdeckt die Welt, bis er auf ein anderes
 *          Farbpad stoesst. Er faehrt nun zwischen den beiden Farbpads hin und her, sammelt bei
 *          Ankunft auf einem Farbpad ein in der Naehe befindliches Hindernis in sein Transportfach ein
 *          und bringt dieses zum anderen Farbpad. Auf seinem Weg zur Abladestelle oder waehrend des
 *          Entdeckens der Welt zur Farbpadsuche weicht er Hindernissen geschickt aus.
 *          Es kann mittels des Wand-Explore Verhaltens nur mittels der Farbpads gefahren werden ohne sich
 *          Koordinaten zu merken, womit er nicht unbedingt zielgerichtet von einem zum anderen Farbpad
 *          fahren kann. Mittels Zuschalten der Verwendung der MAP-Koordinaten werden die Koords der Pads
 *          gemerkt und sich dorthin ausgerichtet. Es kann nun mit einem Fahrverhalten zwischen beiden hin- und
 *          hergefahren werden, wobei entweder der Wandfolger dient oder auch andere Fahrverhalten
 *          Anwendung finden (Auswahl jeweils per Define).
 *          Der Verhaltensstart erfolgt entweder via Remotecall oder Taste 9. Befindet sich der Bot auf einem Farbpad, so kann
 *          via Taste 7 dieses als Zielpad definiert werden (Simfarben werden automatisch erkannt; Real ist dies schwierig, daher
 *          manuell definierbar)
 *          Zur Steuerung mit Tasten und der Positionsanzeigen wurde ein eigener Screen definiert
 *
 * @author 	Frank Menzel (menzelfr@gmx.net)
 * @date 	23.10.2007
 */

#include "bot-logic/available_behaviours.h"
#include "ui/available_screens.h"
#include "bot-logic/bot-logik.h"
#include <math.h>
#include <stdlib.h>
#include "math_utils.h"
#include "map.h"
#include "display.h"
#include "rc5-codes.h"

#ifdef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE

// auskommentieren wenn mit Farbpads und ohne Koordinaten gefahren werden soll trotz vorhandener Map
// Weltkoordinaten trotzdem mitgefuehrt zum Drehen genau zur anderen Position
#define USE_KOORDS

// zum Startbeginn explorert der Bot die Welt und erkennt damit ein bekanntes Farbpad automatisch
// soll dies nicht automatisch erfolgen, dann define auskommentieren; mittels Taste 7 kann dann manuell
// das Ziel-Farbpad definiert werden wenn sich der bot auf ein entsprechendes befindet
// wegen den realen Schwierigkeiten mit Farben nur im Sim standardmaessig
#ifdef PC
#define AUTOSEARCH_PADCOL
#endif

// bei Ankunft auf Zielposition wird Klappe auf und zu gemacht; geht natuerlich nur wenn Servoverhalten an sind
#ifdef BEHAVIOUR_SERVO_AVAILABLE
#define SHOW_CLAPS_ON_DEST
#endif

/* ******************************************************************
 * ab hier die moeglichen Fahrverhalten, ausgewaehlt je nach Define
 * ******************************************************************
 */

// Solve_Maze als Explorerverhalten nehmen, natuerlich nur wenn verfuegbar
#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
//#define GO_WITH_SOLVE_MAZE
#endif

// Fahren nach Karte wenn verfuegbar
#ifdef BEHAVIOUR_MAP_GO_DESTINATION_AVAILABLE
//#define GO_WITH_MAP_GO_DESTINATION    // auskommentieren wenn nicht nach Karte gefahren werden soll
#undef  GO_WITH_SOLVE_MAZE          // nach Karte den Wandfolger entdefinieren
#endif

// Fahren mittels goto_pos Verhalten
#ifdef USE_KOORDS   // geht nur mit Koordinaten selbst, nicht mit Fahren  nach Farbe
#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE
#define   GO_WITH_GOTO_POS    // auskommentieren wenn nicht mit Goto_pos Verhalten gefahren werden soll
#undef  GO_WITH_SOLVE_MAZE          // nach Karte den Wandfolger entdefinieren
#undef  GO_WITH_MAP_GO_DESTINATION
#endif
#endif

//trotz aktiviertem Pillarverhalten nur Hin- und Herfahren ohne Dosen einzufangen
//#define NO_PILLAR_BEHAVIOUR

// zusaetzlich zu den Liniensensoren muessen die Farben laut Abgrundsensoren passen; damit sollten
// beim Realbot Fehlausloesungen vermieden beim Fahren nach Farbe
//#define CHECK_BORDERSENS_FOR_COL

static uint8 state=0;

// Weltkoordinaten zum Speichern der Start- und Zielposition, zwischen denen Hin- und Hergefahren wird
static float startpad_x=0;
static float startpad_y=0;
static float destpad_x=0;
static float destpad_y=0;

static float target_x=0; /*!< Zwischenzielkoordinaten X des xy-Fahrverhaltens */
static float target_y=0; /*!< Zwischenzielkoordinaten Y des xy-Fahrverhaltens */

#ifdef USE_KOORDS
/*!
 * Check, ob die Koordinate xy innerhalb eines Radius-Umkreises befindet; verwendet um das
 * Zielfahren mit gewisser Toleranz zu versehen
 * @param x x-Ordinate
 * @param y y-Ordinate
 * @param destx Ziel-x-Ordinate
 * @param desty Ziel-y-Ordinate
 * @return True wenn xy im Umkreis liegt6
 */
uint8 koord_in_circle_world (float x, float y, float destx, float desty) {
	//Punktdifferenzen
	float distx=destx-x;
	float disty=desty-y;
	uint16 radquad=0; // Vergleichs-Radiusabstand im Quadrat

	// bin ich auf Linie, kann Genauigkeit hoeher sein
	radquad=8100;

	// Fahre ich gerade mit Linienfolger, dann Umkreis kleiner
#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
	if (behaviour_is_activated(bot_follow_line_behaviour))
	radquad=900; // 3cm Umkreis
#endif

	// Ist Abstand im Radiusabstand, dann ist der Punkt innerhalb des Umkreises
	// gerechnet mit Quadratzahlen, sparen der Wurzel
	return (distx*distx + disty*disty) < radquad; // 9cm Umkreis des Bots

}
#else
//hier wird immer die Farbe des Zielpads eingetragen, welche es zu erreichen gilt
static uint16 destpad_value=0;

// Farbe der Abgrundsensoren; im Sim identisch der Liniensensoren
#ifdef CHECK_BORDERSENS_FOR_COL
static uint16 destpad_bvalue=0;
#endif

#endif

//Startpad1 wird mit Farbe bei Start belegt und Startpad2 fuer das andere Pad
static uint16 STARTPAD_COL1=0;
static uint16 STARTPAD_COL2=0; // weiss geht somit nicht

static uint8 key_pressed=0; // Kennung ob Taste gedrueckt fuer man. Zielvorgabe

//real haben die Bordersensoren andere Werte als die Liniensensoren, daher hier mitfuehren
#ifdef CHECK_BORDERSENS_FOR_COL
static uint16 STARTPAD_BCOL1=0;
static uint16 STARTPAD_BCOL2=0; // weiss geht somit nicht
#endif

// Farbe des selbst definierbaren Zielpads; wird zusaetzlich zu den bekannten Standardfarben gecheckt wenn <> 0
static uint16 PAD_MYCOL=0;

//kurze Wartezeit wenn bot auf Zielposition
#define DELAY_ROLLTIME  800

/*!
 * Liefert True wenn sich Bot auf Pad mit dem Wert value_pad befindet
 * @param value_pad   zu checkender Farbwert der Liniensensoren
 * @param value_bpad  zu checkender farbwert der Bordersensoren
 */
uint8 check_pad(uint16 value_pad,uint16 value_bpad) {
#define COL_TOL 5

	// beide Liniensensoren muessen auf der Zielfarbe sein oder nur einer, wenn der andere dafuer Linie erkennt
	uint8 ret = (sensLineL>=value_pad-COL_TOL && sensLineL<=value_pad+COL_TOL &&
			sensLineR>=value_pad-COL_TOL && sensLineR<=value_pad+COL_TOL);

	if (!ret)
	ret= (sensLineL>=value_pad-COL_TOL && sensLineL<=value_pad+COL_TOL && sensLineR >= LINE_SENSE) ||
	(sensLineR>=value_pad-COL_TOL && sensLineR<=value_pad+COL_TOL && sensLineL >= LINE_SENSE);

	if (ret) {
#ifdef CHECK_BORDERSENS_FOR_COL
		// Bordersensoren nur auswerten wenn das Zielpad schon gefunden wurde
		if (STARTPAD_COL2>0) {
			ret = ((sensBorderL>=value_bpad-COL_TOL && sensBorderL<=value_bpad+COL_TOL) ||
					(sensBorderR>=value_bpad-COL_TOL && sensBorderR<=value_bpad+COL_TOL));
		}
#endif

	}
	return ret;
}

/*!
 * Abbruchbedingung des Explorers wenn bereits Start- und Ziel(pad)koordinaten festgelegt sind
 * und er sich immer von einer zur anderen bewegt; wurde die Zielkoord per Hand festgelegt, so
 * gibt es keine Padfelder rot/ gruen und es muss beendet werden wenn sich der bot diesen Koords
 * annaehert
 * true wenn sich bot auf einem Startfeld befindet
 */
uint8 bot_on_pad(void) {
	// es gilt: es sind immer die anderen Koords zu checken, wo ich mich nicht im Umkreis befinde oder fuer
	// Farbpads: immer den anderen checken wo ich gerade nicht drauf bin, solange laeuft der Explorer
	if (STARTPAD_COL2==0) {
		// noch kein Ziel definiert bzw. gefunden; mit bekannten Farb-Pad-Werten auf Linien-Sensorebene
		// vergleichen; Bordersensoren egal
		return (check_pad(STARTPAD1,0)||check_pad(STARTPAD2,0)||check_pad(GROUND_GOAL,0)||
				(PAD_MYCOL>0 && check_pad(PAD_MYCOL,0)))? True : False;

	}
	else
	{ // Zielpad/- koordinaten hier bereits festgelegt
		// Wenn Umkreispunkt erreicht in Weltkoordinaten Schluss bei Koordinatenverwendung
#ifdef USE_KOORDS
		return koord_in_circle_world(x_pos,y_pos,target_x,target_y)?True:False;
#else
		// finden keine Koordinaten Verwendung, dann nur nach Farbpads fahren
#ifdef CHECK_BORDERSENS_FOR_COL
		return check_pad(destpad_value,destpad_bvalue)?True:False;
#else
		return check_pad(destpad_value,0)?True:False;
#endif
#endif
	}
}

/*!
 * Start-Abbruchbedingung des Explorer Verhaltens zur Suche nach dem Ziel-Farbpad; dieses laeuft solange,
 * bis der Explorer ueber ein anderes bekanntes Farbfeld faehrt oder per Taste das Farbpad definiert wurde
 */
uint8 destpad_found(void) {

#ifdef AUTOSEARCH_PADCOL  // automatische Suche eingeschaltet
	// Check ob Bot auf einem bekannten Farbpad rot/blau/gruen oder Linienfarbe steht,
	// welche nicht der Farbe zum Startzeitpunkt entspricht
	if (bot_on_pad() && !check_pad(STARTPAD_COL1,0)) {
		STARTPAD_COL2 = sensLineL; // einfach mal linken genommen
#ifdef CHECK_BORDERSENS_FOR_COL
		STARTPAD_BCOL2 = (sensBorderL + sensBorderR) / 2; // Mittelwert beider Abgrundsensoren
#endif
#ifndef USE_KOORDS
		destpad_value=STARTPAD_COL2;
#ifdef CHECK_BORDERSENS_FOR_COL
		destpad_bvalue = STARTPAD_BCOL2;
#endif
#endif
		// Weltkoordinaten speichern fuer Drehung genau zu diesen Koordinaten
		destpad_x=x_pos;destpad_y=y_pos;

		return True;
	}
#endif
	return False; // nicht auf einem Startpadfield
}

/*!
 * Endebedingung des Explorerverhaltens
 * @return True wenn Endebedingung erfuellt
 */
uint8 check_end_exploring(void) {
	if (key_pressed) {
		key_pressed=0;
		return True;
	}
	return (STARTPAD_COL2==0) ? destpad_found():bot_on_pad();
}

/*!
 * Das Transport_Pillar-Verhalten
 * @param *data	Verhaltensdatensatz
 */
void bot_transport_pillar_behaviour(Behaviour_t *data) {
#define BOT_CHECK_STARTPAD 0
#define BOT_EXPLORE        1
#define INITIAL_TURN       2
#define GOTO_NEXTPAD       3
	//#define EXPL_LINE          4
#define BOT_ON_PAD         5
#define GET_PILLAR         6
#define UNLOAD_PILLAR      7
#define BOT_ROLL           8
#define CHECK_LINE_FOLLOW  9
#define UNLOAD             10

	switch (state) {

	case BOT_CHECK_STARTPAD:
		// Startkoords und Startpad setzen
		state=BOT_EXPLORE;

		// Start-Farbpad belegen mit Farbwert
		STARTPAD_COL1 = sensLineL; // linken nehmen
#ifdef CHECK_BORDERSENS_FOR_COL
		STARTPAD_BCOL1 = (sensBorderL + sensBorderR) / 2; // Mittelwert beider Abgrundsensoren
#endif
		// Start-Weltkoordinaten speichern
		startpad_x=x_pos;
		startpad_y=y_pos;
		break;

	case BOT_EXPLORE:
		// Solange exploren bis anderes bekanntes Farbpad gruen oder rot gefunden oder Zielkoords/Pad
		// manuell gesetzt wurde;
		// wird bot zu Beginn bei Koordinatenverwendung auf Linie gesetzt, dann geht follow_line los

		state = BOT_ROLL; // Pad gefunden oder waehrend der Fahrt Taste 7 -> ausrollen lassen
#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
#ifdef USE_KOORDS
		if (sensLineL >= LINE_SENSE || sensLineR >= LINE_SENSE) {
#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
			deactivateBehaviour(bot_avoid_border_behaviour);
#endif

			// Linienfolger uebernimmt
			bot_follow_line(data);

			// Linienfolger deaktivieren, wenn bot im Zielbereich angekommen ist, welcher ja dann manuell
			// fuer die Zielkoordinaten definiert werden muss
			// das cancel-Verhalten laeuft als Paralelverhalten; sobald Linienfolger beendet ist,
			// wird dieses auch beendet
			bot_cancel_behaviour(data, bot_follow_line_behaviour, check_end_exploring);

			break;
		}
#endif  // nach koordinaten fahren
#endif  // Linienfolger ueberhaupt an
#ifdef GO_WITH_SOLVE_MAZE
		bot_solve_maze(data);
		// Abbruchverhalten starten und aktiv belassen trotz Inaktivitaet des Explorerverhaltens
		bot_cancel_behaviour(data, bot_solve_maze_behaviour, check_end_exploring);

#else
#ifdef GO_WITH_MAP_GO_DESTINATION
		// nach Karte kann ich nur fahren, wenn auch Zielpad schon bekannt ist sonst follow_wall
		if (STARTPAD_COL2==0) {
			bot_follow_wall(data,check_end_exploring);
		}
		else
		{
			bot_gotodest_map(data);
		}
#else
#ifdef BEHAVIOUR_GOTO_POS_AVAILABLE

		// nach Karte kann ich nur fahren, wenn auch Zielpad schon bekannt ist sonst follow_wall
		if (STARTPAD_COL2==0) {
			bot_follow_wall(data,check_end_exploring);
		}
		else
		{
			bot_goto_pos(data,target_x,target_y,999);
		}
#else
		// hier koennte man noch andere Explorerverhalten einbinden und verwenden
		bot_follow_wall(data, check_end_exploring);
#endif
#endif

#endif

		break;

	case BOT_ROLL:
		deactivateBehaviour(bot_cancel_behaviour_behaviour); // deaktivieren falls noch aktiv
		// Bot erst mal ausrollen lassen
#ifndef SHOW_CLAPS_ON_DEST
		// nicht warten wenn kurz Klappe auf und zu geht; ist bereits genug Verzoegerung
		bot_delay(data, DELAY_ROLLTIME); // kurzzeitig nix tun
#endif
		state = BOT_ON_PAD;
		break;

	case BOT_ON_PAD:
		// hier wird entschieden, ob ich ein Objekt suchen muss zum mitnehmen oder wenn ich was im
		// Bauch habe, muss dieses entladen werden
		state= (sensTrans==0) ? GET_PILLAR : UNLOAD_PILLAR;

#ifdef SHOW_CLAPS_ON_DEST
		if (state==GET_PILLAR) {
			bot_servo(data, SERVO1, DOOR_OPEN); // Klappe auf
		}
#endif

		break;

	case GET_PILLAR:
		// bin jetzt auf dem Zielpad/ Koords angekommen und starte das Pillarverhalten falls es was zu holen gibt
#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
#ifndef NO_PILLAR_BEHAVIOUR  // darf nicht explizit augeschaltet sein
		bot_catch_pillar(data);
#endif
#endif

		state=GOTO_NEXTPAD;
		break;

	case UNLOAD_PILLAR:
		// bin jetzt auf dem Zielpad/ Koords angekommen und starte das Pillar-Unload-Verhalten,
		// da ich ja was im Bauch habe
		state=GOTO_NEXTPAD;
		// evtl. noch etwas vorwaerts fahren, damit Objekt etwas weiter vorn abgestellt wird
		// sonst dreht sich bot spaeter schon vorher weg
#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
#ifndef NO_PILLAR_BEHAVIOUR  // darf nicht explizit ausgeschaltet sein
		bot_drive_distance(data,-50,BOT_SPEED_FOLLOW,OPTIMAL_DISTANCE/10);
		state=UNLOAD;
#endif
#endif

		break;

#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
#ifndef NO_PILLAR_BEHAVIOUR
		case UNLOAD:
		state=GOTO_NEXTPAD;
		bot_unload_pillar(data);
		break;
#endif
#endif

	case GOTO_NEXTPAD:
		// hier werden wechselseitig das andere Zielpad/ Koords gesetzt
		if (target_x==startpad_x && target_y==startpad_y) {
			target_x=destpad_x;
			target_y=destpad_y;
		} else {
			target_x=startpad_x;
			target_y=startpad_y;
		}

#ifdef GO_WITH_MAP_GO_DESTINATION
		bot_set_destination(map_world_to_map(target_x),map_world_to_map(target_y));
#endif

		// bei fahren nach Farbpads die Zielpadfarbe wechseln; nur ausgewertet, wenn auch nicht
		// nach Koords gefahren wird
#ifndef USE_KOORDS
		// zuerst fuer Abgrundsensoren
#ifdef CHECK_BORDERSENS_FOR_COL
		destpad_bvalue= (destpad_value==STARTPAD_COL1)? STARTPAD_BCOL2: STARTPAD_BCOL1;
#endif
		destpad_value= (destpad_value==STARTPAD_COL1)? STARTPAD_COL2: STARTPAD_COL1;
#endif
		state=INITIAL_TURN;

#ifdef SHOW_CLAPS_ON_DEST
		bot_servo(data, SERVO1, DOOR_CLOSE); // Klappe zu
#endif

		break;

	case INITIAL_TURN:
		// Drehung genau in Richtung Zielkoordinaten, auch wenn ich gerade von einer Wand komme und damit nicht
		// aus der Zielrichtung; Koordinaten auch benutzt beim Fahren nach Farben
		bot_turn(data, calc_angle_diff(target_x-x_pos, target_y-y_pos));
		state = BOT_EXPLORE;

		break;

	default:
		return_from_behaviour(data);
		break;
	}
}

/*!
 * Ruft das Pillarverhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_transport_pillar(Behaviour_t * caller) {

	state=0;

	startpad_x=0;
	startpad_y=0;
	destpad_x=0;
	destpad_y=0;
	target_x=-1;
	target_y=-1;// nicht 0, weil bei 0 auch Startpos losgeht

	STARTPAD_COL1=0;
	STARTPAD_COL2=0;
#ifdef CHECK_BORDERSENS_FOR_COL
	STARTPAD_BCOL1=0;STARTPAD_BCOL2=0;
#endif
	switch_to_behaviour(caller, bot_transport_pillar_behaviour, OVERRIDE);
}

/*!
 * Routine zum Setzen der Zielkoordinaten auf der Zielposition/ Zielpad
 * @param x X-World-Zielkoordinate
 * @param y Y-World-Zielkoordinate
 */
void bot_set_destkoords(float x, float y) {

	// Farbe zum Zeitpunkt des Tastendruckes wird als Zielfarbe definiert
	// Weiss -wohl nur so im Sim so moeglich- bekommt 1, Hauptsache ungleich 0
	// Wess kann nur manuell als Zielfarbe gesetzt werden
	STARTPAD_COL2 = sensLineL>0 ? sensLineL : 1;
	key_pressed=True;

#ifdef CHECK_BORDERSENS_FOR_COL
	STARTPAD_BCOL2 = (sensBorderL + sensBorderR) / 2; // Mittelwert beider Abgrundsensoren
#endif

	// Weltkoordinaten speichern; diese werden nur verwendet
	// um sich auf diese bei Drehung auf Farbpad ausrichten zu koennen
	if (x==0 && y==0) {
		destpad_x=x_pos;
		destpad_y=y_pos;
	} else {// Koordinaten setzen auf Uebergabekoordinaten
		destpad_x=x;
		destpad_y=y;
	}
}

#ifdef DISPLAY_TRANSPORT_PILLAR
/*!
 * Keyhandler fuer Transport_Pillar-Verhalten
 */
static void trpill_disp_key_handler(void) {
	/* Keyhandling fuer Transport_Pillar-Verhalten */
	switch (RC5_Code) {

	case RC5_CODE_7:
		// akt. Position als Ziel festlegen
		RC5_Code = 0;
		bot_set_destkoords(x_pos, y_pos);
		break;
	case RC5_CODE_9:
		// Start des Verhaltens
		RC5_Code = 0;
		bot_transport_pillar(NULL);
		break;
	} // switch
} // Ende Keyhandler

/*!
 * @brief	Display zum Start der Transport_Pillar-Routinen
 */
void transportpillar_display(void) {
	display_cursor(1, 1);
	display_printf("S-Z %1d %1d %1d %1d",(int16)startpad_x,(int16)startpad_y,(int16)destpad_x,(int16)destpad_y);
	display_cursor(2, 1);
	display_printf("akt.Ziel: %1d %1d",(int16)target_x,(int16)target_y);
	display_cursor(3, 1);
	display_printf("akt.Pos: %1d %1d",(int16)x_pos,(int16)y_pos);
	display_cursor(4, 1);
	display_printf("Go: 9/SetPos: 7");

	trpill_disp_key_handler(); // aufrufen des Key-Handlers
}
#endif	// DISPLAY_TRANSPORT_PILLAR
#endif	// BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
