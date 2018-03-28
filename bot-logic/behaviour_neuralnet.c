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
 * \file 	behaviour_neuralnet.c
 * \brief 	neuronales Netz-Verhalten unter Verwendung der Library:
 *          lightweight backpropagation neural network sourceforge.net
 * \author 	Frank Menzel (Menzelfr@gmx.de)
 * \date 	10.03.2015
 */

/* Konfiguration fuer ct-Bot */
#include "bot-logic.h"
#ifdef BEHAVIOUR_NEURALNET_AVAILABLE

/** \todo: fix warnings */
#pragma GCC diagnostic warning "-Wdouble-promotion"

#include <stdlib.h>
#include <timer.h>
#ifdef PC
#include <time.h>
#endif

#include "ui/available_screens.h"
#include "display.h"
#include "rc5-codes.h"
#include "log.h"

#include "lwneuralnet.h"

uint16_t max_trainings;	/**< max. Anzahl der Lern-Iterationen; temporaer gesetzt */

static uint8_t neuralnet_state = 0;	/**< Status des neuralnet-Verhaltens */

#define STATE_NEURALNET_INIT 0	/**< die Verhaltens-Zustaende */
#define STATE_NEURALNET_WORK 1
#define STATE_NEURALNET_DONE 3

//#define DEBUG_BEHAVIOUR_NN	/**<  Schalter fuer Debug-Code */
#ifndef LOG_AVAILABLE
#undef DEBUG_BEHAVIOUR_NN
#endif
#ifndef DEBUG_BEHAVIOUR_NN
#undef LOG_DEBUG
#define LOG_DEBUG(a, ...) {}
#endif

/**
 * hier einige vordefinierte Lernpatterns fuer die 6 Input-Neuronen und die zugehoerigen 2 Output-Neuronen mit folgender Bedeutung an den 6 Indexstellen:
 * Es erfolgt eine Unterteilung in 3 Sektoren jeweils links vom Bot, gradeaus und rechts vom Bot und fuer diese Sektoren gibt es jeweils
 * 3 Neuronen fuer die Abstandswerte zu Hindernissen und weitere 3 fuer die Abgrundsensoren; also Dist links, Dist gradeaus, Dist rechts, Abgrund links, gradeaus und rechts;
 * das hier aufsetzende Fahrverhalten (extra Define zum Einschalten) dekodiert die Kombination der beiden Output-Neuronen zu den Fahrverhalten:
 * 0 0 -> Stop/ 1 0 -> links fahren/ 0 1 -> rechts fahren/ 1 1 -> gradeaus fahren
 * von den beiden Abstandssensoren wird jeweils der geringere Abstand zum Hindernis normiert zwischen 0 und 1 eingetragen, auch Zwischenwerte, und fuer die Abgrundwerte
 * nur 0(kein Abgrund) oder 1(Abgrund)
 * mit diesen Lernpatterns wird das neuronale Netz sofort trainiert und der Gesamtfehler im Screen ausgegeben, umso kleiner desto besser (also umso naeher liegen die beiden Outputs
 * an den hier vorgegebenen Soll-Outputwerten)
 */
