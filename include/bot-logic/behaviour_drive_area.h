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
 * \file 	behaviour_drive_area.h
 * \brief 	Flaechendeckendes Fahren als Verhalten (Staubsauger)
 *
 * Der Bot faehrt geradeaus bis zu einem Hindernis, dort dreht er sich zu einer Seite und faehrt auf der Nebenspur wieder zurueck.
 * Der andere moegliche Weg wird als Alternative auf den Stack gespeichert. Falls es irgendwo nicht weitergeht, so wird eine
 * solche Strecke vom Stack geholt und angefahren. Leider gibt es noch kein Map-Planungsverhalten, welches einen anderen Punkt
 * unter Umgehung von Hindernissen anfaehrt. In dieser Version wird etwas tricky gefahren und versucht, diese Strecke anzufahren.
 * Im Falle aber des nicht moeglichen Anfahrens wird eben diese Strecke verworfen. Ein Planungsverhalten, welches moeglichst auch
 * nur ueber befahrene Abschnitte plant, wuerde entscheidend helfen.
 * \author 	Frank Menzel (Menzelfr@gmx.net)
 * \date 	16.07.2008
 */

#ifndef BEHAVIOUR_DRIVE_AREA_H_
#define BEHAVIOUR_DRIVE_AREA_H_

#ifdef BEHAVIOUR_DRIVE_AREA_AVAILABLE
/*!
 * Observer links; jeweils ein selbstaendiges Verhalten, welches die Nachbarbahn beobachtet und eine befahrbare Strecke bis zu einem Hindernis
 * auf den Stack legt fuer spaeteres Anfahren; ebenfalls wird eine Alternativroute auf dem Stack gemerkt
 * Verhalten wurde so geschrieben, dass es zwar 2 Verhalten sind, der Code jedoch identisch ist und daher in Subroutinen ausgelagert ist
 * \param *data	Verhaltensdatensatz
 */
void bot_observe_left_behaviour(Behaviour_t * data);

/*!
 * Observer rechts; jeweils ein selbstaendiges Verhalten, welches die Nachbarbahn beobachtet und eine befahrbare Strecke bis zu einem Hindernis
 * auf den Stack legt fuer spaeteres Anfahren; ebenfalls wird eine Alternativroute auf dem Stack gemerkt
 * Verhalten wurde so geschrieben, dass es zwar 2 Verhalten sind, der Code jedoch identisch ist und daher in Subroutinen ausgelagert ist
 * \param *data	Verhaltensdatensatz
 */
void bot_observe_right_behaviour(Behaviour_t * data);

/*!
 * Das Fahrverhalten selbst; Fahren bis zu einem Hindernis, drehen zu einer Seite und merken des anderen Weges auf den Stack; waehrend der
 * Fahrt werden die Nebenspuren beobachtet und bei Hindernissen in der Nebenspur automatisch Teilstrecken auf den Stack gelegt
 * \param *data	Der Verhaltensdatensatz
 */
void bot_drive_area_behaviour(Behaviour_t * data);

/*!
 * Startet das Verhalten bot_drive_area_behaviour
 * \param *caller	Der obligatorische Verhaltensdatensatz des Aufrufers
 */
void bot_drive_area(Behaviour_t * caller);

/*!
 * Notfallhandler, ausgefuehrt bei Abgrunderkennung und muss registriert werden
 */
void border_drive_area_handler(void);

#endif // BEHAVIOUR_DRIVE_AREA_AVAILABLE
#endif // BEHAVIOUR_DRIVE_AREA_H_
