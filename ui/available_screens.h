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

#ifndef available_screens_H_
#define available_screens_H_

#include "ct-Bot.h"

#ifdef DISPLAY_AVAILABLE

#define DISPLAY_SCREENS 8				/*!< Anzahl der Screens */

#define SENSOR_DISPLAY_AVAILABLE		/*!< zeigt die Sensordaten an */
//#define DISPLAY_REGELUNG_AVAILABLE		/*!< Gibt Debug-Infos der Motorregelung aus */
//#define DISPLAY_BEHAVIOUR_AVAILABLE		/*!< zeigt Verhalten an */
//#define MISC_DISPLAY_AVAILABLE			/*!< aehm ja, der Rest irgendwie... */
#define DISPLAY_ODOMETRIC_INFO			/*!< zeigt Positions- und Geschwindigkeitsdaten an */
#define DISPLAY_MMC_INFO				/*!< Zeigt die Daten der MMC-Karte an */
//#define DISPLAY_MINIFAT_INFO			/*!< Zeigt Ausgaben des MiniFAT-Treibers an */
//#define RESET_INFO_DISPLAY_AVAILABLE	/*!< Zeigt Informationen ueber Resets an */
#define RAM_DISPLAY_AVAILABLE			/*!< Ausgabe des freien RAMs */
#define DISPLAY_MAP_GO_DESTINATION      /*!< Steuerung Map-Verhalten auf diesem Screen */
#define DISPLAY_MAP_AVAILABLE			/*!< Zeigt Map-Display an */

#ifndef SPEED_CONTROL_AVAILABLE
	#undef DISPLAY_REGELUNG_AVAILABLE
#endif
#ifndef MCU
	#undef RESET_INFO_DISPLAY_AVAILABLE
	#undef RAM_DISPLAY_AVAILABLE
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
#endif


#endif	// DISPLAY_AVAILABLE
#endif	// available_screens_H_
