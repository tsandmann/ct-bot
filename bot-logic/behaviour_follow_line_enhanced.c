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
 * @file 	behaviour_follow_line_enhanced.c
 * @brief 	erweiterter Linienverfolger, der auch mit Unterbrechungen und Hindernissen klarkommt
 * @author 	Frank Menzel (Menzelfr@gmx.de)
 * @date 	25.02.2009
 */


#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE
#include "math_utils.h"

// Merkposition nach Umrunden Hindernis; erst wieder gewisse Strecke fahren bis Linienfolger abgebrochen werden kann fuer Hindernis
static position_t pos_on_line;

// Statusvariable des Verhaltens
static uint8_t line_state_enh = 0;

// naechster Verhaltenseinsprung nach cancel
static uint8_t next_linestate = 0;

// Verhaltenszustaende
#define START_ENH           0
#define START_LINE_ENH      1
#define CHECK_BREAK_IN_LINE 2
#define START_SOLVE_MAZE    3
#define TURN_AROUND_HAZARD  4
#define GO_AROUND_HAZARD    5
#define OTHER_SIDE_ON_LINE  6
#define TURN_NEXT_PART      7
#define SEARCH_LINE         8
#define END_LINE_ENH        99

/*!
 * Check-Routine zum Erkennen ob sich bot schon auf der Linie befindet
 * @return True wenn Linie erkannt wurde
 */
static uint8_t check_line_sensors(void) {
	if (sensLineL >= LINE_SENSE || sensLineR >= LINE_SENSE) {
		line_state_enh = next_linestate; // jeweils naechster Eintrittspunkt
		return True;
	}
	return False;
}

/*!
 * Check-Routine zum Erkennen ob sich linker Liniensensor allein schon auf der Linie befindet; dies waere
 * ja optimal fuer den Linienfolger, da dieser ja auf linker Kante faehrt; wird verwendet nach Umrunden des
 * Hindernisses, um sich wieder auf Linie in richtiger Richtung, also vom Hindernis weg, auszurichten
 * @return True wenn linker Liniensensor allein auf Linie steht
 */
static uint8_t check_left_line_sensor(void) {
	if (sensLineL >= LINE_SENSE && sensLineR < LINE_SENSE)
		return True;
	return False;
}

/*!
 * Check-Routine zum Erkennen, ob sich bot an einer Linienunterberechung befindet oder Hindernis gesehen wird; wird
 * verwendet zum Abbrechen des Linienfolgers an Unterbrechung oder Hindernis
 * @return True wenn Unterbrechung erkannt wird oder Hindernis voraus gesehen wird
 */
static uint8_t check_sensors(void) {
	// Unterbrechung wird daran erkannt, wenn sich der Linienfolger wegen keiner Linie mehr voraus nach links dreht und dabei
	// der rechte Abgrundsensor die unterbrochene Linie erkennt; Unterbrechung darf auch nur ca. maximal 3cm sein
	if (sensBorderR >= BORDER_DANGEROUS && sensLineL < LINE_SENSE && sensLineR
			< LINE_SENSE) {
		line_state_enh = CHECK_BREAK_IN_LINE;
		return True;
	}

	// Abstand seit letztem Check ermitteln; nur bei Ueberschreitung des Mindestabstandes seit letzter Hindernisumrundung
	// darf wegen Hindernis abgebrochen werden, da sonst nach Umrundung und gleichem Hindernis beim Ausrichten des Linienfolgers
	// dieser sonst gleich wieder moeglicherweise abgebrochen wird
	uint16_t diff = (uint16_t) get_dist(x_pos, y_pos, pos_on_line.x, pos_on_line.y);
	if ((diff >= 100 * 100) || (pos_on_line.x == 0 && pos_on_line.y == 0)) {
		// Merkpos auf 0, wenn Abstand ueberschritten wurde
		pos_on_line.x = 0;
		pos_on_line.y = 0;
		if (sensDistL <= OPTIMAL_DISTANCE || sensDistR <= OPTIMAL_DISTANCE) {
			line_state_enh = TURN_AROUND_HAZARD;
			return True;
		}
	}

	return False;
}

/*!
 * erweiterter Linienfolger, der auch Linienunterbrechungen und Hindernisse handhabt, waehrend der Bot die Linie verfolgt;
 * die Linienunterbrechung darf nur relativ klein sein (~3cm), so dass sich beim Drehen am Ende der Linie der rechte Abgrundsensor
 * ueber dem Neubeginn der unterbrochenen Linie drehn muss
 * @param *data	Verhaltensdatensatz
 */
