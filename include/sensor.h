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

extern volatile uint8 sensTrans;		/*!< Sensor Ueberwachung Transportfach [0/1]*/

extern volatile uint8 sensDoor;		/*!< Sensor Ueberwachung Klappe [0/1] */

extern volatile uint8 sensError;		/*!< Ueberwachung Motor oder Batteriefehler [0/1]  1= alles ok */

extern uint16 RC5_Code;        /*!< Letzter empfangener RC5-Code */

extern volatile int8 sensMouseDX;		/*!< Maussensor Delta X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
extern volatile int8 sensMouseDY;		/*!< Maussensor Delta Y, positive Werte zeigen in Fahrtrichtung */

extern volatile int16 sensMouseX;		/*!< Mausposition X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
extern volatile int16 sensMouseY;		/*!< Mausposition Y, positive Werte zeigen in Fahrtrichtung  */

extern volatile int16 sensEncL;		/*!< Encoder linkes Rad */
extern volatile int16 sensEncR;		/*!< Encoder rechtes Rad */
extern volatile float heading_enc;		/*!< Blickrichtung aus Encodern */
extern volatile float x_enc;			/*!< X-Koordinate aus Encodern [mm] */
extern volatile float y_enc;			/*!< Y-Koordinate aus Encodern [mm] */
extern volatile float v_enc_left;		/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
extern volatile float v_enc_right;		/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
extern volatile float v_enc_center;	/*!< Schnittgeschwindigkeit ueber beide Raeder */

#ifdef MEASURE_MOUSE_AVAILABLE
	extern volatile float heading_mou;		/*!< Aktuelle Blickrichtung relativ zur Startposition aus Mausmessungen */
	extern volatile float x_mou;			/*!< Aktuelle X-Koordinate in mm relativ zur Startposition aus Mausmessungen */
	extern volatile float y_mou;			/*!< Aktuelle Y-Koordinate in mm relativ zur Startposition aus Mausmessungen */
	extern volatile float v_mou_center;	/*!< Geschwindigkeit in mm/s ausschliesslich aus den Maussensorwerten berechnet */
	extern volatile float v_mou_left;		/*!< ...aufgeteilt auf linkes Rad */
	extern volatile float v_mou_right;		/*!< ...aufgeteilt auf rechtes Rad */
#endif

#ifdef MEASURE_COUPLED_AVAILABLE
	extern volatile float heading;			/*!< Aktuelle Blickrichtung aus gekoppelten Werten */
	extern volatile float x_pos;			/*!< Aktuelle X-Position aus gekoppelten Werten */
	extern volatile float y_pos;			/*!< Aktuelle Y-Position aus gekoppelten Werten */
	extern volatile float v_left;			/*!< Geschwindigkeit linkes Rad aus gekoppelten Werten */
	extern volatile float v_right;			/*!< Geschwindigkeit rechtes Rad aus gekoppelten Werten */
	extern volatile float v_center;		/*!< Geschwindigkeit im Zentrum des Bots aus gekoppelten Werten */
#endif


extern volatile int8 sensors_initialized;	/*!< Wird 1 sobald die Sensorwerte zur Verfügung stehen */

#ifdef SRF10_AVAILABLE
	extern volatile uint16 sensSRF10;	/*!< Messergebniss Ultraschallsensor */
#endif

/*! Sensor_update
* Kümmert sich um die Weiterverarbeitung der rohen Sensordaten 
*/
void sensor_update(void);

/*! Linearisiert die Sensorwerte
 * @param left Linker Rohwert [0-1023]
 * @param right Rechter Rohwert [0-1023]
 */
void sensor_abstand(uint16 left, uint16 right);

#endif /*SENSOR_H_*/
