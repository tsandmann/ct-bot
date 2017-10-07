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
 * \file 	behaviour_drive_neuralnet.c
 * \brief 	Fahrverhalten zum neuronalen Netz
 * \author 	Frank Menzel (Menzelfr@gmx.de)
 * \date 	10.03.2015
 */

#include "bot-logic.h"
#ifdef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
#include "log.h"
#include "ui/available_screens.h"

#include "display.h"
#include "rc5-codes.h"

#include <stdlib.h>

/** \todo: fix warnings */
#pragma GCC diagnostic warning "-Wsign-conversion"

static uint8_t nn_drive_state = 0; /**< Status des Fahr-Verhaltens */

static uint8_t check_sector_state = 0; /**< Status des Sector-Pruef-Verhaltens */

static uint8_t border_in_front = False; /**< Kennung fuer Abgrund voraus */

#define BOT_STOP                     99
#define BOT_CHECK_SECTORS            0
#define BOT_GET_NN_DIR               1
#define BOT_FORWARD                  2
#define BOT_LEFT_TURN                3
#define BOT_RIGHT_TURN               4
#define BOT_CHECK_WALL_AFTER_FORWARD 5

/** Farbe des Zielfeldes, via Default auf Standard-Gruen festgelegt,
 *  kann fuer MCU spaeter angepasst werden, daher auf extra Define */

#define GROUND_GOAL_DEF      GROUND_GOAL

static uint8_t append_learnpattern = False; /**< Kennung ob Lernpattern dem Lernarray zugefuegt werden soll oder nicht */
static uint8_t teachout0 = 0; /**< Belegungen der Outputneuronen beim Speichern der Patterns */
static uint8_t teachout1 = 0;

/** Array zum Speichern der Belegungen der einzelnen Sektoren; Werte werden im
 * Wertebereich 0-1 gespeichert als Reziproke der Entfernung, d.h. umso weiter weg ein
 * Hindernis im jeweiligen Sektor ist, umso naeher liegt der Wert an 0; Hindernis umso naeher,
 * desto mehr liegt der Wert an 1
 * Abstandssensoren die ersten 3, die naechsten 3 die Abgrundwerte
 * von links 90(ueberdreht mit 95) Grad, dann Blick gradeaus 0, dann rechts -90(-95)
 */
static float sector_arr[NO_INPUT_NEURONS] = { 0, 0, 0, 0, 0, 0 };

/** Neuronen-Outputs des Netzes
 */
uint8_t netout0;
uint8_t netout1;

uint8_t bot_wish_action; // Wunschaktion nach Scannen

//#define DEBUG_BEHAVIOUR_DRIVE_NN          /**<  Schalter fuer Debug-Code */
#ifndef LOG_AVAILABLE
#undef DEBUG_BEHAVIOUR_DRIVE_NN
#endif
#ifndef DEBUG_BEHAVIOUR_DRIVE_NN
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif

/**
 * Aus der Kombination der beiden Netz-Ausgabewerte wird die Richtung des Bots ermittelt
 * \param out0 Outputneuron 0
 * \param out1 Outputneuron 1
 * 0 0 Stop
 * 0 1 Rechtsdrehung
 * 1 0 Linksdrehung
 * 1 1 Vorwaertsfahrt
 */
static uint8_t get_direction(uint8_t out0, uint8_t out1) {
	if (out0 == 1 && out1 == 0) {
		return BOT_LEFT_TURN;
	}
	if (out0 == 0 && out1 == 1) {
		return BOT_RIGHT_TURN;
	}
	if (out0 == 1 && out1 == 1) {
		return BOT_FORWARD;
	}

	return BOT_STOP;
}

/** Wenn die Liniensensoren das Gruenfeld erreichen, ist die Fahrt zu Ende, Ziel erreicht
 * \return	True wenn Ziel erreicht, Bot auf den Gruenfeld
 */
