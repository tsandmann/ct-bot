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
 * @file 	ir-rc5.h
 * @brief 	Routinen fuer die Dekodierung von RC5-Fernbedienungs-Codes
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
 */
#ifndef ir_rc5_H_
#define ir_rc5_H_

#include "ct-Bot.h"
#include "global.h"

#ifdef MCU
#define RC5_PORT	PORTB /*!< Port B fuer RC5-Fernbedienung */
#define BPS_PORT	PORTA /*!< Port A fuer BPS-Sensor */
#define RC5_DDR		DDRB /*!< DDR fuer RC5-Fernbedienung */
#define BPS_DDR		DDRA /*!< DDR fuer BPS-Sensor */
#define RC5_PINR	PINB /*!< Port B input fuer RC5-Fernbedienung */
#define BPS_PINR	PINA /*!< Port A input fuer BPS-Sensor */
#define RC5_PIN		1 /*!< Pin 1 fuer RC5-Fernbedienung */
#define BPS_PIN		PA4 /*!< Pin 4 fuer BPS-Sensor */
#else
#define RC5_PORT	(*(volatile uint8_t *)NULL) /*!< Port B fuer RC5-Fernbedienung */
#define BPS_PORT	(*(volatile uint8_t *)NULL) /*!< Port A fuer BPS-Sensor */
#define RC5_DDR		(*(volatile uint8_t *)NULL) /*!< DDR fuer RC5-Fernbedienung */
#define BPS_DDR		(*(volatile uint8_t *)NULL) /*!< DDR fuer BPS-Sensor */
#define RC5_PINR	0 /*!< Port B input fuer RC5-Fernbedienung */
#define BPS_PINR	0 /*!< Port A input fuer BPS-Sensor */
#define RC5_PIN		0 /*!< Pin 1 fuer RC5-Fernbedienung */
#define BPS_PIN		0 /*!< Pin 4 fuer BPS-Sensor */
#endif // MCU

#define RC5_PAUSE_SAMPLES	250	/*!< Startbit ist erst nach 250 Samples ohne Pegelaenderung gueltig -- eigentlich muesste
								 * man rund 500 Samples abwarten (50 x Bitzeit), doch weil der Samplezaehler ein
								 * Byte ist, beschraenken wir uns hier auf ein Minimum von 250 Samples */
#define RC5_SAMPLES_PER_BIT 10	/*!< 10 Samples per Bit */
#define RC5_BITS			14	/*!< Anzahl der Bits, die empfangen werden sollen (inkl. Startbit) */

#define BPS_PAUSE_SAMPLES	20	/*!< Startbit ist erst nach 20 Samples ohne Pegelaenderung gueltig */
#define BPS_SAMPLES_PER_BIT 6	/*!< 6 Samples per Bit */
#define BPS_BITS			5	/*!< Anzahl der Bits, die empfangen werden sollen (inkl. Startbit) */

typedef struct {
	uint8_t ir_lastsample;		/*!< zuletzt gelesenes Sample */
	uint8_t ir_bittimer;		/*!< zaehlt die Aufrufe von ir_isr() */
	uint16_t ir_data_tmp;		/*!< RC5-Bitstream */
	uint8_t ir_bitcount;		/*!< Anzahl gelesener Bits */
	volatile uint16_t ir_data;	/*!< letztes komplett gelesenes RC5-Paket */
	uint16_t no_data;			/*!< RC5-Code, der gesetzt wird, falls nichts empfangen wurde */
} ir_data_t;	/*!< Daten fuer RC-Decoder */

/*! @todo Das gehoert eigentlich nicht hierhin */
#ifdef RC5_AVAILABLE
extern ir_data_t rc5_ir_data;
#endif
#ifdef BPS_AVAILABLE
extern ir_data_t bps_ir_data;
#endif

/*!
 * Init IR-System
 * @param *port Port fuer RC5-Sensor
 * @param *ddr DDR-Register fuer RC5-Sensor
 * @param pin Pin fuer RC5-Sensor
 */
static inline void ir_init(volatile uint8_t * port, volatile uint8_t * ddr, uint8_t pin) {
#ifdef MCU
	*ddr = (uint8_t)(*ddr & ~pin); // Pin auf Input
	*port |= pin; // Pullup an
#else // PC
	/* keine warnings */
	port = port;
	ddr = ddr;
	pin = pin;
#endif // MCU
}

/*!
 * IR-Daten lesen
 * @param *data Zeiger auf Arbeitsdaten
 * @return Wert von ir_data, loescht anschliessend ir_data
 */
uint16_t ir_read(ir_data_t * data);

/*!
 * Interrupt Serviceroutine,
 * wird alle 176 us aufgerufen
 * @param *data Zeiger auf Arbeitsdaten
 * @param pin_r Input-Port
 * @param pin Input-Pin
 * @param pause_samples Anzahl der Samples, bevor ein Startbit erkannt wird
 * @param samples_per_bit Anzahl der Samples / Bit
 * @param bits Anzahl der Bits, die Empfangen werden sollen (inkl. Startbit)
 */
void ir_isr(ir_data_t * data, volatile uint8_t * pin_r, const uint8_t pin, const uint8_t pause_samples,
		const uint8_t samples_per_bit, const uint8_t bits);
#endif	/*ir_rc5_H_*/
