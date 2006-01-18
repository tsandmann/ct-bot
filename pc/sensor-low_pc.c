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

/*! @file 	bot-sens_pc.c  
 * @brief 	Low-Level Routinen fuer die Sensor Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include "sensor-low.h"

/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void){
}

/*!
 * Alle Sensoren aktualisieren.
 * Das geschieht auf der PC Seite anders -- in einem eigenen Thread.
 * Diese Funktion ist nur ein Dummy zur Kompatibilitaet.
 */
void bot_sens_isr(void){
}
#endif
