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

/*! @file 	mouse_pc.c 
 * @brief 	Routinen fuer die Ansteuerung eines opt. Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/
#include "global.h"

#include "ct-Bot.h"
#include "log.h"

#ifdef PC

uint8 mousePicture[18*18];

/*! 
 * Initialisiere Maussensor
 */ 
void maus_sens_init(void){
}

uint16 pixIndex=0;

/*!
 * Bereitet das auslesen eines ganzen Bildes vor
 */
void maus_image_prepare(void){
	pixIndex=0;
	
	int x,y;
	for (x=0; x<18;x++)
		for (y=0; y<18;y++)
			mousePicture[y + x*18] =  0x40 | y*4;	
			
	mousePicture[0]|= 0x80;	// Start of Frame	
	
}

/*!
 * Liefert bei jedem Aufruf das naechste Pixel des Bildes
 * Insgesamt gibt es 324 Pixel
 * <pre>
 * 18 36 ... 324
 * .. .. ... ..
 *  2 20 ... ..
 *  1 19 ... 307
 * </pre>
 * Bevor diese Funktion aufgerufen wird, muss maus_image_prepare() aufgerufen werden!
 * @return Die Pixeldaten (Bit 0 bis Bit5), Pruefbit, ob Daten gueltig (Bit6), Markierung fuer den Anfang eines Frames (Bit7)
 */
int8 maus_image_read(void){
	
	if (pixIndex==324)
		pixIndex =0;
		
	return mousePicture[pixIndex++];
}

/*!
 * Gibt den SQUAL-Wert zurueck. Dieser gibt an, wieviele Merkmale der Sensor 
 * im aktuell aufgenommenen Bild des Untergrunds wahrnimmt.
 * Fuer simulierten Bot zur Zeit mit einem fixen Wert belegt, da fuer den
 * Boden im Sim keine Texturen verwendet werden
 */
int8 maus_get_squal(void) {
	return 70;
}

#endif