static uint8_t goal_reached(void) {

	if ((sensLineL > GROUND_GOAL_DEF - 5 && sensLineL < GROUND_GOAL_DEF + 5) || (sensLineR > GROUND_GOAL_DEF - 5 && sensLineR < GROUND_GOAL_DEF + 5)) {
		LOG_DEBUG("Zielfeld erreicht->Endebedingung");
		nn_drive_state = BOT_STOP;
		return True;
	}
	return False;
}

/**
 * Abbruchfunktion fuer das Fahrverhalten
 * \return	True wenn Hindernis nah voraus oder Abgrund
 */
static uint8_t drive_cancel_check(void) {
	/* rechts */
	if (sensDistL <= OPTIMAL_DISTANCE || sensDistR <= OPTIMAL_DISTANCE || goal_reached() || sensBorderL >= BORDER_DANGEROUS || sensBorderR >= BORDER_DANGEROUS) {
		LOG_DEBUG("Fahr-Abbruchbedingung erreicht Border: %i %i, Abstaende: %u %u", sensBorderL, sensBorderR, sensDistL, sensDistR);
		return True;
	} else {
		return False;
	}
}

/**
 *  dient zur Ausrichtung an der Wand nach Gradeausfahren
 */
static uint8_t parallel_wall_rightturn_cancel_check(void) {
	return (sensDistL <= sensDistR) ? True : False;
}

/**
 *  dient zur Ausrichtung an der Wand nach Gradeausfahren
 */
static uint8_t parallel_wall_leftturn_cancel_check(void) {
	return (sensDistR <= sensDistL) ? True : False;
}

/*
 * Berechnung der Werte fuer die Abgrundsensoren normalisiert fuer den
 * Wertebereich 0-1 gespeichert, 1 fuer Abgrund und 0 kein Abgrund
 * \param borderl linker Abgrundsensor
 * \param borderr rechter Abgrundsensor
 */
static float check_borders(int16_t borderl, int16_t borderr) {
	return (borderl >= BORDER_DANGEROUS || borderr >= BORDER_DANGEROUS) ? 1 : 0;
}

/* fahren nach neuronalem Netz
 * 1.einfach gradeaus
 * 2.gewisse Zeit warten und so weiterfahren
 * 3.dann NN abfragen und Faktorwerte holen -> 2.
 */
