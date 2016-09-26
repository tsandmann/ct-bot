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
 * \file 	bot-logic.h
 * \brief 	High-Level-Routinen fuer die Steuerung des c't-Bots
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	01.12.2005
 */

#ifndef BOT_LOGIC_H_
#define BOT_LOGIC_H_

#include "ct-Bot.h"

#ifdef BEHAVIOUR_AVAILABLE
#define BEHAVIOUR_INACTIVE	0	/**< Verhalten ist aus */
#define BEHAVIOUR_ACTIVE	1	/**< Verhalten ist an */

#define BEHAVIOUR_NOOVERRIDE	(0 << 0) /**< Konstanten, wenn Verhalten beim Aufruf alte Wuensche nicht ueberschreiben sollen */
#define BEHAVIOUR_OVERRIDE		(1 << 0) /**< Konstante, wenn Verhalten beim Aufruf alte Wuensche ueberschreiben sollen */
#define BEHAVIOUR_FOREGROUND	(0 << 1) /**< Konstante, wenn Verhalten im Vordergrund laufen sollen (default), also z.B. die Motoren beeinflussen */
#define BEHAVIOUR_BACKGROUND	(1 << 1) /**< Konstante, wenn Verhalten im Hintergrund laufen sollen */

#define BEHAVIOUR_SUBFAIL		0 /**< Konstante fuer Behaviour_t->subResult: Aufgabe nicht abgeschlossen */
#define BEHAVIOUR_SUBSUCCESS	1 /**< Konstante fuer Behaviour_t->subResult: Aufgabe erfolgreich abgeschlossen */
#define BEHAVIOUR_SUBRUNNING	2 /**< Konstante fuer Behaviour_t->subResult: Aufgabe wird noch bearbeitet */
#define BEHAVIOUR_SUBCANCEL		3 /**< Konstante fuer Behaviour_t->subResult: Aufgabe wurde unterbrochen */
#define BEHAVIOUR_SUBBACKGR		4 /**< Konstange fuer Behaviour_t->subResult: Aufgabe wird im Hintergrund bearbeitet */


/** Verwaltungsstruktur fuer die Verhaltensroutinen */
typedef struct _Behaviour_t {
   void (* work) (struct _Behaviour_t * data); 	/**< Zeiger auf die Funktion, die das Verhalten bearbeitet */
   uint8_t priority;							/**< Prioritaet */
   struct _Behaviour_t * caller;				/**< aufrufendes Verhalten */
   unsigned active:1;							/**< Ist das Verhalten aktiv */
   unsigned subResult:3;						/**< War das aufgerufene Unterverhalten erfolgreich (==1)? */
   struct _Behaviour_t * next;					/**< Naechster Eintrag in der Liste */
} PACKED Behaviour_t;

/** Dieser Typ definiert eine Funktion die das eigentliche Verhalten ausfuehrt */
typedef void (* BehaviourFunc_t)(Behaviour_t * data);

typedef struct {
	unsigned override:1;	/**< 0 wenn Verhalten beim Aufruf alte Wuensche nicht ueberschreiben sollen; 1 sonst */
	unsigned background:1;	/**< 0 wenn Verhalten im Vordergrund laufen sollen (default), also z.B. die Motoren beeinflussen; 1 sonst */
} PACKED behaviour_mode_t;

extern int16_t target_speed_l;	/**< Sollgeschwindigkeit linker Motor */
extern int16_t target_speed_r;	/**< Sollgeschwindigkeit rechter Motor */
extern int16_t speedWishLeft;	/**< Puffervariable fuer die Verhaltensfunktionen absolute Geschwindigkeit links */
extern int16_t speedWishRight;	/**< Puffervariable fuer die Verhaltensfunktionen absolute Geschwindigkeit rechts */
extern float factorWishLeft;	/**< Puffervariable fuer die Verhaltensfunktionen Modifikationsfaktor links */
extern float factorWishRight;	/**< Puffervariable fuer die Verhaltensfunktionen Modifikationsfaktor rechts */


/**
 * Zentrale Verhaltens-Routine, wird regelmaessig aufgerufen.
 */
void bot_behave(void);

/**
 * Initialisiert alle Verhalten
 */
void bot_behave_init(void);

/**
 * Liefert das Verhalten zurueck, welches durch function implementiert ist
 * \param function	Die Funktion, die das Verhalten realisiert
 * \return			Zeiger auf Verhaltensdatensatz oder NULL
 */
Behaviour_t * get_behaviour(BehaviourFunc_t function);

/**
 * Zu ein Verhalten mit der gegebenen Prioritaet
 * \param prio	Prioritaet des gesuchten Verhaltens
 * \return		Zeiger auf Verhaltensdatensatz oder NULL
 */
Behaviour_t * get_behaviour_from_prio(uint8_t prio);

/**
 * Deaktiviert ein Verhalten
 * \param beh Das zu deaktivierende Verhalten
 */
void deactivate_behaviour(Behaviour_t * beh);

