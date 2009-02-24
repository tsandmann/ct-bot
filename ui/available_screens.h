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
 * @file 	available_screens.h
 * @brief 	Die Schalter fuer sichtbare Screens finden sich hier
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007
 */

#ifndef AVAILABLE_SCREENS_H_
#define AVAILABLE_SCREENS_H_

#include "ct-Bot.h"

#ifdef DISPLAY_AVAILABLE

#define DISPLAY_SCREENS 10				/*!< Anzahl der Screens */

#define SENSOR_DISPLAY_AVAILABLE		/*!< zeigt die Sensordaten an */
//#define DISPLAY_REGELUNG_AVAILABLE		/*!< Gibt Debug-Infos der Motorregelung aus */
//#define DISPLAY_BEHAVIOUR_AVAILABLE		/*!< zeigt Verhalten an */
//#define MISC_DISPLAY_AVAILABLE			/*!< aehm ja, der Rest irgendwie... */
#define DISPLAY_ODOMETRIC_INFO			/*!< zeigt Positions- und Geschwindigkeitsdaten an */
//#define DISPLAY_MMC_INFO				/*!< Zeigt die Daten der MMC-Karte an */
//#define DISPLAY_MINIFAT_INFO			/*!< Zeigt Ausgaben des MiniFAT-Treibers an */
//#define RESET_INFO_DISPLAY_AVAILABLE	/*!< Zeigt Informationen ueber Resets an */
#define DISPLAY_OS_AVAILABLE			/*!< Zeigt die CPU-Auslastung an und bietet Debugging-Funktionen */
#define RAM_DISPLAY_AVAILABLE			/*!< Ausgabe des freien RAMs */
#define DISPLAY_MAP_AVAILABLE			/*!< Zeigt Map-Display an */
#define DISPLAY_TRANSPORT_PILLAR        /*!< Steuerung Transport-Pillar-Verhalten auf diesem Screen */
#define DISPLAY_DRIVE_STACK_AVAILABLE	/*!< Steuerung Stack-Verhalten auf diesem Screen */
#define PATHPLANING_DISPLAY				/*!< Display zur Pfadplanung */
#define DISPLAY_LINE_SHORTEST_WAY_AVAILABLE	/*!< Steuerung des Verhaltens auf diesem Screen */

#ifndef SPEED_CONTROL_AVAILABLE
	#undef DISPLAY_REGELUNG_AVAILABLE
#endif
#ifndef MCU
	#undef RESET_INFO_DISPLAY_AVAILABLE
	#undef RAM_DISPLAY_AVAILABLE
	#undef DISPLAY_OS_AVAILABLE
#endif
#ifndef MMC_AVAILABLE
	#undef DISPLAY_MMC_INFO
	#undef DISPLAY_MINIFAT_INFO
	#ifdef PC
		#ifndef MMC_VM_AVAILABLE
			#undef DISPLAY_MINIFAT_INFO
		#endif
	#endif	// PC
#endif	// MMC_AVAILABLE
#ifndef MAP_AVAILABLE
	#undef DISPLAY_MAP_AVAILABLE
#else
	// MMC-Zugriff geht nur, wenn gerade kein Map-Update laueft
	#undef DISPLAY_MMC_INFO
#endif
#ifndef BEHAVIOUR_TRANSPORT_PILLAR_AVAILABLE
	#undef DISPLAY_TRANSPORT_PILLAR
#endif
#ifndef BEHAVIOUR_DRIVE_STACK_AVAILABLE
	#undef DISPLAY_DRIVE_STACK_AVAILABLE
#endif
#ifndef OS_AVAILABLE
	#undef DISPLAY_OS_AVAILABLE
#endif

#endif	// DISPLAY_AVAILABLE
#endif	// AVAILABLE_SCREENS_H_
