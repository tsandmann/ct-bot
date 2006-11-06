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

/*! @file 	behaviour_turn.c
 * @brief 	Drehe den Bot
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/


#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_TURN_AVAILABLE
#ifdef MCU
	#include <avr/eeprom.h>
#endif
#include <stdlib.h>
#include <math.h>

/* Parameter fuer das bot_turn_behaviour() */
#ifndef MEASURE_MOUSE_AVAILABLE
int16 turn_targetR;				/*!< Zu drehender Winkel bzw. angepeilter Stand des Radencoders sensEncR */
int16 turn_targetL;				/*!< Zu drehender Winkel bzw. angepeilter Stand des Radencoders sensEncL */
#else
int8 angle_correct=0;			/*!< Drehabschnitt 0=0-15Grad, 1=16-45 Grad, 2= >45 Grad */
int16 to_turn;					/*!< Wieviel Grad sind noch zu drehen? */
	#ifdef MCU
		uint8 __attribute__ ((section (".eeprom"))) err15=1;					/*!< Fehler bei Drehungen unter 15 Grad */
		uint8 __attribute__ ((section (".eeprom"))) err45=2;					/*!< Fehler bei Drehungen zwischen 15 und 45 Grad */
		uint8 __attribute__ ((section (".eeprom"))) err_big=4;				/*!< Fehler bei groesseren Drehungen */
	#else
		uint8 err15=0;
		uint8 err45=0;
		uint8 err_big=0;
	#endif
#endif
int8 turn_direction;			/*!< Richtung der Drehung */


/* Hilfsfunktion zur Berechnung einer Winkeldifferenz */
inline int16 calc_turned_angle(int8 direction, int16 angle1, int16 angle2) {
	int16 diff_angle=0;
	
	if (direction>0){
		// Drehung in mathematisch positivem Sinn 
		if (angle1>angle2) {
			// Es gab einen Ueberlauf
			diff_angle=360-angle1+angle2;
		} else {
			diff_angle=angle2-angle1;	
		}
	} else {
		// Drehung in mathematisch negativem Sinn
		if (angle1<angle2) {
			// Es gab einen Ueberlauf
			diff_angle=angle1+360-angle2;
		} else {
			diff_angle=angle1-angle2;
		}
	}
	return diff_angle;
}

#ifdef MEASURE_MOUSE_AVAILABLE
 /*!
  * Das Verhalten laesst den Bot eine Punktdrehung durchfuehren. 
+ * Drehen findet in drei Schritten statt. Die Drehung wird dabei
+ * bei Winkeln > 15 Grad zunaechst mit hoeherer Geschwindigkeit ausgefuehrt. Bei kleineren
+ * Winkeln oder wenn nur noch 15 Grad zu drehen sind, nur noch mit geringer Geschwindigkeit
  * @param *data der Verhaltensdatensatz
  * @see bot_turn()
  */
