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

/*! @file 	rc5.c
 * @brief 	RC5-Fernbedienung
 * Um RC5-Codes fuer eine eigene Fernbedienung anzupassen, reicht es diese
 * in eine Header-Datei auszulagern und anstatt der rc5code.h einzubinden.
 * Die Maskierung fuer die Auswertung der Codes nicht vergessen!
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#include "ct-Bot.h"
#include "ir-rc5.h"
#include "motor.h"
#include "bot-logik.h"
#include "display.h"

#include "rc5-codes.h"

#include <stdlib.h>

#ifdef RC5_AVAILABLE

#define RC5_TOGGLE		0x0800		/*!< Das RC5-Toggle-Bit */
#define RC5_ADDRESS	0x07C0		/*!< Der Adressbereich */
#define RC5_COMMAND	0x103F		/*!< Der Kommandobereich */

/*! 
 * Dieser Typ definiert Parameter fuer RC5-Kommando-Code Funktionen.
 */
typedef struct {
	uint16 value1;	/*!< Wert 1 */
	uint16 value2;	/*!< Wert 2 */
} RemCtrlFuncPar;

/*! Dieser Typ definiert die RC5-Kommando-Code Funktion. */
typedef void (*RemCtrlFunc)(RemCtrlFuncPar *par);

/*! Dieser Typ definiert den RC5-Kommando-Code und die auszufuehrende Funktion. */
typedef struct {
	uint16 command;		/*!< Kommando Code */
	RemCtrlFunc func;	/*!< Auszufuehrende Funktion */
	RemCtrlFuncPar par;	/*!< Parameter */
} RemCtrlAction;

/*!
 * Diese Funktion setzt das Display auf eine andere Ausgabe.
 * @param par Parameter mit dem zu setzenden Screen.
 */	
#ifdef DISPLAY_SCREENS_AVAILABLE
	static void rc5_screen_set(RemCtrlFuncPar *par);
#endif

/*!
 * Diese Funktion setzt die Geschwindigkeit auf den angegebenen Wert.
 * @param par Parameter mit den zu setzenden Geschwindigkeiten.
 */	
static void rc5_bot_set_speed(RemCtrlFuncPar *par);

/*!
 * Diese Funktion aendert die Geschwindigkeit um den angegebenen Wert.
 * @param par Parameter mit den relativen Geschwindigkeitsaenderungen.
 */	
static void rc5_bot_change_speed(RemCtrlFuncPar *par);

/*!
 * Diese Funktion setzt die zu laufenden Encoderschritte.
 * @param par Parameter mit den zu laufenden Encoderschritten.
 */	
static void rc5_bot_goto(RemCtrlFuncPar *par);

uint16 RC5_Code;	/*!< Letzter empfangener RC5-Code */

