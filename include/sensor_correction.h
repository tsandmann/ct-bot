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

/*! @file 	sensor_correction.h
 * @brief 	Konstanten, um die Sensoren zu linearisieren
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	17.02.06
*/
#ifndef SENSOR_CORRECTION_H_
#define SENSOR_CORRECTION_H_

#define SENSDISTSLOPELEFT	48333	/*!< Stuetzwert a fuer Linearisierung des linken Distanzsensors */
#define SENSDISTOFFSETLEFT	53		/*!< Stuetzwert b fuer Linearisierung des linken Distanzsensors */
#define SENSDISTSLOPERIGHT	48333	/*!< Stuetzwert a fuer Linearisierung des rechten Distanzsensors */
#define SENSDISTOFFSETRIGHT	48	/*!< Stuetzwert b fuer Linearisierung des rechten Distanzsensors */


/* Parameter fuer die IR-Sensoren*/
#define SENS_IR_MAX_DIST	800		/*!< Obergrenze des Erfassungsbereichs */
#define SENS_IR_INFINITE	999		/*!< Kennzeichnung fuer "kein Objekt im Erfassungsbereich" */


#endif /*SENSOR_CORRECTION_H_*/
