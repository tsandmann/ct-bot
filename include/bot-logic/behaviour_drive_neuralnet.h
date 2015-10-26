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
 * \file 	behaviour_drive_neuralnet.h
 * \brief 	Fahrverhalten zum neuronalen Netz
 * \author 	Frank Menzel (Menzelfr@gmx.de)
 * \date 	10.03.2015
 */

#ifndef BEHAVIOUR_DRIVE_NEURALNET_H_
#define BEHAVIOUR_DRIVE_NEURALNET_H_

#ifdef BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE

/**
 * das neuronale Netz Fahrverhalten; Richtungsentscheidung wird aus dem Netz genommen
 * \param *data der Verhaltensdatensatz
 */
void bot_drive_neuralnet_behaviour(Behaviour_t * data);

/**
 * Ruft das NN-Fahr-Verhalten auf
 * \param caller Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_drive_neuralnet(Behaviour_t * caller);

/**
 * Display fuer das Fahrverhalten
 */
void neuralnet_learn_display(void);

/**
 * prueft die Sektoren neben dem Bot auf Befahrbarkeit
 * dreht sich dazu jeweils nach links 90 Grad, dann rechts 90 Grad
 * \param *data	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_check_sector_behaviour(Behaviour_t * data);

/**
 * die Botenfunktion zum Start des Sectorcheck-Verhaltens
 * \param *caller	Verhaltensdatensatz
 */
void bot_check_sectors(Behaviour_t * caller);

/**
 * Verhalten analog oben, jedoch zusaetzliche Parameter mitgegeben zum Ausfuehren und Speichern eines Patterns
 * der Wunschaktion im neur. Netz
 * \param *caller	  Verhaltensdatensatz
 * \param out0        Wert fuer die Belegung des Outputneurons 0 fuer die Wunschaktion
 * \param out1        Wert fuer die Belegung des Outputneurons 1 fuer die Wunschaktion
 * \param wishaction  nach Sectorcheck soll der Bot diese Aktion ausfuehren, was den Werten in out0 out1 entspricht
 */
void bot_check_sectors_savepattern(Behaviour_t * caller, uint8_t out0, uint8_t out1, uint8_t wishaction );

/**
 * das Fahrverhalten wird nur mit einer bestimmten Wunschaktion gestartet und dann beendet ohne weiterzufahren;
 * wird am Ende des Sektor-Check-Verhaltens gestartet, wenn Patterns gelernt werden solln mit Speicherung im Netz
 * \param *caller	  Verhaltensdatensatz
 * \param wishaction  gewuenschte Aktion, also vorwaerts fahren, links fahren oder rechts fahren
 */
void bot_simple_drivewish(Behaviour_t * caller, uint8_t wishaction);

#endif // BEHAVIOUR_DRIVE_NEURALNET_AVAILABLE
#endif // BEHAVIOUR_DRIVE_NEURALNET_H_