/*! Fernbedienungsaktionen */
static RemCtrlAction gRemCtrlAction[] = {
	/* RC5-Code,		Funktion,				Parameter */
	{ RC5_CODE_PWR,		rc5_bot_set_speed,		{ BOT_SPEED_STOP, BOT_SPEED_STOP } },
	{ RC5_CODE_UP,		rc5_bot_change_speed,	{ 10, 10 } },
	{ RC5_CODE_DOWN,	rc5_bot_change_speed,	{ -10, -10 } },
	{ RC5_CODE_LEFT,	rc5_bot_change_speed,	{ 10, 0 } },
	{ RC5_CODE_RIGHT,	rc5_bot_change_speed,	{ 0, 10 } },
	{ RC5_CODE_1,		rc5_bot_set_speed,		{ BOT_SPEED_SLOW, BOT_SPEED_SLOW } },
	{ RC5_CODE_3,		rc5_bot_set_speed,		{ BOT_SPEED_MAX, BOT_SPEED_MAX } },
	{ RC5_CODE_5,		rc5_bot_goto,			{ 0, 0 } },
	{ RC5_CODE_6,		rc5_bot_goto,			{ 20, -20 } },
	{ RC5_CODE_4,		rc5_bot_goto,			{ -20, 20 } },
	{ RC5_CODE_2,		rc5_bot_goto,			{ 100, 100 } },
	{ RC5_CODE_8,		rc5_bot_goto,			{ -100, -100 } },
	{ RC5_CODE_7,		rc5_bot_goto,			{ -40, 40 } },
	{ RC5_CODE_9,		rc5_bot_goto,			{ 40, -40 } },
#ifdef DISPLAY_SCREENS_AVAILABLE
	{ RC5_CODE_RED,		rc5_screen_set,			{ 0, 0 } },
	{ RC5_CODE_GREEN,	rc5_screen_set,			{ 1, 0 } },
	{ RC5_CODE_YELLOW,	rc5_screen_set,			{ 2, 0 } },
	{ RC5_CODE_BLUE,	rc5_screen_set,			{ 3, 0 } },
#endif
#ifdef JOGDIAL
	{ RC5_CODE_JOG_MID,	rc5_bot_set_speed,		{ BOT_SPEED_MAX, BOT_SPEED_MAX } },
	{ RC5_CODE_JOG_L1,	rc5_bot_set_speed,		{ BOT_SPEED_FAST, BOT_SPEED_MAX } },
	{ RC5_CODE_JOG_L2,	rc5_bot_set_speed,		{ BOT_SPEED_NORMAL, BOT_SPEED_MAX } },
	{ RC5_CODE_JOG_L3,	rc5_bot_set_speed,		{ BOT_SPEED_SLOW, BOT_SPEED_MAX } },
	{ RC5_CODE_JOG_L4,	rc5_bot_set_speed,		{ BOT_SPEED_STOP, BOT_SPEED_MAX } },
	{ RC5_CODE_JOG_L5,	rc5_bot_set_speed,		{ -BOT_SPEED_NORMAL, BOT_SPEED_MAX } },
	{ RC5_CODE_JOG_L6,	rc5_bot_set_speed,		{ -BOT_SPEED_FAST, BOT_SPEED_MAX } },
	{ RC5_CODE_JOG_L7,	rc5_bot_set_speed,		{ -BOT_SPEED_MAX, BOT_SPEED_MAX } },
	{ RC5_CODE_JOG_R1,	rc5_bot_set_speed,		{ BOT_SPEED_MAX, BOT_SPEED_FAST } },
	{ RC5_CODE_JOG_R2,	rc5_bot_set_speed,		{ BOT_SPEED_MAX, BOT_SPEED_NORMAL } },
	{ RC5_CODE_JOG_R3,	rc5_bot_set_speed,		{ BOT_SPEED_MAX, BOT_SPEED_SLOW } },
	{ RC5_CODE_JOG_R4,	rc5_bot_set_speed,		{ BOT_SPEED_MAX, BOT_SPEED_STOP } },
	{ RC5_CODE_JOG_R5,	rc5_bot_set_speed,		{ BOT_SPEED_MAX, -BOT_SPEED_NORMAL } },
	{ RC5_CODE_JOG_R6,	rc5_bot_set_speed,		{ BOT_SPEED_MAX, -BOT_SPEED_FAST } },
	{ RC5_CODE_JOG_R7,	rc5_bot_set_speed,		{ BOT_SPEED_MAX, -BOT_SPEED_MAX } }
#endif	/* JOGDIAL */
};

/*!
 * Diese Funktion setzt das Display auf eine andere Ausgabe.
 * @param par Parameter mit dem zu setzenden Screen.
 */	
#ifdef DISPLAY_SCREENS_AVAILABLE
static void rc5_screen_set(RemCtrlFuncPar *par) {
	if (par) {
		display_screen = par->value1;
		display_clear();
	}
}
#endif

/*!
 * Diese Funktion setzt die Geschwindigkeit auf den angegebenen Wert.
 * @param par Parameter mit den zu setzenden Geschwindigkeiten.
 */	
static void rc5_bot_set_speed(RemCtrlFuncPar *par) {
	if (par) {
		target_speed_l = par->value1;
		target_speed_r = par->value2;
	}
}

/*!
 * Diese Funktion aendert die Geschwindigkeit um den angegebenen Wert.
 * @param par Parameter mit den relativen Geschwindigkeitsaenderungen.
 */	
static void rc5_bot_change_speed(RemCtrlFuncPar *par) {
	if (par) {
		target_speed_l += par->value1;
		target_speed_r += par->value2;
	}
}

/*!
 * Diese Funktion setzt die zu laufenden Encoderschritte.
 * @param par Parameter mit den zu laufenden Encoderschritten.
 */	
static void rc5_bot_goto(RemCtrlFuncPar *par) {
	if (par) 	
		bot_goto(par->value1, par->value2);
}

/*!
 * Liest ein RC5-Codeword und wertet es aus
 */
void rc5_control(void){
	uint16 run;
	uint16 rc5 = ir_read();
	
	if (rc5 != 0) {
		RC5_Code= rc5 & RC5_MASK;	/* Alle uninteressanten Bits ausblenden */
		
		/* Suchen der auszufuehrenden Funktion */
		for(run=0; run<sizeof(gRemCtrlAction)/sizeof(RemCtrlAction); run++) {
			/* Funktion gefunden? */
			if (gRemCtrlAction[run].command == RC5_Code) {
				/* Funktion ausfuehren */
				gRemCtrlAction[run].func(&gRemCtrlAction[run].par);
			}
		}
	}
}
#endif
