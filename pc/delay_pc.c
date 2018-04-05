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
 * \file 	delay_pc.c
 * \brief 	Hilfsroutinen
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#ifdef PC
#define _POSIX_C_SOURCE 200809L // nanosleep

#include "ct-Bot.h"
#include "delay.h"

#ifdef WIN32
/* Windows */
#include <windows.h>
/** Sleep Funktion */
#define SLEEP(__value)	Sleep(__value)
#else
/* Linux */
#include <time.h>
/** Sleep Funktion */
#define SLEEP(__value) { struct timespec s; s.tv_sec = 0; s.tv_nsec = (__value) * 1000000L; nanosleep(&s, NULL); }
#endif // WIN32

/**
 * Verzoegert um ms Millisekunden
 * \param ms Anzahl der Millisekunden
 */
void delay(uint16_t ms) {
	int16_t wait = ms;
	int16_t steps = wait/999; // max sleep-time = 10^6-1 us
	int16_t i;
	/* in 999 ms Schritten warten */
	for (i=0; i<steps; i++) {
		SLEEP(999);
	}
	/* Rest warten */
	SLEEP(wait - i*999);
}
#endif // PC
