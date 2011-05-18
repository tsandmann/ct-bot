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
#include "motor.h"
#include "bot-logic/bot-logic.h"


/**
 * Hauptprogramm des Bots. Diese Schleife kuemmert sich um seine Steuerung.
 */
int main(int argc, char * argv[]) {
	/* Alles initialisieren */
	ctbot_init(argc, argv);

#ifdef TEST_AVAILABLE_MOTOR
	uint16_t calls = 0;	// Im Testfall zaehle die Durchlaeufe
#endif

	/* Hauptschleife des Bots */
	for (;;) {
		pre_behaviour();

#ifdef TEST_AVAILABLE_MOTOR
/** \todo: Testcode in ein Verhalten verschieben */
		/* Testprogramm, das den Bot erst links-, dann rechtsrum dreht */
		if (calls < 1001) {
			calls++;
			if (calls == 1) {
				motor_set(BOT_SPEED_SLOW, -BOT_SPEED_SLOW);
			} else if (calls == 501) {
				motor_set(-BOT_SPEED_SLOW, BOT_SPEED_SLOW);
			} else if (calls == 1001) {
				motor_set(BOT_SPEED_STOP, BOT_SPEED_STOP);
			}
		} else
#endif // TEST_AVAILABLE_MOTOR
		{
#ifdef BEHAVIOUR_AVAILABLE
			/* hier drin steckt der Verhaltenscode */
			bot_behave();
#endif // BEHAVIOUR_AVAILABLE
		}

		post_behaviour();
	}
}