void bot_turn_behaviour(Behaviour_t *data){
 	// Zustaende fuer das bot_turn_behaviour-Verhalten 
 	#define NORMAL_TURN				0
 	#define STOP_TURN				1
	#define FULL_STOP				2
 	static int8 turnState=NORMAL_TURN;
 	static int16 old_heading=-1;
	static int16 head_count=0;
	uint8 e15;
	uint8 e45;
	uint8 ebig;
	
	// seit dem letzten mal gedrehte Grad
	int16 turned=0;
	// aktueller Winkel als int16
	int16 akt_heading=(int16)heading_mou;
	
	// erster Aufruf? -> alter Winkel==neuer Winkel
	if (old_heading==-1) old_heading=akt_heading;
	
	// berechnen, wieviel Grad seit dem letzten Aufruf gedreht wurden
	turned=calc_turned_angle(turn_direction,old_heading,akt_heading);
	if (turned > 300) turned -= 360; // hier ging etwas schief
	
	// aktueller Winkel wird alter Winkel
	old_heading=akt_heading;
	// aktuelle Drehung von zu drehendem Winkel abziehen
	to_turn-=turned;

 	switch(turnState) {
 		case NORMAL_TURN:
			// Solange drehen, bis Drehwinkel erreicht ist
			// oder gar zu weit gedreht wurde
			if (to_turn<1) {
 				/* Nachlauf abwarten */
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
 				turnState=STOP_TURN;
 				break;
 			}
         	speedWishLeft = (turn_direction > 0) ? -BOT_SPEED_NORMAL : BOT_SPEED_NORMAL;	                         speedWishLeft = (turn_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
         	speedWishRight = (turn_direction > 0) ? BOT_SPEED_NORMAL : -BOT_SPEED_NORMAL;	                         speedWishRight = (turn_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
	        break;
 			
 		case STOP_TURN:
			// Abwarten, bis Nachlauf beendet
			if (akt_heading!=old_heading){
				head_count=0;
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				break;
			}		
			if (head_count<10) {
				head_count++;
				speedWishLeft=BOT_SPEED_STOP;
				speedWishRight=BOT_SPEED_STOP;
				break;					
 			}
			#ifdef MCU
				e15=eeprom_read_byte(&err15);
				e45=eeprom_read_byte(&err45);
				ebig=eeprom_read_byte(&err_big);
			#else
				e15=err15;
				e45=err45;
				ebig=err_big;
			#endif

			// Neue Abweichung mit alter vergleichen und ggfs neu bestimmen
			
			switch(angle_correct) {
				case 0:
					if (abs(to_turn)-e15>1) {
						e15=(int8)(abs(to_turn)+e15)/2;
						#ifdef MCU
							eeprom_write_byte(&err15,e15);
						#else
							err15=e15;
						#endif
					}
					break;
					
				case 1:
					if (abs(to_turn)-e45>1) {
						e45=(int8)(abs(to_turn)+e45)/2;
						#ifdef MCU
							eeprom_write_byte(&err45,e45);
						#else
							err45=e45;
						#endif
					}
					break;	
					
				case 2:
				if (abs(to_turn)-ebig>1) {
						ebig=(int8)(abs(to_turn)+ebig)/2;
						#ifdef MCU
							eeprom_write_byte(&err_big,ebig);
						#else
							err_big=ebig;
						#endif
					}
					break;
			}			
			// ok, verhalten beenden
			speedWishLeft=BOT_SPEED_STOP;
			speedWishRight=BOT_SPEED_STOP;
 			turnState=NORMAL_TURN;
 			old_heading=-1;
 			return_from_behaviour(data);
 			break;			
 	}
}

void bot_turn(Behaviour_t *caller, int16 degrees)
{
 	// Richtungsgerechte Umrechnung in den Zielwinkel
 	if(degrees < 0) turn_direction = -1;
 	else turn_direction = 1;
	to_turn=abs(degrees);
	#ifdef MCU
		if (eeprom_read_byte(&err15)==255 && eeprom_read_byte(&err45)==255 && eeprom_read_byte(&err_big)==255) {
			eeprom_write_byte(&err15,1);
			eeprom_write_byte(&err45,2);
			eeprom_write_byte(&err_big,4);
		}
		if (to_turn>45) {
			to_turn-=eeprom_read_byte(&err_big);
			angle_correct=2;
		} else if (to_turn<=45 && to_turn>15) {
		to_turn-=eeprom_read_byte(&err45);
			angle_correct=1;
		} else {
			to_turn-=eeprom_read_byte(&err15);
			angle_correct=0;
		}
	#else
			if (to_turn>45) {
			to_turn-=err_big;
			angle_correct=2;
		} else if (to_turn<=45 && to_turn>15) {
			to_turn-=err45;
			angle_correct=1;
		} else {
			to_turn-=err15;
			angle_correct=0;
		}
	#endif
 	switch_to_behaviour(caller, bot_turn_behaviour,NOOVERRIDE);
 }
#else
/*!
 * Das Verhalten laesst den Bot eine Punktdrehung durchfuehren. 
 * @see bot_turn()
 * */
void bot_turn_behaviour(Behaviour_t* data){
	/* Drehen findet in vier Schritten statt. Die Drehung wird dabei
	 * bei Winkeln > 90 Grad zunaechst mit maximaler Geschwindigkeit ausgefuehrt. Bei kleineren
	 * Winkeln oder wenn nur noch 90 Grad zu drehen sind, nur noch mit normaler Geschwindigkeit
	 */
	 /* Zustaende fuer das bot_turn_behaviour-Verhalten */
	#define NORMAL_TURN				0
	#define SHORT_REVERSE				1
	#define CORRECT_POSITION			2
	#define FULL_STOP					3
	static int8 turnState=NORMAL_TURN;
	/* zu drehende Schritte in die korrekte Drehrichtung korrigieren */
	int16 to_turnR = turn_direction*(turn_targetR - sensEncR);
	int16 to_turnL = -turn_direction*(turn_targetL - sensEncL);

	switch(turnState) {
		case NORMAL_TURN:
			/* Solange drehen, bis beide Encoder nur noch zwei oder weniger Schritte zu fahren haben */

			if (to_turnL <= 2 && to_turnR<=2){
				/* nur noch 2 Schritte oder weniger, abbremsen einleiten */
				turnState=SHORT_REVERSE;
				break;	
			}
			
			/* Bis 90 Grad kann mit maximaler Geschwindigkeit gefahren werden, danach auf Normal reduzieren */
			/* Geschwindigkeit fuer beide Raeder getrennt ermitteln */
			if(abs(to_turnL) < ANGLE_CONSTANT*0.25) {
				speedWishLeft = (turn_direction > 0) ? -BOT_SPEED_MEDIUM : BOT_SPEED_MEDIUM;
			} else {
				speedWishLeft = (turn_direction > 0) ? -BOT_SPEED_NORMAL : BOT_SPEED_NORMAL;
			}
			
			if(abs(to_turnR) < ANGLE_CONSTANT*0.25) {
				speedWishRight = (turn_direction > 0) ? BOT_SPEED_MEDIUM : -BOT_SPEED_MEDIUM;
			} else {	
				speedWishRight = (turn_direction > 0) ? BOT_SPEED_NORMAL : -BOT_SPEED_NORMAL;
			}	
			
			break;
			
		case SHORT_REVERSE:
			/* Ganz kurz durch umpolen anbremsen */ 
			speedWishLeft = (turn_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			speedWishRight = (turn_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			turnState=CORRECT_POSITION;
			break;
		
		case CORRECT_POSITION:
			/* Evtl. etwas zuruecksetzen, falls wir zu weit gefahren sind */
			if (to_turnR<0) {
				/* rechts zu weit gefahren..langsam zurueck */
				speedWishRight = (turn_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			} else if (to_turnR>0) {
				/* rechts noch nicht weit genug...langsam vor */
				speedWishRight = (turn_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, rechtes Rad anhalten */
				speedWishRight = BOT_SPEED_STOP;
			}		
			
			if (to_turnL<0) {
				/* links zu weit gefahren..langsam zurueck */
				speedWishLeft = (turn_direction > 0) ? BOT_SPEED_SLOW : -BOT_SPEED_SLOW;				
			} else if (to_turnL>0) {
				/* links noch nicht weit genug...langsam vor */
				speedWishLeft = (turn_direction > 0) ? -BOT_SPEED_SLOW : BOT_SPEED_SLOW;
			} else {
				/* Endposition erreicht, linkes Rad anhalten */
				speedWishLeft = BOT_SPEED_STOP;
			}
			
			if (speedWishLeft == BOT_SPEED_STOP && speedWishRight == BOT_SPEED_STOP) {
				/* beide Raeder haben nun wirklich die Endposition erreicht, daher anhalten */
				turnState=FULL_STOP;
			}
			break;
		
			
		default: 
			/* ist gleichzeitig FULL_STOP, da gleiche Aktion 
			 * Stoppen, State zuruecksetzen und Verhalten beenden */
			speedWishLeft = BOT_SPEED_STOP;
			speedWishRight = BOT_SPEED_STOP;
			turnState=NORMAL_TURN;
			return_from_behaviour(data);
			break;			
	}
}

/*! 
 * Dreht den Bot im mathematisch positiven Sinn. 
 * @param degrees Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * Die Aufloesung betraegt rund 3 Grad
 */
void bot_turn(Behaviour_t* caller,int16 degrees){
	/* Umrechnung von Grad in Encoder-Markierungen.
	 * Hinweis: Eigentlich muessten der Umfang von Bot und Rad verwendet werden. Die Rechnung wird
	 * allerdings viel einfacher, wenn man Pi auskuerzt.
	 * Ist degrees negativ, ist die Drehung negativ und der rechte Encoder muss kleiner werden.
	 */
	
	if(degrees < 0) turn_direction = -1;
	else turn_direction = 1;
	/* Anzahl zu fahrender Encoderschritte berechnen */
	turn_targetR=(degrees*ANGLE_CONSTANT)/360;
	/* linkes Rad dreht entgegengesetzt, daher negativer Wert */
 	turn_targetL=-turn_targetR;

 	/* aktuellen Sensorwert zu zu drehenden Encoderschritten addieren */
 	turn_targetR+=sensEncR;
 	turn_targetL+=sensEncL;
	switch_to_behaviour(caller, bot_turn_behaviour,OVERRIDE);
}
#endif
#endif

