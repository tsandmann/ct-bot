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
 * \file 	ct-Bot.h
 * \brief 	globale Schalter fuer die einzelnen Bot-Funktionalitaeten
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */
#ifndef CT_BOT_H_
#define CT_BOT_H_

/*** Konfigurierbare Optionen
 * Die Optionen lassen sich jeweils per #define ein- oder ausschalten. Das Ausschalten erfolgt durch die Kommentarzeichen am Anfang der Zeile.
 * Ausgeschaltete Optionen belegen keinen Platz im Flash-Speicher des Controllers. ***/

/* Logging-Funktionen */
//#define LOG_CTSIM_AVAILABLE				/**< Logging zum ct-Sim (PC und MCU) */
//#define LOG_DISPLAY_AVAILABLE				/**< Logging ueber das LCD-Display (PC und MCU) */
//#define LOG_UART_AVAILABLE					/**< Logging ueber UART (nur fuer MCU) */
//#define LOG_RPI_AVAILABLE					/**< Logging vom ATmega zum ARM-Linux Board z.B. RPi (nur MCU) */
#define LOG_STDOUT_AVAILABLE 				/**< Logging auf die Konsole (nur fuer PC) */
//#define LOG_MMC_AVAILABLE					/**< Logging in eine txt-Datei auf MMC */
#define USE_MINILOG							/**< schaltet auf schlankes Logging um */
//#define CREATE_TRACEFILE_AVAILABLE			/**< Aktiviert das Schreiben einer Trace-Datei (nur PC) */


/* Kommunikation */
#define BOT_2_SIM_AVAILABLE					/**< Soll der Bot mit dem Sim kommunizieren? */
//#define BOT_2_BOT_AVAILABLE				/**< Sollen Bots untereinander kommunizieren? */
#define BOT_2_BOT_PAYLOAD_AVAILABLE			/**< Aktiviert Payload-Versand per Bot-2-Bot Kommunikation */


/* Display-Funktionen */
#define DISPLAY_AVAILABLE					/**< Display-Funktionen aktiv */
#define KEYPAD_AVAILABLE						/**< Keypad-Eingabe vorhanden? */
#define DISPLAY_MCU_AVAILABLE				/**< lokales Display (an ATmega) vorhanden */
#define DISPLAY_REMOTE_AVAILABLE				/**< Sende LCD Anzeigedaten an den Simulator */
//#define WELCOME_AVAILABLE					/**< kleiner Willkommensgruss */


/* Sensorauswertung */
//#define MOUSE_AVAILABLE					/**< Maus Sensor */
#define MEASURE_MOUSE_AVAILABLE				/**< Geschwindigkeiten werden aus den Maussensordaten berechnet */
//#define MEASURE_COUPLED_AVAILABLE			/**< Geschwindigkeiten werden aus Maus- und Encoderwerten ermittelt und gekoppelt */
//#define MEASURE_POSITION_ERRORS_AVAILABLE	/**< Fehlerberechnungen bei der Positionsbestimmung */
//#define BPS_AVAILABLE						/**< Bot Positioning System */
//#define SRF10_AVAILABLE					/**< Ultraschallsensor SRF10 vorhanden */
//#define CMPS03_AVAILABLE					/**< Kompass CMPS03 vorhanden */


/* Motoransteuerung */
#define SPEED_CONTROL_AVAILABLE 				/**< Aktiviert die Motorregelung */
//#define ADJUST_PID_PARAMS					/**< macht PID-Paramter zur Laufzeit per FB einstellbar */
//#define SPEED_LOG_AVAILABLE 				/**< Zeichnet Debug-Infos der Motorregelung auf MMC auf */


/* Umgebungskarte */
#define MAP_AVAILABLE						/**< Aktiviert die Kartographie; wenn aktiviert, funktioniert ui/available-screens.h/DISPLAY_MMC_INFO nicht */
#define MAP_2_SIM_AVAILABLE					/**< Sendet die Map zur Anzeige an den Sim */


/* MMC-/SD-Karte als Speichererweiterung (opt. Erweiterungsmodul) */
//#define MMC_AVAILABLE						/**< Aktiviert Unterstuetzung von MMC/SD-Karten im Erweiterungsmodul */
#define SDFAT_AVAILABLE						/**< Unterstuetzung fuer FAT-Dateisystem (FAT16 und FAT32) auf MMC/SD-Karte */