void bot_drive_neuralnet_behaviour(Behaviour_t * data) {
	switch (nn_drive_state) {
	case BOT_CHECK_SECTORS:
		// Umschalten zum eigentlichen Verhalten; damit bei Aktivierung ueber Verhaltensscreen
		// diese Verhalten auch ausgeschaltet werden, kommt ja dann nicht ueber Botenfunktion
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
		// verwaltet selbst Abstand zur Wand
		deactivateBehaviour(bot_avoid_col_behaviour);
#endif

#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE // Abgruende Abstand zur Wand
		deactivateBehaviour(bot_avoid_border_behaviour);
#endif
		bot_check_sectors(data);
		nn_drive_state = BOT_GET_NN_DIR;
		LOG_DEBUG("Sektoren checken, Abstaende voraus: %u %u", sensDistL, sensDistR);
		break;

	case BOT_GET_NN_DIR:
		LOG_DEBUG("NN abfragen und Richtung ermitteln");
		test_net(sector_arr);
		LOG_DEBUG("NN abfragen Dist Sektoren: %f %f %f ", sector_arr[0], sector_arr[1], sector_arr[2]);

		// 4 Kombinationen moeglich fuer Definition der Aktionen
		// 0 0 Stop, keine Bewegung
		// 0 1 Rechts drehen
		// 1 0 Links drehen
		// 1 1 Vorwaerts
		net_get_test_out(&netout0, &netout1);
		LOG_DEBUG("NN Outputs: %u %u", netout0, netout1);

		nn_drive_state = get_direction(netout0, netout1);
		break;

	case BOT_FORWARD: //fahre gradeaus
		LOG_DEBUG("Fahre gradeaus Abstaende: %u %u", sensDistL, sensDistR);

		// bei Fahren mit Fahrtwunsch ist Schluss nach dieser Aktion

		nn_drive_state = BOT_CHECK_WALL_AFTER_FORWARD;

		//nicht abfangen ob Abstand zu klein ist, moeglicherweise soll ja auf
		//engen Abstand trainiert werden; Trotzdem bei langer freien Sicht bis zum
		//optimalen Abstand fahren, darunter nur immer stueckchenweise

		if (drive_cancel_check()) {
			bot_drive_distance(data, 0, BOT_SPEED_NORMAL, 2); //kurze x cm vor
		} else {
			bot_goto_dist(data, 600, 1);

			bot_cancel_behaviour(data, bot_goto_pos_behaviour, drive_cancel_check);
		}

		break;

	case BOT_LEFT_TURN: // links drehen und dann fahren
		LOG_DEBUG("links drehen");
		bot_turn(data, 90); //links 90 Grad
		nn_drive_state = BOT_FORWARD;
		break;

	case BOT_RIGHT_TURN: // rechts drehen und dann fahren
		nn_drive_state = BOT_FORWARD;
		LOG_DEBUG("rechts drehen");
		bot_turn(data, -90); //rechts 90 Grad
		break;

	case BOT_CHECK_WALL_AFTER_FORWARD: // kommt hierher nach gradeausfahren, um sich wieder parallel zur Wand auszurichten
		//komme ja nach Fahren hierher, evtl. hat Abbruchbedingung zugeschlagen; dann darf ich mich bei Abgrund
		//nicht drehen-erst mal abpruefen
		if (check_borders(sensBorderL, sensBorderR) == 1) {
			LOG_DEBUG("Abgrund Abbruchbed erkannt -> zurueck");
			bot_drive_distance(data, 0, -BOT_SPEED_NORMAL, 10); // etwas zurueck
			border_in_front = True; //fuer Arrayabfrage Abgrund voraus setzen
			break;
		}

		nn_drive_state = BOT_CHECK_SECTORS;
		LOG_DEBUG("parallel zur Wand ausrichten l/r:  %u %u", sensDistL, sensDistR);
		uint16_t diff = 0;
		if (sensDistL > sensDistR) {
			diff = sensDistL - sensDistR;
		} else {
			diff = sensDistR - sensDistL;
		}

		if (diff >= 10) {
			nn_drive_state = BOT_CHECK_SECTORS; // Check Abstaende auf zu klein

			LOG_DEBUG("Drehen weil Ausrichtung notwendig l/r:  %u %u, diff: %u", sensDistL, sensDistR, diff);
			if (sensDistL < sensDistR) {
				LOG_DEBUG("links naeher dran, also nach links drehen");
				bot_turn(data, 90); //nach links maximal x Grad
				bot_cancel_behaviour(data, bot_turn_behaviour, parallel_wall_leftturn_cancel_check);
			}

			if (sensDistL > sensDistR) {
				LOG_DEBUG("rechts naeher dran, also nach rechts drehen");
				bot_turn(data, -90); //nach rechts maximal x Grad
				//bot_turn_maxspeed(data, 360, BOT_SPEED_SLOW);
				bot_cancel_behaviour(data, bot_turn_behaviour, parallel_wall_rightturn_cancel_check);
			}
		}
		if (bot_wish_action != 0) {
			nn_drive_state = BOT_STOP;
		}
		break;

	default:
		LOG_DEBUG("Fahr-Verhalten Ende");
		nn_drive_state = 0;
		bot_wish_action = 0;
		border_in_front = False;
		return_from_behaviour(data);
		break;
	}

}

