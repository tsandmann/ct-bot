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
 * \file 	bot-logic.h
 * \brief 	High-Level-Routinen fuer die Steuerung des c't-Bots
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	01.12.2005
 */

#ifndef BOT_LOGIC_H_
#define BOT_LOGIC_H_

#include "ct-Bot.h"

#ifdef BEHAVIOUR_AVAILABLE
#include "motor.h"
#include "sensor.h"

#define INACTIVE	0	/*!< Verhalten ist aus */
#define ACTIVE		1	/*!< Verhalten ist an */

#define NOOVERRIDE	(0<<0)	/*!< Konstanten, wenn Verhalten beim Aufruf alte Wuensche nicht ueberschreiben sollen */
#define OVERRIDE	(1<<0)	/*!< Konstante, wenn Verhalten beim Aufruf alte Wuensche ueberschreiben sollen */
#define FOREGROUND	(0<<1)	/*!< Konstante, wenn Verhalten im Vordergrund laufen sollen (default), also z.B. die Motoren beeinflussen */
#define BACKGROUND	(1<<1)	/*!< Konstante, wenn Verhalten im Hintergrund laufen sollen */

#define SUBSUCCESS	1	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe erfolgreich abgeschlossen */
#define SUBFAIL		0	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe nicht abgeschlossen */
#define SUBRUNNING 	2	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe wird noch beabeitet */
#define SUBCANCEL	3	/*!< Konstante fuer Behaviour_t->subResult: Aufgabe wurde unterbrochen */
#define SUBBACKGR	4	/*!< Konstange fuer Behaviour_t->subResult: Aufgabe wird im Hintergrund bearbeitet */

#define RECURSIVE	255	/*!< Konstante, die anzeigt, dass auch die Aufrufer eines Verhaltens mit deaktiviert werden */
#define NORECURSIVE 0	/*!< Konstante, die anzeigt, dass die Aufrufer eines Verhaltens nicht mit deaktiviert werden */


typedef struct _Behaviour_t {
   void (* work) (struct _Behaviour_t * data); 	/*!< Zeiger auf die Funktion, die das Verhalten bearbeitet */
   uint8_t priority;							/*!< Prioritaet */
   struct _Behaviour_t * caller;				/*!< aufrufendes Verhalten */
   unsigned active:1;							/*!< Ist das Verhalten aktiv */
   unsigned subResult:3;						/*!< War das aufgerufene unterverhalten erfolgreich (==1)? */
   struct _Behaviour_t * next;					/*!< Naechster Eintrag in der Liste */
} PACKED Behaviour_t; /*!< Verwaltungsstruktur fuer die Verhaltensroutinen */

/*! Dieser Typ definiert eine Funktion die das eigentliche Verhalten ausfuehrt */
typedef void (* BehaviourFunc)(Behaviour_t * data);

typedef struct {
	unsigned override:1;	/*!< 0 wenn Verhalten beim Aufruf alte Wuensche nicht ueberschreiben sollen; 1 sonst */
	unsigned background:1;	/*!< 0 wenn Verhalten im Vordergrund laufen sollen (default), also z.B. die Motoren beeinflussen; 1 sonst */
} PACKED behaviour_mode_t;

extern int16_t speedWishLeft;		/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit links */
extern int16_t speedWishRight;	/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit rechts */

extern float factorWishLeft;	/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor links */
extern float factorWishRight;	/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor rechts */

extern int16_t target_speed_l;	/*!< Sollgeschwindigkeit linker Motor */
extern int16_t target_speed_r;	/*!< Sollgeschwindigkeit rechter Motor */

/*!
 * Zentrale Verhaltens-Routine, wird regelmaessig aufgerufen.
 */
void bot_behave(void);

/*!
 * Initilaisert das ganze Verhalten
 */
void bot_behave_init(void);

/*!
 * Liefert das Verhalten zurueck, welches durch function implementiert ist
 * \param function	Die Funktion, die das Verhalten realisiert
 * \return			Zeiger auf Verhaltensdatensatz oder NULL
 */
Behaviour_t * get_behaviour(BehaviourFunc function);

/*!
 * Zu ein Verhalten mit der gegebenen Prioritaet
 * \param prio	Prioritaet des gesuchten Verhaltens
 * \return		Zeiger auf Verhaltensdatensatz oder NULL
 */
Behaviour_t * get_behaviour_from_prio(uint8_t prio);

/*!
 * Deaktiviert eine Regel mit gegebener Funktion
 * \param function Die Funktion, die das Verhalten realisiert.
 */
void deactivateBehaviour(BehaviourFunc function);

/*!
 * Rueckgabe von True, wenn das Verhalten gerade laeuft (aktiv ist) sonst False
 * \param function Die Funktion, die das Verhalten realisiert.
 * \return True wenn Verhalten aktiv sonst False
 */