/* Hardware-Treiber */
#define ADC_AVAILABLE						/**< A/D-Konverter */
#define SHIFT_AVAILABLE						/**< Shift Register */
#define ENA_AVAILABLE						/**< Enable-Leitungen */
#define LED_AVAILABLE						/**< LEDs aktiv */
#define IR_AVAILABLE							/**< Infrarot Fernbedienung aktiv */
#define RC5_AVAILABLE						/**< Key-Mapping fuer IR-RC aktiv */
//#define SP03_AVAILABLE						/**< Sprachmodul SP03 vorhanden */


/* Sonstiges */
#define BEHAVIOUR_AVAILABLE					/**< Nur wenn dieser Parameter gesetzt ist, exisitiert das Verhaltenssystem */
#define POS_STORE_AVAILABLE					/**< Positionsspeicher vorhanden */
#define OS_AVAILABLE							/**< Aktiviert BotOS fuer Threads und Scheduling */
//#define BOOTLOADER_AVAILABLE				/**< Aktiviert den Bootloadercode - das ist nur noetig fuer die einmalige "Installation" des Bootloaders */
#define ARM_LINUX_BOARD						/**< Code fuer ARM-Linux Board aktivieren, wenn ein ARM-Linux-* Target ausgewaehlt wurde. Fuehrt den high-level Code und die Verhalten aus */
//#define BOT_2_RPI_AVAILABLE				/**< Kommunikation von ATmega mit einem Linux-Board (z.B. Rapsberry Pi) aktivieren. Fuehrt auf dem ATmega den low-level Code aus */



/*** Abhaengigkeiten ***/

#include "global.h" // ct-Bot Datentypen
#include "bot-local.h" // Hardwarekonfigurationen, die bei den Bots verschieden sein koennen


#if ! (defined PC && defined ARM_LINUX_BOARD && defined __arm__ && defined __gnu_linux__)
#undef ARM_LINUX_BOARD
#endif

#ifndef IR_AVAILABLE
#undef RC5_AVAILABLE
#endif

#ifndef RC5_AVAILABLE
#undef KEYPAD_AVAILABLE
#endif

#include "rc5-codes.h"

#if ! defined RC5_CODE_DOT || ! defined RC5_CODE_STOP || ! defined RC5_CODE_PLAY
#undef KEYPAD_AVAILABLE
#endif

#ifdef PC
#undef EXPANSION_BOARD_MOD_AVAILABLE
#endif

#ifdef EXPANSION_BOARD_MOD_AVAILABLE // Anpassungen fuer modifiziertes Erweiterungsboard
#undef EXPANSION_BOARD_AVAILABLE	// deaktiviert Erweiterungsboard (gem. Bausatz)
#undef ENABLE_RX0_PULLUP // Verwendung von Pull-down fuer RX0, also Kurzschluss verhindern
#undef MOUSE_AVAILABLE // deaktiviert Maus-Sensor wegen Nutzung der ATMega SPI-Schnittstelle fuer den SD-Schacht
#define SPI_AVAILABLE // Hardware-SPI-Modus des Controllers für die Anbindung des SD-Schachts.
#endif // EXPANSION_BOARD_AVAILABLE

#ifdef EXPANSION_BOARD_AVAILABLE
#undef ENABLE_RX0_PULLUP // Erweiterungsboard verwendet pull-down fuer RX0, also Kurzschluss verhindern
#endif

#ifndef MOUSE_AVAILABLE
#undef MEASURE_MOUSE_AVAILABLE
#undef MEASURE_COUPLED_AVAILABLE
#endif

#ifdef BOT_2_BOT_AVAILABLE
#define BOT_2_SIM_AVAILABLE
#endif

#ifdef PC
#ifndef DOXYGEN
/* Folgende Optionen deaktivieren, es gibt sie nicht fuer PC */
#undef UART_AVAILABLE
#undef SHIFT_AVAILABLE
#undef SRF10_AVAILABLE
#undef TWI_AVAILABLE
#undef SPEED_CONTROL_AVAILABLE
#undef SPEED_LOG_AVAILABLE
#undef MMC_AVAILABLE
#undef I2C_AVAILABLE
#undef CMPS03_AVAILABLE
#undef SP03_AVAILABLE
#undef BOT_2_RPI_AVAILABLE
#undef BOOTLOADER_AVAILABLE
#endif // ! DOXYGEN

