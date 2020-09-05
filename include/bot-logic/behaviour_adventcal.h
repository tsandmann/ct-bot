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
 * @file 	behaviour_adventcal.h
 * @brief 	Adventskalender-Verhalten
 * @author 	anonybot (anonybot@riseup.net), based on template "behaviour_prototype.h" by Benjamin Benz (bbe@heise.de), created with a lot of help by Timo Sandmann (mail@timosandmann.de)
 * @date 	2019-11-30
 */

//
#ifndef BEHAVIOUR_ADVENTCAL_H_
#define BEHAVIOUR_ADVENTCAL_H_

#ifdef BEHAVIOUR_ADVENTCAL_AVAILABLE
/*!
 * Verhalten für einen Adventskalender:
 * @param *data Der Verhaltensdatensatz
 *
 * Der Bot faehrt direkt nach dem Einschalten (da in bot-logic/bot-logic.c unter BEHAVIOUR_ADVENTCAL_AVAILABLE "BEHAVIOUR_ACTIVE" statt "BEHAVIOUR_INACTIVE" gesetzt ist)
 * eine Linie ab, auf der 24 Behaelter (bspw. Fotodosen) stehen.
 * Faengt er einen Behaelter ein, bringt er diesen zum Startpunkt zurueck und gibt ihn frei.
 * Da der Bot noch kein Verdauungssystem besitzt, duerfen die Sueszigkeiten,
 * die zuvor in den Dosen platziert wurden, von der Person gegessen werden,
 * die den Bot eingeschaltet hatte. ;)
 *
 * Zur Aktivierung muss in include/bot-logic/available_behaviours.h neben
 * - "BEHAVIOUR_ADVENTCAL_AVAILABLE"
 * auch
 * - "BEHAVIOUR_FOLLOW_LINE_AVAILABLE" aktiviert sein, damit das Verhalten funktioniert.
 * Achtung: Im Default wird davon ausgegangen, dass das Verhalten zunaechst im Sim ausprobiert wird,
 * weswegen fuer reale Tests in bot-logic/behaviour_adventcal.c fuer die Abbruch-Funktion "cancel_follow_line_on_border"
 * die entsprechend kommentierte Zeile deaktiviert bzw. aktiviert werden muss, damit der Bot die Linie in einer realen Umgebung findet.
 *
 * Als Test-Parcours im ct-Sim dient parcours/adventcal.xml - beim Nachbau fuer den realen Kalender sollte Folgendes beachtet werden:
 * - zwischen Start-Position des Bots und erster Fotodose muss ein gewisser Abstand sein, falls gewuenscht ist,
 * dass die Transportfach-Klappe nach dem Einfangen einer Fotodose ordentlich schliesst (zur Zeit
 * der Erstellung dieses Verhaltens waren das 15cm zwischen geschlossener Bot-Klappe und erster Fotodose).
 * Dies ist noetig, damit der Transportfach-Klappen-Servo wahrend der Fahrt unmittelbar nach dem Start genuegend Zeit hat,
 * seinen Arbeitsvorgang abschliessen zu koennen, da sonst das Verhalten zum Schliessen desselben aufgerufen wird,
 * waehrend der Klappen-Servo noch mit der Oeffnen-Aktion beschaeftig ist, wodurch das Schlieszen verhindert wird.
 *
 * Bei der realen "Kalenderflaeche" muss die Ziel-Linie eventuell in doppelter Breite angelegt werden, also bei Verwendung von
 * 1cm schwarzem Klebeband (fuer die Fahrtlinie wunderbar) ca. 2cm, damit der Oeffnungswinkel der Kanten-Sensoren genuegend
 * Flaeche hat, sie zu erkennen.
 *
 * Das Verhalten geht davon aus, dass der Bot bereits eine ideale Startposition hat,
 * bevor er angeschaltet wird, woraufhin sich das Verhalten automatisch startet.
 * Die ideale Startposition ist:
 * - linker Linien-Sensor steht auf der schwarzen Linke
 * - rechter Linien-Sensor steht nicht auf der schwarzen Linke
 * - die erste Dose findet sich in "Blickrichtung" des Bots
 * - im Sim findet der Bot durch die optimalen Bedingungen die Linie auch in Aufruf-Position,
 * falls doch nicht, muss im Sim nach Aufruf und vor Start des Bots
 * fuer "X [m]" der Wert "0.363" und fuer "Richtung" der Wert "0" eingestellt werden.
 * Dies bedeutet leider auch, dass der Bot nach dem Abliefern der Dose evtl. neu ideal positioniert werden muss,
 * falls er am Ende des Verhaltens nicht so zum Stehen gekommen ist, dass sich der linke Linien-Sensor auf der schwarzen Linie
 * und der rechte Linien-Sensor nicht auf der schwarzen Linie befindet.
 */
void bot_adventcal_behaviour(Behaviour_t * data);

/*!
 * Rufe das Adventskalender-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_adventcal(Behaviour_t * caller);

#endif // BEHAVIOUR_ADVENTCAL_AVAILABLE
#endif // BEHAVIOUR_ADVENTCAL_H_