uint8_t behaviour_is_activated(BehaviourFunc function);

/*!
 * Deaktiviert alle Verhalten bis auf Grundverhalten. Bei Verhaltensauswahl werden die Aktivitaeten vorher
 * in die Verhaltens-Auswahlvariable gesichert.
 */
void deactivateAllBehaviours(void);

/*!
 * Deaktiviert alle von diesem Verhalten aufgerufenen Verhalten.
 * Das Verhalten selbst bleibt aktiv und bekommt ein SUBCANCEL in seine Datanestruktur eingetragen.
 * \param function	Die Funktion, die das Verhalten realisiert.
 */
void deactivateCalledBehaviours(BehaviourFunc function);

/*!
 * Ruft ein anderes Verhalten auf und merkt sich den Ruecksprung
 * return_from_behaviour() kehrt dann spaeter wieder zum aufrufenden Verhalten zurueck
 * \param *from		aufrufendes Verhalten
 * \param *to		aufgerufenes Verhalten
 * \param mode		Hier sind vier Werte moeglich:
 * 		1. OVERRIDE:	Das Zielverhalten to wird aktiviert, auch wenn es noch aktiv ist.
 *						Das Verhalten, das es zuletzt aufgerufen hat wird dadurch automatisch
 *						wieder aktiv und muss selbst sein eigenes Feld subResult auswerten, um zu pruefen, ob das
 *						gewuenschte Ziel erreicht wurde, oder vorher ein Abbruch stattgefunden hat.
 * 		2. NOOVERRIDE:	Das Zielverhalten wird nur aktiviert, wenn es gerade nichts zu tun hat.
 *						In diesem Fall kann der Aufrufer aus seinem eigenen subResult auslesen,
 *						ob seinem Wunsch Folge geleistet wurde.
 *		3. FOREGROUND	Das Verhalten laeuft im Fordergrund (Aufrufer wird solange deaktiviert)
 *		4. BACKGROUND	Das Verhalten laeuft im Hintergrund (Aufrufer bleibt aktiv)
 */
void switch_to_behaviour(Behaviour_t * from, void (* to)(Behaviour_t *), uint8_t mode);

/*!
 * Aktiviert eine Regel mit gegebener Funktion, impliziert NOOVERRIDE.
 * Im Gegensatz zu switch_to_behaviour() wird der Aufrufer jedoch nicht deaktiviert (Hintergrundausfuehrung).
 * \param *from		aufrufendes Verhalten
 * \param *to		aufgerufendes Verhalten
 */
static inline void activateBehaviour(Behaviour_t * from, void (* to)(Behaviour_t *)) {
	switch_to_behaviour(from, to, NOOVERRIDE | BACKGROUND);
}

/*!
 * Kehrt zum aufrufenden Verhalten zurueck und setzt den Status auf Erfolg oder Misserfolg
 * \param *data	laufendes Verhalten
 * \param state	Abschlussstatus des Verhaltens (SUBSUCCESS oder SUBFAIL)
 */
void exit_behaviour(Behaviour_t * data, uint8_t state);

/*!
 * Kehrt zum aufrufenden Verhalten zurueck
 * \param *data laufendes Verhalten
 */
static inline void return_from_behaviour(Behaviour_t * data) {
	exit_behaviour(data, SUBSUCCESS);
}

/*!
 * Beim Ausloesen eines Notfalls wird diese Routine angesprungen
 * und ruft alle registrierten Prozeduren der Reihe nach auf
 */
void start_registered_emergency_procs(void);

/*!
 * Gibt das naechste Verhalten der Liste zurueck
 * \param *beh	Zeiger auf Verhalten, dessen Nachfolger gewuenscht ist, NULL fuer Listenanfang
 * \return		Zeiger auf Nachfolger von beh
 */
Behaviour_t * get_next_behaviour(Behaviour_t * beh);

/* Includes aller verfuegbaren Verhalten */
#include "bot-logic/available_behaviours.h"

/*!
 * Zeigt Informationen ueber Verhalten an, 'A' fuer Verhalten aktiv, 'I' fuer Verhalten inaktiv.
 * Es werden zwei Spalten mit jeweils 4 Verhalten angezeigt. Gibt es mehr Verhalten in der Liste, kommt man
 * mit der Taste DOWN auf eine weitere Seite (die aber kein extra Screen ist). Mit der Taste UP geht's bei Bedarf
 * wieder zurueck. Vor den Prioritaeten steht eine Nummer von 1 bis 8, drueckt man die entsprechende Zifferntaste
 * auf der Fernbedienung, so wird das Verhalten aktiv oder inaktiv geschaltet, komplementaer zum aktuellen Status.
 * Den Keyhandler dazu stellt beh_disp_key_handler() dar.
 */
void behaviour_display(void);

#endif // BEHAVIOUR_AVAILABLE
#endif // BOT_LOGIC_H_