static float inputs_def[NO_PAIRS][NO_INPUT_NEURONS] = { { 0, 0, 0, 0, 0, 0 },	/**< nirgends Hindernisse oder Abgruende */
{ 0, 0, 1, 0, 0, 0 },	/**< rechts Hindernis */
{ 0, 1, 0, 0, 0, 0 },	/**< Hindernis voraus */
{ 0, 1, 1, 0, 0, 0 },	/**< rechte Ecke, also vorn und rechts Hindernis */
{ 1, 0, 0, 0, 0, 0 },	/**< Wand links */
{ 1, 0, 1, 0, 0, 0 },	/**< Wand links und rechts, vorn frei */
{ 1, 1, 0, 0, 0, 0 },	/**< linke Ecke, also links und voraus Wand */
{ 1, 1, 1, 0, 0, 0 },	/**< nichts geht mehr, vorn rechts und links Wand */
{ 0.7f, 1, 0, 0, 0, 0 },	/**< mal ein paar Zwischenwerte, Wand im jeweiligen Sektor nicht ganz so nah */
{ 0, 1, 0.7f, 0, 0, 0 },	/**< Je naeher die Wand, desto mehr liegt der Wert an 1, also Wand voraus aber rechts noch etwas entfernt */
{ 1, 0.7f, 1, 0, 0, 0 }, { 0.28f, 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 1, 0 },	/**< Abgrund voraus, keine Wand gesehen */
{ 0.5f, 0, 0, 0, 1, 0 },	/**< Abgrund voraus, links etwas weiter Wand, rechts komplett frei */
{ 0, 0, 0.5f, 0, 1, 0 }	/**< Abgrund voraus, links frei, Wand rechts etwas weiter weg */
};
/**
 * 4 Kombinationen moeglich fuer Definition der Aktionen im Zieloutput-Array
 * 00 Stop, keine Bewegung
 * 01 Rechts drehen
 * 10 Links drehen
 * 11 Vorwaerts
 */
static float targets_def[NO_PAIRS][NO_OUTPUT_NEURONS] = { { 1, 1 }, { 1, 1 },	/**< fahren */
	{ 1, 0 }, { 1, 0 },	/**< nach links fahren */
	{ 1, 1 }, { 1, 1 },	/**< fahren */
	{ 0, 1 },	/**< nach rechts fahren */
	{ 0, 0 },	/**< Stop */
	{ 0, 1 },	/**< nach rechts fahren */
	{ 1, 0 },	/**< ... */
	{ 1, 1 }, { 0, 1 }, { 1, 0 }, { 0, 1 }, { 1, 0 } };

uint16_t no_of_pairs = NO_PAIRS;	/**< Anzahl der vorkommenden Testpaare */

static float inputs[NO_PAIRS][NO_INPUT_NEURONS];	/**< hier werden die zu erlernenden Patterns fuer das Netz gespeichert */

static float targets[NO_PAIRS][NO_OUTPUT_NEURONS];	/**< hier werden die zu erlernenden Ziel-Outputs fuer das Netz gespeichert */

float outputarr[NO_OUTPUT_NEURONS];	/**< Netz-Rueckgabewerte fuer den Ist-Output */

/**< Zeiger auf das neuronale Netz ueberhaupt */
network_t *net = NULL;

/**< Errorwert des aktuell getesteten Inputpatterns*/
float error;
/**< Gesamt-Fehlerwert aller Patterns */
float total_error;

static uint16_t t;

/**
 * Initialisierung Zufallsgenerator
 */
static void srand_statement(void) {
#ifdef PC
	srand(time(NULL));
#else
	srand(TIMER_GET_TICKCOUNT_16);
#endif
}

#ifdef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
/**
 * das Array der Sectorbelegungen mit den gewuenschten Outputs wird eingetragen im Array an der Stelle index,
 * also Teachin- und Output wird gefuellt
 * Outputs sind bisher nur 2, bei noch mehr waere sicher auch Array sinnvoll
 * \param *sectorarray Array der Sektorbelegungen
 * \param index       Indexstelle im Array
 * \param out0        Outputneuron 0
 * \param out1        Outputneuron 1
 */
void fill_pattern_in_lernarray(float *sectorarray, int16_t index, uint8_t out0, uint8_t out1) {
	uint8_t i;

	// NO_PAIRS ist ja das Define mit der Anzahl der Indexeintraege des Input-Lernarrays zum Initialzeitpunkt; ist also
	// damit gleichzeitig die maximale Anzahl der Indexeintraege und darf nicht ueberschritten werden->abpruefen
	LOG_DEBUG("Maximum %u, index %u", NO_PAIRS, index);
	if (index >= NO_PAIRS) {
		LOG_DEBUG("Maximum im Array erreicht %u, index %u", NO_PAIRS, index);
		return;
	}

	for (i = 0; i < NO_INPUT_NEURONS; i++)
		inputs[index][i] = sectorarray[i];

	// nun die zugehoerigen Teach-Outputs, also Aktion ob links rechts gradeaus oder Stop
	targets[index][0] = out0;
	targets[index][1] = out1;
}
#endif // BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE

/**
 * Testen des NN Netzes mit den Sektor-Eingabewerten, die normalisiert sein muessen im Wertebereich 0-1
 * Der Bot-Sichtbereich wurde dazu in 3 Sektoren aufgeteilt mit dieser Reihenfolge:
 * links 90 Grad,  vorn, rechts -90Grad; die rechtesten 3 Werte sind die Werte fuer den Abgrund
 * alle 6 Werte werden dem Netz als Input-Neuronen vorgelegt
 */
void test_net(float *sectorarray) {
	uint8_t i;

	// zum Testen des Netzwerks
	static float testinputs[1][NO_INPUT_NEURONS] = { { 0, 0, 0, 0, 0, 0 } };

	for (i = 0; i < NO_INPUT_NEURONS; i++)
		testinputs[0][i] = sectorarray[i];

	net_compute(net, testinputs[0], outputarr);
}

/**
 * Auslesen der Werte der Output-Neuronen aus dem Netz nach Test durch Vorlage der Inputwerte
 * 4 Kombinationen moeglich fuer 2 Outputs, wobei die Outputs binaer codiert sind
 *  Definition der Aktionen:
 *	0 0 Stop, keine Bewegung
 *	0 1 Rechts drehen
 *	1 0 Links drehen
 *	1 1 Vorwaerts
 *
 * \param *out1        Outputneuron 1
 * \param *out2        Outputneuron 2
 * */
void net_get_test_out(uint8_t *out1, uint8_t *out2) {
	// die berechneten IST-Outputwerte digitalisieren auf 0 oder 1
	*out1 = (outputarr[0] < 0.5f) ? 0 : 1;
	*out2 = (outputarr[1] < 0.5f) ? 0 : 1;
}

/**
 *  Initialzustand mit den vordefinierten Inputpatterns und deren Teachoutput wiederherstellen
 */
void set_init_patterns_in_array(void) {
	uint16_t i, j;

	no_of_pairs = NO_PAIRS;	// Anzahl der Eintrage wieder auf Initial setzen

	// Arbeitsarray der Input-Patterns mit dem Default-Array belegen
	for (i = 0; i < no_of_pairs; i++) {
		for (j = 0; j < NO_INPUT_NEURONS; j++)
			inputs[i][j] = inputs_def[i][j];
	}

	// Arbeitsarray mit den Ziel-Outputs nach dem Default-Array belegen
	for (i = 0; i < no_of_pairs; i++) {
		//inputs[i][1],outputarr[0],outputarr[1],targets[i][0], targets[i][1]);
		for (j = 0; j < NO_OUTPUT_NEURONS; j++)
			targets[i][j] = targets_def[i][j];
	}

	// nun auch wieder die Zufallsgewichtungen in den Neuronenverbindungen einstellen
	net_set_rnd_weights();
}

/**
 *  Setzen der zufaelligen Verbindungsgewichte
 */
void net_set_rnd_weights(void) {
	if (net != NULL) {	// falls Netz initial, wird dies alles beim Erzeugen des Netzes selbst gemacht
		srand_statement();

		/* initialize weights and deltas */
		net_randomize(net, 1.0);
		net_reset_deltas(net);

		/* permanently set output of bias neurons to 1 */
		net_use_bias(net, 1);
	}
}

/**
 * Netz wird neu initialisiert mit Zufallsgewichtungen, Bias und Deltas wie beim neu erzeugen des Netzes
 */
