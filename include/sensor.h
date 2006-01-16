/*! @file 	sensor.h
 * @brief 	Architekturunabhängiger Teil der Sensorsteuerung
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

extern volatile char sensTrans;		///< Sensor Überwachung Transportfach

extern volatile char sensDoor;		///< Sensor Überwachung Klappe

extern volatile char sensError;		///< Überwachung Motor oder Batteriefehler

//extern volatile int sensRc5;			///< Fernbedienungssensor

extern volatile char setSensMouseDX;	///< Maussensor Delta X
extern volatile char setSensMouseDY;	///< Maussensor Delta X


#endif /*SENSOR_H_*/
