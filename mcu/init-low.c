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
 * \file 	init-low.c
 * \brief 	Initialisierungsroutinen fuer MCU
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	09.03.2010
 */

#include "eeprom.h"

uint8_t EEPROM resetsEEPROM = 0; /**< Reset-Counter im EEPROM */

#ifdef MCU
#include "ct-Bot.h"
#include "init.h"
#include "ui/available_screens.h"
#include "os_thread.h"
#include "led.h"
#include "ena.h"
#include "uart.h"
#include "motor.h"
#include "sensor-low.h"
#include "sdcard_wrapper.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <stdlib.h>

/** Kopie des MCU(C)SR Registers */
uint8_t mcucsr __attribute__((section(".noinit")));

#ifdef DISPLAY_RESET_INFO_AVAILABLE
uint8_t soft_resets __attribute__((section(".noinit"))); /**< Reset-Counter im RAM */
#endif // DISPLAY_RESET_INFO_AVAILABLE

void init_before_main(void) __attribute__((section(".init3"))) __attribute__((naked));

/**
 * MCU(C)SR auslesen und zuruecksetzen, Watchdog ausschalten
 */
void init_before_main(void) {
	PORTA = 0;
	DDRA  = 0; // Alles Eingang -> alles 0
	PORTB = 0;
	DDRB  = 0;
	PORTC = 0;
	DDRC  = 0;
	PORTD = 0;
	DDRD  = 0;

	/* unbenoetigte Module abschalten */
#if defined MCU_ATMEGA644X || defined __AVR_ATmega1284P__
    PRR0 =
#ifdef __AVR_ATmega644__
			0 // ATmega644 hat nur 1 UART
#else
			_BV(PRUSART1)
#endif // ATmega644
#ifndef I2C_AVAILABLE
			| _BV(PRTWI)
#endif // !I2C_AVAILABLE
#ifndef UART_AVAILABLE
			| _BV(PRUSART0)
#endif // !UART_AVAILABLE
#ifndef SPI_AVAILABLE
			| _BV(PRSPI)
#endif // !SPI_AVAILABLE
#ifndef ADC_AVAILABLE
			| _BV(PRADC)
#endif // !ADC_AVAILABLE
		;
#endif // MCU_ATMEGA644X || ATmega1284P

	/* Watchdog aus */
	wdt_reset();
#if defined MCU_ATMEGA644X || defined __AVR_ATmega1284P__
	MCUSR = (uint8_t) (MCUSR & ~_BV(WDRF));
	WDTCSR |= _BV(WDCE) | _BV(WDE);
	WDTCSR = 0;

	/* Statusregister sichern */
    mcucsr = MCUSR;
    MCUSR = 0;
#elif defined __AVR_ATmega32__
	MCUCSR = (uint8_t) (MCUCSR & ~_BV(WDRF));
	WDTCR |= _BV(WDTOE) | _BV(WDE);
	WDTCR = 0;

	/* Statusregister sichern */
	mcucsr = MCUCSR;
	MCUCSR = 0;
#else
#error "Nicht unterstuetzter MCU-Typ"
#endif // MCU-Typ

#ifdef DISPLAY_RESET_INFO_AVAILABLE
	if ((mcucsr & _BV(PORF)) == _BV(PORF)) {
		soft_resets = 0;
	}
	soft_resets++;
#endif // DISPLAY_RESET_INFO_AVAILABLE
}

#ifdef OS_AVAILABLE
/**
 * Hilfsfunktion, die beide Servos ausschaltet
 * @param p_data wird nicht ausgewertet
 */
static void servo_init_stop(void* p_data) {
	(void) p_data;

	servo_set(SERVO1, SERVO_OFF);
	servo_set(SERVO2, SERVO_OFF);
}
#endif // OS_AVAILABLE

/**
 * Hardwareabhaengige Initialisierungen, die zuerst ausgefuehrt werden sollen
 * \param argc Anzahl der Kommandozeilenparameter
 * \param *argv Zeiger auf Kommandozeilenparameter
 */
void ctbot_init_low_1st(int argc, char * argv[]) {
	/* keine Warnings */
	(void) argc;
	(void) argv;

	_delay_ms(100);

	uint8_t resets = (uint8_t) (ctbot_eeprom_read_byte(&resetsEEPROM) + 1);
#ifdef DISPLAY_RESET_INFO_AVAILABLE
	ctbot_eeprom_write_byte(&resetsEEPROM, resets);
#else
	(void) resets;
#endif // DISPLAY_RESET_INFO_AVAILABLE

#ifdef OS_AVAILABLE
	const uint8_t sreg = SREG;
	__builtin_avr_cli();

	os_create_thread((void*) SP, NULL); // Hauptthread anlegen
#ifdef OS_DEBUG
	void* ptr = malloc(1);
	if (ptr) {
		os_mask_stack(ptr + __malloc_margin, SP - (size_t) ptr);
		free(ptr);
	}
#endif // OS_DEBUG
#ifdef OS_KERNEL_LOG_AVAILABLE
	os_kernel_log_init();
#endif // OS_KERNEL_LOG_AVAILABLE
	SREG = sreg;
#endif // OS_AVAILABLE
}

/**
 * Hardwareabhaengige Initialisierungen, die _nach_ der allgemeinen Initialisierung
 * ausgefuehrt werden sollen
 */
void ctbot_init_low_last(void) {
#ifdef OS_AVAILABLE
#ifdef OS_DEBUG
	os_mask_stack(os_idle_stack, OS_IDLE_STACKSIZE);
#endif
	os_create_thread(&os_idle_stack[OS_IDLE_STACKSIZE - 1], os_idle);

	servo_set(SERVO1, DOOR_OPEN);
	servo_set(SERVO2, CAM_CENTER);
	if (os_delay_func(servo_init_stop, NULL, 3000)) {
		LED_on(LED_TUERKIS);
	}
#endif // OS_AVAILABLE

#ifdef EXPANSION_BOARD_MOD_AVAILABLE
   ENA_on(ENA_VOLTAGE_3V3); // Die 3,3V Versorgung ist standardmaessig eingeschaltet.
   ENA_on(ENA_DISPLAYLIGHT); // Die Displaybeleuchtung ist standardmaessig eingeschaltet.
#endif
}

/**
 * Faehrt den low-level Code des Bots sauber herunter
 */
void ctbot_shutdown_low() {
#if defined SDFAT_AVAILABLE && defined SPEED_LOG_AVAILABLE
	sdfat_close(speedlog_file);
#endif

#ifdef EXPANSION_BOARD_MOD_AVAILABLE
	ENA_off(ENA_VOLTAGE_3V3); // 3,3V Versorgung aus
	ENA_off(ENA_DISPLAYLIGHT); // Displaybeleuchtung aus
#endif

#ifdef UART_AVAILABLE
	while (uart_outfifo.count > 0) {} // Commands flushen
#endif

	__builtin_avr_cli();
	UCSRB = 0; // UART aus

#ifdef LED_AVAILABLE
	LED_off(0xff); // LEDs aus
#endif

	ENA_off(0xff); // Enable-Leitungen aus

	do {
		_SLEEP_CONTROL_REG = (uint8_t) (((_SLEEP_CONTROL_REG & ~(_BV(SM0) | _BV(SM1) | _BV(SM2))) | (SLEEP_MODE_PWR_DOWN)));
	} while (0);
	sleep_enable();
	sleep_cpu();

	/* kehrt nie zurueck */
}

#endif // MCU
