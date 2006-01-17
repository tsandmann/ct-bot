/*! @file 	mouse.h 
 * @brief 	Routinen f�r die Ansteuerung eines opt. Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef mouse_H_
#define mouse_H_

#define MAUS_Y  0x02		///< Kommando um DY auszulesen
#define MAUS_X  0x03		///< Kommando um DX auszulesen

/*! 
 * Initialisiere Maussensor
 */ 
void maus_sens_init(void);

/*!
 * Schickt ein Lesekommando an den Sensor
 * und liest ein Byte zurück
 * @param adr die Adresse
 * @return das Datum
 */
int8 maus_sens_read(char adr);


#endif