/**
 * Ruft das Fahrverhalten auf
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_drive_neuralnet(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_drive_neuralnet_behaviour, BEHAVIOUR_OVERRIDE);
	nn_drive_state = 0;
	bot_wish_action = 0;
}

/**
 * das Fahrverhalten wird nur mit einer bestimmten Wunschaktion gestartet und dann beendet ohne weiterzufahren;
 * wird am Ende des Sektor-Check-Verhaltens gestartet, wenn Patterns gelernt werden solln mit Speicherung im Netz
 * \param *caller	  Verhaltensdatensatz
 * \param wishaction  gewuenschte Aktion, also vorwaerts fahren, links fahren oder rechts fahren
 */
void bot_simple_drivewish(Behaviour_t * caller, uint8_t wishaction) {
	switch_to_behaviour(caller, bot_drive_neuralnet_behaviour, BEHAVIOUR_OVERRIDE);
	nn_drive_state = 0;
	bot_wish_action = wishaction;
	nn_drive_state = wishaction;

}

/*
 * Berechnung der Werte fuer die Belegungen der einzelnen Sektoren; Werte werden im
 * Wertebereich 0-1 gespeichert als Reziproke der Entfernung, d.h. umso weiter weg ein
 * Hindernis im jeweiligen Sektor ist, umso naeher liegt der Wert an 0; Hindernis umso naeher,
 * desto mehr liegt der Wert an 1 ran
 */
static float check_distances(uint16_t sensl, uint16_t sensr) {
	float value = 0;
	float minval = 0;

	if (sensl < 200 || sensr < 200) {
		LOG_DEBUG("1 zurueckgegeben, Wand nah");
		return 1;
	} else {
		if (sensl > 800 && sensr > 800) {
			LOG_DEBUG("0 zurueckgegeben, Wand weit weg");
			return 0;
		} else {
			// umso freier umso naeher an 0 dran,
			// umso naeher ein Hindernis gesehen wird, umso naeher an 1 dran
			if (sensl < sensr) {
				minval = sensl;
			} else {
				minval = sensr;
			}
			value = minval / 100;

			value = 1 / value;
			LOG_DEBUG("%f zurueckgegeben, min Abstand: %f", value, minval);
			return value;
		}
	}
}

/**
 * prueft die Sektoren neben dem Bot auf Befahrbarkeit
 * dreht sich dazu jeweils nach links 90 Grad, dann rechts 90 Grad
 * \param *data	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_check_sector_behaviour(Behaviour_t * data) {
	//float border=0;
	switch (check_sector_state) {
	case 0:
#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
		// verwaltet selbst Abstand zur Wand
		deactivateBehaviour(bot_avoid_col_behaviour);
#endif
		LOG_DEBUG("Sektoren pruefen, zuerst 95 Grad links drehen %u %u", sensDistL, sensDistR);
		bot_turn(data, 95); //links 90 Grad

		check_sector_state = 1;
		break;

	case 1:
		LOG_DEBUG("bin jetzt 95 Grad links %u %u", sensDistL, sensDistR);
		bot_turn(data, -95); //und wieder urspruengliche Blickrichtung
		check_sector_state = 2;
		sector_arr[0] = check_distances(sensDistL, sensDistR);
		sector_arr[3] = check_borders(sensBorderL, sensBorderR);
		break;

	case 2:
		LOG_DEBUG("bin urspruengliche Richtung %u %u", sensDistL, sensDistR);
		bot_turn(data, -95); //90 Grad nach rechts
		sector_arr[1] = check_distances(sensDistL, sensDistR);
		if (border_in_front) {
			sector_arr[4] = 1; // Hindernis als Abgrund voraus
			border_in_front = False;
		} else {
			sector_arr[4] = check_borders(sensBorderL, sensBorderR);
		}
		check_sector_state = 3;
		break;

	case 3: //ist jetzt 90 Grad rechts
		LOG_DEBUG("bin jetzt 95 Grad rechts %u %u", sensDistL, sensDistR);
		bot_turn(data, 95); //wieder zurueck auf 45
		check_sector_state = 4;
		sector_arr[2] = check_distances(sensDistL, sensDistR);
		sector_arr[5] = check_borders(sensBorderL, sensBorderR);
		break;

	case 4:
		// das Sectorarray ist hier gefuellt, NN kann fuer naechste Aktion abgefargt werden
		check_sector_state = 99;

		LOG_DEBUG("bin wieder urspruengliche Richtung %u %u", sensDistL, sensDistR);
		sector_arr[1] = check_distances(sensDistL, sensDistR);
		break;

	default:
		// Abspeichern des Lernpatterns wenn bei Aufruf des Verhaltens gewuenscht
		if (append_learnpattern) {
			//LOG_DEBUG("Pattern gespeichertin NN index %u ,Outputs: %u  %u",no_of_pairs, teachout0, teachout1);
			fill_pattern_in_lernarray(sector_arr, no_of_pairs, teachout0, teachout1);
			no_of_pairs++;
			//jetzt auch die Aktion ausfuehren, die ausgewaehlt wurde, bei Stop nicht notwendig
			if (bot_wish_action > 0 && bot_wish_action != BOT_STOP) {
				bot_simple_drivewish(NULL, bot_wish_action);
			}
		}

		return_from_behaviour(data);
		break;
	}

} //bot_check_Sector_behaviour

/**
 * die Botenfunktion zum Start des Sectorcheck-Verhaltens
 * \param *caller	Verhaltensdatensatz
 */
