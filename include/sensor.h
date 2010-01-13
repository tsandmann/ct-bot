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
 * @file 	sensor.h
 * @brief 	Architekturunabhaengiger Teil der Sensorsteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.2005
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include "global.h"
#include "ct-Bot.h"
#include "cmps03.h"

/*! Datenstruktur zur Ablage eines IR-Sensor-Wertepaares (Spannung | Distanz) */
typedef struct {
	uint8_t voltage;	/*!< Spannung des jeweiligen Eintrags (halbiert) */
	uint8_t dist;		/*!< Entfernung des jeweiligen Eintrags (gefuenftelt) */
} distSens_t;

/* Analoge Sensoren: Der Wertebereich aller analogen Sensoren umfasst 10 Bit. Also 0 bis 1023 */
extern int16_t sensDistL;			/*!< Distanz linker IR-Sensor [mm] ca. 100 bis 800 */
extern int16_t sensDistR;			/*!< Distanz rechter IR-Sensor [mm] ca. 100 bis 800  */
extern uint8_t sensDistLToggle;	/*!< Toggle-Bit des linken IR-Sensors */
extern uint8_t sensDistRToggle;	/*!< Toggle-Bit des rechten IR-Sensors */
/*! Zeiger auf die Auswertungsfunktion fuer die Distanzsensordaten, const. solange sie nicht kalibriert werden */
extern void (* sensor_update_distance)(int16_t * const p_sens, uint8_t * const p_toggle, const distSens_t * ptr, int16_t volt);
extern distSens_t sensDistDataL[];	/*!< kalibrierte Referenzdaten fuer linken IR-Sensor */
extern distSens_t sensDistDataR[];	/*!< kalibrierte Referenzdaten fuer rechten IR-Sensor */
extern uint8_t sensDistOffset;		/*!< Spannungs-Offset IR-Sensoren */


extern int16_t sensLDRL;		/*!< Lichtsensor links [0-1023];  1023 = dunkel*/
extern int16_t sensLDRR;		/*!< Lichtsensor rechts [0-1023];  1023 = dunkel*/

extern int16_t sensBorderL;	/*!< Abgrundsensor links [0-1023];  1023 = dunkel*/
extern int16_t sensBorderR;	/*!<  Abgrundsensor rechts [0-1023];  1023 = dunkel*/

extern int16_t sensLineL;	/*!<  Lininensensor links [0-1023];  1023 = dunkel*/
extern int16_t sensLineR;	/*!<  Lininensensor rechts [0-1023];  1023 = dunkel*/

/* digitale Sensoren */
extern int16_t sensEncL;	/*!< Encoder linker Motor [-32768 bis 32767] */
extern int16_t sensEncR;	/*!< Encoder rechter Motor [-32768 bis 32767] */

extern uint8_t sensTrans;	/*!< Sensor Ueberwachung Transportfach [0/1]*/
extern uint8_t sensDoor;	/*!< Sensor Ueberwachung Klappe [0/1] */
extern uint8_t sensError;	/*!< Ueberwachung Motor oder Batteriefehler [0/1]  1= alles ok */

extern uint16_t RC5_Code;	/*!< Letzter empfangener RC5-Code */

#ifdef BPS_AVAILABLE
extern int16_t sensBPSF;	/*!< Bot Positioning System vorn */
extern int16_t sensBPSR;	/*!< Bot Positioning System hinten */
#endif	// BPS_AVAILABLE


#ifdef MOUSE_AVAILABLE
extern int8_t sensMouseDX;		/*!< Maussensor Delta X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
extern int8_t sensMouseDY;		/*!< Maussensor Delta Y, positive Werte zeigen in Fahrtrichtung */
extern int16_t sensMouseX;		/*!< Mausposition X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
extern int16_t sensMouseY;		/*!< Mausposition Y, positive Werte zeigen in Fahrtrichtung  */
#endif	// MOUSE_AVAILABLE

extern float heading_enc;		/*!< Blickrichtung aus Encodern */
extern float x_enc;			/*!< X-Koordinate aus Encodern [mm] */
extern float y_enc;			/*!< Y-Koordinate aus Encodern [mm] */
extern int16_t v_enc_left;		/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
extern int16_t v_enc_right;		/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
extern int16_t v_enc_center;	/*!< Schnittgeschwindigkeit ueber beide Raeder */

