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

/**
 * \file 	mouse.h
 * \brief 	Routinen fuer die Ansteuerung eines optischen Maussensors
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#ifndef MOUSE_H_
#define MOUSE_H_

#define ADNS2610 /**< Welcher Sensortyp? ADNS2610 oder 2620 */

#ifdef ADNS2610
/* ADNS2610 */
#define MOUSE_CONFIG_REG				0x00 /**< Reset, Power Down, Forced Awake, etc */
#define MOUSE_STATUS_REG				0x01 /**< Product ID, Mouse state of Asleep or Awake */
#define MOUSE_DELTA_Y_REG				0x02 /**< Y Movement */
#define MOUSE_DELTA_X_REG				0x03 /**< X Movement */
#define MOUSE_SQUAL_REG		 			0x04 /**< Measure of the number of features visible by the sensor */
#define MOUSE_MAX_PIXEL_REG				0x05 /**< Maximum Pixel value in current frame */
#define MOUSE_MIN_PIXEL_REG				0x06 /**< Minimum Pixel value in current frame */
#define MOUSE_PIXEL_SUM_REG				0x07 /**< This register is used to find the average pixel value */
#define MOUSE_PIXEL_DATA_REG			0x08 /**< Actual picture of surface */
#define MOUSE_SHUTTER_UPPER_REG			0x09 /**< The sensor adjusts the shutter to keep the average and maximum pixel values within normal operating ranges */
#define MOUSE_SHUTTER_LOWER_REG			0x0A /**< The sensor adjusts the shutter to keep the average and maximum pixel values within normal operating ranges */
#define MOUSE_INVERSE_PRODUCT_ID_REG	0x11 /**< Inverse Product ID */

#define MOUSE_CFG_RESET				0x80 /**< Reset Circuit */
#define MOUSE_CFG_POWERDOWN			0x40 /**< Power Down analog Circuit */
#define MOUSE_CFG_FORCEAWAKE		0x01 /**< Keep Sensor awake */

#else
/* ADNS2620 */
#define MOUSE_CONFIG_REG			0x40 /**< Reset, Power Down, Forced Awake, etc */
#define MOUSE_STATUS_REG			0x41 /**< Product ID, Mouse state of Asleep or Awake */
#define MOUSE_DELTA_Y_REG			0x42 /**< Y Movement */
#define MOUSE_DELTA_X_REG			0x43 /**< X Movement */
#define MOUSE_SQUAL_REG		 		0x44 /**< Measure of the number of features visible by the sensor */
#define MOUSE_MAX_PIXEL_REG			0x45 /**< Maximum Pixel value in current frame */
#define MOUSE_MIN_PIXEL_REG			0x46 /**< Minimum Pixel value in current frame */
#define MOUSE_PIXEL_SUM_REG			0x47 /**< This register is used to find the average pixel value */
#define MOUSE_PIXEL_DATA_REG		0x48 /**< Actual picture of surface */
#define MOUSE_SHUTTER_UPPER_REG		0x49 /**< The sensor adjusts the shutter to keep the average and maximum pixel values within normal operating ranges */
#define MOUSE_SHUTTER_LOWER_REG		0x4A /**< The sensor adjusts the shutter to keep the average and maximum pixel values within normal operating ranges */
#define MOUSE_FRAME_PERIOD_REG		0x4B /**< The frame period counter counts up until it overflows */
#endif // ADNS2610

/**
 * Initialisiert den Maussensor
 */
void mouse_sens_init(void);

/**
 * Schickt ein Lesekommando an den Sensor und liest ein Byte zurueck
 * \param addr die Adresse
 * \return das Datum
 */
uint8_t mouse_sens_read(uint8_t addr);

/** Stellt sicher, dass der Maussensor nicht mehr die serielle Datenleitung treibt */
#define mouse_sens_highZ() mouse_sens_write(0x77, 0x00)

/**
 * Uebertraegt ein write-Kommando an den Sensor
 * \param addr Adresse
 * \param data Datum
 */
void mouse_sens_write(uint8_t addr, uint8_t data);

#if defined BOT_2_SIM_AVAILABLE
/**
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
void mouse_transmit_picture(void);
#endif // BOT_2_SIM_AVAILABLE

/**
 * Gibt den SQUAL-Wert zurueck. Dieser gibt an, wieviele Merkmale der Sensor
 * im aktuell aufgenommenen Bild des Untergrunds wahrnimmt
 * \return SQUAL Wert
 */
uint8_t mouse_get_squal(void);

#endif // MOUSE_H_
