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
 * \file 	init.h
 * \brief 	Initialisierungsroutinen
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	09.03.2010
 */

#ifndef INIT_H_
#define INIT_H_

#include "ct-Bot.h"
#include "eeprom.h"
#include "bot-logic.h"
#include "sensor-low.h"
#include "sdfat_fs.h"

extern uint8_t EEPROM resetsEEPROM;	/**< Reset-Counter im EEPROM */

#define GET_MMC_BUFFER(name)	mmc_buffers.data.name /**< Zugriff auf einzelne MMC-Puffer */

typedef union {
	struct {
#ifdef MAP_AVAILABLE
		uint8_t map_buffer[512]; /**< Map-Puffer */
#else
		uint8_t map_buffer[0]; /**< Map-Puffer inaktiv */
#endif // MAP_AVAILABLE
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
		uint8_t ubasic_buffer[SD_BLOCK_SIZE]; /**< uBasic-MMC-Puffer */
#else
		uint8_t ubasic_buffer[0]; /**< uBasic-MMC-Puffer inaktiv */
#endif
#ifdef BEHAVIOUR_ABL_AVAILABLE
		uint8_t abl_buffer[SD_BLOCK_SIZE]; /**< ABL-MMC-Puffer */
#else
		uint8_t abl_buffer[0]; /**< ABL-MMC-Puffer inaktiv */
#endif
		uint8_t end[0];
	} PACKED data;
	uint8_t dummy[0];
} mmc_buffers_t; /**< Puffer-Struktur fuer alle MMC-Puffer */

extern mmc_buffers_t mmc_buffers; /**< Puffer fuer alle MMC-Transfers */

#ifdef MCU
/** Kopie des MCU(C)SR Registers */
extern uint8_t mcucsr __attribute__((section(".noinit")));

int main(int argc, char * argv[]) __attribute__((OS_main)); // kein Caller, Interrupts disabled	// explizit ** int **

extern uint8_t soft_resets __attribute__((section(".noinit"))); /**< Reset-Counter im RAM */
#endif // MCU

/**
 * Initialisierung
 * \param argc Anzahl der Kommandozeilenparameter
 * \param *argv Zeiger auf Kommandozeilenparameter
 */
void ctbot_init(int argc, char * argv[]);	// explizit ** int **

/**
 * Hardwareabhaengige Initialisierungen, die zuerst ausgefuehrt werden sollen
 * \param argc Anzahl der Kommandozeilenparameter
 * \param *argv Zeiger auf Kommandozeilenparameter
 */
void ctbot_init_low_1st(int argc, char * argv[]);	// explizit ** int **

/**
 * Hardwareabhaengige Initialisierungen, die _nach_ der allgemeinen Initialisierung
 * ausgefuehrt werden sollen
 */
void ctbot_init_low_last(void);

/**
 * Faehrt den low-level Code des Bots sauber herunter
 */
void ctbot_shutdown_low(void);
#endif // INIT_H_
