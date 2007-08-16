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
 * @file 	bot-logik.h
 * @brief 	High-Level-Routinen fuer die Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
 */

#ifndef bot_logik_H_
#define bot_logik_H_

#include "global.h"
#include "ct-Bot.h"
#include "motor.h"
#include "sensor.h"
#include "bot-local.h"


#define INACTIVE 0	/*!< Verhalten ist aus */
#define ACTIVE 1	/*!< Verhalten ist an */

#define OVERRIDE	1	/*!< Konstante, wenn Verhalten beim Aufruf alte Wuensche ueberschreiben sollen */
#define NOOVERRIDE 0	/*!< Konstanten, wenn Verhalten beim Aufruf alte Wuensche nicht ueberschreiben sollen */

#define SUBSUCCESS	1	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe erfolgreich abgeschlossen */
#define SUBFAIL	0	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe nicht abgeschlossen */
#define SUBRUNNING 2	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe wird noch beabeitet */
#define SUBCANCEL	3	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe wurde unterbrochen */

#define RECURSIVE 	 255	/*!< Konstante, die anzeigt, dass auch die Aufrufer eines Verhaltens mit deaktiviert werden */
#define NORECURSIVE 0	/*!< Konstante, die anzeigt, dass die Aufrufer eines Verhaltens nicht mit deaktiviert werden */


/*! Verwaltungsstruktur fuer die Verhaltensroutinen */
typedef struct _Behaviour_t {
   void (*work) (struct _Behaviour_t *data); 	/*!< Zeiger auf die Funktion, die das Verhalten bearbeitet */
   
   uint8 priority;								/*!< Prioritaet */
   struct _Behaviour_t *caller;					/*!< aufrufendes verhalten */
    
   uint8 active:1;								/*!< Ist das Verhalten aktiv */
   uint8 subResult:2;							/*!< War das aufgerufene unterverhalten erfolgreich (==1)? */
   struct _Behaviour_t *next;					/*!< Naechster Eintrag in der Liste */
#ifndef DOXYGEN
	}__attribute__ ((packed)) Behaviour_t;
#else
	} Behaviour_t;
#endif

/*! Dieser Typ definiert eine Funktion die das eigentliche Verhalten ausfuehrt. */
typedef void (*BehaviourFunc)(Behaviour_t *data);

/*! Liste mit allen Verhalten */
extern Behaviour_t *behaviour;

extern int16 speedWishLeft;				/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit links*/
extern int16 speedWishRight;				/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit rechts*/

extern float faktorWishLeft;				/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor links*/
extern float faktorWishRight;				/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor rechts */



extern int16 target_speed_l;	/*!< Sollgeschwindigkeit linker Motor */
extern int16 target_speed_r;	/*!< Sollgeschwindigkeit rechter Motor */

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
void activateBehaviour(BehaviourFunc function);

/*!
 * Deaktiviert eine Regel mit gegebener Funktion
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void deactivateBehaviour(BehaviourFunc function);

/*!
 * Rueckgabe von True, wenn das Verhalten gerade laeuft (aktiv ist) sonst False
 * @param function Die Funktion, die das Verhalten realisiert.
 * @return True wenn Verhalten aktiv sonst False
 */
uint8_t behaviour_is_activated(BehaviourFunc function);

/*!
 * Deaktiviert alle Verhalten bis auf Grundverhalten. Bei Verhaltensauswahl werden die Aktivitaeten vorher
 * in die Verhaltens-Auswahlvariable gesichert.
 */
void deactivateAllBehaviours(void);

