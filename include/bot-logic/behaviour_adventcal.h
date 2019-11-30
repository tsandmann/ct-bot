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
 * @author 	Benjamin Benz (bbe@heise.de) (Template "behaviour_prototype.h")
 * @author 	anonybotATriseupDOTnet (behaviour_adventcal.h)
 * @date 	2019-11-07
 */

//
#ifndef BEHAVIOUR_ADVENTCAL_H_
#define BEHAVIOUR_ADVENTCAL_H_

#ifdef BEHAVIOUR_ADVENTCAL_AVAILABLE
/*!
 * Verhalten für einen Adventskalender:
 * Der Bot faehrt eine Linie ab, auf der 24 Fotodosen stehen.
 * Faengt er einen Behaelter ein, bringt er diesen zum Startpunkt zurueck und
 * gibt ihn frei.
 * Da der Bot noch kein Verdauungssystem besitzt, duerfen die Sueszigkeiten,
 * die zuvor in den Dosen platziert wurden, von der Person gegessen werden,
 * die den Bot angeschaltet hatte.
 *
 * Als Test-Parcours im ct-Sim dient parcours/adventcal.xml - beim Nachbau fuer den realen Kalender sollte Folgendes beachtet werdne:
 * - zwischen Start-Position des Bots und erster Fotodose muss Abstand sein (10cm sollten reichen),
 * damit der Transportfach-Klappen-Servo wahrend der Fahrt seinen Arbeitsvorgang abschliessen kann, sonst wird das Verhalten zum Schliessen desselben aufgerufen, wenn er noch mit der Oeffnen-Aktion beschaeftig ist, wodurch das Schlieszen verhindert wird
 *
 * Bei der realen "Kalenderflaeche" muss die Ziel-Linie eventuell in doppelter Breite angelegt werden, also bei Verwendung von 1cm schwarzem Klebeband (fuer die Fahrtlinie wunderbar) ca. 2cm, damit der Oeffnungswinkel der Kanten-Sensoren genuegend Flaeche hat, sie zu erkennen.
 *
 * Das Verhalten geht davon aus, dass der Bot bereits eine ideale Startposition hat,
 * bevor er angeschaltet wird, woraufhin sich das Verhalten automatisch startet.
 * Die ideale Startposition ist:
 * - linker Linien-Sensor steht auf der schwarzen Linke
 * - rechter Linien-Sensor steht nicht auf der schwarzen Linke
 * - die erste Dose findet sich in Fahrtrichtung
 * - im Sim findet der Bot durch die optimalen Bedingungen die Linie auch in Aufruf-Position, falls doch nicht fuer "X [m]" den Wert "0.363" und fuer "Richtung" den Wert "0" verwenden
 * Dies bedeutet leider auch, dass der Bot nach dem Abliefern der Dose neu ideal positioniert werden muss.
 */
void bot_adventcal_behaviour(Behaviour_t * data);

/*!
 * Rufe das Adventskalender-Verhalten auf
 * @param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_adventcal(Behaviour_t * caller);


// Alternative Botenfunktion mit Uebergabeparameter
/*!
 * Rufe das Adventskalender-Verhalten auf
 * und nutze dabei einen Uebergabeparameter
 * @param *caller Der obligatorische Verhaltensdatensatz des Aufrufers
 * @param param Uebergabeparameter
 */

//void bot_adventcal(Behaviour_t * caller, int16_t param);

#endif // BEHAVIOUR_ADVENTCAL_AVAILABLE
#endif // BEHAVIOUR_ADVENTCAL_H_