#ifdef PC
extern int16_t simultime;	/*!< Simulierte Zeit */
#endif

#ifdef MEASURE_MOUSE_AVAILABLE
extern float heading_mou;		/*!< Aktuelle Blickrichtung relativ zur Startposition aus Mausmessungen */
extern float x_mou;			/*!< Aktuelle X-Koordinate in mm relativ zur Startposition aus Mausmessungen */
extern float y_mou;			/*!< Aktuelle Y-Koordinate in mm relativ zur Startposition aus Mausmessungen */
extern int16_t v_mou_center;	/*!< Geschwindigkeit in mm/s ausschliesslich aus den Maussensorwerten berechnet */
extern int16_t v_mou_left;		/*!< ...aufgeteilt auf linkes Rad */
extern int16_t v_mou_right;		/*!< ...aufgeteilt auf rechtes Rad */
#endif	// MEASURE_MOUSE_AVAILABLE

extern float heading;			/*!< Aktuelle Blickrichtung aus Encoder-, Maus- oder gekoppelten Werten */
extern int16_t heading_int;		 /*!< (int16_t) heading */
extern int16_t heading_10_int;	/*!< = (int16_t) (heading * 10.0f) */
extern float heading_sin;		/*!< = sin(heading * DEG2RAD) */
extern float heading_cos;		/*!< = cos(heading * DEG2RAD) */
extern int16_t x_pos;			/*!< Aktuelle X-Position aus Encoder-, Maus- oder gekoppelten Werten */
extern int16_t y_pos;			/*!< Aktuelle Y-Position aus Encoder-, Maus- oder gekoppelten Werten */
extern int16_t v_left;			/*!< Geschwindigkeit linkes Rad aus Encoder-, Maus- oder gekoppelten Werten */
extern int16_t v_right;			/*!< Geschwindigkeit rechtes Rad aus Encoder-, Maus- oder gekoppelten Werten */
extern int16_t v_center;			/*!< Geschwindigkeit im Zentrum des Bots aus Encoder-, Maus- oder gekoppelten Werten */

#ifdef SRF10_AVAILABLE
extern uint16_t sensSRF10;	/*!< Messergebniss Ultraschallsensor */
#endif

#ifdef CMPS03_AVAILABLE
cmps03_t sensCmps03;		/*!< Lage laut CMPS03-Kompass */
#endif

/*!
 * Kuemmert sich um die Weiterverarbeitung der rohen Sensordaten
 */
void sensor_update(void);

/*!
 * Setzt die Auswertungen der Sensorendaten zurueck
 */
void sensor_reset(void);

/*!
 * Errechnet aus den rohren Distanzsensordaten die zugehoerige Entfernung
 * @param p_sens	Zeiger auf den (Ziel-)Sensorwert
 * @param p_toggle	Zeiger auf die Toggle-Variable des Zielsensors
 * @param ptr		Zeiger auf auf Sensorrohdaten im EEPROM fuer p_sens
 * @param volt_16	Spannungs-Ist-Wert, zu dem die Distanz gesucht wird (in 16 Bit)
 */
void sensor_dist_lookup(int16_t * const p_sens, uint8_t * const p_toggle, const distSens_t * ptr, int16_t volt_16);

/*!
 * Die Funktion gibt aus, ob sich innerhalb einer gewissen Entfernung ein Objekt-Hindernis befindet.
 * @param distance	Entfernung in mm, bis zu welcher ein Objekt gesichtet wird.
 * @return 			Gibt False (0) zurueck, wenn kein Objekt innerhalb von distance gesichtet wird. Ansonsten die Differenz
 * zwischen dem linken und rechten Sensor. Negative Werte besagen, dass das Objekt naeher am linken, positive, dass
 * es naeher am rechten Sensor ist. Sollten beide Sensoren den gleichen Wert haben, gibt die Funktion 1 zurueck, um
 * von False unterscheiden zu koennen.
 */
int16_t is_obstacle_ahead(int16_t distance);

/*!
 * Updatet die LEDs je nach Sensorwert
 */
void led_update(void);

#ifdef DISPLAY_AVAILABLE
/*!
 * Displayhandler fuer Sensoranzeige
 */
void sensor_display(void);

/*!
 * Displayhandler fuer Odometrieanzeige
 */
void odometric_display(void);
#endif	// DISPLAY_AVAILABLE

#endif	/*SENSOR_H_*/
