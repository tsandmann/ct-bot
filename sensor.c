/*! @file 	sensor.c  
 * @brief 	Architekturunabhängiger Teil der Sensorsteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
*/

#include "ct-Bot.h"

volatile int16 sensLDRL=0;		///< Lichtsensor links
volatile int16 sensLDRR=0;		///< Lichtsensor rechts

volatile int16 sensDistL=0;		///< Distanz linker IR-Sensor
volatile int16 sensDistR=0;		///< Distanz rechter IR-Sensor

volatile int16 sensBorderL=0;	///< Abgrundsensor links
volatile int16 sensBorderR=0;	///< Abgrundsensor rechts

volatile int16 sensLineL=0;	///< Lininensensor links
volatile int16 sensLineR=0;	///< Lininensensor rechts

volatile int16 sensLdrL=0;		///< Helligkeitssensor links
volatile int16 sensLdrR=0;		///< Helligkeitssensor links

volatile char sensTrans=0;		///< Sensor Überwachung Transportfach

volatile char sensDoor=0;		///< Sensor Überwachung Klappe

volatile char sensError=0;		///< Überwachung Motor oder Batteriefehler

//volatile int sensRc5;			///< Fernbedienungssensor

volatile char sensMouseDX;	///< Maussensor Delta X
volatile char sensMouseDY;	///< Maussensor Delta X


volatile int sensEncL=0;	///< Encoder linker Motor
volatile int sensEncR=0;	///< Encoder rechter Motor
