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
 * @file 	init.c
 * @brief 	Initialisierungsroutinen
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	09.03.2010
 */

#include "init.h"
#include "timer.h"
#include "uart.h"
#include "bot-2-sim.h"
#include "display.h"
#include "led.h"
#include "motor.h"
#include "ena.h"
#include "mmc.h"
#include "sensor-low.h"
#include "ir-rc5.h"
#include "mouse.h"
#include "map.h"
#include "log.h"
#include "i2c.h"
#include "twi.h"
#include "gui.h"

/*!
 * Initialisierung
 */
void ctbot_init(int argc, char * argv[]) {
	ctbot_init_low_1st(argc, argv);

	timer_2_init();

#ifdef UART_AVAILABLE
	uart_init();
#endif
#ifdef BOT_2_SIM_AVAILABLE
	bot_2_sim_init();
#endif
#ifdef DISPLAY_AVAILABLE
	display_init();
#endif
#ifdef LED_AVAILABLE
	LED_init();
#endif
	motor_init();
#ifdef ENA_AVAILABLE
	ENA_init();
#endif
#ifdef MMC_AVAILABLE
	mmc_init();
#endif
	bot_sens_init();
#ifdef BEHAVIOUR_AVAILABLE
	bot_behave_init();
#endif
#ifdef RC5_AVAILABLE
	ir_init(&RC5_PORT, &RC5_DDR, RC5_PIN);
#endif
#ifdef BPS_AVAILABLE
	ir_init(&BPS_PORT, &BPS_DDR, BPS_PIN);
#endif
#ifdef MOUSE_AVAILABLE
	mouse_sens_init();
#endif
#ifdef MAP_AVAILABLE
	map_init();
#endif
#ifdef LOG_MMC_AVAILABLE
	log_mmc_init();
#endif
#ifdef I2C_AVAILABLE
	i2c_init(42); // 160 kHz
#endif
#ifdef TWI_AVAILABLE
	Init_TWI();
#endif
#ifdef DISPLAY_AVAILABLE
	gui_init();
#endif

	ctbot_init_low_last();
}
