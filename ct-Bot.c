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
 * \file 	ct-Bot.c
 * \brief 	Bot-Hauptprogramm
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#include "ct-Bot.h"
#include "init.h"
#include "botcontrol.h"
#include "bot-logic.h"


/**
 * Hauptprogramm des Bots. Diese Schleife kuemmert sich um seine Steuerung.
 * \param argc Anzahl der Kommandozeilenparameter
 * \param *argv Array aller Kommandozeilenparameter
 * \return Exit-Code
 */
int main(int argc, char * argv[]) {
	/* alles initialisieren */
	ctbot_init(argc, argv);

	/* Hauptschleife des Bots */
	for (;;) {
		pre_behaviour();

#ifdef BEHAVIOUR_AVAILABLE
		bot_behave(); // hier drin steckt der Verhaltenscode, siehe bot-logic/bot-logic.c
#endif

		post_behaviour();
	}

	return 0;
}
