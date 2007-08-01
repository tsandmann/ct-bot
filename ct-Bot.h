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
 * @file 	ct-Bot.h
 * @brief 	globale Schalter fuer die einzelnen Bot-Funktionalitaeten
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */
#ifndef CT_BOT_H_DEF
#define CT_BOT_H_DEF

#include "global.h"

/************************************************************
* Module switches, to make code smaller if features are not needed
************************************************************/
//#define LOG_CTSIM_AVAILABLE		/*!< Logging zum ct-Sim (PC und MCU) */
//#define LOG_DISPLAY_AVAILABLE		/*!< Logging ueber das LCD-Display (PC und MCU) */
//#define LOG_UART_AVAILABLE		/*!< Logging ueber UART (NUR fuer MCU) */
//#define LOG_STDOUT_AVAILABLE 		/*!< Logging auf die Konsole (NUR fuer PC) */
//#define LOG_MMC_AVAILABLE			/*!< Logging in eine txt-Datei auf MMC */			


#define LED_AVAILABLE		/*!< LEDs for local control */

#define IR_AVAILABLE		/*!< Infrared Remote Control */
#define RC5_AVAILABLE		/*!< Key-Mapping for IR-RC	 */

#define BOT_2_PC_AVAILABLE	/*!< Soll der Bot mit dem PC kommunmizieren? */

//#define TIME_AVAILABLE		/*!< Gibt es eine Systemzeit im s und ms? */

#define DISPLAY_AVAILABLE	/*!< Display for local control */
#define DISPLAY_REMOTE_AVAILABLE /*!< Sende LCD Anzeigedaten an den Simulator */
#define MEASURE_MOUSE_AVAILABLE			/*!< Geschwindigkeiten werden aus den Maussensordaten berechnet */
//#define MEASURE_COUPLED_AVAILABLE		/*!< Geschwindigkeiten werden aus Maus- und Encoderwerten ermittelt und gekoppelt */


//#define WELCOME_AVAILABLE	/*!< kleiner Willkommensgruss */

#define ADC_AVAILABLE		/*!< A/D-Converter */

#define MAUS_AVAILABLE		/*!< Maus Sensor */

#define ENA_AVAILABLE		/*!< Enable-Leitungen */
#define SHIFT_AVAILABLE		/*!< Shift Register */

//#define TEST_AVAILABLE_ANALOG		/*!< Sollen die LEDs die analoge Sensorwerte anzeigen */
//#define TEST_AVAILABLE_DIGITAL	/*!< Sollen die LEDs die digitale Sensorwerte anzeigen */
//#define TEST_AVAILABLE_MOTOR		/*!< Sollen die Motoren ein wenig drehen */

#define BEHAVIOUR_AVAILABLE /*!< Nur wenn dieser Parameter gesetzt ist, exisitiert das Verhaltenssystem */

//#define MAP_AVAILABLE /*!< Aktiviere die Kartographie */

//#define SPEED_CONTROL_AVAILABLE /*!< Aktiviert die Motorregelung */
//#define ADJUST_PID_PARAMS		/*!< macht PID-Paramter zur Laufzeit per FB einstellbar */
//#define SPEED_LOG_AVAILABLE 	/*!< Zeichnet Debug-Infos der Motorregelung auf MMC auf */

//#define SRF10_AVAILABLE		/*!< Ultraschallsensor SRF10 vorhanden */

//#define MMC_AVAILABLE			/*!< haben wir eine MMC/SD-Karte zur Verfuegung */
//#define SPI_AVAILABLE			/*!< verwendet den Hardware-SPI-Modus des Controllers, um mit der MMC zu kommunizieren - Hinweise in mcu/mmc.c beachten! */
//#define MMC_VM_AVAILABLE		/*!< Virtual Memory Management mit MMC / SD-Card oder PC-Emulation */

// Achtung, Linkereinstellungen anpassen !!!!! (siehe Documentation/Bootloader.html)!
//#define BOOTLOADER_AVAILABLE	/*!< Aktiviert den Bootloadercode - das ist nur noetig fuer die einmalige "Installation" des Bootloaders. Achtung, Linkereinstellungen anpassen (siehe mcu/bootloader.c)! */
/************************************************************
* Some Dependencies!!!
************************************************************/

#ifdef DOXYGEN
	#define PC		/*!< Beim generieren der Doku alles anschalten */
	#define MCU		/*!< Beim generieren der Doku alles anschalten */
	#define TEST_AVAILABLE_MOTOR	/*!< Beim generieren der Doku alles anschalten */
#endif


#ifndef DISPLAY_AVAILABLE
	#undef WELCOME_AVAILABLE
	#undef DISPLAY_REMOTE_AVAILABLE
#endif

#ifndef IR_AVAILABLE
	#undef RC5_AVAILABLE
#endif

#ifndef MAUS_AVAILABLE
	#undef MEASURE_MOUSE_AVAILABLE
	#undef MEASURE_COUPLED_AVAILABLE
#endif

#ifdef PC
	#ifndef DOXYGEN
		#undef UART_AVAILABLE
		#undef BOT_2_PC_AVAILABLE
		#undef SRF10_AVAILABLE
		#undef TWI_AVAILABLE
		#undef SPEED_CONTROL_AVAILABLE // Deaktiviere die Motorregelung 
		#undef MMC_AVAILABLE
	#endif

	#define COMMAND_AVAILABLE		/*!< High-Level Communication */
