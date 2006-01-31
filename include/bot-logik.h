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

/*! @file 	bot-logik.h
 * @brief 	High-Level-Routinen fuer die Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#ifndef bot_logik_H_
#define bot_logik_H_

extern volatile int16 target_speed_l;	/*!< Sollgeschwindigkeit linker Motor */
extern volatile int16 target_speed_r;	/*!< Sollgeschwindigkeit rechter Motor */

/*!
 * Drehe die Raeder um die gegebene Zahl an Encoder-Schritten weiter
 * @param left Schritte links
 * @param right Schritte rechts
 */
extern void bot_goto(int16 left, int16 right);

/*!
 * Kuemmert sich intern um die Ausfuehrung der goto-Kommandos
 * @see bot_goto()
 */
extern void bot_behave(void);

/*!
 * Initilaisert das ganze Verhalten
 */
extern void bot_behave_init(void);

#endif
