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

/*! @file 	mouse.h 
 * @brief 	Routinen fuer die Ansteuerung eines optischen Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef mouse_H_
#define mouse_H_

#define MAUS_Y  0x42		/*!< Kommando, um DY auszulesen */
#define MAUS_X  0x43		/*!< Kommando, um DX auszulesen */

#define MAUS_CONF		0x40	/*!< Kommando für Konfiguration */
#define MAUS_STATUS	0x41	/*!< Kommando für Status */

/*! 
 * Initialisiere Maussensor
 */ 
void maus_sens_init(void);

/*!
 * Schickt ein Lesekommando an den Sensor
 * und liest ein Byte zurueck
 * @param adr die Adresse
 * @return das Datum
 */
int8 maus_sens_read(char adr);


#endif
