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

/* Analoge Sensoren: Der Wertebereich aller analogen Sensoren umfasst 10 Bit. Also 0 bis 1023 */

extern volatile int16 sensDistL;	/*!< Distanz linker IR-Sensor [mm] ca. 100 bis 800 */
extern volatile int16 sensDistR;	/*!< Distanz rechter IR-Sensor [mm] ca. 100 bis 800  */

extern volatile int16 sensLDRL;		/*!< Lichtsensor links [0-1023];  1023 = dunkel*/
extern volatile int16 sensLDRR;		/*!< Lichtsensor rechts [0-1023];  1023 = dunkel*/

extern volatile int16 sensBorderL;	/*!< Abgrundsensor links [0-1023];  1023 = dunkel*/
extern volatile int16 sensBorderR;	/*!<  Abgrundsensor rechts [0-1023];  1023 = dunkel*/

extern volatile int16 sensLineL;	/*!<  Lininensensor links [0-1023];  1023 = dunkel*/
extern volatile int16 sensLineR;	/*!<  Lininensensor rechts [0-1023];  1023 = dunkel*/

/* digitale Sensoren */
extern volatile int16 sensEncL;		/*!< Encoder linker Motor [-32768 bis 32767] */
extern volatile int16 sensEncR;		/*!< Encoder rechter Motor [-32768 bis 32767] */

extern volatile char sensTrans;		/*!< Sensor Ueberwachung Transportfach [0/1]*/

extern volatile char sensDoor;		/*!< Sensor Ueberwachung Klappe [0/1] */

extern volatile char sensError;		/*!< Ueberwachung Motor oder Batteriefehler [0/1]  1= alles ok */

extern uint16 RC5_Code;        /*!< Letzter empfangener RC5-Code */

extern volatile char sensMouseDX;		/*!< Maussensor Delta X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
extern volatile char sensMouseDY;		/*!< Maussensor Delta Y, positive Werte zeigen in Fahrtrichtung */

extern volatile int sensMouseX;		/*!< Mausposition X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
extern volatile int sensMouseY;		/*!< Mausposition Y, positive Werte zeigen in Fahrtrichtung  */


extern volatile int16 v_left;			/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
extern volatile int16 v_right;			/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */

/*! Sensor_update
* Kümmert sich um die Weiterverarbeitung der rohen Sensordaten 
*/
void sensor_update(void);

#endif /*SENSOR_H_*/
