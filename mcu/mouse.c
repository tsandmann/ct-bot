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
 * @file 	mouse.c
 * @brief 	Routinen fuer die Ansteuerung eines opt. Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef MOUSE_AVAILABLE
#include <avr/io.h>
#include "mouse.h"
#include "delay.h"
#include "ena.h"
#include "command.h"

#define MOUSE_DDR 		DDRB	/*!< DDR fuer Maus */
#define MOUSE_PORT 		PORTB	/*!< PORT fuer Maus */
#define MOUSE_SCK_PIN	(1<<7)	/*!< PIN fuer Maus-SCLK */

#define MOUSE_SDA_NR	6		/*!< Pin an dem die SDA-Leitung haengt */
#define MOUSE_SDA_PINR 	PINB	/*!< Leseregister */
#define MOUSE_SDA_PIN 	(1<<MOUSE_SDA_NR)	/*!< Bit-Wert der SDA-Leitung */

#define MOUSE_Enable()	ENA_on(ENA_MOUSE_SENSOR)

/*!
 * Uebertraegt ein Byte an den Sensor
 * @param data das Byte
 */
static void mouse_sens_writeByte(uint8_t data) {
	int8_t i;
	MOUSE_DDR |= MOUSE_SDA_PIN; // SDA auf Output

	for (i = 7; i >= 0;) {
		MOUSE_PORT = (uint8_t) (MOUSE_PORT & ~MOUSE_SCK_PIN); // SCK auf low, vorbereiten

		/* Daten rausschreiben */
		if ((data & 0x80) == 0x80) {
			MOUSE_PORT |= MOUSE_SDA_PIN;
		} else {
			MOUSE_PORT = (uint8_t) (MOUSE_PORT & ~MOUSE_SDA_PIN);
		}

		i--; // etwas warten
		__asm__ __volatile__("nop");

		MOUSE_PORT |= MOUSE_SCK_PIN; // SCK =1 Sensor uebernimmt auf steigender Flanke

		data = (uint8_t) (data << 1); // naechstes Bit vorbereiten
	}
}

/*!
 * Liest ein Byte vom Sensor
 * @return das Byte
 */
static uint8_t mouse_sens_readByte(void) {
	int8_t i;
	uint8_t data = 0;

	MOUSE_DDR = (uint8_t) (MOUSE_DDR & ~MOUSE_SDA_PIN); // SDA auf Input

	for (i = 7; i >= 0;) {
		MOUSE_PORT = (uint8_t) (MOUSE_PORT & ~MOUSE_SCK_PIN); // SCK =0 Sensor bereitet Daten auf fallender Flanke vor!
		data = (uint8_t) (data << 1); // Platz schaffen

		i--; // etwas warten
		__asm__ __volatile__("nop");

		MOUSE_PORT |= MOUSE_SCK_PIN; // SCK =1 Daten lesen auf steigender Flanke

		/* Daten lesen */
		if ((MOUSE_SDA_PINR & MOUSE_SDA_PIN) == MOUSE_SDA_PIN) {
			data |= 1;
		}
	}

	return data;
}

/*!
 * wartet 100 us
 */
static void mouse_sens_wait(void) {
	delay_us(100);
}

/*!
 * Uebertraegt ein write-Kommando an den Sensor
 * @param adr Adresse
 * @param data Datum
 */
void mouse_sens_write(uint8_t adr, uint8_t data) {
	MOUSE_Enable();
	mouse_sens_writeByte(adr |= 0x80); // MSB muss 1 sein, Datenblatt S.12 Write Operation
	mouse_sens_writeByte(data);
	mouse_sens_wait(); // 100 us Pause
}

/*!
 * Schickt ein Lesekommando an den Sensor
 * und liest ein Byte zurueck
 * @param adr die Adresse
 * @return das Datum
 */
uint8_t mouse_sens_read(uint8_t adr) {
	MOUSE_Enable();
	mouse_sens_writeByte(adr);
	mouse_sens_wait(); // 100 us Pause
	return mouse_sens_readByte();
}

/*!
 * Initialisiere Maussensor
 */
void mouse_sens_init(void) {
	MOUSE_Enable();
	delay(100);

	MOUSE_DDR |= MOUSE_SCK_PIN; // SCK auf Output
	MOUSE_PORT = (uint8_t) (MOUSE_PORT & ~MOUSE_SCK_PIN); // SCK auf 0

	delay(10);

	mouse_sens_write(MOUSE_CONFIG_REG, MOUSE_CFG_RESET); // reset sensor
	mouse_sens_write(MOUSE_CONFIG_REG, MOUSE_CFG_FORCEAWAKE); // always on
}

#ifdef BOT_2_SIM_AVAILABLE
/*!
 * Uebertraegt ein Bild vom Maussensor an den PC
 * Insgesamt gibt es 324 Pixel
 * <pre>
 * 18 36 ... 324
 * .. .. ... ..
 *  2 20 ... ..
 *  1 19 ... 307
 * </pre>
 * Gesendet weren: Pixeldaten (Bit 0 bis Bit 5), Pruefbit, ob Daten gueltig (Bit 6), Markierung fuer den Anfang eines Frames (Bit 7)
 */
void mouse_transmit_picture(void) {
	int16_t dummy = 1;
	uint8_t data, i, pixel;
	mouse_sens_write(MOUSE_PIXEL_DATA_REG, 0x00); // Frame grabben anstossen

	for (i = 0; i < 6; i++, dummy += 54) {
		command_write(CMD_SENS_MOUSE_PICTURE, SUB_CMD_NORM, dummy, 0, 54);
		for (pixel = 0; pixel < 54; pixel++) {
			do {
				data = mouse_sens_read(MOUSE_PIXEL_DATA_REG);
			} while ((data & 0x40) != 0x40);
			low_write_data(&data, 1);
		}
	}
}
#endif // BOT_2_SIM_AVAILABLE

/*!
 * Gibt den SQUAL-Wert zurueck. Dieser gibt an, wieviele Merkmale der Sensor
 * im aktuell aufgenommenen Bild des Untergrunds wahrnimmt
 */
uint8_t mouse_get_squal(void) {
	return mouse_sens_read(MOUSE_SQUAL_REG);
}

#endif // MOUSE_AVAILABLE
#endif // MCU
