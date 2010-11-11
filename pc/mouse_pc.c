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
 * @file 	mouse_pc.c
 * @brief 	Routinen fuer die Ansteuerung eines opt. Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#ifdef PC
#include "ct-Bot.h"

#ifdef MOUSE_AVAILABLE
#include "mouse.h"

/*!
 * Initialisiere Maussensor
 */
void mouse_sens_init(void) {
}

/*!
 * Uebertraegt ein Bild vom Maussensor an den PC
 */
void mouse_transmit_picture(void) {
}

/*!
 * Gibt den SQUAL-Wert zurueck. Dieser gibt an, wieviele Merkmale der Sensor
 * im aktuell aufgenommenen Bild des Untergrunds wahrnimmt.
 * Fuer simulierten Bot zur Zeit mit einem fixen Wert belegt, da fuer den
 * Boden im Sim keine Texturen verwendet werden
 */
uint8_t mouse_get_squal(void) {
	return 70;
}

#endif // MOUSE_AVAILABLE
#endif // PC
