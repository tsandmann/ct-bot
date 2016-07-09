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
 * \file 	bot-2-linux.h
 * \brief 	Verbindung zwischen c't-Bot und Linux-Board (z.B. BeagleBoard oder Raspberry Pi)
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.08.2009
 */
#ifndef BOT2LINUX_H_
#define BOT2LINUX_H_

#include "ct-Bot.h"
#if defined MCU && defined BOT_2_RPI_AVAILABLE

/**
 * Initialisiert die Kommunikation
 */
void bot_2_linux_init(void);

/**
 * Empfaengt alle Kommondos vom Linux-Board.
 * Die Funktion nimmt die Daten vom Linux-Board entgegen und
 * wertet sie aus. Dazu nutzt sie die Funktion command_evaluate().
 */
void bot_2_linux_listen(void);

/**
 * Diese Funktion informiert den Steuercode auf dem Linux-Board ueber alle Sensor- und Aktuator-Werte
 */
void bot_2_linux_inform(void);

/**
 * Display Screen fuer Inhalte vom Linux-Board
 */
void linux_display(void);

#endif // MCU && BOT_2_RPI_AVAILABLE
#endif // BOT2LINUX_H_
