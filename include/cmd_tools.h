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
 * @file 	cmd_tools.h
 * @brief 	Funktionen, die per Commandline-Switch aufgerufen werden koennen
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	19.02.2008
 */

#ifndef CMD_TOOLS_H_
#define CMD_TOOLS_H_

#ifdef PC
/*!
 * Behandelt die Kommandozeilen-Argumente
 * @param argc	Anzahl der Argumente
 * @param *argv	Zeiger auf String-Array der Argumente
 */
void hand_cmd_args(int argc, char * argv[]);

/*!
 * Initialisiert die Eingabekonsole fuer RemoteCalls
 */
void cmd_init(void);

#endif // PC
#endif // CMD_TOOLS_H_
