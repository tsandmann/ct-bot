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
 * @file 	init.h
 * @brief 	Initialisierungsroutinen
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	09.03.2010
 */

#ifndef INIT_H_
#define INIT_H_

#include "ct-Bot.h"
#include "eeprom.h"

extern uint8_t EEPROM resetsEEPROM;	/*!< Reset-Counter im EEPROM */

#ifdef MCU
/*! Kopie des MCU(C)SR Registers */
extern uint8_t mcucsr __attribute__((section(".noinit")));

int main(int argc, char * argv[]) __attribute__((OS_main)); // kein Caller, Interrupts disabled

#ifdef RESET_INFO_DISPLAY_AVAILABLE
extern uint8_t soft_resets __attribute__((section(".noinit"))); /*!< Reset-Counter im RAM */
#endif // RESET_INFO_DISPLAY_AVAILABLE
#endif // MCU

/*!
 * Initialisierung
 * @param argc Anzahl der Kommandozeilenparameter
 * @param *argv Zeiger auf Kommandozeilenparameter
 */
void ctbot_init(int argc, char * argv[]);

/*!
 * Hardwareabhaengige Initialisierungen, die zuerst ausgefuehrt werden sollen
 * @param argc Anzahl der Kommandozeilenparameter
 * @param *argv Zeiger auf Kommandozeilenparameter
 */
void ctbot_init_low_1st(int argc, char * argv[]);

/*!
 * Hardwareabhaengige Initialisierungen, die _nach_ der allgemeinen Initialisierung
 * ausgefuehrt werden sollen
 */
void ctbot_init_low_last(void);
#endif /* INIT_H_ */
