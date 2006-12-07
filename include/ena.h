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

/*! @file 	ena.h 
 * @brief 	Routinen zur Steuerung der Enable-Leitungen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef ENA_H_
#define ENA_H_

#include "global.h"

#define ENA_ABSTAND	(1<<0)		/*!< Enable-Leitung Abstandssensoren */
#define ENA_RADLED		(1<<1)		/*!< Enable-Leitung Radencoder */
#define ENA_SCHRANKE	(1<<2)		/*!< Enable-Leitung Fachueberwachung */
#define ENA_KANTLED	(1<<3)		/*!< Enable-Leitung Angrundsensor */
#define ENA_KLAPPLED	(1<<4)		/*!< Enable-Leitung Schieberueberwachung */
#define ENA_MAUS		(1<<5)		/*!< Enable-Leitung Liniensensor auf Mausplatine */
#define ENA_MMC		(1<<6)		/*!< Enable-Leitung Reserve 1 */
#define ENA_MOUSE_SENSOR		(1<<7)		/*!< Enable-Leitung Reserve 2 */
/*!
 * Initialisiert die Enable-Leitungen
 */
void ENA_init(void);

/*! 
 * Schaltet einzelne Enable-Leitungen an,
 * andere werden nicht beeinflusst
 * @param enable Bitmaske der anzuschaltenden LEDs
 */
void ENA_on(uint8 enable);

/*! 
 * Schaltet einzelne Enable-Leitungen aus,
 * andere werden nicht beeinflusst
 * @param enable Bitmaske der anzuschaltenden LEDs
 */
void ENA_off(uint8 enable);

/*!
 * Schaltet die Enable-Leitungen
 * @param enable Wert der eingestellt werden soll
 */
void ENA_set(uint8 enable);
#endif
