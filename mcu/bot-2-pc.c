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
 * @file 	bot-2-pc.c 
 * @brief 	Verbindung zwischen c't-Bot und PC
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	28.2.06
 */

#include "ct-Bot.h"
#include "command.h"
#include "bot-2-bot.h"
#include "uart.h"
#include "bot-2-pc.h"
#include "sensor.h"
#include "motor.h"
#include "led.h"
#include "mouse.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef MCU
#ifdef BOT_2_PC_AVAILABLE

/*! 
 * Diese Funktion nimmt die Daten vom PC entgegen
 * und wertet sie aus. Dazu nutzt sie die Funktion command_evaluate()
 */
void bot_2_pc_listen(void) {
//	LOG_DEBUG("%d bytes recvd",uart_data_available());
	if (uart_data_available() >= sizeof(command_t)) {
//		LOG_DEBUG("%d bytes recvd",uart_data_available());
		if (command_read() == 0) {
//			LOG_DEBUG("command received");
			command_evaluate();
		} else {
			// Fehler werden bereits in command_read() per LOG ausgegeben
		}
	}
}

/*! 
 * Diese Funktion informiert den PC ueber alle Sensor und Aktuator-Werte
 */
void bot_2_pc_inform(void) {
	static int16_t value1 = 0;
	command_write(CMD_AKT_MOT, SUB_CMD_NORM, &speed_l, &speed_r, 0);		
	command_write(CMD_SENS_IR, SUB_CMD_NORM, &sensDistL, &sensDistR, 0);
	command_write(CMD_SENS_ENC, SUB_CMD_NORM, (int16_t *)&sensEncL, (int16_t *)&sensEncR, 0);
	command_write(CMD_SENS_BORDER, SUB_CMD_NORM, &sensBorderL, &sensBorderR, 0);
	command_write(CMD_SENS_LINE, SUB_CMD_NORM, &sensLineL, &sensLineR, 0);
	command_write(CMD_SENS_LDR, SUB_CMD_NORM , &sensLDRL, &sensLDRR, 0);
	
	#ifdef LED_AVAILABLE
		value1 = led;
		command_write(CMD_AKT_LED, SUB_CMD_NORM, &value1, NULL, 0);
	#endif
	value1 = sensTrans;
	command_write(CMD_SENS_TRANS, SUB_CMD_NORM, &value1, NULL, 0);
	value1 = sensDoor;
	command_write(CMD_SENS_DOOR, SUB_CMD_NORM, &value1, NULL, 0);
	value1 = sensError;
	command_write(CMD_SENS_ERROR, SUB_CMD_NORM, &value1, NULL, 0);
	
#ifdef MOUSE_AVAILABLE
	static int16_t value2;
	value1 = sensMouseDX;
	value2 = sensMouseDY;
	command_write(CMD_SENS_MOUSE, SUB_CMD_NORM, &value1, &value2, 0);
#endif	
	
	command_write(CMD_DONE, SUB_CMD_NORM, NULL, 0, 0);
}

/*! 
 * Meldet den Bot am c't-Sim an
 */
void bot_2_pc_init(void) {	
	uart_init();
	
	command_init();
}

#endif	// BOT_2_PC_AVAILABLE
#endif	// MCU