#if !defined BOT_2_SIM_AVAILABLE && ! defined ARM_LINUX_BOARD
#define BOT_2_SIM_AVAILABLE // simulierte Bots brauchen immer Kommunikation zum Sim
#endif

#define COMMAND_AVAILABLE /**< High-Level Kommunikation */
#endif // PC

#ifdef MCU
#ifdef LOG_CTSIM_AVAILABLE
#define BOT_2_SIM_AVAILABLE
#endif

#if defined MCU && defined BOT_2_RPI_AVAILABLE
#define UART_AVAILABLE
#define COMMAND_AVAILABLE

#undef BOT_2_BOT_AVAILABLE
#undef BOT_2_SIM_AVAILABLE
#undef LOG_UART_AVAILABLE
#undef BEHAVIOUR_AVAILABLE
#undef POS_STORE_AVAILABLE
#undef MAP_AVAILABLE
#ifdef DISPLAY_MCU_AVAILABLE
#undef DISPLAY_REMOTE_AVAILABLE
#endif
#endif // MCU && BOT_2_RPI_AVAILABLE

#ifdef BOT_2_SIM_AVAILABLE
#define UART_AVAILABLE		/**< Serielle Kommunikation */
#define COMMAND_AVAILABLE	/**< High-Level Kommunikation */
#else // ! BOT_2_SIM_AVAILABLE
#ifndef BOT_2_RPI_AVAILABLE
#undef DISPLAY_REMOTE_AVAILABLE
#endif
#undef MAP_2_SIM_AVAILABLE
#endif // BOT_2_SIM_AVAILABLE

#if ! (defined DISPLAY_MCU_AVAILABLE || defined DISPLAY_REMOTE_AVAILABLE || defined ARM_LINUX_DISPLAY)
#undef DISPLAY_AVAILABLE
#endif

#ifndef DISPLAY_AVAILABLE
#undef WELCOME_AVAILABLE
#undef DISPLAY_MCU_AVAILABLE
#undef DISPLAY_REMOTE_AVAILABLE
#endif

#undef CREATE_TRACEFILE_AVAILABLE

#if ! defined __AVR_ATmega1284P__ && ! defined __AVR_ATmega644__ && ! defined __AVR_ATmega644P__
#undef MMC_AVAILABLE
#undef SDFAT_AVAILABLE
#endif // ATmega1284P / ATmega644X
#endif // MCU

#ifndef SPEED_CONTROL_AVAILABLE
#undef ADJUST_PID_PARAMS
#endif

#ifndef MMC_AVAILABLE
#ifdef MCU
#undef MAP_AVAILABLE // Map geht auf dem MCU nur mit MMC
#undef SDFAT_AVAILABLE
#endif // MCU
#endif // ! MMC_AVAILABLE

#if ! defined BEHAVIOUR_AVAILABLE && defined POS_STORE_AVAILABLE
#undef POS_STORE_AVAILABLE
#warning "POS_STORE_AVAILABLE benoetigt BEHAVIOUR_AVAILABLE"
#endif

#if ! defined OS_AVAILABLE && defined SDFAT_AVAILABLE
#undef SDFAT_AVAILABLE
#warning "SDFAT_AVAILABLE benoetigt OS_AVAILABLE"
#endif

#ifdef PC
#undef LOG_UART_AVAILABLE // Auf dem PC gibts kein Logging ueber UART
#endif // PC

#ifdef MCU
/* Mit Bot zu Sim Kommunikation auf dem MCU gibts kein Logging ueber UART.
 * Ohne gibts keine Kommunikation ueber ct-Sim. */
#undef LOG_STDOUT_AVAILABLE // MCU hat kein LOG_STDOUT
#ifdef BOT_2_SIM_AVAILABLE
#undef LOG_UART_AVAILABLE
#else // ! BOT_2_SIM_AVAILABLE
#ifndef BOT_2_RPI_AVAILABLE
#undef LOG_CTSIM_AVAILABLE
#endif // ! BOT_2_RPI_AVAILABLE
#endif // BOT_2_SIM_AVAILABLE
#else // ! MCU
#undef LOG_RPI_AVAILABLE
#endif // MCU

	/* Ohne Display gibts auch keine Ausgaben auf diesem. */
