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
 * \file 	behaviour_drive_chess.h
 * \brief	Der Bot spielt Schach
 *
 * Das Schachverhalten ermoeglicht die manuelle Eingabe
 * eines Schachzuges des Spielers 1 (weiss) ueber das Screeninterface. Nach Betaetigen
 * der GO-Taste zieht der Bot-Spieler und der Bot faehrt auf dem Schachbrett diesen
 * Zug nach. Nun ist der menschliche Spieler wieder am Zug via Screeneingabe bzw. drueckt sofort GO,
 * um fuer sich den Zug machen zu lassen. Am besten ist es (weil ja die Schachfiguren nicht visualisiert werden),
 * sich parallel dazu ein echtes Schachbrett aufzubauen und manuell die Zuege nachzuvollziehen. In meinem ersten Test
 * musste ich eine Niederlage gegen den Bot einstecken:-)
 * Fuer das Verhalten ist das Chess-Layout zu laden, damit wenigstens die Schachfelder visualisiert werden und damit die
 * Botfahrten der gegnerischen Schachzuege. Die gruenen Felder stehen hierbei stellvertretend fuer die weissen Felder.
 * Inspiriert zu diesem Verhalten wurde ich durch folgende Seiten:
 * http://bralug.de/wiki/BLIT2008-Board_spielt_Schach
 * Detailliertes zu dem Micro-Max Schach ist hier zu finden:
 * http://home.hccnet.nl/h.g.muller/max-src2.html
 * 
 * Brainstorming fuer moegliche Erweiterungen:
 * Der Bot koennte gegen den anderen spielen ohne Mensch, d.h. Bot 1 ist Spieler 1 und der andere der Gegenspieler.
 * Jeder bot macht jeweils einen Zug und faehrt diesen ab. Danach ist der andere dran. Dies muesste/koennte dann ueber die
 * bot_2_bot-Kommunikation laufen. Noch schoener waere es natuerlich, echte Spielfiguren auf dem Java3D-Schachbrett zu haben
 * (oder zuerst auch einfache Objekte), wobei der bot sich jeweils die Figur greift (bot_catch_pillar) und an der Zielposition
 * ablaedt (bot_unload_pillar)...
 *
 * \author 	Frank Menzel (Menzelfr@gmx.de)
 * \date 	15.09.2009
 */
#ifndef BEHAVIOUR_DRIVE_CHESS_H_
#define BEHAVIOUR_DRIVE_CHESS_H_

#ifdef BEHAVIOUR_DRIVE_CHESS_AVAILABLE
/*!
 * Das eigentliche Schach-Fahrverhalten
 * Fuer den Bot-Spieler Schwarz wird nach dessen Zugberechnung der Zug vom Bot nachgefahren, faehrt also von der Zug-Startposition
 * zur Zug-Endposition. Das Verhalten ist dann damit abgeschlossen
 * @param *data	Der Verhaltensdatensatz
 */
void bot_drive_chess_behaviour(Behaviour_t * data);

/*!
 * Neustart des eigentlichen Schach-Fahrverhalten
 * Fuer den Bot-Spieler Schwarz wird nach dessen Zugberechnung der Zug vom Bot nachgefahren, faehrt also von der Zug-Startposition
 * zur Zug-Endposition. Das Verhalten ist dann damit abgeschlossen
 * @param *caller	Der Verhaltensdatensatz
 */
void bot_drive_chess(Behaviour_t * caller);

/*!
 * Start des Schach-Fahrverhalten nach GO-Betaetigung, d.h. nach Eingabe des Zuges wurde GO betaetigt
 * @param *caller	Der Verhaltensdatensatz
 */
void bot_drive_chess_go(Behaviour_t * caller);

/*!
 * Display zum Steuern des Schachverhaltens
 */
void drive_chess_display(void);

#endif // BEHAVIOUR_DRIVE_CHESS_AVAILABLE
#endif // BEHAVIOUR_DRIVE_CHESS_H_
