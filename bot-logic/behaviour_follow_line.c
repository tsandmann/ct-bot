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

/*! @file 	behaviour_follow_line.c
 * @brief 	Linienverfolger
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE

/*! Folgt einer Linie, sobald beide Liniensensoren ausloesen
 * Die Linie sollte in etwa die Breite beider CNY70 haben
 */
void bot_follow_line_behaviour(Behaviour_t *data) {
	/* Konstanten fuer das Verhalten */
	#define CORNER_LEFT					1
	#define CORNER_RIGHT				2
	/* Zustaende fuer das Verhalten */
	#define CHECK_LINE					0	/* !< Feststellen ob wir ueber einer Linie sind */
	#define FOLLOW_LINE					1	/* !< Folgen einer geraden oder leicht gekruemmten Linie */
	#define CHECK_BORDER				2	/* !< Abgrundsensoren haben Alarm geschlagen. Feststellen ob wirklich Abgrund oder Ecke */
	#define CORNER_TURN 				3	/* !< Drehung in Richtun detektiertem Abgrund */
	#define CORRECT_POS					4	/* !< Nach der Drehung 1cm vorfahren zum Ausgleichen */
	#define ADVANCE_CORNER				5	/* !< Auf die Ecke zufahren, bis die Linie verschwindet */
	#define RETREAT_AND_STOP			6	/* !< Zurueckfahren mit voller Geschwindigkeit, dann Stop und Verhalten verlassen */
	
	/* Status- und Hilfsvariablen */
	static int8 lineState=CHECK_LINE;
	static int8 cornerDetected=False;	

	switch(lineState) {
		case CHECK_LINE:			/* sind beide Sensoren ueber einer Linie? */
			if (sensLineL>=LINE_SENSE && sensLineR>=LINE_SENSE) {
				/* zunaechst alle Hilfsverhalten ausschalten, die den Algorithmus stoeren */
				/* Abgrund- und Kollisions-Verhalten ausschalten */
				#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE
					deactivateBehaviour(bot_avoid_col_behaviour,NORECURSIVE);
				#endif
				#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE
					deactivateBehaviour(bot_avoid_border_behaviour,NORECURSIVE);
				#endif
				/* bot_glance() stoert bot_turn() */
				//deactivateBehaviour(bot_glance_behaviour);
				/* losfahren und nach FOLLOW_LINE wechseln */
				speedWishLeft=BOT_SPEED_FOLLOW;	
				speedWishRight=BOT_SPEED_FOLLOW;
				lineState=FOLLOW_LINE;
			}
			break;
			
		case FOLLOW_LINE:
			/* Pruefen, ob die Abgrundsensoren einen Abgrund sehen */
			if (sensBorderL>BORDER_DANGEROUS || sensBorderR>BORDER_DANGEROUS) {
				/* Abgrund erkannt, das kann jetzt eine Linie sein oder ein richtiger Abgrund.*/
				if (sensBorderL>BORDER_DANGEROUS && sensBorderR>BORDER_DANGEROUS) {
					/* Wenn beidseitig erkannt, koennen wir damit nicht umgehen ->
					 * Ende des Verhaltens */
					speedWishLeft=BOT_SPEED_STOP;
				 	speedWishRight=BOT_SPEED_STOP;
				 	lineState=CHECK_LINE;		/* Verhaltensstatus zuruecksetzen */
				 	//LOG_INFO(("Stopp in FOLLOW_LINE"));
				 	return_from_behaviour(data);
				 	break;
				}
				/* nachsehen, ob der linke oder rechte Liniensensor ohne Kontakt zur Linie ist
				 * und ggfs. gegensteuern */
				 if (sensBorderL>BORDER_DANGEROUS) {
				 	cornerDetected=CORNER_LEFT;
				 } else {
				 	cornerDetected=CORNER_RIGHT;
				 }
				 /* nun zur vermuteten Ecke vorfahren */
				 lineState=CHECK_BORDER;
				 bot_drive_distance(data,0,BOT_SPEED_FOLLOW,3);
				 break;
			}
			if (sensLineL<LINE_SENSE && sensLineR>LINE_SENSE) {
				/* links von der Linie abgekommen, daher nach rechts drehen */
				//LOG_DEBUG(("Drehe rechts"));
				speedWishLeft=BOT_SPEED_FOLLOW;
			 	speedWishRight=-BOT_SPEED_FOLLOW;
			} else if (sensLineL>LINE_SENSE && sensLineR<LINE_SENSE) {
			 	/* andersrum, also links drehen */
			 	//LOG_DEBUG(("Drehe links"));
			 	speedWishLeft=-BOT_SPEED_FOLLOW;
			 	speedWishRight=BOT_SPEED_FOLLOW;
			} else if (sensLineL>LINE_SENSE && sensLineR>LINE_SENSE) {
			 	/* noch ueber der Linie, also einfach geradeaus weiter */
			 	//LOG_DEBUG(("Fahre geradeaus"));
			 	speedWishLeft=BOT_SPEED_FOLLOW;
			 	speedWishRight=BOT_SPEED_FOLLOW;
			} 
			break;
 
		 case CHECK_BORDER:
			/* wir sollten jetzt direkt an der Kante zum Abgrund stehen, wenn es
			 * denn wirklich eine ist. In dem Fall fahren wir ein Stueck zurueck.
			 * sonst gehen wir von einer Linie aus, drehen uns in die Richtung,
			 * in der wir den "Abgrund" entdeckt haben und machen dann weiter mit
			 * der Linienverfolgung */
			if (sensBorderL>BORDER_DANGEROUS || sensBorderR>BORDER_DANGEROUS) {
				/* scheint wirklich ein Abgrund zu sein */
				lineState=RETREAT_AND_STOP;
				speedWishLeft=-BOT_SPEED_MAX;
				speedWishRight=-BOT_SPEED_MAX;
				break;
			}
			/* war nur eine Ecke, noch weiter darauf zu bis kein Kontakt mehr zur Linie */
			lineState=ADVANCE_CORNER;
			speedWishLeft=BOT_SPEED_FOLLOW;
			speedWishRight=BOT_SPEED_FOLLOW;
			break;
			
		case ADVANCE_CORNER:
			/* auf die Ecke zufahren, bis die Linie verschwindet */
			if (sensLineL<LINE_SENSE && sensLineR<LINE_SENSE) {
				/* Linie weg, also Stop, kurz zurueck und drehen */
				lineState=CORNER_TURN;
				speedWishLeft=-BOT_SPEED_SLOW;
				speedWishRight=-BOT_SPEED_SLOW;
				break;
			}
			speedWishLeft=BOT_SPEED_FOLLOW;
			speedWishRight=BOT_SPEED_FOLLOW;
			break;				
			
		case CORNER_TURN:
			/* 90� in Richtung des detektierten Abgrunds drehen */
			lineState=CORRECT_POS;
			bot_turn(data,(cornerDetected==CORNER_LEFT)?90:-90);	
			cornerDetected=False;
			break;
			
		case CORRECT_POS:
			lineState=FOLLOW_LINE;
			bot_drive_distance(data,0,BOT_SPEED_SLOW,2);
			break;
			
		case RETREAT_AND_STOP:
			/* wir sind an einem Abgrund, Stop und Ende des Verhaltens */
			speedWishLeft=BOT_SPEED_STOP;
			speedWishRight=BOT_SPEED_STOP;
			cornerDetected=False;
			lineState=CHECK_LINE;
			return_from_behaviour(data);
			break;
	}
}

/*! Folgt einer Linie, sobald beide Liniensensoren ausloesen
 * Die Linie sollte in etwa die Breite beider CNY70 haben
 */
void bot_follow_line(Behaviour_t *caller) {
	switch_to_behaviour(caller, bot_follow_line_behaviour,NOOVERRIDE);
}
#endif
