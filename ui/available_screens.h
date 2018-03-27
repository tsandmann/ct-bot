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
 * \file 	available_screens.h
 * \brief 	Die Schalter fuer sichtbare Screens finden sich hier
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	12.02.2007
 */

#ifndef AVAILABLE_SCREENS_H_
#define AVAILABLE_SCREENS_H_

#ifdef DISPLAY_AVAILABLE

#define DISPLAY_SCREENS 22 /**< max. Anzahl an Screens */

#define DISPLAY_SENSOR_AVAILABLE			/**< zeigt die Sensordaten an */
#define DISPLAY_REMOTECALL_AVAILABLE		/**< Steuerung der Verhalten inkl. Parametereingabe */
#define DISPLAY_ODOMETRIC_INFO				/**< zeigt Positions- und Geschwindigkeitsdaten an */
//#define DISPLAY_REGELUNG_AVAILABLE		/**< Gibt Debug-Infos der Motorregelung aus */
/** TODO: BUG: aktiviertes #define DISPLAY_MISC_AVAILABLE erzeugt build-fehler
 * "/home/travis/build/$USER/ct-bot/tests/avr8-gnu-toolchain-linux_x86_64/bin/../lib/gcc/avr/4.9.2/../../../../avr/bin/ld: ct-Bot.elf section `.data' will not fit in region `text'
 * /home/travis/build/$USER/ct-bot/tests/avr8-gnu-toolchain-linux_x86_64/bin/../lib/gcc/avr/4.9.2/../../../../avr/bin/ld: region `text' overflowed by 266 bytes -- collect2: error: ld returned 1 exit status -- make: *** [ct-Bot.elf] Error 1
 * TEST /home/travis/build/$USER/ct-bot/tests/mcu/20_all_behaviours_1_mcu.h FOR MCU FAILED."
 */
//#define DISPLAY_MISC_AVAILABLE			/**< aehm ja, der Rest irgendwie... */
#define DISPLAY_MMC_INFO					/**< Zeigt die Daten der MMC-Karte an; funktioniert nicht, wenn ct-Bot.h/MAP_AVAILABLE aktiviert ist */
//#define DISPLAY_RESET_INFO_AVAILABLE		/**< Zeigt Informationen ueber Resets an */
#define DISPLAY_NEURALNET_AVAILABLE		    /**< Screen des neuronalen Netzes */
#define DISPLAY_DRIVE_NEURALNET_AVAILABLE	/**< Screen des Fahrverhaltens des neuronalen Netzes */
#define DISPLAY_OS_AVAILABLE				/**< Zeigt die CPU-Auslastung an und bietet Debugging-Funktionen */
#define DISPLAY_RAM_AVAILABLE				/**< Ausgabe des freien RAMs */
#define DISPLAY_MAP_AVAILABLE				/**< Zeigt Map-Display an */
#define DISPLAY_TRANSPORT_PILLAR        	/**< Steuerung Transport-Pillar-Verhalten auf diesem Screen */
#define DISPLAY_DRIVE_STACK_AVAILABLE		/**< Steuerung Stack-Verhalten auf diesem Screen */
#define DISPLAY_PATHPLANING_AVAILABLE		/**< Display zur Pfadplanung */
#define DISPLAY_LINE_SHORTEST_WAY_AVAILABLE	/**< Steuerung des Verhaltens auf diesem Screen */
#define DISPLAY_DRIVE_CHESS_AVAILABLE		/**< Steuerung des Schach-Verhaltens */
#define DISPLAY_UBASIC_AVAILABLE	    	/**< Steuerung des uBasic-Verhaltens */
#define DISPLAY_ABL_STACK_AVAILABLE			/**< Stack Trace des ABL-Interpreters */
#define DISPLAY_LINUX_AVAILABLLE			/**< zeigt den vom Linux-Board uebertragenen Displayinhalt an */
#define DISPLAY_ATMEGA_AVAILABLLE			/**< zeigt den vom ATmega uebertragenen Displayinhalt an */

#endif // DISPLAY_AVAILABLE
#endif // AVAILABLE_SCREENS_H_
