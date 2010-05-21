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
 * @file 	init-low_pc.c
 * @brief 	Initialisierungsroutinen fuer PC
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	09.03.2010
 */

#ifdef PC
#include "init.h"
#include "log.h"
#include "cmd_tools.h"
#include "trace.h"
#include "eeprom.h"
#include <stdio.h>

/*!
 * Hardwareabhaengige Initialisierungen, die zuerst ausgefuehrt werden sollen
 * @param argc Anzahl der Kommandozeilenparameter
 * @param *argv Zeiger auf Kommandozeilenparameter
 */
void ctbot_init_low_1st(int argc, char * argv[]) {
	/* PC-EEPROM-Init vor hand_cmd_args() */
	if (init_eeprom_man(0) != 0) {
		LOG_ERROR("EEPROM-Manager nicht korrekt initialisiert!");
	}

	/* Kommandozeilen-Argumente auswerten */
	hand_cmd_args(argc, argv);

	printf("c't-Bot\n");

#ifdef CREATE_TRACEFILE_AVAILABLE
	trace_init();
#endif // CREATE_TRACEFILE_AVAILABLE
}

/*!
 * Hardwareabhaengige Initialisierungen, die _nach_ der allgemeinen Initialisierung
 * ausgefuehrt werden sollen
 */
void ctbot_init_low_last(void) {
	cmd_init();
}
#endif // PC