void net_init(void) {
	uint16_t i, j;

	net_set_rnd_weights();

	// Arbeitsarray mit den Inputpatterns auf 0
	for (i = 0; i < NO_PAIRS; i++) {
		for (j = 0; j < NO_INPUT_NEURONS; j++)
			inputs[i][j] = 0;
	}

	// Arbeitsarray mit den Zieloutputs auf 0
	for (i = 0; i < NO_PAIRS; i++) {
		for (j = 0; j < NO_OUTPUT_NEURONS; j++)
			targets[i][j] = 0;
	}

	no_of_pairs = 0;	// nix im Array, total leer
}

/**
 * Das neuralnet-Verhalten selbst, das Netz wird mit den Inputpatterns im input-Array trainiert
 * bis zum maximalen Zaehlerwert in max_trainings
 * \param *data	Verhaltensdatensatz des Aufrufers
 */
void bot_neuralnet_behaviour(Behaviour_t* data) {
	switch (neuralnet_state) {
	case STATE_NEURALNET_INIT:
		neuralnet_state = STATE_NEURALNET_WORK; //1

		// Falls bei Start sofort active, dann muss ohne Botenfunktion der Wert gesetzt werden
		if (max_trainings == 0)
			max_trainings = MAX_TRAINING_DEF;

		if (net == NULL) {
			// In- Output-Arrays uebertragen mit den vordefinierten Patterns
			set_init_patterns_in_array();

			LOG_DEBUG("Netz Init mit Zufallswerten: (max %i)\n", max_trainings);
			// Zufallswerte init
			srand_statement();

			net = net_allocate(NO_LAYERS, NO_INPUT_NEURONS, NO_HIDDEN_NEURONS, NO_OUTPUT_NEURONS);
		} else {
			LOG_DEBUG("Netz schon vorhanden, wird wiederverwendet und weitergelernt");
		}
		total_error = 0;
		t = 0;
		break;

	case STATE_NEURALNET_WORK:
		if (t >= max_trainings || ((total_error > 0) && (total_error < ERROR_THRESHOLD))) {
			neuralnet_state = STATE_NEURALNET_DONE; //Ende
		} else {
			uint16_t i = 0;
			error = 0;

			// hier werden alle vorhandenen Patterns dem Netz vorgelegt zur immer weiteren Verringerung
			// des Gesamtfehlers via Backpropagating
			while (i < no_of_pairs) {
				/* compute the outputs for inputs(i) */
				net_compute(net, inputs[i], outputarr);

				/* find the error with respect to targets(i) */
				error = error + net_compute_output_error(net, targets[i]);

				/* train the network one step */
				net_train(net);
				i++;

			} // while, naechstes zu trainierendes Pattern

			error = error / no_of_pairs; // geteilt durch anzahl der Testpatterns

			/* Gesamtfehler berechnen */
			total_error = (t == 0) ? error : 0.9f * total_error + 0.1f * error;

			/* naechster Lernschritt */
			t++;
		}
		break;

	default:
#ifdef DEBUG_BEHAVIOUR_NN
		LOG_DEBUG("Number of training performed: %i (max %i)\n", t, max_trainings);
		LOG_DEBUG("End: output error: %f \n", total_error);

		// jetzt die Wunsch und Istwerte ausgeben der Input-Patterns
		for (i=0; i< no_of_pairs; i++) {
			net_compute (net, /*inputs (i)*/ inputs[i], outputarr);
			error = net_compute_output_error (net, targets[i]);
			LOG_DEBUG("Pattern: %f %f Out:%f %f Ziel:%f %f\n", inputs[i][0], inputs[i][1],outputarr[0],outputarr[1],targets[i][0], targets[i][1]);
		} // for
#endif // DEBUG_BEHAVIOUR_NN
		// wieder auf 0 setzen; wird entweder durch Botenfunktion auf gewuenschten Wert gesetzt oder
		// vom Verhalten selbst falls Aufruf erfolgt ohne Botenfunktion durch einfaches Aktivieren via Screen
		max_trainings = 0;
		return_from_behaviour(data);
		break;
	}
} // bot_neuralnet_behaviour

