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
#define LOG_STDOUT_AVAILABLE 				/**< Logging auf die Konsole (nur fuer PC) */
#define USE_MINILOG							/**< schaltet auf schlankes Logging um */
#define CREATE_TRACEFILE_AVAILABLE			/**< Aktiviert das Schreiben einer Trace-Datei (nur PC) */

/* Kommunikation */
#define BOT_2_SIM_AVAILABLE					/**< Soll der Bot mit dem Sim kommunizieren? */
#define BOT_2_BOT_AVAILABLE					/**< Sollen Bots untereinander kommunizieren? */
#define BOT_2_BOT_PAYLOAD_AVAILABLE			/**< Aktiviert Payload-Versand per Bot-2-Bot Kommunikation */

/* Display-Funktionen */
#define DISPLAY_AVAILABLE					/**< Display-Funktionen aktiv */
#define KEYPAD_AVAILABLE						/**< Keypad-Eingabe vorhanden? */
#define DISPLAY_REMOTE_AVAILABLE				/**< Sende LCD Anzeigedaten an den Simulator */
#define ARM_LINUX_DISPLAY	"stdout"			/**< Konsole fuer Display-Ausgaben auf ARM-Linux-Board. "stdout" fuer Ausgabe auf stdout */
#define WELCOME_AVAILABLE					/**< kleiner Willkommensgruss */

/* Sensorauswertung */
#define MOUSE_AVAILABLE						/**< Maus Sensor */
#define MEASURE_MOUSE_AVAILABLE				/**< Geschwindigkeiten werden aus den Maussensordaten berechnet */
#define MEASURE_COUPLED_AVAILABLE			/**< Geschwindigkeiten werden aus Maus- und Encoderwerten ermittelt und gekoppelt */
#define MEASURE_POSITION_ERRORS_AVAILABLE	/**< Fehlerberechnungen bei der Positionsbestimmung */
#define BPS_AVAILABLE						/**< Bot Positioning System */

/* Umgebungskarte */
#define MAP_AVAILABLE						/**< Aktiviert die Kartographie */
#define MAP_2_SIM_AVAILABLE					/**< Sendet die Map zur Anzeige an den Sim */

/* MMC-/SD-Karte als Speichererweiterung (Erweiterungsmodul) */
#define SDFAT_AVAILABLE						/**< Unterstuetzung fuer FAT-Dateisystem (FAT16 und FAT32) auf MMC/SD-Karte */

/* Hardware-Treiber */
#define ADC_AVAILABLE						/**< A/D-Konverter */
#define SHIFT_AVAILABLE						/**< Shift Register */
#define ENA_AVAILABLE						/**< Enable-Leitungen */
#define LED_AVAILABLE						/**< LEDs aktiv */
#define IR_AVAILABLE							/**< Infrarot Fernbedienung aktiv */
#define RC5_AVAILABLE						/**< Key-Mapping fuer IR-RC aktiv */

/* Sonstiges */
#define BEHAVIOUR_AVAILABLE					/**< Nur wenn dieser Parameter gesetzt ist, exisitiert das Verhaltenssystem */
#define POS_STORE_AVAILABLE					/**< Positionsspeicher vorhanden */
#define OS_AVAILABLE							/**< Aktiviert BotOS fuer Threads und Scheduling */
#define ARM_LINUX_BOARD						/**< Code fuer ARM-Linux Board aktivieren, wenn ein ARM-Linux-* Target ausgewaehlt wurde. Fuehrt den high-level Code und die Verhalten aus */

#endif /* INCLUDE_BOT_LOCAL_OVERRIDE_H_ */
