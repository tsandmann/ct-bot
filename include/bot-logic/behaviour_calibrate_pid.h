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
 * @file 	behaviour_calibrate_pid.h
 * @brief 	Kalibriert die Motorregelung des Bots
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	14.04.2007
 */

#ifndef BEHAVIOUR_CALIBRATE_PID_H_
#define BEHAVIOUR_CALIBRATE_PID_H_

#ifdef BEHAVIOUR_CALIBRATE_PID_AVAILABLE
#include "bot-logic/bot-logik.h"

uint16_t cal_pid_ete; /*!< verbleibende Zeit bis zum Ende der Kalibrierung in Sekunden */

/*!
 * @brief		Das eigentliche Verhalten
 * @param *data	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @see			bot_calibrate_pid()
 * Die Funktionalitaet des Verhaltens ist aufgeteilt in: 
 * @see find_Kp_region(), @see find_best_Kp_Ki(), @see find_best_Kp() und @see find_best_Kd()
 */
void bot_calibrate_pid_behaviour(Behaviour_t * data);

/*!
 * @brief			Kalibriert die Motorregelung des ct-Bots
 * @param *caller	Zeiger auf den Verhaltensdatensatz des Aufrufers
 * @param speed		Geschwindigkeit, mit der Kalibriert werden soll (normalerweise BOT_SPEED_SLOW)
 * Die ermittelten Parameter werden eingestellt, aber nicht dauerhaft gespeichert!
 */
void bot_calibrate_pid(Behaviour_t * caller, int16_t speed);

#endif	// BEHAVIOUR_CALIBRATE_PID_AVAILABLE
#endif	// BEHAVIOUR_CALIBRATE_PID_H_
