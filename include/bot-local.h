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
 * \file 	bot-local.h
 * \brief 	Konstanten, die den Bot an reale Umgebungen anpassen
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author  Christoph Grimmer (c.grimmer@futurio.de)
 * \date 	28.02.2006
 */

#ifndef BOTLOCAL_H_
#define BOTLOCAL_H_


/*** Bot-Geometrie ***/

#define BOT_DIAMETER			120			/**< Bot-Durchmesser [mm] */
#define ENCODER_MARKS			60			/**< Anzahl der Flanken, die ein Encoder bei einer Radumdrehung liefert, also Anzahl der weissen + Anzahl der schwarzen Felder */
#ifdef PC
#define WHEEL_DIAMETER			56.7		/**< Durchmesser eines Rades (Sim) [mm] */
#define WHEEL_PERIMETER			178.1283 	/**< Umfang eines Rades (Sim) [mm] */
#define WHEEL_TO_WHEEL_DIAMETER 97.2 		/**< Abstand der beiden Raeder (Sim) [mm] */
#else // MCU
/* hier kann man die genauen Werte fuer den eigenen Bot eintragen */
#define WHEEL_DIAMETER			56.7		/**< Durchmesser eines Rades [mm] */
#define WHEEL_PERIMETER			178.1283 	/**< Umfang eines Rades [mm] */
#define WHEEL_TO_WHEEL_DIAMETER 97.2 		/**< Abstand der beiden Raeder [mm] */
#endif // PC

#define DISTSENSOR_POS_FW		47			/**< Abstand der Distanzsensoren von der Radachse (in Fahrtrichtung) [mm] */
#define DISTSENSOR_POS_SW		32			/**< Abstand der Distanzsensoren von der Mittelachse (in Querrichtung) [mm] */

#define BORDERSENSOR_POS_FW		DISTSENSOR_POS_FW  /**< Abgrundsensoren unter Distsensoren */
#define BORDERSENSOR_POS_SW		(DISTSENSOR_POS_SW + 5) /**< Abgrundsensoren 5 mm weiter aussen als Distsensoren */

/*** einstellbare Parameter ***/

/* Parameter der Motorregelung */
#define PID_Kp				70				/**< PID-Parameter proportional */
#define PID_Ki				10				/**< PID-Parameter intergral */
#define PID_Kd				20				/**< PID-Parameter differential */
#define PID_Ta				1				/**< Abtastzeit */
#define PID_SHIFT			4				/**< Rechtsshift der Stellgroessenkorrektur */
#define PID_TIME			333				/**< max. Aufrufinterval [ms] */
#define PID_SPEED_THRESHOLD	BOT_SPEED_FOLLOW/**< Grenzgeschwindigkeit, ab der die Regelgroesse interpoliert wird */
#define PWMMAX				511				/**< Maximaler PWM-Wert */
#define PWMMIN				0				/**< Minimaler PWM-Wert */
#define PWMSTART_L			100				/**< Basis-PWM-Wert linker Motor (falls keine dauerhaft gespeicherte PWM-LT vorhanden ist) */
#define PWMSTART_R			100				/**< Basis-PWM-Wert rechter Motor (falls keine dauerhaft gespeicherte PWM-LT vorhanden ist) */
#define PID_START_DELAY		20				/**< Dauer der Anfahrverzoegerung */
#define ENC_CORRECT_L		5				/**< Korrekturoffset fuer linken Radencoder (falls nicht kalibriert, sonst 0) */
#define ENC_CORRECT_R		5				/**< Korrekturoffset fuer rechten Radencoder (falls nicht kalibriert, sonst 0) */

/* Odometrie-Konstanten */
#ifdef PC
#define MOUSE_CPI		400		/**< CPI-Wert aus Kalibrierung (Wert fuer den Sim) */
#define MOUSE_FULL_TURN	1484	/**< Mausaenderung in X-Richtung fuer einen vollen Kreis (Wert fuer den Sim) */
#else // MCU
#define MOUSE_CPI		415		/**< CPI-Wert aus Kalibrierung */
#define MOUSE_FULL_TURN	1600	/**< Mausaenderung in X-Richtung fuer einen vollen Kreis */
#endif // PC

#define WHEEL_DISTANCE		(WHEEL_TO_WHEEL_DIAMETER / 2.0f)	/**< Abstand eines Rades zur Mitte des Bots [mm] */
#define STUCK_DIFF			100		/**< ab welcher Differenz haben wir durchdrehende Raeder? */
#define G_SPEED				0.5		/**< Kopplung Encoder- und Maussensor fuer Geschwindigkeiten (0.0=nur Radencoder, 1.0=nur Maussensor) */
#define G_POS				0.5		/**< Kopplung Encoder- und Maussensor fuer Positionen und Winkel (0.0=nur Radencoder, 1.0=nur Maussensor) */

#define BPS_NO_DATA			0xffff /**< Wert des BPS-Sensors, falls keine Daten verfuegbar sind */

/* System-Konstanten */
#define F_CPU	16000000L	/**< CPU-Frequenz [Hz] */
//#define F_CPU	20000000L	/**< CPU-Frequenz [Hz] */
#define XTAL	F_CPU		/**< CPU-Frequenz [Hz] */

//#define UART_BAUD	57600	/**< Baudrate  57600 fuer UART-Kommunikation */
#define UART_BAUD	115200	/**< Baudrate 115200 fuer UART-Kommunikation */
//#define UART_BAUD	230400	/**< Baudrate 230400 fuer UART-Kommunikation */
//#define UART_BAUD	500000	/**< Baudrate 500000 fuer UART-Kommunikation */