void bot_check_sectors(Behaviour_t * caller) {
	append_learnpattern = False;
	switch_to_behaviour(caller, bot_check_sector_behaviour, BEHAVIOUR_OVERRIDE);
	check_sector_state = 0;
	bot_wish_action = 0;
}

/**
 * Verhalten analog oben, jedoch zusaetzliche Parameter mitgegeben zum Ausfuehren und Speichern eines Patterns
 * der Wunschaktion im neur. Netz
 * \param *caller	  Verhaltensdatensatz
 * \param out0        Wert fuer die Belegung des Outputneurons 0 fuer die Wunschaktion
 * \param out1        Wert fuer die Belegung des Outputneurons 1 fuer die Wunschaktion
 * \param wishaction  nach Sectorcheck soll der Bot diese Aktion ausfuehren, was den Werten in out0 out1 entspricht
 */
void bot_check_sectors_savepattern(Behaviour_t * caller, uint8_t out0, uint8_t out1, uint8_t wishaction) {
	if (no_of_pairs >= NO_PAIRS) {
		LOG_DEBUG("Max in Lernarray erreicht, nichts mehr speicherbar %u, index %u", NO_PAIRS, no_of_pairs);

	} else {
		append_learnpattern = True;
		switch_to_behaviour(caller, bot_check_sector_behaviour, BEHAVIOUR_OVERRIDE);
		check_sector_state = 0;
		//gewuenschte Outputs mit den Sectorwerten zusammen abspeichern
		teachout0 = out0;
		teachout1 = out1;
		bot_wish_action = wishaction;
	}
}

#ifdef DISPLAY_DRIVE_NEURALNET_AVAILABLE

/**
 * Aus der Botaktion heraus wird die Belegung der binaeren Outputwerte der Ausgangsneuronen zurueckgegeben
 * Routine wird nur vom Display verwendet
 * 0 0 Stop
 * 0 1 Rechtsdrehung   BOT_RIGHT_TURN
 * 1 0 Linksdrehung    BOT_LEFT_TURN
 * 1 1 Vorwaertsfahrt  BOT_FORWARD
 */
static void get_outputs_from_direction(uint8_t botaction, uint8_t *out0, uint8_t *out1) {
	*out0 = 0;
	*out1 = 0;

	if (botaction == BOT_FORWARD) {
		*out0 = 1;
		*out1 = 1;
	} else {
		if (botaction == BOT_LEFT_TURN) {
			*out0 = 1;
			*out1 = 0;
		} else {
			if (botaction == BOT_RIGHT_TURN) {
				*out0 = 0;
				*out1 = 1;
			}
		}
	}
}

/**
 * Keyhandler um die Fahrsituationen zu speichern zum nachfolgenden Lernen
 */