void bot_follow_line_enh_behaviour(Behaviour_t * data) {
	position_t point; // Var fuer Positionsberechnung nach Unterbrechung

	switch (line_state_enh) {
	case START_ENH: // maximal 60 cm vorwaerts bis Linie voraus
		bot_goto_dist(data, 600, 1);
		next_linestate = START_LINE_ENH; // Eintritt nachdem Cancel Linie gefunden hat
		bot_cancel_behaviour(data, bot_goto_pos_behaviour, check_line_sensors); // Ende der Vorwaertsfahrt wenn Linie erkannt wurde
		line_state_enh = END_LINE_ENH; // Ende falls keine Linie voraus gefunden wird
		break;

	case START_LINE_ENH: // Einsprung falls sich bot nun auf Linie befindet, weiter mit Linienfolger
		bot_follow_line(data); // Linienfolger laeuft los
		bot_cancel_behaviour(data, bot_follow_line_behaviour, check_sensors); // Abbruch Linienfolger bei Unterbrechung oder Hindernis
		line_state_enh = END_LINE_ENH; // einfach mal auf Ende, bei Cancel Linienfolger wird dort Einsprung gesetzt
		break;

	case CHECK_BREAK_IN_LINE: // Einsprung falls Linienunterbrechung erkannt wurde
		// Linie geht ja weiter nach Unterbrechung genau dort, wo rechter Abgrundsensor sich befindet; dort also Punkt berechnen und anfahren
		point = calc_point_in_distance(heading, BORDERSENSOR_POS_FW - 10,
				-BORDERSENSOR_POS_SW);
		bot_goto_pos(data, point.x, point.y, 999);
		line_state_enh = START_LINE_ENH; // Linienfolger sollte hier wieder greifen nach Ueberwindung der Unterbrechung
		break;

	case TURN_AROUND_HAZARD: // auf jeden Fall erst mal in Ausgangsrichtung drehen
/*! \todo	beim Drehen in beide Richtungen ermitteln, ob schon Kante, also Ende, des Hindernisses (Stein auf Linie) gesehen wird und
			weil umgehen in diese Richtung kuerzer ist, dortlang drehen und Hindernisumgeher starten; solve_maze geht leider nur wenn bot sich
			hier links dreht und dann rechts die Wand hat */
		bot_turn(data, 70); // links drehen
		line_state_enh = GO_AROUND_HAZARD; // naechsten Einsprung setzen
		break;

	case GO_AROUND_HAZARD: // etwas vorwaerts fahren um von Linie wegzukommen
/*! \todo	beim Drehen in beide Richtungen ermitteln, ob schon Kante, also Ende, des Hindernisses (Stein auf Linie) gesehen wird und
			weil umgehen in diese Richtung kuerzer ist, dortlang drehen und Hindernisumgeher starten */
		bot_goto_dist(data, 50, 1); // etwas vorwaerts fahren um von Linie wegzukommen
		line_state_enh = START_SOLVE_MAZE; // naechster Einsprung: Start solve_maze zum Umrunden des Hindernisses
		break;

	case START_SOLVE_MAZE: // Start Verhalten solve_maze zum Umrunden des Hindernisses
		bot_solve_maze(data); // Start solve_maze
		next_linestate = OTHER_SIDE_ON_LINE; // Eintritt nachdem Cancel Linie erkannt hat
		// Solve_maze abbrechen, wenn wieder Linie hinter Hindernis erkannt wird
		bot_cancel_behaviour(data, bot_solve_maze_behaviour, check_line_sensors);
		break;

	case OTHER_SIDE_ON_LINE: // Bot steht auf anderer Seite des Hindernisses wieder auf der Linie
		// hier muss der bot in die vom Hindernis wegweisende Richtung ausgerichtet werden
		// da er links um den Stein gegangen ist, blickt er entweder etwas Richtung Stein oder fast parallel;
		// einfach links wegdrehen, um ungefaehr die richtige Richtung einzunehmen

		// Sicherheit einbauen, dass er beim sich auf Linie Ausrichten und Hindernis sehen nicht wieder cancelt
		// Pos merken und erst wieder nach gewissem Abstand Hinderniserkennung mit Abbruch Linienfolger erlauben
		pos_on_line.x = x_pos;
		pos_on_line.y = y_pos;
		line_state_enh = TURN_NEXT_PART; // naechster Einsprung weiterdrehen mit Cancelueberwachung
		bot_turn(data, 40); // etwas links drehen
		break;

	case TURN_NEXT_PART: // weitere Drehung in richtiger Richtung fuer Linienfolger mit Cancelueberwachung
		// zweiter Teil der Drehung, hier aber Abbruch wenn linker Sensor auf Linie, da dies dann optimal fuer den Linienfolger ist
		bot_turn(data, 20);
		bot_cancel_behaviour(data, bot_turn_behaviour, check_left_line_sensor); // Abbruch Drehung wenn optimal fuer Linienfolger
		line_state_enh = SEARCH_LINE; // naechster Eintrittspunkt
		break;

	case SEARCH_LINE: // etwas vorfahren, um optimale Position fuer Linienfolger zu erreichen, falls er noch nicht optimal steht
		if (!check_left_line_sensor()) {
			bot_goto_dist(data, 50, 1); // etwas vor bis nur links Linie erkannt wird, dann optimal fuer Linienfolger
			bot_cancel_behaviour(data, bot_goto_pos_behaviour,
					check_left_line_sensor); // Vorfahren abbrechen wenn optimal fuer Linienfolger
		}
		// eigentlich wurde hier alles getan, um den bot optimal fuer den Linienfolger hinzusetezn in die richtige Richtung nach Hindernisumrundung; mit dieser
		// Hoffnung gehts weiter mit dem Linienfolger
		line_state_enh = START_LINE_ENH; // naechster Einsprung->Linienfolger geht von vorn los
		break;

	default:
		return_from_behaviour(data);
		break;
	}

}

/*!
 * Botenverhalten fuer den erweiterten Linienfolger
 * @param *caller Verhaltensdatensatz des Aufrufers
 */
void bot_follow_line_enh(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_follow_line_enh_behaviour, BEHAVIOUR_NOOVERRIDE);
	line_state_enh = 0;
	pos_on_line.x = 0;
	pos_on_line.y = 0;
}

#endif // BEHAVIOUR_FOLLOW_LINE_ENHANCED_AVAILABLE
