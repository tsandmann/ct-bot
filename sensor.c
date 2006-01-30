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

/*! @file 	sensor.c  
 * @brief 	Architekturunabhaengiger Teil der Sensorsteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
*/

#include "ct-Bot.h"

volatile int16 sensLDRL=0;		/*!< Lichtsensor links */
volatile int16 sensLDRR=0;		/*!< Lichtsensor rechts */

volatile int16 sensDistL=1023;		/*!< Distanz linker IR-Sensor */
volatile int16 sensDistR=1023;		/*!< Distanz rechter IR-Sensor */

volatile int16 sensBorderL=0;	/*!< Abgrundsensor links */
volatile int16 sensBorderR=0;	/*!< Abgrundsensor rechts */

volatile int16 sensLineL=0;	/*!< Lininensensor links */
volatile int16 sensLineR=0;	/*!< Lininensensor rechts */

volatile int16 sensLdrL=0;		/*!< Helligkeitssensor links */
volatile int16 sensLdrR=0;		/*!< Helligkeitssensor links */

volatile char sensTrans=0;		/*!< Sensor Ueberwachung Transportfach */

volatile char sensDoor=0;		/*!< Sensor Ueberwachung Klappe */

volatile char sensError=0;		/*!< Ueberwachung Motor oder Batteriefehler */

//volatile int sensRc5;			/*!< Fernbedienungssensor */

volatile char sensMouseDX;	/*!< Maussensor Delta X */
volatile char sensMouseDY;	/*!< Maussensor Delta X */


volatile int sensMouseX=0; /*! Mausposition X */
volatile int sensMouseY=0; /*! Mausposition Y */

volatile int sensEncL=0;	/*!< Encoder linker Motor */
volatile int sensEncR=0;	/*!< Encoder rechter Motor */