static void neuralnet_learn_disp_key_handler(void) {
	uint8_t out0;
	uint8_t out1;

	/* Keyhandling fuer Transport_Pillar-Verhalten */
	switch (RC5_Code) {

	case RC5_CODE_1:
		// Lernpattern, also Sektorbelegungen drumrum anhand der Abstandssensoren bestimmen und
		// Fahraktion dazu fuer vorwaerts speichern
		RC5_Code = 0;
		//bot_check_sectors(NULL);//Verhalten zum Pruefen der Umgebungssektoren aufrufen
		get_outputs_from_direction(BOT_FORWARD, &out0, &out1);
		bot_check_sectors_savepattern(NULL, out0, out1, BOT_FORWARD);
		break;

	case RC5_CODE_2:
		// Lernpattern, also Sektorbelegungen drumrum anhand der Abstandssensoren bestimmen und
		// Fahraktion dazu fuer vorwaerts speichern
		RC5_Code = 0;
		//bot_check_sectors(NULL);//Verhalten zum Pruefen der Umgebungssektoren aufrufen
		get_outputs_from_direction(BOT_STOP, &out0, &out1);
		bot_check_sectors_savepattern(NULL, out0, out1, BOT_STOP);
		break;

	case RC5_CODE_3:
		// Lernpattern, also Sektorbelegungen drumrum anhand der Abstandssensoren bestimmen und
		// Fahraktion dazu fuer vorwaerts speichern
		RC5_Code = 0;
		//bot_check_sectors(NULL);//Verhalten zum Pruefen der Umgebungssektoren aufrufen
		get_outputs_from_direction(BOT_LEFT_TURN, &out0, &out1);
		bot_check_sectors_savepattern(NULL, out0, out1, BOT_LEFT_TURN);
		break;

	case RC5_CODE_4:
		// Lernpattern, also Sektorbelegungen drumrum anhand der Abstandssensoren bestimmen und
		// Fahraktion dazu fuer vorwaerts speichern
		RC5_Code = 0;
		//bot_check_sectors(NULL);//Verhalten zum Pruefen der Umgebungssektoren aufrufen
		get_outputs_from_direction(BOT_RIGHT_TURN, &out0, &out1);
		bot_check_sectors_savepattern(NULL, out0, out1, BOT_RIGHT_TURN);
		break;

	case RC5_CODE_5:
		// Start des Fahrverhaltens, Richtungsentscheidungen aus dem Netz genommen
		RC5_Code = 0;
		if (total_error == 1 || total_error == 0) {
			LOG_DEBUG("Netz noch ungelernt, wird jetzt wenigstens 1x trainiert");
			// Koennte hier auch gleich ueber max Anzahl Iterationen trainieren,
			// aber fuer Anschauungseffekt zum Fahren mit untrainiertem Netz so gelassen
			net_check_one_step(NULL); //wenigstens 1x Netz durchlaufen lassen
		}
		bot_drive_neuralnet(NULL);

		break;

	case RC5_CODE_9:
		// Netz wird neu initialisiert mit Zufallsgewichtungen und einmaligem Test
		RC5_Code = 0;
		total_error = 1; //auf max setzen
		net_init();
		break;
	} // switch
} // Ende Keyhandler

/**
 * hier der NN-Screen zum Lernen verschiedener Fahr-Situationen
 */
void neuralnet_learn_display(void) {
	display_cursor(1, 1);
	display_printf("NET Learn,Patt: %2u", no_of_pairs);
	display_cursor(2, 1);
	display_puts("GO/Stop      : 1/2");
	display_cursor(3, 1);
	display_puts("Left/Right   : 3/4");

	display_cursor(4, 1);
	display_puts("NET Drive/Clr: 5/9");

	neuralnet_learn_disp_key_handler(); // aufrufen des Key-Handlers
}

#endif // DISPLAY_DRIVE_NEURALNET_AVAILABLE

#endif // BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
