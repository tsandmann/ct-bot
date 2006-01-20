/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	sensor.h
 * @brief 	Architekturunabhaengiger Teil der Sensorsteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
*/

#ifndef SENSOR_H_
#define SENSOR_H_

#include "global.h"

extern int16 sensDistL;				///< Distanz linker IR-Sensor
extern int16 sensDistR;				///< Distanz rechter IR-Sensor

extern volatile int16 sensLDRL;		///< Lichtsensor links
extern volatile int16 sensLDRR;		///< Lichtsensor rechts


extern volatile int16 sensEncL;		///< Encoder linker Motor
extern volatile int16 sensEncR;		///< Encoder rechter Motor

extern volatile int16 sensBorderL;	///< Abgrundsensor links
extern volatile int16 sensBorderR;	///<  Abgrundsensor rechts

extern volatile int16 sensLineL;	///<  Lininensensor links
extern volatile int16 sensLineR;	///<  Lininensensor rechts

extern volatile int16 sensLdrL;		///< Helligkeitssensor links
extern volatile int16 sensLdrR;		///< Helligkeitssensor links

extern volatile char sensTrans;		///< Sensor Ueberwachung Transportfach

extern volatile char sensDoor;		///< Sensor Ueberwachung Klappe

extern volatile char sensError;		///< Ueberwachung Motor oder Batteriefehler

//extern volatile int sensRc5;			///< Fernbedienungssensor

extern volatile char sensMouseDX;	///< Maussensor Delta X
extern volatile char sensMouseDY;	///< Maussensor Delta X

extern volatile int sensMouseX;
extern volatile int sensMouseY;

#endif /*SENSOR_H_*/
