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

/*! @file 	mouse.c 
 * @brief 	Routinen fuer die Ansteuerung eines opt. Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/
#include "global.h"


#ifdef MCU 

#include <avr/io.h>
#include "ct-Bot.h"
#include "mouse.h"
#include "delay.h"
#include "ena.h"

#ifdef MAUS_AVAILABLE

#define MAUS_DDR 	DDRB			/*!< DDR fuer Maus-SCLK */
#define MAUS_PORT 	PORTB			/*!< PORT fuer Maus-SCLK */
#define MAUS_SCK_PIN	(1<<7)		/*!< PIN fuer Maus-SCLK */

#define MAUS_SDA_NR		6		/*!< Pin an dem die SDA-Leitung haengt */
#define MAUS_SDA_PINR 	PINB		/*!< Leseregister */
#define MAUS_SDA_PIN 	(1<<MAUS_SDA_NR)	/*!< Bit-Wert der SDA-Leitung */

#define MOUSE_Enable() ENA_on(ENA_MOUSE_SENSOR)

/*!
 * Uebertraegt ein Byte an den Sensor
 * @param data das Byte
 */
void maus_sens_writeByte(uint8 data){
	int8 i;
	MAUS_DDR  |= MAUS_SDA_PIN; 		// SDA auf Output
	
	for (i=7; i>=0; i--){
		MAUS_PORT &= ~MAUS_SCK_PIN;		// SCK auf Low, vorbereiten
		
		//Daten rausschreiben
		MAUS_PORT = (MAUS_PORT & (~MAUS_SDA_PINR)) |  ((data >> (7 - MAUS_SDA_NR)) & MAUS_SDA_PIN);	
		data = data <<1;		// naechstes Bit vorbereiten
		asm volatile("nop"); 			// Etwas warten 
		
		MAUS_PORT |= MAUS_SCK_PIN;		// SCK =1 Sensor uebernimmt auf steigender Flanke
	}
}

/*!
 * Liest ein Byte vom Sensor
 * @return das Byte
 */
uint8 maus_sens_readByte(void){
	int i;
	char data=0;

	MAUS_DDR  &= ~MAUS_SDA_PIN; 	// SDA auf Input

	for (i=7; i>-1; i--){
		MAUS_PORT &= ~MAUS_SCK_PIN;		// SCK =0 Sensor bereitet Daten auf fallender Flanke vor !
		data=data<<1;					// Platz schaffen

		asm volatile("nop"); 					// Etwas warten 
		MAUS_PORT |= MAUS_SCK_PIN;		// SCK =1 Daten lesen  auf steigender Flanke
		
		data |= (MAUS_SDA_PINR >> MAUS_SDA_NR) & 0x01;			//Daten lesen
	}

	return data;
}

/*!
 * Uebertraegt ein write-Kommando an den Sensor
 * @param adr Adresse
 * @param data Datum
 */
void maus_sens_write(int8 adr, uint8 data){
	int16 i;
	
	MOUSE_Enable();
	
	maus_sens_writeByte(adr|=0x80);  //rl MSB muss 1 sein Datenblatt S.12 Write Operation
	maus_sens_writeByte(data);
	for (i=0; i<300; i++){ asm volatile("nop"); 	}	// mindestens 100 Mikrosekunden Pause!!!
}

/*!
 * Schickt ein Lesekommando an den Sensor
 * und liest ein Byte zurueck
 * @param adr die Adresse
 * @return das Datum
 */
uint8 maus_sens_read(uint8 adr){
	MOUSE_Enable();
	int16 i;
	maus_sens_writeByte(adr);
	for (i=0; i<300; i++){asm volatile("nop");}	// mindestens 100 Mikrosekunden Pause!!!
	
	return maus_sens_readByte();
}

/*! 
 * Initialisiere Maussensor
 */ 
void maus_sens_init(void){
	delay(100);
	
	MAUS_DDR  |= MAUS_SCK_PIN; 	// SCK auf Output
	MAUS_PORT &= ~MAUS_SCK_PIN;	// SCK auf 0
	
	delay(10);
	
	maus_sens_write(MOUSE_CONFIG_REG,MOUSE_CFG_RESET);	//Reset sensor
	maus_sens_write(MOUSE_CONFIG_REG,MOUSE_CFG_FORCEAWAKE);	//Always on
}

/*! muessen wir nach dem ersten Pixel suchen?*/
static uint8 firstRead;
/*!
 * Bereitet das auslesen eines ganzen Bildes vor
 */
void maus_image_prepare(void){
	maus_sens_write(MOUSE_CONFIG_REG,MOUSE_CFG_FORCEAWAKE);	//Always on

	maus_sens_write(MOUSE_PIXEL_DATA_REG,0x00);	// Frame grabben anstossen
	firstRead=1; //suche erstes Pixel 
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
	int8 pixel=maus_sens_read(MOUSE_PIXEL_DATA_REG);
	if ( firstRead ==1){
		while ( (pixel & 0x80) != 0x80){
			pixel=maus_sens_read(MOUSE_PIXEL_DATA_REG);
//			if ((pixel & 0x70) != 0x70)
//				return 0;
		}
		firstRead=0;
	}
	
	return pixel;
}

/*!
 * Gibt den SQUAL-Wert zurueck. Dieser gibt an, wieviele Merkmale der Sensor 
 * im aktuell aufgenommenen Bild des Untergrunds wahrnimmt
 */
int8 maus_get_squal(void) {
	return maus_sens_read(MOUSE_SQUAL_REG);
}

#endif
#endif