#endif

#ifdef MCU
	#ifdef LOG_CTSIM_AVAILABLE
		#define BOT_2_PC_AVAILABLE	/*!< Soll der Bot mit dem PC kommunmizieren? */
	#endif
	#ifdef BOT_2_PC_AVAILABLE
		#define UART_AVAILABLE		/*!< Serial Communication */
		#define COMMAND_AVAILABLE	/*!< High-Level Communication */
	#else
		#undef DISPLAY_REMOTE_AVAILABLE
	#endif
#endif


#ifdef TEST_AVAILABLE_MOTOR
	#define TEST_AVAILABLE			/*!< brauchen wir den Testkrams */
	#define TEST_AVAILABLE_DIGITAL	/*!< Sollen die LEDs die digitale Sensorwerte anzeigen */
#endif

#ifdef TEST_AVAILABLE_DIGITAL
	#define TEST_AVAILABLE			/*!< brauchen wir den Testkrams */
	#undef TEST_AVAILABLE_ANALOG
#endif

#ifdef TEST_AVAILABLE_ANALOG
	#define TEST_AVAILABLE			/*!< brauchen wir den Testkrams */
#endif

#ifndef SPEED_CONTROL_AVAILABLE
	#undef ADJUST_PID_PARAMS
	#undef SPEED_LOG_AVAILABLE	
#endif

#ifdef LOG_UART_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif 
#ifdef LOG_CTSIM_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif 
#ifdef LOG_DISPLAY_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif 
#ifdef LOG_STDOUT_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif 
#ifdef LOG_MMC_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif

#ifndef MMC_AVAILABLE
	#undef SPI_AVAILABLE
	#ifdef MCU
		#undef MMC_VM_AVAILABLE
	#endif
#endif

#ifndef MMC_AVAILABLE
	#undef SPEED_LOG_AVAILABLE
#endif

#ifdef LOG_AVAILABLE
	#ifdef PC
		/* Auf dem PC gibts kein Logging ueber UART. */
		#undef LOG_UART_AVAILABLE
	#endif
	
	#ifdef MCU
		/* Mit Bot zu PC Kommunikation auf dem MCU gibts kein Logging ueber UART.
		 * Ohne gibts keine Kommunikation ueber ct-Sim.
		 */
		#undef LOG_STDOUT_AVAILABLE		/*!< MCU hat kein STDOUT */
		#ifdef BOT_2_PC_AVAILABLE
			#undef LOG_UART_AVAILABLE
		#else
			#undef LOG_CTSIM_AVAILABLE
		#endif
	#endif
	
	/* Ohne Display gibts auch keine Ausgaben auf diesem. */
	#ifndef DISPLAY_AVAILABLE
		#undef LOG_DISPLAY_AVAILABLE
	#endif
	
	/* Es kann immer nur ueber eine Schnittstelle geloggt werden. */
	
	#ifdef LOG_UART_AVAILABLE
		#define UART_AVAILABLE			/*!< UART vorhanden? */
		#undef LOG_CTSIM_AVAILABLE
		#undef LOG_DISPLAY_AVAILABLE
		#undef LOG_STDOUT_AVAILABLE
		#undef LOG_MMC_AVAILABLE
	#endif
	
	#ifdef LOG_CTSIM_AVAILABLE
		#undef LOG_DISPLAY_AVAILABLE
		#undef LOG_STDOUT_AVAILABLE
		#undef LOG_MMC_AVAILABLE
	#endif
	
	#ifdef LOG_DISPLAY_AVAILABLE
		#undef LOG_STDOUT_AVAILABLE
		#undef LOG_MMC_AVAILABLE
	#endif
	
	#ifdef LOG_STDOUT_AVAILABLE
		#undef LOG_MMC_AVAILABLE
	#endif
	
	#ifndef MMC_VM_AVAILABLE
		#undef LOG_MMC_AVAILABLE
	#endif 

	// Wenn keine sinnvolle Log-Option mehr uebrig, loggen wir auch nicht
	#ifndef LOG_CTSIM_AVAILABLE
		#ifndef LOG_DISPLAY_AVAILABLE
			#ifndef LOG_UART_AVAILABLE
				#ifndef LOG_STDOUT_AVAILABLE
					#ifndef LOG_MMC_AVAILABLE
						#undef LOG_AVAILABLE
					#endif
				#endif
			#endif
		#endif
	#endif

#endif


#ifdef SRF10_AVAILABLE
	#define TWI_AVAILABLE				/*!< TWI-Schnittstelle (I2C) nutzen */
#endif


#define F_CPU	16000000L    /*!< Crystal frequency in Hz */
#define XTAL F_CPU			 /*!< Crystal frequency in Hz */

#ifdef WIN32
	#define LINE_FEED "\n\r"	/*!< Linefeed fuer Windows */
#else
	#define LINE_FEED "\n"		/*!< Linefeed fuer nicht Windows */
#endif

#ifdef MCU
	#ifndef MMC_LOW_H_
		#include <avr/interrupt.h>
	#endif
	#ifdef SIGNAL
		#define NEW_AVR_LIB	/*!< AVR_LIB-Version */
	#endif
#endif

#endif