#ifndef DISPLAY_AVAILABLE
#undef LOG_DISPLAY_AVAILABLE
#endif

#if ! defined SDFAT_AVAILABLE && defined USE_MINILOG
#undef LOG_MMC_AVAILABLE
#endif // SDFAT_AVAILABLE && USE_MINILOG

#ifndef SDFAT_AVAILABLE
#undef SPEED_LOG_AVAILABLE
#undef MAP_AVAILABLE
#endif

#ifndef BOT_2_RPI_AVAILABLE
#undef LOG_RPI_AVAILABLE
#endif

#ifdef LOG_UART_AVAILABLE
#define LOG_AVAILABLE
#endif

#ifdef LOG_CTSIM_AVAILABLE
#define LOG_AVAILABLE
#endif

#ifdef LOG_DISPLAY_AVAILABLE
#undef USE_MINILOG
#define LOG_AVAILABLE
#endif

#if defined LOG_STDOUT_AVAILABLE && defined PC
#undef USE_MINILOG
#define LOG_AVAILABLE
#endif

#ifdef LOG_RPI_AVAILABLE
#define LOG_AVAILABLE
#endif

#ifdef LOG_MMC_AVAILABLE
#define LOG_AVAILABLE
#endif

#ifndef BEHAVIOUR_AVAILABLE
#undef MAP_AVAILABLE
#endif

#ifdef MAP_AVAILABLE
#define OS_AVAILABLE // Map braucht BotOS
#else // ! MAP_AVAILABLE
#undef MAP_2_SIM_AVAILABLE
#endif // MAP_AVAILABLE

#ifndef BOT_2_BOT_AVAILABLE
#undef BOT_2_BOT_PAYLOAD_AVAILABLE
#endif

#ifdef LOG_AVAILABLE
	/* Es kann immer nur ueber eine Schnittstelle geloggt werden. */
#ifdef LOG_UART_AVAILABLE
#define UART_AVAILABLE /**< Serielle Kommunikation */
#undef LOG_CTSIM_AVAILABLE
#undef LOG_DISPLAY_AVAILABLE
#undef LOG_STDOUT_AVAILABLE
#undef LOG_MMC_AVAILABLE
#undef LOG_RPI_AVAILABLE
#endif

#ifdef LOG_CTSIM_AVAILABLE
#undef LOG_DISPLAY_AVAILABLE
#undef LOG_STDOUT_AVAILABLE
#undef LOG_MMC_AVAILABLE
#undef LOG_RPI_AVAILABLE
#endif

#ifdef LOG_DISPLAY_AVAILABLE
#undef LOG_STDOUT_AVAILABLE
#undef LOG_MMC_AVAILABLE
#undef LOG_RPI_AVAILABLE
#endif

#ifdef LOG_STDOUT_AVAILABLE
#undef LOG_RPI_AVAILABLE
#undef LOG_MMC_AVAILABLE
#endif

#ifdef LOG_RPI_AVAILABLE
#undef LOG_MMC_AVAILABLE
#endif

#ifndef SDFAT_AVAILABLE
#undef LOG_MMC_AVAILABLE
#undef SPEED_LOG_AVAILABLE
#endif

#if ! defined LOG_CTSIM_AVAILABLE && ! defined LOG_DISPLAY_AVAILABLE && ! defined LOG_UART_AVAILABLE && \
	! defined LOG_STDOUT_AVAILABLE && ! defined LOG_MMC_AVAILABLE && ! defined LOG_RPI_AVAILABLE
	// Wenn keine sinnvolle Log-Option mehr uebrig, loggen wir auch nicht
#undef LOG_AVAILABLE
#endif

#endif // LOG_AVAILABLE


#ifdef SRF10_AVAILABLE
#define TWI_AVAILABLE /**< TWI-Schnittstelle (I2C) */
#endif

#ifdef CMPS03_AVAILABLE
#define I2C_AVAILABLE /**< I2C-Treiber */
#endif

#ifdef SP03_AVAILABLE
#define I2C_AVAILABLE /**< I2C-Treiber */
#endif

#ifdef TWI_AVAILABLE
#define I2C_AVAILABLE /**< I2C-Treiber statt TWI-Implementierung benutzen */
#endif

#if defined CREATE_TRACEFILE_AVAILABLE && ! defined OS_AVAILABLE
#define OS_AVAILABLE
#endif

#endif // CT_BOT_H_
