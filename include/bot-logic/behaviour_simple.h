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


/*! @file 	behaviour_simple.h
 * @brief 	ganz einfache Beispielverhalten
 * Diese Datei sollte der Einstiegspunkt fuer eigene Experimente sein, 
 * den Roboter zu steuern.
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	03.11.06
*/

#ifndef BEHAVIOUR_SIMPLE_H_
#define BEHAVIOUR_SIMPLE_H_

#include "ct-Bot.h"
#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
/*! 
 * Ein ganz einfaches Beispiel fuer ein Hilfsverhalten, 
 * das selbst SpeedWishes aussert und 
 * nach getaner Arbeit die aufrufende Funktion wieder aktiviert
 * Zudem prueft es, ob eine Uebergabebedingung erfuellt ist.
 * 
 * Zu diesem Verhalten gehoert die Botenfunktion bot_simple2()
 * 
 * Hier kann man auf ganz einfache Weise die ersten Schritte wagen. 
 * Wer die Moeglichkeiten des ganzen Verhaltensframeworks ausschoepfen will, kann diese Funktion getrost auskommentieren
 * und findet dann in bot_behave_init() und bot_behave() weitere Hinweise fuer elegante Bot-Programmierung....
 * 
 * Das Verhalten ist per default abgeschaltet. Daher muss man es erst in bot_behave_init() aktivieren. 
 * Dort steht aber bereits eine auskommentierte Zeile dazu, von der man nur die zwei Kommentarzeichen wegnehmen muss.
 * Achtung, da bot_simple2_behaviour() maximale Prioritaet hat, kommt es vor den anderen Regeln, wie dem Schutz vor Abgruenden, etc. zum Zuge
 * Das sollte am Anfang nicht stoeren, spaeter sollte man jedoch die Prioritaet herabsetzen.
 * 
 * bot_simple2_behaviour faehrt den Bot solange geradeaus, bis es dunkler als im Uebergabeparameter spezifiziert ist wird
 * 
 * @param *data der Verhaltensdatensatz
 */
void bot_simple2_behaviour(Behaviour_t *data);

/*! 
 * Ein ganz einfaches Verhalten, es hat maximale Prioritaet
 * Hier kann man auf ganz einfache Weise die ersten Schritte wagen. 
 * Wer die Moeglichkeiten des ganzen Verhaltensframeworks ausschoepfen will, kann diese Funktion getrost auskommentieren
 * und findet dann in bot_behave_init() und bot_behave() weitere Hinweise fuer elegante Bot-Programmierung....
 * 
 * Das Verhalten ist per default abgeschaltet. Daher muss man es erst in bot_behave_init() aktivieren. 
 * Dort steht aber bereits eine auskommentierte Zeile dazu, von der man nur die zwei Kommentarzeichen wegnehmen muss.
 * Achtung, da bot_simple_behaviour() maximale Prioritaet hat, kommt es vor den anderen Regeln, wie dem Schutz vor Abgruenden, etc. zum Zuge
 * Das sollte am Anfang nicht stoeren, spaeter sollte man jedoch die Prioritaet herabsetzen.
 * 
 * @param *data der Verhaltensdatensatz
 */
void bot_simple_behaviour(Behaviour_t *data);
#endif

#endif /*BEHAVIOUR_SIMPLE_H_*/