/*!
 * Deaktiviert alle von diesem Verhalten aufgerufenen Verhalten. 
 * Das Verhalten selbst bleibt Aktiv und bekommt ein SUBCANCEL in seine datanestruktur eingetragen.
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void deactivateCalledBehaviours(BehaviourFunc function);

/*! 
 * Ruft ein anderes Verhalten auf und merkt sich den Ruecksprung 
 * return_from_behaviour() kehrt dann spaeter wieder zum aufrufenden Verhalten zurueck
 * @param from aufrufendes Verhalten
 * @param to aufgerufenes Verhalten
 * @param override Hier sind zwei Werte Moeglich:
 * 		1. OVERRIDE : Das Zielverhalten to wird aktiviert, auch wenn es noch aktiv ist. 
 * 					  Das Verhalten, das es zuletzt aufgerufen hat wird dadurch automatisch 
 * 					  wieder aktiv und muss selbst sein eigenes Feld subResult auswerten, um zu pruefen, ob das
 * 					  gewuenschte Ziel erreicht wurde, oder vorher ein Abbruch stattgefunden hat. 
 * 		2. NOOVERRIDE : Das Zielverhalten wird nur aktiviert, wenn es gerade nichts zu tun hat.
 * 						In diesem Fall kann der Aufrufer aus seinem eigenen subResult auslesen,
 * 						ob seibem Wunsch Folge geleistet wurde.
 */ 
void switch_to_behaviour(Behaviour_t * from, void (*to)(Behaviour_t *), uint8 override );

/*! 
 * @brief		Kehrt zum aufrufenden Verhalten zurueck
 * @param data 	laufendes Verhalten
 */
void return_from_behaviour(Behaviour_t * data);

/*!
 * Fuegt ein Verhalten der Verhaltenliste anhand der Prioritaet ein.
 * @param list Die Speicherstelle an der die globale Verhaltensliste anfaengt
 * @param behave Einzufuegendes Verhalten
 */
void insert_behaviour_to_list(Behaviour_t **list, Behaviour_t *behave);

/*! 
 * Routine zum Registrieren einer Notfallfunktion, die beim Ausloesen eines Abgrundsensors
 * aufgerufen wird; hierdurch kann ein Verhalten vom Abgrund benachrichtigt werden und
 * entsprechend dem Verhalten reagieren
 * @param fkt die zu registrierende Routine, welche aufzurufen ist
 * @return Index, den die Routine im Array einnimmt, bei -1 ist alles voll
 */
int8_t register_emergency_proc(void* fkt);

/*! 
 * Beim Ausloesen eines Abgrundes wird diese Routine am Ende des Notfall-Abgrundverhaltens angesprungen 
 * und ruft alle registrierten Prozeduren der Reihe nach auf 
 */
void start_registered_emergency_procs(void); 

/*! 
 * @brief			Erzeugt ein neues Verhalten 
 * @param priority 	Die Prioritaet
 * @param *work 	Den Namen der Funktion, die sich drum kuemmert
 * @param active	Booleand, ob das Verhalten aktiv oder inaktiv erstellt wird
 */
Behaviour_t *new_behaviour(uint8 priority, void (*work) (struct _Behaviour_t *data), int8 active);


/* Includes aller verfuegbaren Verhalten */
#include "bot-logic/available_behaviours.h"


#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
	/*!
	 * @brief	Zeigt Informationen ueber Verhalten an, 'A' fuer Verhalten aktiv, 'I' fuer Verhalten inaktiv.
	 * @author 	Timo Sandmann (mail@timosandmann.de)
 	 * @date 	12.02.2007	 
 	 * Es werden zwei Spalten mit jeweils 4 Verhalten angezeigt. Gibt es mehr Verhalten in der Liste, kommt man 
 	 * mit der Taste DOWN auf eine weitere Seite (die aber kein extra Screen ist). Mit der Taste UP geht's bei Bedarf
 	 * wieder zurueck. Vor den Prioritaeten steht eine Nummer von 1 bis 8, drueckt man die entsprechende Zifferntaste
 	 * auf der Fernbedienung, so wird das Verhalten aktiv oder inaktiv geschaltet, komplementaer zum aktuellen Status.
 	 * Den Keyhandler dazu stellt beh_disp_key_handler() dar. 
	 */
	void behaviour_display(void);
#endif	// DISPLAY_BEHAVIOUR_AVAILABLE

#endif	// bot_logik_H_
