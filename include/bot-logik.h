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

/*! @file 	bot-logik.h
 * @brief 	High-Level-Routinen fuer die Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#ifndef bot_logik_H_
#define bot_logik_H_

#include "global.h"
#include "ct-Bot.h"


/*! Verwaltungsstruktur fuer die Verhaltensroutinen */
typedef struct _Behaviour_t {
   void (*work) (struct _Behaviour_t *data); 	/*!< Zeiger auf die Funktion, die das Verhalten bearbeitet */
   
   uint8 priority;				/*!< Prioritaet */
   struct _Behaviour_t *caller ; /* aufrufendes verhalten */
   
   uint8 active:1;				/*!< Ist das Verhalten aktiv */
   uint8 subResult:2;			/*!< War das aufgerufene unterverhalten erfolgreich (==1)?*/
   struct _Behaviour_t *next;					/*!< Naechster Eintrag in der Liste */
#ifndef DOXYGEN
	}__attribute__ ((packed)) Behaviour_t;
#else
	} Behaviour_t;
#endif


extern volatile int16 target_speed_l;	/*!< Sollgeschwindigkeit linker Motor */
extern volatile int16 target_speed_r;	/*!< Sollgeschwindigkeit rechter Motor */

/*!
 * Kuemmert sich intern um die Ausfuehrung der goto-Kommandos
 * @see bot_goto()
 */
extern void bot_behave(void);

/*!
 * Initilaisert das ganze Verhalten
 */
extern void bot_behave_init(void);

/*!
 * Aktiviert eine Regel mit gegebener Funktion
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void activateBehaviour(void *function);

/*!
 * Aktiviert eine Regel mit gegebener Funktion
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void deactivateBehaviour(void *function);

/*!
 * Beispiel fuer ein Verhalten, das einen Zustand besitzt
 * es greift auf andere Verhalten zurueck und setzt daher 
 * selbst keine speedWishes
 * Laesst den Roboter ein Quadrat abfahren
 * @param *data der Verhaltensdatensatz
 */
void bot_drive_square_behaviour(Behaviour_t *data);

/*!
 * Kuemmert sich intern um die Ausfuehrung der goto-Kommandos,
 * @param *data der Verhaltensdatensatz
 * @see bot_goto()
 */
void bot_goto_behaviour(Behaviour_t *data);

/*!
 * Drehe die Raeder um die gegebene Zahl an Encoder-Schritten weiter
 * @param left Schritte links
 * @param right Schritte rechts
 */
void bot_goto(int16 left, int16 right, Behaviour_t * caller);

/*! 
 * Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren. Dabei legt die Geschwindigkeit fest, ob der Bot vorwaerts oder rueckwaerts fahren soll.
 * @param curve Gibt an, ob der Bot eine Kurve fahren soll. Werte von -127 (So scharf wie moeglich links) ueber 0 (gerade aus) bis 127 (so scharf wie moeglich rechts)
 * @param speed Gibt an, wie schnell der Bot fahren soll. Negative Werte lassen den Bot rueckwaerts fahren.
 * @param cm Gibt an, wie weit der Bot fahren soll. In cm :-) Die Strecke muss positiv sein, die Fahrtrichtung wird ueber speed geregelt.
 * */

void bot_drive_distance(Behaviour_t* caller,int8 curve, int speed, int cm);

/*! 
 * Dreht den Bot im mathematisch positiven Sinn. 
 * @param degrees Grad, um die der Bot gedreht wird. Negative Zahlen drehen im (mathematisch negativen) Uhrzeigersinn.
 * */
void bot_turn(Behaviour_t* caller,int16 degrees);

/*!
 * Das Verhalten setzt sich aus 3 Teilverhalten zusammen: 
 * Nach Licht suchen, auf das Licht zufahren, im Licht Slalom fahren. */
void bot_olympic_behaviour(Behaviour_t *data);

/*!
 * Das Verhalten laesst den Bot eine vorher festgelegte Strecke fahren.
 * @see bot_drive_distance() 
 * */

void bot_drive_distance_behaviour(Behaviour_t* data);

/*!
 * Das Verhalten laesst den Bot eine Punktdrehung durchfuehren. 
 * @see bot_turn()
 * */
void bot_turn_behaviour(Behaviour_t* data);

/*!
 * Das Verhalten laesst den Roboter den Raum durchsuchen. 
 * Das Verhalten hat mehrere unterschiedlich Zustaende:
 * 1. Zu einer Wand oder einem anderen Hindernis fahren.
 * 2. Zu einer Seite drehen, bis der Bot parallel zur Wand ist. 
 * Es macht vielleicht Sinn, den Maussensor auszulesen, um eine Drehung um 
 * einen bestimmten Winkel zu realisieren. Allerdings muesste dafuer auch der
 * Winkel des Bots zur Wand bekannt sein.
 * 3. Eine feste Strecke parallel zur Wand vorwaerts fahren.
 * Da bot_glance abwechselnd zu beiden Seiten schaut, ist es fuer die Aufgabe, 
 * einer Wand auf einer Seite des Bots zu folgen, nur bedingt gewachsen und muss
 * evtl. erweitert werden.
 * 4. Senkrecht zur Wand drehen.
 * Siehe 2.
 * 5. Einen Bogen fahren, bis der Bot wieder auf ein Hindernis stoesst. 
 * Dann das Ganze von vorne beginnen, nur in die andere Richtung und mit einem
 * weiteren Bogen. So erforscht der Bot einigermassen systematisch den Raum.
 * 
 * Da das Verhalten jeweils nach 10ms neu aufgerufen wird, muss der Bot sich
 * 'merken', in welchem Zustand er sich gerade befindet.
 * */
void bot_explore_behaviour(Behaviour_t *data);

/*!
 * Das Verhalten laesst den Bot einen Slalom fahren.
 * @see bot_do_slalom()
 * */
void bot_do_slalom_behaviour(Behaviour_t *data);

#endif
