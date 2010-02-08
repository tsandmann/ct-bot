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
 * @file 	srf10.h
 * @brief 	Ansteuerung des Ultraschall Entfernungssensors SRF10
 * @author 	Chris efstathiou (hendrix@otenet.gr) & Carsten Giesen (info@cnau.de)
 * @date 	08.04.06
 */

#include "ct-Bot.h"
#ifdef SRF10_AVAILABLE

#include "global.h"

#ifndef SRF10_H_
#define SRF10_H_

/*!
 * Es sind alle moeglichen Adressen eingetragen. In de Regel reicht aber die erste              
 * Der Code geht nur von SRF10_UNIT_0 aus, daher sind die anderen auskommentiert                
*/

#define SRF10_UNIT_0   0xE0  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_1   0xE2  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_2   0xE4  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_3   0xE6  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_4   0xE8  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_5   0xEA  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_6   0xEC  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_7   0xEE  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_8   0xF0  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_9   0xF2  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_10  0xF4  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_11  0xF6  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_12  0xF8  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_13  0xFA  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_14  0xFC  /* the SRF10 MODULE I2C address */
//#define SRF10_UNIT_15  0xFE  /* the SRF10 MODULE I2C address */

#define SRF10_MIN_GAIN        0      /*!< setze Verstaerung auf 40 */
#define SRF10_MAX_GAIN        16     /*!< setze Verstaerung auf 700 */
#define SRF10_MIN_RANGE       0      /*!< Minimale Reichweite 43mm */
#define SRF10_MAX_RANGE       5977   /*!< Maximale Reichweite 5977mm */

#define SRF10_INCHES          0X50	 /*!< Messung in INCHES */
#define SRF10_CENTIMETERS     0X51	 /*!< Messung in CM */
#define SRF10_MICROSECONDS    0X52	 /*!< Messung in Millisekunden */

#define SRF10_COMMAND         0		 /*!< W=Befehls-Register R=Firmware*/
#define SRF10_LIGHT           1		 /*!< W=Verstaerkungsfaktor R=Nicht benutzt */
#define SRF10_HIB             2		 /*!< W=Reichweite R=Messung High-Byte */
#define SRF10_LOB             3		 /*!< W=Nicht benutzt R=Messung Low-Byte */

/*!
 * SRF10 initialsieren
 */
void srf10_init(void);

/*!
 * Verstaerkungsfaktor setzen
 * @param gain Verstaerkungsfaktor
 */
void srf10_set_gain(uint8_t gain);

/*!
 * Reichweite setzen, hat auch Einfluss auf die Messdauer
 * @param millimeters Reichweite in mm
 */
void srf10_set_range(uint16_t millimeters);

/*!
 * Messung ausloesen
 * @param metric_unit 0x50 in Zoll, 0x51 in cm, 0x52 ms
 * @return Resultat der Aktion
 */
uint8_t srf10_ping(uint8_t metric_unit);

/*!
 * Register auslesen
 * @param SRF10_register welches Register soll ausgelsen werden
 * @return Inhalt des Registers
 */
uint8_t srf10_read_register(uint8_t SRF10_register);

/*!
 * Messung starten Ergebniss aufbereiten und zurueckgeben
 * @return Messergebniss
 */
uint16_t srf10_get_measure(void);

#endif	// SRF10_AVAILABLE
#endif  /* SRF10_H_ */
