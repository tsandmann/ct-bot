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
 * @file 	sensor_correction.h
 * @brief 	Kalibrierungsdaten fuer die IR-Sensoren
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	21.04.2007
 */
#ifndef SENSOR_CORRECTION_H_
#define SENSOR_CORRECTION_H_

/* Mit diesen Daten wird das EEPROM des realen Bots initialisiert.
 * Im Falle eines simulierten Bots fuer den Sim liegen die Daten im RAM.
 * Das Kalibrierungsverhalten gibt die hier eizutragenden Daten am Ende 
 * schon richtig vorformatiert per LOG aus, so dass man sie nur per 
 * copy & paste aus dem LOG-Fenster hierher uebernehmen muss.
 */
#ifdef MCU

#define SENSDIST_OFFSET	0	/*!< Offset (BOT), das vom Sensorrohwert abgezogen wird vor der Entfernungsberechnung */
/*! 
 * Wertepaare (BOT) fuer IR-Sensoren LINKS. Es ist jeweils (Spannung/2 | Distanz/5) gespeichert. 
 * Aufsteigende Sortierung! 
 */
#define SENSDIST_DATA_LEFT { \
	{502/2,100/5},{354/2,150/5},{274/2,200/5},{220/2,250/5},{184/2,300/5},{158/2,350/5},{142/2,400/5}, \
	{114/2,450/5},{102/2,500/5},{90/2,550/5},{72/2,600/5},{64/2,650/5},{58/2,700/5},{48/2,750/5} \
};
/*! 
 * Wertepaare (BOT) fuer IR-Sensoren RECHTS. Es ist jeweils (Spannung/2 | Distanz/5) gespeichert. 
 * Aufsetigende Sortierung! 
 */
#define SENSDIST_DATA_RIGHT { \
	{496/2,100/5},{356/2,150/5},{276/2,200/5},{224/2,250/5},{184/2,300/5},{156/2,350/5},{138/2,400/5}, \
	{116/2,450/5},{104/2,500/5},{92/2,550/5},{78/2,600/5},{76/2,650/5},{70/2,700/5},{58/2,750/5} \
};	
								
#else

#define SENSDIST_OFFSET	0	/*!< Offset (SIM), das vom Sensorrohwert abgezogen wird vor der Entfernungsberechnung */
/*!
 * Wertepaare (SIM) fuer IR-Sensoren LINKS. Es ist jeweils (Spannung/2 | Distanz/5) gespeichert. 
 * Aufsteigende Sortierung! 
 */
#define SENSDIST_DATA_LEFT { \
	{510/2,100/5},{376/2,150/5},{292/2,200/5},{244/2,250/5},{204/2,300/5},{184/2,350/5},{168/2,400/5}, \
	{156/2,450/5},{144/2,500/5},{136/2,550/5},{130/2,600/5},{126/2,650/5},{120/2,700/5},{114/2,750/5} \
};
/*!
 * Wertepaare (SIM) fuer IR-Sensoren RECHTS. Es ist jeweils (Spannung/2 | Distanz/5) gespeichert. 
 * Aufsetigende Sortierung! 
 */
#define SENSDIST_DATA_RIGHT { \
	{494/2,100/5},{356/2,150/5},{276/2,200/5},{230/2,250/5},{188/2,300/5},{164/2,350/5},{144/2,400/5}, \
	{128/2,450/5},{116/2,500/5},{106/2,550/5},{98/2,600/5},{90/2,650/5},{84/2,700/5},{80/2,750/5} \
};	
#endif	// MCU

/* Grenzwerte fuer die IR-Sensoren */
#define SENS_IR_MIN_DIST	100		/*!< Untergrenze des Erfassungsbereichs */
#define SENS_IR_MAX_DIST	690		/*!< Obergrenze des Erfassungsbereichs */
#define SENS_IR_INFINITE	995		/*!< Kennzeichnung fuer "kein Objekt im Erfassungsbereich" */	

#endif // SENSOR_CORRECTION_H_