/**
 * Deaktiviert eine Regel mit gegebener Funktion
 * \param function Die Funktion, die das Verhalten realisiert.
 */
static inline void deactivateBehaviour(BehaviourFunc_t function) {
	deactivate_behaviour(get_behaviour(function));
}

/**
 * Rueckgabe von True, wenn das Verhalten gerade laeuft (aktiv ist), sonst False
 * \param function Die Funktion, die das Verhalten realisiert.
 * \return True wenn Verhalten aktiv, sonst False
 */
uint8_t behaviour_is_activated(BehaviourFunc_t function);

/**
 * Deaktiviert alle Verhalten bis auf Grundverhalten. Bei Verhaltensauswahl werden die Aktivitaeten vorher
 * in die Verhaltens-Auswahlvariable gesichert.
 */
void deactivateAllBehaviours(void);

/**
 * Deaktiviert alle von diesem Verhalten aufgerufenen Verhalten.
 * Das Verhalten selbst bleibt aktiv und bekommt ein BEHAVIOUR_SUBCANCEL in seine Datanestruktur eingetragen.
 * \param *caller Zeiger auf den Aufrufer
 */
void deactivate_called_behaviours(Behaviour_t * caller);

/**
 * Ruft ein anderes Verhalten auf und merkt sich den Ruecksprung
 * return_from_behaviour() kehrt dann spaeter wieder zum aufrufenden Verhalten zurueck
 * \param *from		aufrufendes Verhalten
 * \param *to		aufgerufenes Verhalten
 * \param mode		Hier sind vier Werte moeglich:
 * 		1. BEHAVIOUR_OVERRIDE:	Das Zielverhalten to wird aktiviert, auch wenn es noch aktiv ist.
 *						Das Verhalten, das es zuletzt aufgerufen hat wird dadurch automatisch
 *						wieder aktiv und muss selbst sein eigenes Feld subResult auswerten, um zu pruefen, ob das
 *						gewuenschte Ziel erreicht wurde, oder vorher ein Abbruch stattgefunden hat.
 * 		2. BEHAVIOUR_NOOVERRIDE:	Das Zielverhalten wird nur aktiviert, wenn es gerade nichts zu tun hat.
 *						In diesem Fall kann der Aufrufer aus seinem eigenen subResult auslesen,
 *						ob seinem Wunsch Folge geleistet wurde.
 *		3. BEHAVIOUR_FOREGROUND	Das Verhalten laeuft im Fordergrund (Aufrufer wird solange deaktiviert)
 *		4. BEHAVIOUR_BACKGROUND	Das Verhalten laeuft im Hintergrund (Aufrufer bleibt aktiv)
 * \return			Zeiger auf Verhaltensdatensatz des aufgerufenen Verhaltens, oder NULL im Fehlerfall
 */
Behaviour_t * switch_to_behaviour(Behaviour_t * from, void (* to)(Behaviour_t *), uint8_t mode);

/**
 * Aktiviert eine Regel mit gegebener Funktion, impliziert BEHAVIOUR_NOOVERRIDE.
 * Im Gegensatz zu switch_to_behaviour() wird der Aufrufer jedoch nicht deaktiviert (Hintergrundausfuehrung).
 * \param *from		aufrufendes Verhalten
 * \param *to		aufgerufendes Verhalten
 * \return			Zeiger auf Verhaltensdatensatz des aufgerufenen Verhaltens, oder NULL im Fehlerfall
 */
static inline Behaviour_t * activateBehaviour(Behaviour_t * from, void (* to)(Behaviour_t *)) {
	return switch_to_behaviour(from, to, BEHAVIOUR_NOOVERRIDE | BEHAVIOUR_BACKGROUND);
}

/**
 * Kehrt zum aufrufenden Verhalten zurueck und setzt den Status auf Erfolg oder Misserfolg
 * \param *data	laufendes Verhalten
 * \param state	Abschlussstatus des Verhaltens (BEHAVIOUR_SUBSUCCESS oder BEHAVIOUR_SUBFAIL)
 */
void exit_behaviour(Behaviour_t * data, uint8_t state);

/**
 * Kehrt zum aufrufenden Verhalten zurueck
 * \param *data laufendes Verhalten
 */
static inline void return_from_behaviour(Behaviour_t * data) {
	exit_behaviour(data, BEHAVIOUR_SUBSUCCESS);
}

/**
 * Beim Ausloesen eines Notfalls wird diese Routine angesprungen
 * und ruft alle registrierten Prozeduren der Reihe nach auf
 */
void start_registered_emergency_procs(void);

/**
 * Gibt das naechste Verhalten der Liste zurueck
 * \param *beh	Zeiger auf Verhalten, dessen Nachfolger gewuenscht ist, NULL fuer Listenanfang
 * \return		Zeiger auf Nachfolger von beh
 */
Behaviour_t * get_next_behaviour(Behaviour_t * beh);

/* Includes aller verfuegbaren Verhalten */
#include "bot-logic/available_behaviours.h"

#endif // BEHAVIOUR_AVAILABLE
#endif // BOT_LOGIC_H_