#define UART_LINUX_PORT		"/dev/ttyAMA0" /**< UART Port vom ARM-Linux-Board fuer Verbinung zum ATmega */
#define BOT_RESET_GPIO		"/sys/class/gpio/gpio17/value" /**< Pfad zum Reset-GPIO vom ARM-Linux-Board */
//#define ARM_LINUX_DISPLAY	"/dev/tty1" /**< Konsole fuer Display-Ausgaben auf ARM-Linux-Board. "stdout" fuer Ausgabe auf stdout */

#define EXPANSION_BOARD_AVAILABLE 		/**< Erweiterungsmodul (MMC / WiPort) installiert */

#define EXPANSION_BOARD_MOD_AVAILABLE	/**< modifiziertes Erweiterungsmodul (MMC / WiPort) installiert */

#if defined EXPANSION_BOARD_MOD_AVAILABLE
#undef EXPANSION_BOARD_AVAILABLE		// EXPANSION_BOARD_MOD_AVAILABLE deaktiviert EXPANSION_BOARD_AVAILABLE
#undef MOUSE_AVAILABLE					// EXPANSION_BOARD_MOD_AVAILABLE deaktiviert MOUSE_AVAILABLE
#endif // EXPANSION_BOARD_AVAILABLE

//#define BPS_AVAILABLE		/**< Bot Positioning System */
//#define SPI_AVAILABLE		/**< verwendet den Hardware-SPI-Modus des Controllers, um mit der MMC zu kommunizieren. Muss ausserdem _immer_ an sein, wenn der Hardware-SPI-Umbau durchgefuehrt wurde! Hinweise in mcu/mmc.c beachten! */

/* Servo-Parameter */
#ifndef __AVR_ATmega1284P__
#if F_CPU == 16000000L
#define DOOR_CLOSE 	7  /**< Rechter Anschlag Servo 1 */
#define DOOR_OPEN	14 /**< Linker Anschlag Servo 1 */
#define CAM_LEFT 	7  /**< Rechter Anschlag Servo 2 */
#define CAM_RIGHT	14 /**< Linker Anschlag Servo 2 */
#else
#define DOOR_CLOSE 	10 /**< Rechter Anschlag Servo 1 */
#define DOOR_OPEN	18 /**< Linker Anschlag Servo 1 */
#define CAM_LEFT 	10 /**< Rechter Anschlag Servo 2 */
#define CAM_RIGHT	18 /**< Linker Anschlag Servo 2 */
#endif // F_CPU
#else
#define DOOR_CLOSE 	64  /**< Rechter Anschlag Servo 1 */
#define DOOR_OPEN	180 /**< Linker Anschlag Servo 1 */
#define CAM_LEFT 	64  /**< Rechter Anschlag Servo 2 */
#define CAM_RIGHT	180 /**< Linker Anschlag Servo 2 */
#endif // ! __AVR_ATmega1284P__


/*** Einstellungen fuer die Verhaltensregeln ***/

/* bot_avoid_border_behaviour() */
#define BORDER_DANGEROUS	0x3A0	/**< Wert, ab dem wir sicher sind, dass es eine Kante ist */

/* bot_avoid_col_behaviour() */
#define COL_CLOSEST			200		/**< Abstand [mm], den wir als zu nah betrachten -- je nach echtem Sensor ist das schon zu nah! */
#define COL_NEAR			300		/**< Nahbereich [mm] */
#define COL_FAR				400		/**< Fernbereich [mm] */

/* Zustaende und Konstanten fuer das bot_solve_maze_behaviour-Verhalten */
#define OPTIMAL_DISTANCE	(int16_t)(BOT_DIAMETER * 1.25f)	/**< Optimale Distanz zur Wand [mm]. Etwas mehr als Bot-Durchmesser ist ideal (vergroessert aufgrund der Kennlinien der Sharps) */
#define ADJUST_DISTANCE		10		/**< Toleranzbereich [mm] */
#define IGNORE_DISTANCE		240		/**< Entfernung, ab der eine Wand ignoriert wird [mm] */
#define GROUND_GOAL			0x221	/**< Farbe des Zielpads */
#define STARTPAD1			0x2B2	/**< Farbe des Startpads1 */
#define STARTPAD2			0x332	/**< Farbe des Startpads2 */

/* bot_follow_line_behaviour() */
#ifdef PC
/** Konstante fuer das bot_follow_line_behaviour-Verhalten im Sim */
#define LINE_SENSE		0x350	/**< Linie im Sim = 0x350 */
#else
/** Konstante fuer das bot_follow_line_behaviour-Verhalten auf dem echten Bot*/
#define LINE_SENSE		0x200	/**< Ab wann ist es eine Linie? (schwarz ca. 0x300, helle Tischflaeche 0x50) */
#endif // PC

/* Konstanten fuer bot_catch_pillar_behaviour() */
#define MAX_PILLAR_DISTANCE	500 /**< max. Entfernung zum Objekt [mm] */

/* Konstanten fuer Verhaltensanzeige, Verhalten mit prio von bis sichtbar */
#define PRIO_VISIBLE_MIN	3	/**< Prioritaet, die ein Verhalten mindestens haben muss, um angezeigt zu werden */
#define PRIO_VISIBLE_MAX	200	/**< Prioritaet, die ein Verhalten hoechstens haben darf, um angezeigt zu werden */


#include <bot-local-override.h>

#endif // BOTLOCAL_H_
