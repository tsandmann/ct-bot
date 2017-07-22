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

#define BOT_DIAMETER			120	/**< Bot-Durchmesser [mm] */
#define ENCODER_MARKS			60	/**< Anzahl der Flanken, die ein Encoder bei einer Radumdrehung liefert, also Anzahl der weissen + Anzahl der schwarzen Felder */
#ifdef PC
#define WHEEL_DIAMETER			56.7		/**< Durchmesser eines Rades (Sim) [mm] */
#define WHEEL_PERIMETER			178.1283	/**< Umfang eines Rades (Sim) [mm] */
#define WHEEL_TO_WHEEL_DIAMETER 97.2		/**< Abstand der beiden Raeder (Sim) [mm] */
#else // MCU
/* hier kann man die genauen Werte fuer den eigenen Bot eintragen */
#define WHEEL_DIAMETER			56.7		/**< Durchmesser eines Rades [mm] */
#define WHEEL_PERIMETER			178.1283	/**< Umfang eines Rades [mm] */
#define WHEEL_TO_WHEEL_DIAMETER 97.2		/**< Abstand der beiden Raeder [mm] */
#endif // PC

#define DISTSENSOR_POS_FW		47	/**< Abstand der Distanzsensoren von der Radachse (in Fahrtrichtung) [mm] */
#define DISTSENSOR_POS_SW		32	/**< Abstand der Distanzsensoren von der Mittelachse (in Querrichtung) [mm] */

#define BORDERSENSOR_POS_FW		DISTSENSOR_POS_FW		/**< Abgrundsensoren unter Distsensoren */
#define BORDERSENSOR_POS_SW		(DISTSENSOR_POS_SW + 5)	/**< Abgrundsensoren 5 mm weiter aussen als Distsensoren */


/*** Parameter zur Abstimmung des eigenen ct-Bots ***/

/* Parameter der Motorregelung */
#define PID_Kp				70	/**< PID-Parameter proportional */
#define PID_Ki				10	/**< PID-Parameter intergral */
#define PID_Kd				20	/**< PID-Parameter differential */
#define PID_Ta				1	/**< Abtastzeit */
#define PID_SHIFT			4	/**< Rechtsshift der Stellgroessenkorrektur */
#define PID_TIME			333	/**< max. Aufrufinterval [ms] */
#define PID_SPEED_THRESHOLD	BOT_SPEED_FOLLOW	/**< Grenzgeschwindigkeit, ab der die Regelgroesse interpoliert wird */
#define PWMMAX				511	/**< Maximaler PWM-Wert */
#define PWMMIN				0	/**< Minimaler PWM-Wert */
#define PWMSTART_L			100	/**< Basis-PWM-Wert linker Motor (falls keine dauerhaft gespeicherte PWM-LT vorhanden ist) */
#define PWMSTART_R			100	/**< Basis-PWM-Wert rechter Motor (falls keine dauerhaft gespeicherte PWM-LT vorhanden ist) */
#define PID_START_DELAY		20	/**< Dauer der Anfahrverzoegerung */
#define ENC_CORRECT_L		5	/**< Korrekturoffset fuer linken Radencoder (falls nicht kalibriert, sonst 0) */
#define ENC_CORRECT_R		5	/**< Korrekturoffset fuer rechten Radencoder (falls nicht kalibriert, sonst 0) */

/* Odometrie-Konstanten */
#ifdef PC
#define MOUSE_CPI		400		/**< CPI-Wert aus Kalibrierung (Wert fuer den Sim) */
#define MOUSE_FULL_TURN	1484	/**< Mausaenderung in X-Richtung fuer einen vollen Kreis (Wert fuer den Sim) */
#else // MCU
#define MOUSE_CPI		415		/**< CPI-Wert aus Kalibrierung */
#define MOUSE_FULL_TURN	1600	/**< Mausaenderung in X-Richtung fuer einen vollen Kreis */
#endif // PC

#define WHEEL_DISTANCE	(WHEEL_TO_WHEEL_DIAMETER / 2.0f)	/**< Abstand eines Rades zur Mitte des Bots [mm] */
#define STUCK_DIFF		100		/**< ab welcher Differenz haben wir durchdrehende Raeder? */
#define G_SPEED			0.5		/**< Kopplung Encoder- und Maussensor fuer Geschwindigkeiten (0.0=nur Radencoder, 1.0=nur Maussensor) */
#define G_POS			0.5		/**< Kopplung Encoder- und Maussensor fuer Positionen und Winkel (0.0=nur Radencoder, 1.0=nur Maussensor) */

/* Servo-Parameter */
#define DOOR_CLOSE 	65	/**< Rechter Anschlag Servo 1 (fuer ATmega32/644: Schrittweite 18, Offset 7) */
#define DOOR_OPEN	185	/**< Linker Anschlag Servo 1 (fuer ATmega32/644: Schrittweite 18, Offset 7) */
#define CAM_LEFT 	10	/**< Rechter Anschlag Servo 2 */
#define CAM_RIGHT	250	/**< Linker Anschlag Servo 2 */
#define CAM_CENTER	120	/**< Mittelstellung Servo 2 */

/* System-Konfiguration
   --> Diese Einstellungen sind von der lokalen Hardware abhaengig, Veraenderungen sind nur bei Hardware-Umbauten noetig */
