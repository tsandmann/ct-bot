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

#ifndef INCLUDE_BOT_LOCAL_OVERRIDE_H_
#define INCLUDE_BOT_LOCAL_OVERRIDE_H_

/* Logging-Funktionen */
#undef  LOG_CTSIM_AVAILABLE					/**< Logging zum ct-Sim (PC und MCU) */
#undef  LOG_DISPLAY_AVAILABLE				/**< Logging ueber das LCD-Display (PC und MCU) */
#undef  LOG_UART_AVAILABLE					/**< Logging ueber UART (nur fuer MCU) */
#define LOG_RPI_AVAILABLE					/**< Logging vom ATmega zum ARM-Linux Board z.B. RPi (nur MCU) */
#undef  LOG_MMC_AVAILABLE					/**< Logging in eine txt-Datei auf MMC */
#define USE_MINILOG							/**< schaltet auf schlankes Logging um */

/* Kommunikation */
#define BOT_2_SIM_AVAILABLE					/**< Soll der Bot mit dem Sim kommunizieren? */
#define BOT_2_BOT_AVAILABLE					/**< Sollen Bots untereinander kommunizieren? */
#define BOT_2_BOT_PAYLOAD_AVAILABLE			/**< Aktiviert Payload-Versand per Bot-2-Bot Kommunikation */

/* Display-Funktionen */
#define DISPLAY_AVAILABLE					/**< Display-Funktionen aktiv */
#define KEYPAD_AVAILABLE					/**< Keypad-Eingabe vorhanden? */
#define DISPLAY_MCU_AVAILABLE				/**< lokales Display (an ATmega) vorhanden */
#define DISPLAY_REMOTE_AVAILABLE			/**< Sende LCD Anzeigedaten an den Simulator */
#define WELCOME_AVAILABLE					/**< kleiner Willkommensgruss */

/* Sensorauswertung */
#define MOUSE_AVAILABLE						/**< Maus Sensor */
#define MEASURE_MOUSE_AVAILABLE				/**< Geschwindigkeiten werden aus den Maussensordaten berechnet */
#define MEASURE_COUPLED_AVAILABLE			/**< Geschwindigkeiten werden aus Maus- und Encoderwerten ermittelt und gekoppelt */
#define MEASURE_POSITION_ERRORS_AVAILABLE	/**< Fehlerberechnungen bei der Positionsbestimmung */
#define BPS_AVAILABLE						/**< Bot Positioning System */
#define SRF10_AVAILABLE						/**< Ultraschallsensor SRF10 vorhanden */
#define CMPS03_AVAILABLE					/**< Kompass CMPS03 vorhanden */

/* Motoransteuerung */
#define SPEED_CONTROL_AVAILABLE 			/**< Aktiviert die Motorregelung */
#undef  ADJUST_PID_PARAMS					/**< macht PID-Paramter zur Laufzeit per FB einstellbar */
#define SPEED_LOG_AVAILABLE 				/**< Zeichnet Debug-Infos der Motorregelung auf MMC auf */

/* Umgebungskarte */
#define MAP_AVAILABLE						/**< Aktiviert die Kartographie */
#define MAP_2_SIM_AVAILABLE					/**< Sendet die Map zur Anzeige an den Sim */

/* MMC-/SD-Karte als Speichererweiterung (Erweiterungsmodul) */
#define MMC_AVAILABLE						/**< haben wir eine MMC/SD-Karte zur Verfuegung? */
#define BOT_FS_AVAILABLE					/**< Aktiviert das Dateisystem BotFS (auf MCU nur mit MMC moeglich) */

/* Hardware-Treiber */
#define ADC_AVAILABLE						/**< A/D-Konverter */
#define SHIFT_AVAILABLE						/**< Shift Register */
#define ENA_AVAILABLE						/**< Enable-Leitungen */
#define LED_AVAILABLE						/**< LEDs aktiv */
#define IR_AVAILABLE						/**< Infrarot Fernbedienung aktiv */
#define RC5_AVAILABLE						/**< Key-Mapping fuer IR-RC aktiv */
#define SP03_AVAILABLE						/**< Sprachmodul SP03 vorhanden */

/* Sonstiges */
#define BEHAVIOUR_AVAILABLE					/**< Nur wenn dieser Parameter gesetzt ist, exisitiert das Verhaltenssystem */
#define POS_STORE_AVAILABLE					/**< Positionsspeicher vorhanden */
#define OS_AVAILABLE						/**< Aktiviert BotOS fuer Threads und Scheduling */
#define BOOTLOADER_AVAILABLE				/**< Aktiviert den Bootloadercode - das ist nur noetig fuer die einmalige "Installation" des Bootloaders */
#define ARM_LINUX_BOARD						/**< Code fuer ARM-Linux Board aktivieren, wenn ein ARM-Linux-* Target ausgewaehlt wurde. Fuehrt den high-level Code und die Verhalten aus */
#define BOT_2_RPI_AVAILABLE					/**< Kommunikation von ATmega mit einem Linux-Board (z.B. Rapsberry Pi) aktivieren. Fuehrt auf dem ATmega den low-level Code aus */

#endif /* INCLUDE_BOT_LOCAL_OVERRIDE_H_ */
