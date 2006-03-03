/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	ct-Bot.h
 * @brief 	Demo-Hauptprogramm
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include "global.h"

/************************************************************
* Module switches, to make code smaller if features are not needed
************************************************************/

#define LED_AVAILABLE		/*!< LEDs for local control */

#define IR_AVAILABLE		/*!< Infrared Remote Control */
#define RC5_AVAILABLE		/*!< Key-Mapping for IR-RC	 */

//#define BOT_2_PC_AVAILABLE	/*!< Soll der Bot mit dem PC kommunmizieren? */

#define TIME_AVAILABLE		/*!< Gibt es eine Systemzeit? */

//#define DISPLAY_AVAILABLE	/*!< Display for local control */
//#define DISPLAY_REMOTE_AVAILABLE /*!< Sende LCD Anzeigedaten an den Simulator */
#define DISPLAY_SCREENS_AVAILABLE	/*!< Ermoeglicht vier verschiedene Screen */
#define DISPLAY_SCREEN_RESETINFO	/*!< Zeigt auf Screen 4 Informationen über Resets an */

//#define WELCOME_AVAILABLE	/*!< kleiner Willkommensgruss */

#define ADC_AVAILABLE		/*!< A/D-Converter for sensing Power */

#define MAUS_AVAILABLE		/*!< Maus Sensor */

#define ENA_AVAILABLE		/*!< Enable-Leitungen */
#define SHIFT_AVAILABLE		/*!< Shift Register */

//#define TEST_AVAILABLE_ANALOG	/*!< Sollen die LEDs die analoge Sensorwerte anzeigen */
#define TEST_AVAILABLE_DIGITAL	/*!< Sollen die LEDs die digitale Sensorwerte anzeigen */
//#define TEST_AVAILABLE_MOTOR	/*!< Sollen die Motoren ein wenig drehen */
//#define TEST_AVAILABLE_COUNTER /*!< Gibt einen Endlos-Counter auf Screen 3 aus und aktiviert Screen 3 */

//#define DOXYGEN		/*!< Nur zum Erzeugen der Doku, wenn dieser schalter an ist, jammert der gcc!!! */

#define BEHAVIOUR_AVAILABLE /*!< Nur wenn dieser Parameter gesetzt ist, exisitiert das Verhaltenssystem */

/************************************************************
* Some Dependencies!!!
************************************************************/

#ifdef DOXYGEN
	#define PC			/*!< Beim generieren der Doku alles anschalten */
	#define MCU		/*!< Beim generieren der Doku alles anschalten */
	#define TEST_AVAILABLE_MOTOR	/*!< Beim generieren der Doku alles anschalten */
#endif


#ifndef DISPLAY_AVAILABLE
	#undef WELCOME_AVAILABLE
   #undef DISPLAY_REMOTE_AVAILABLE
   #undef DISPLAY_SCREENS_AVAILABLE
   #undef DISPLAY_SCREEN_RESETINFO
#endif

#ifndef IR_AVAILABLE
	#undef RC5_AVAILABLE
#endif

#ifdef PC
	#ifndef DOXYGEN
		#undef UART_AVAILABLE
		#undef MAUS_AVAILABLE
		#undef BOT_2_PC_AVAILABLE
	#endif
	#define COMMAND_AVAILABLE		/*!< High-Level Communication */
   #undef DISPLAY_SCREEN_RESETINFO
   #undef TEST_AVAILABLE_COUNTER
#endif

#ifdef MCU
	#ifdef BOT_2_PC_AVAILABLE
		#define UART_AVAILABLE	/*!< Serial Communication */
		#define COMMAND_AVAILABLE	/*!< High-Level Communication */
	#endif
#endif


#ifdef TEST_AVAILABLE_MOTOR
	#define TEST_AVAILABLE			/*!< brauchen wir den Testkrams */
	#define TEST_AVAILABLE_DIGITAL /*!< Sollen die LEDs die digitale Sensorwerte anzeigen */
#endif

#ifdef TEST_AVAILABLE_DIGITAL
	#define TEST_AVAILABLE			/*!< brauchen wir den Testkrams */
	#undef TEST_AVAILABLE_ANALOG
#endif

#ifdef TEST_AVAILABLE_ANALOG
	#define TEST_AVAILABLE			/*!< brauchen wir den Testkrams */
#endif

#ifdef TEST_AVAILABLE_COUNTER
	#define TEST_AVAILABLE			/*!< brauchen wir den Testkrams */
	#define DISPLAY_SCREENS_AVAILABLE
	#define DISPLAY_SCREEN_RESETINFO
#endif

#define F_CPU	16000000L    /*!< Crystal frequency in Hz */
#define XTAL F_CPU			 /*!< Crystal frequency in Hz */

