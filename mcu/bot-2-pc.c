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
#include "uart.h"
#include "bot-2-pc.h"
#include "sensor.h"
#include "motor.h"
#include "led.h"
#include "mouse.h"
#include "log.h"
#include <stdlib.h>

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
void bot_2_pc_inform(void){
	int16 value1, value2;
	
	command_write(CMD_AKT_MOT, SUB_CMD_NORM ,(int16*)&speed_l,(int16*)&speed_r,0);	
	#ifdef LED_AVAILABLE
		value1=(int16)led;
		command_write(CMD_AKT_LED, SUB_CMD_NORM ,&value1,&value1,0);
	#endif
	
	command_write(CMD_SENS_IR, SUB_CMD_NORM ,(int16*)&sensDistL,(int16*)&sensDistR,0);
	command_write(CMD_SENS_ENC, SUB_CMD_NORM ,(int16*)&sensEncL,(int16*)&sensEncR,0);
	command_write(CMD_SENS_BORDER, SUB_CMD_NORM ,(int16*)&sensBorderL,(int16*)&sensBorderR,0);
	command_write(CMD_SENS_LINE, SUB_CMD_NORM ,(int16*)&sensLineL,(int16*)&sensLineR,0);

	command_write(CMD_SENS_LDR, SUB_CMD_NORM ,(int16*)&sensLDRL,(int16*)&sensLDRR,0);
	
	value1= (int16) sensTrans; value2=0;
	command_write(CMD_SENS_TRANS, SUB_CMD_NORM ,&value1,&value2,0);
	
	value1= (int16) sensDoor;
	command_write(CMD_SENS_DOOR, SUB_CMD_NORM ,&value1,&value2,0);
	
	#ifdef MAUS_AVAILABLE
		value1=(int16)sensMouseDX;
		value2=(int16)sensMouseDY;
		command_write(CMD_SENS_MOUSE, SUB_CMD_NORM ,&value1,&value2,0);
	#endif	
	
	value1=(int16)sensError;  value2=0;
	command_write(CMD_SENS_ERROR, SUB_CMD_NORM ,&value1,&value2,0);
//	command_write(CMD_SENS_RC5, SUB_CMD_NORM ,(int16*)&RC5_Code,&value2,0);
}



#include <stdio.h>
#include <string.h>

/*! 
 * Meldet den Bot am c't-Sim an
 */
void bot_2_pc_init(void){
	int16 null =0;
	uint8 j;
	
	uart_init();

	for(j=0;j<5;j++) 
		command_write(CMD_WELCOME, SUB_WELCOME_REAL ,&null,&null,0);
}

#endif
#endif

