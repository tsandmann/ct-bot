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
 * \file 	bot-2-atmega.h
 * \brief 	Verbindung ARM-Board zu ATmega
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	04.05.2013
 */

#ifndef BOT_2_ATMEGA_H_
#define BOT_2_ATMEGA_H_

#include "command.h"

/**
 * Fuehrt einen Reset auf dem ATmega aus
 * \return Fehlercode: 0 wenn alles OK
 */
int8_t atmega_reset(void);

/**
 * Initialisiert die Verbindung
 * \return Fehlercode: 0 korrekt initialisiert
 */
int8_t bot_2_atmega_init(void);

/**
 * Empfaengt alle Daten vom ATmega
 */
void bot_2_atmega_listen(void);

/**
 * Setzt den aktiven Kommunikationskanal auf UART
 */
void set_bot_2_atmega(void);

#endif // BOT_2_ATMEGA_H_
