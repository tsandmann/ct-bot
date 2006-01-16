/*! @file 	bot-mot_pc.c
 * @brief 	Low-Level Routinen für die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include <stdlib.h>
#include "command.h"
#include "bot-2-sim.h"	
#include "motor-low.h"

/*!
 *  Initilisiert alles für die Motosteuerung 
 */
void motor_low_init(){
}

/*!
 * unmittelbarere Zugriff auf die beiden Motoren
 * normalerweise NICHT verwenden!!!!!
 * @param left PWM links
 * @param right PWM rechts
*/
void bot_motor(int16 left, int16 right){
	bot_2_sim_tell(CMD_AKT_MOT, SUB_CMD_NORM ,&left,&right);
}

#endif
