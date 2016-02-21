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
 * \file 	sensor_correction.h
 * \brief 	Kalibrierungsdaten fuer die IR-Sensoren
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	21.04.2007
 */
#ifndef SENSOR_CORRECTION_H_
#define SENSOR_CORRECTION_H_

//#define DISTSENS_AVERAGE	/**< Aktiviert die Mittelung ueber 4 Werte aus den Roh-Daten der Sensoren */

/* Mit diesen Daten wird das EEPROM des realen Bots initialisiert.
 * Im Falle eines simulierten Bots fuer den Sim liegen die Daten im RAM.
 * Das Kalibrierungsverhalten gibt die hier eizutragenden Daten am Ende
 * schon richtig vorformatiert per LOG aus, so dass man sie nur per
 * copy & paste aus dem LOG-Fenster hierher uebernehmen muss.
 */
#ifdef MCU

#define SENSDIST_OFFSET	0	/**< Offset (BOT), das vom Sensorrohwert abgezogen wird vor der Entfernungsberechnung */
/**
 * Wertepaare (BOT) fuer IR-Sensoren LINKS. Es ist jeweils (Spannung | Distanz) gespeichert.
 * Aufsteigende Sortierung!
 */
//	{498,100},{346,150},{266,200},{216,250},{180,300},{152,350},{138,400},{116,450},{104,500},{96,550},{84,600},{74,650},{76,700},{64,750}
#define SENSDIST_DATA_LEFT { \
	{ 896,  60},{ 661, 110},{ 477, 160},{ 372, 210},{ 315, 260},{ 273, 310},{ 239, 360},{ 220, 410},{ 204, 460},{ 188, 510},{ 178, 560},{ 166, 610},{ 154, 660},{ 150, 710} \
}
/**
 * Wertepaare (BOT) fuer IR-Sensoren RECHTS. Es ist jeweils (Spannung | Distanz) gespeichert.
 * Aufsetigende Sortierung!
 */
//	{496,100},{344,150},{266,200},{214,250},{176,300},{144,350},{128,400},{112,450},{96,500},{78,550},{70,600},{72,650},{70,700},{54,750}
#define SENSDIST_DATA_RIGHT { \
	{ 894,  60},{ 659, 110},{ 475, 160},{ 400, 210},{ 340, 260},{ 290, 310},{ 250, 360},{ 230, 410},{ 215, 460},{ 200, 510},{ 175, 560},{ 165, 610},{ 155, 660},{ 150, 710} \
}

#else

#define SENSDIST_OFFSET	0	/**< Offset (SIM), das vom Sensorrohwert abgezogen wird vor der Entfernungsberechnung */
/**
 * Wertepaare (SIM) fuer IR-Sensoren LINKS. Es ist jeweils (Spannung/2 | Distanz/5) gespeichert.
 * Aufsteigende Sortierung!
 */
#define SENSDIST_DATA_LEFT { \
	{510,100},{376,150},{292,200},{244,250},{204,300},{184,350},{168,400},{156,450},{144,500},{136,550},{130,600},{126,650},{120,700},{114,750} \
}
/**
 * Wertepaare (SIM) fuer IR-Sensoren RECHTS. Es ist jeweils (Spannung/2 | Distanz/5) gespeichert.
 * Aufsetigende Sortierung!
 */
#define SENSDIST_DATA_RIGHT { \
	{494,100},{356,150},{276,200},{230,250},{188,300},{164,350},{144,400},{128,450},{116,500},{106,550},{98,600},{90,650},{84,700},{80,750} \
}
#endif	// MCU

#endif // SENSOR_CORRECTION_H_
