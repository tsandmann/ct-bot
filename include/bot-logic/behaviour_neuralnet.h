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
 * \file 	behaviour_neuralnet.h
 * \brief 	neuronales Netz-Verhalten unter Verwendung der Library:
 * 	        lightweight backpropagation neural network sourceforge.net
 * \author 	Frank Menzel (Menzelfr@gmx.de)
 * \date 	10.03.2015
 */

#ifndef BEHAVIOUR_NEURALNET_H_
#define BEHAVIOUR_NEURALNET_H_

#ifdef BEHAVIOUR_NEURALNET_AVAILABLE


#define NO_LAYERS 3	/**< Anzahl der Layers (1 Inputlayer, 1 Hiddenlayer, 1 Outputlayer) */
#define NO_PAIRS 15	/**< Gesamtanzahl der Eingabepatterns */

#define NO_INPUT_NEURONS  6	/**< Anzahl Input-Neuronen */
#define NO_HIDDEN_NEURONS 3	/**< Anzahl Hidden-Neuronen */
#define NO_OUTPUT_NEURONS 2	/**< Anzahl Output-Neuronen */

#define MAX_TRAINING_DEF 1500	/**< max. Anzahl Iterationen zum Lernen pro Lernaufruf */
#define ERROR_THRESHOLD 0.0003f	/**< Ende der Lernschleife bei Erreichen dieser Fehlergrenze */

extern float total_error;	/**< Gesamtfehler des Netzes */

extern uint16_t no_of_pairs;	/**< Anzahl der im Array vorhandenen gueltigen Lernpatterns */

/**
 *  Netz wird mit allen Gewichtungen neu initialisiert
 */
void net_init (void);

/**
 * Das neuralnet-Verhalten selbst, das Netz wird mit den Inpupatterns trainiert
 * \param *data	Verhaltensdatensatz des Aufrufers
 */
void bot_neuralnet_behaviour(Behaviour_t * data);

/**
 * Rufe das neuralnet-Verhalten auf, das Netz wird mit den Inputpatterns trainiert
 * \param *caller	Verhaltensdatensatz
 */
void bot_neuralnet(Behaviour_t * caller);

/**
 * Rufe das neuralnet-Verhalten auf, wobei das Netz nur 1x durchlaufen wird zur Fehlerermittlung
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void net_check_one_step(Behaviour_t * caller);

/**
 *  Initialzustand mit den vordefinierten Inputpatterns und deren Teachoutput wiederherstellen
 */
void set_init_patterns_in_array(void);

/**
 *  Testen des Netzes mit der aktuellen Sektorbelegung
 */
void test_net(float *sectorarray);

/**
 * das Array der sector-Belegungen mit den gewuenschten Outputs wird eingetragen im Array an der Stelle index,
 * also Teachin- und Output wird gefuellt
 * Outputs sind bisher nur 2, bei noch mehr waere sicher auch Array sinnvoll
 * \param *sectorarray Array der Sektorbelegungen
 * \param index       Indexstelle im Array
 * \param out0        Output-Neuron 0
 * \param out1        Output-Neuron 1
 */
void fill_pattern_in_lernarray (float *sectorarray, int16_t index, uint8_t out0, uint8_t out1);

/**
 * Auslesen der Output-Neuronen aus dem Netz
 * \param *out1        Output-Neuron 1
 * \param *out2        Output-Neuron 2
 */
void net_get_test_out (uint8_t *out1, uint8_t *out2);

/**
 *  Setzen der zufaelligen Verbindungsgewichte
 */
void net_set_rnd_weights (void);

/**
 * Display zum Start / Anzeigen der NN-Routinen/ Daten
 */
void neuralnet_display(void);

#endif // BEHAVIOUR_NEURALNET_AVAILABLE
#endif // BEHAVIOUR_NEURALNET_H_