/**
 * Ruft das neuralnet-Verhalten auf, das Netz wird mit den Inputpatterns trainiert
 * \param *caller	Verhaltensdatensatz
 */
void bot_neuralnet(Behaviour_t * caller) {
	// nur trainierbar wenn auch Patternpaare drin stehen
	if (no_of_pairs == 0) {
		LOG_DEBUG("keine Patterns zum Training vorhanden");
	} else {
		max_trainings = MAX_TRAINING_DEF;
		switch_to_behaviour(caller, bot_neuralnet_behaviour, BEHAVIOUR_OVERRIDE);
		neuralnet_state = STATE_NEURALNET_INIT;
	}
}

/**
 * Ruft das neuralnet-Verhalten auf, wobei das Netz nur 1x durchlaufen wird zur Fehlerermittlung
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void net_check_one_step(Behaviour_t * caller) {
	// nur trainierbar wenn auch Patternpaare drin stehen
	if (no_of_pairs > 0) {
		max_trainings = 1;	// nur 1x Netz durchlaufen lassen
		switch_to_behaviour(caller, bot_neuralnet_behaviour, BEHAVIOUR_OVERRIDE);
		neuralnet_state = STATE_NEURALNET_INIT;
	}
}

#ifdef DISPLAY_NEURALNET_AVAILABLE
/**
 * Keyhandler fuer das neuronale Netz-Verhalten
 */
static void neuralnet_disp_key_handler(void) {
	/* Keyhandling fuer NN-Verhalten */
	switch (RC5_Code) {
	case RC5_CODE_4:
		// Defaultpatterns werden wiederhergestellt wie nach Botstart
		RC5_Code = 0;
		set_init_patterns_in_array();

		// einmaligen Test durchfuehren zur Fehlerermittlung und Anzeige
		net_check_one_step(NULL);
		break;

#ifdef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
		/**
		 * NN Fahrverhalten hier nur aufrufbar, wenn auch das Verhalten selbst existiert
		 */
	case RC5_CODE_5:
		// Start des Fahrverhaltens, Richtungsentscheidungen aus dem Netz genommen
		RC5_Code = 0;
		// Totaler Fehler bewegt sich zwischen 0 und 1 aber nicht genau drauf, sonst ungelernt
		if (total_error == 1 || total_error == 0) {
			LOG_DEBUG("Netz noch ungelernt, wird jetzt wenigstens 1x trainiert");
			// Koennte hier auch gleich ueber max Anzahl Iterationen trainieren,
			// aber fuer Anschauungseffekt zum Fahren mit untrainiertem Netz so gelassen
			net_check_one_step(NULL);	// wenigstens 1x Netz durchlaufen lassen
		}

		bot_drive_neuralnet(NULL);
		break;
#endif // BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE

	case RC5_CODE_9:
		// Netz trainieren
		RC5_Code = 0;
		// nur trainierbar wenn auch zu lernende Patterns im Lernarray drinstehen
		bot_neuralnet(NULL);
		break;
	} // switch
} // Ende NN-Keyhandler

/**
 * Display zum Start/ Anzeigen der NN-Routinen/ Daten
 */
void neuralnet_display(void) {
	display_cursor(1, 1);
	display_printf("NET Err: %f", total_error);
	//display_cursor(2, 1);
	//display_printf("Patterns: %2u", no_of_pairs);
	display_cursor(3, 1);
	display_puts("InitPatt/Train: 4/9");

	display_cursor(4, 1);
#ifdef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
	display_puts("NET Drive: 5");
#endif

	neuralnet_disp_key_handler();	// Aufrufen des NN Key-Handlers
}
#endif // DISPLAY_NEURALNET

#endif // BEHAVIOUR_NEURALNET_AVAILABLE
