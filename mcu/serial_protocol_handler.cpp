/*
 * serial_protocol_handler.cpp
 *
 *  Created on: 26.12.2016
 *      Author: ts
 */

#include "ct-Bot.h"

#ifdef BOT_2_RPI_AVAILABLE

#include "serial_protocol_handler.h"
#include "serial_connection_avr.h"
#include "serial_protocol.h"
#include "ll_command.h"
#include "crc_engine.h"
#include <sstream>
#include <avr/sleep.h>


//#define TEST_SERIAL /**< Test code for serial protocol (ping / pong) */

namespace ctbot {

/** \todo check size of recv buffer */
static char recv_buf[100];
static CommandSens* p_cmd_sens(nullptr);
static CommandAct* p_cmd_act(nullptr);
static CommandLcd* p_cmd_lcd(nullptr);

extern "C" {
#include "sensor.h"
#include "motor.h"
#include "motor-low.h"
#include "led.h"
#include "display.h"
#include "gui.h"
#include "botcontrol.h"
#include "os_thread.h"
#include "command.h"


void update_sens_cmd() {
	static CommandSens cmd({ 0, 0, 0, false, false, false, 0, 0, 0, 0, 0, 0, 0, 0, 0 });

	cmd = CommandSens({ sensEncL, sensEncR, RC5_Code, sensDoor, sensError, sensTrans, static_cast<uint8_t>(sensBPS & 0xf),
		static_cast<uint16_t>(sensDistL), static_cast<uint16_t>(sensDistR), static_cast<uint16_t>(sensBorderL), static_cast<uint16_t>(sensBorderR),
		static_cast<uint16_t>(sensLineL), static_cast<uint16_t>(sensLineR),	static_cast<uint16_t>(sensLDRL), static_cast<uint16_t>(sensLDRR) });

	p_cmd_sens = &cmd;
}

int process_recv_cmd() {
	if (p_cmd_act) {
		motor_set(p_cmd_act->get_data().get_motor_l(), p_cmd_act->get_data().get_motor_r());
		servo_set(1, p_cmd_act->get_data().get_servo1());
		servo_set(2, p_cmd_act->get_data().get_servo2());
		LED_set(p_cmd_act->get_data().get_leds());
		if (p_cmd_act->get_data().get_shutdown()) {
			ctbot_shutdown();
		}
		p_cmd_act = nullptr;
		return 1;
	} else if (p_cmd_lcd) {
#ifdef DISPLAY_MCU_AVAILABLE
		if (screen_functions[display_screen] == linux_display) {
			const auto ctrl(p_cmd_lcd->get_data().get_ctrl());
			switch (ctrl) {
			case 0:
				display_clear();
				break;

			case 1:
				display_cursor(p_cmd_lcd->get_data().get_cursor_r() + 1, p_cmd_lcd->get_data().get_cursor_c() + 1);
				break;

			case 2:
				display_cursor(p_cmd_lcd->get_data().get_cursor_r() + 1, p_cmd_lcd->get_data().get_cursor_c() + 1);
				for (auto i(0U); i < CommandLcd::Type::LINE_SIZE; ++i) {
					const auto tmp(p_cmd_lcd->get_data().get_text()[i]);
					if (! tmp) {
						break;
					}
					display_data(tmp);
				}
				break;

			default:
				break;
			}

		}
#endif // DISPLAY_MCU_AVAILABLE
		p_cmd_lcd = nullptr;
		return 2;
	}

	return -1;
}

int serial_protocol_handler() {
	static SerialConnectionAVR con;
	static SerialProtocol protocol(con, 5, 500);
	static ctbot::SerialProtocol::header head;

	if (! protocol.slave_listen(head)) {
		return -100;
	}

	char* buffer(nullptr);
	if (static_cast<SerialProtocol::header_types>(head.type) == SerialProtocol::header_types::REQUEST) {
#ifndef TEST_SERIAL
		if (head.tag == 0) {
			buffer = reinterpret_cast<char*>(p_cmd_sens);
		} else {
			return -110 - head.tag;
		}
#else
		buffer = recv_buf;
#endif // ! TEST_SERIAL
	} else if (static_cast<SerialProtocol::header_types>(head.type) == SerialProtocol::header_types::SEND) {
		buffer = recv_buf;
		if (head.tag == 1) {
			p_cmd_act = reinterpret_cast<CommandAct*>(recv_buf);
			p_cmd_lcd = nullptr;
		} else if (head.tag == 2) {
			p_cmd_lcd = reinterpret_cast<CommandLcd*>(recv_buf);
			p_cmd_act = nullptr;
		} else {
			return -120 - head.tag;
		}
	} else {
		buffer = recv_buf;
		p_cmd_act = nullptr;
		p_cmd_lcd = nullptr;
		return -130;
	}

	const auto res(protocol.slave_process_request(head, buffer, sizeof(recv_buf)));
	return res;
}

static void wait_handler(const void*) {
	static uint16_t led_state = 0;
	static uint8_t servo_state = 0;

	if (led_state < 1024) {
		if (((PINB >> 2) & 1) == 1) { // error sensor
			LED_on(LED_GRUEN | LED_ORANGE);
		} else {
			LED_off(LED_GRUEN);
			LED_on(LED_ROT);
		}
	} else {
		LED_off(LED_ORANGE | LED_ROT);
	}
	++led_state;
	led_state %= 2048;

	if (! servo_state && TICKS_TO_MS(TIMER_GET_TICKCOUNT_32) > 2000UL) {
		servo_set(SERVO1, SERVO_OFF);
		servo_set(SERVO2, SERVO_OFF);
		servo_state = 1;
	}

	_SLEEP_CONTROL_REG = (uint8_t) (((_SLEEP_CONTROL_REG & ~(_BV(SM0) | _BV(SM1) | _BV(SM2))) | (SLEEP_MODE_IDLE)));
	sleep_enable();
	sleep_cpu();
}

void bot_2_linux_init(void) {
	LED_set(0);
#ifdef DISPLAY_AVAILABLE
	display_clear();
	display_printf("*** Waiting for");
	display_cursor(2, 1);
	display_printf("*** connection...");
#endif // DISPLAY_AVAILABLE

	if (((PINB >> 2) & 1) == 1) { // error sensor
		LED_on(LED_GRUEN | LED_ORANGE);
	} else {
		LED_on(LED_ROT);
#ifdef DISPLAY_AVAILABLE
		display_cursor(3, 1);
		display_printf("Low battery!");
#endif // DISPLAY_AVAILABLE
	}

	SerialConnectionAVR::set_wait_callback(wait_handler);

	servo_low(SERVO1, DOOR_OPEN);
	servo_low(SERVO2, CAM_CENTER);
}

void bot_2_linux_listen(void) {
#ifndef TEST_SERIAL
	update_sens_cmd();
#endif

	int res;
	int processed;
	do {
		processed = 0;
		res = serial_protocol_handler();
#ifdef DISPLAY_AVAILABLE
//		display_clear();
//		display_printf("SP: %4d", res);
#endif // DISPLAY_AVAILABLE
		if (res == 1) {
#ifndef TEST_SERIAL
			processed = process_recv_cmd();
#endif
			if (processed < 0) {
				ctbot_shutdown();
			}
		}
	} while (processed != 1);

	SerialConnectionAVR::set_wait_callback(nullptr);

	os_thread_running->lastSchedule = (uint16_t) (TIMER_GET_TICKCOUNT_32 - (MS_TO_TICKS(OS_TIME_SLICE) + 1)); // don't sleep on os_thread_yield()
}

void bot_2_linux_inform(void) {
	// empty
}

void linux_display(void) {
	// empty, display update is done by process_recv_cmd()
}

} /* extern C */
} /* namespace ctbot */

#endif // BOT_2_RPI_AVAILABLE
