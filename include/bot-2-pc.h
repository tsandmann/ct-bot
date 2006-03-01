/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	bot-2-pc.h 
 * @brief 	Verbindung c't-Bot zum PC
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.03.06
*/
#ifndef __bot_2_pc
#define __bot_2_pc

#include "global.h"

/*! 
 * Diese Funktion nimmt die Daten vom PC entgegen
 * und wertet sie aus. dazu nutzt er die Funktion command_evaluate()
 */
void bot_2_pc_listen(void);

/*! 
 * Diese Funktion informiert den PC ueber alle Sensor und Aktuator-Werte
 */
void bot_2_pc_inform(void);

#endif