#define F_CPU	16000000UL	/**< CPU-Frequenz [Hz] (16000000UL oder 20000000UL) */
#define UART_BAUD	115200	/**< Baudrate fuer UART-Kommunikation (moegliche Werte sind 57600, 115200, 230400, 500000) */
//#define ENABLE_RX0_PULLUP	/**< Aktiviert den internen Pullup fuer die RX-Leitung. Nicht aktivieren, falls entsprechender Hardware-Mod eingebaut ist! */
#define EXPANSION_BOARD_AVAILABLE		/**< Erweiterungsmodul (MMC / WiPort) installiert */
//#define EXPANSION_BOARD_MOD_AVAILABLE	/**< modifiziertes Erweiterungsmodul (MMC / WiPort) installiert */
//#define SPI_AVAILABLE	/**< verwendet den Hardware-SPI-Modus des Controllers, um mit der MMC zu kommunizieren. Muss ausserdem _immer_ an sein, wenn der Hardware-SPI-Umbau durchgefuehrt wurde! Hinweise in mcu/mmc.c beachten! */
#define SPI_SPEED	2	/**< SPI-Clockfrequenz (falls SPI_AVAILABLE) als Teiler von F_CPU (moegliche Werte sind 2, 4, 8, 16, 32, 64, 128) */
//#define DISTSENS_TYPE_GP2Y0A60 /**< Distanzsensor Typ GP2Y0A60 */

/* I/O-Schnittstellen fuer Raspberry Pi */
#define UART_LINUX_PORT		"/dev/ttyAMA0"	/**< UART Port vom ARM-Linux-Board fuer Verbinung zum ATmega */
#define BOT_RESET_GPIO		"/sys/class/gpio/gpio17/value"	/**< Pfad zum Reset-GPIO vom ARM-Linux-Board */
//#define ARM_LINUX_DISPLAY	"/dev/tty1"	/**< Konsole fuer Display-Ausgaben auf ARM-Linux-Board. "stdout" fuer Ausgabe auf stdout */

/* Fernbedienung */
#ifdef MCU
#define RC_HAVE_HQ_RC_UNIVERS29_334 /**< Dies ist die Standard-Fernbedienung */
//#define RC_HAVE_VIVANCO_UR89_TV_CODE_089
//#define RC_HAVE_HAUPPAUGE_WINTV
//#define RC_HAVE_HAUPPAUGE_MediaMPV
//#define RC_HAVE_CONRAD_PROMO8
//#define RC_HAVE_VIVANCO_UR89
//#define RC_HAVE_Technisat_TTS35AI
//#define RC_HAVE_LIFETEC_LT3607
//#define RC_HAVE_TOTAL_CONTROL
//#define RC_HAVE_PHILIPS_DEFAULT
#else // PC
#define RC_HAVE_HQ_RC_UNIVERS29_334	/**< Dies ist die Standard-Fernbedienung */
//#define RC_HAVE_HAUPPAUGE_WINTV
#endif // MCU


/*** Einstellungen fuer die Verhaltensregeln ***/

/* bot_avoid_border_behaviour() */
#define BORDER_DANGEROUS	0x3A0	/**< Wert, ab dem wir sicher sind, dass es eine Kante ist */

/* bot_avoid_col_behaviour() */
#define COL_CLOSEST			200		/**< Abstand [mm], den wir als zu nah betrachten -- je nach echtem Sensor ist das schon zu nah! */
#define COL_NEAR				300		/**< Nahbereich [mm] */
#define COL_FAR				400		/**< Fernbereich [mm] */

/* bot_solve_maze_behaviour() */
#define OPTIMAL_DISTANCE	(int16_t)(BOT_DIAMETER * 1.25f)	/**< Optimale Distanz zur Wand [mm]. Etwas mehr als Bot-Durchmesser ist ideal (vergroessert aufgrund der Kennlinien der Sharps) */
#define ADJUST_DISTANCE		10		/**< Toleranzbereich [mm] */
#define IGNORE_DISTANCE		240		/**< Entfernung, ab der eine Wand ignoriert wird [mm] */
#define GROUND_GOAL			0x221	/**< Farbe des Zielpads */
#define STARTPAD1			0x2B2	/**< Farbe des Startpads1 */
#define STARTPAD2			0x332	/**< Farbe des Startpads2 */

/* bot_follow_line_behaviour() */
#ifdef PC
/* Konstante fuer das bot_follow_line_behaviour-Verhalten im Sim */
#define LINE_SENSE		0x350	/**< Linie im Sim = 0x350 */
#else
/* Konstante fuer das bot_follow_line_behaviour-Verhalten auf dem echten Bot*/
#define LINE_SENSE		0x200	/**< Ab wann ist es eine Linie? (schwarz ca. 0x300, helle Tischflaeche 0x50) */
#endif // PC

/* bot_catch_pillar_behaviour() */
#define MAX_PILLAR_DISTANCE	500	/**< max. Entfernung zum Objekt [mm] */


#include <bot-local-override.h>


/*** Abhaengigkeiten ***/

#ifdef PC
#undef EXPANSION_BOARD_MOD_AVAILABLE
#endif

#ifdef EXPANSION_BOARD_MOD_AVAILABLE
#undef EXPANSION_BOARD_AVAILABLE	// deaktiviert EXPANSION_BOARD_AVAILABLE
#undef ENABLE_RX0_PULLUP // mod. Erweiterungsboard verwendet pull-down fuer RX0, also Kurzschluss verhindern
#undef MOUSE_AVAILABLE // deaktiviert MOUSE_AVAILABLE
#endif // EXPANSION_BOARD_AVAILABLE

#ifdef EXPANSION_BOARD_AVAILABLE
#undef ENABLE_RX0_PULLUP // Erweiterungsboard verwendet pull-down fuer RX0, also Kurzschluss verhindern
#endif

#endif // BOTLOCAL_H_
