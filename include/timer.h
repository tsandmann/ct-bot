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
 * @file 	timer.h
 * @brief 	Timer und Zaehler
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.2005
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "ct-Bot.h"
#include <stdint.h>
#include "log.h"
#include "display.h"

/*!
 * Makro zur Umrechnung von Ticks in ms
 * (ms / ticks evtl. nach uint32_t casten, fuer grosse Werte)
 */
#define TICKS_TO_MS(ticks)	((ticks)*(TIMER_STEPS/8)/(1000/8))

/*!
 * Makro zur Umrechnung von ms in Ticks
 * (ms / ticks evtl. nach uint32_t casten, fuer grosse Werte)
 */
#define MS_TO_TICKS(ms)		((ms)*(1000/8)/(TIMER_STEPS/8))

#ifdef TIME_AVAILABLE
/*!
 * Diese Funktion liefert den Millisekundenanteil der Systemzeit zurueck.
 * @return Millisekunden der Systemzeit
 */
uint16_t timer_get_ms(void);

/*!
 * Diese Funktion liefert den Sekundenanteil der Systemzeit zurueck.
 * @return Sekunden der Systemzeit
 */
uint16_t timer_get_s(void);

/*!
 * Liefert die Millisekunden zurueck, die seit old_s, old_ms verstrichen sind
 * @param old_s		alter Sekundenstand
 * @param old_ms	alter Millisekundenstand
 */
uint16_t timer_get_ms_since(uint16_t old_s, uint16_t old_ms);
#endif // TIME_AVAILABLE

#ifdef PC
/*!
 *  Funktion, die die TickCounts um die vergangene Simulzeit erhoeht
 */
void system_time_isr(void);

/*
 * Setzt die Systemzeit zurueck auf 0
 */
void timer_reset(void);

/*!
 * liefert Ticks in 16 Bit seit Systemstart [176 us]
 */
uint16_t timer_get_tickCount16(void);

/*!
 * liefert Ticks in 32 Bit seit Systemstart [176 us]
 */
uint32_t timer_get_tickCount32(void);

#define TIMER_GET_TICKCOUNT_8  (uint8_t)timer_get_tickCount16()	/*!< Zeit in 8 Bit */
#define TIMER_GET_TICKCOUNT_16 timer_get_tickCount16()			/*!< Zeit in 16 Bit */
#define TIMER_GET_TICKCOUNT_32 timer_get_tickCount32()			/*!< Zeit in 32 Bit */

#else	// MCU

/*! Union fuer TickCount in 8, 16 und 32 Bit */
typedef union {
	uint32_t u32;	/*!< 32 Bit Integer */
	uint16_t u16;	/*!< 16 Bit Integer */
	uint8_t u8;		/*!< 8 Bit Integer */
} tickCount_t;

extern volatile tickCount_t tickCount;			/*!< ein Tick alle 176 us */

/*!
 * Setzt die Systemzeit zurueck auf 0
 */
static inline void timer_reset(void) {
	tickCount.u32 = 0;
}

#define TIMER_GET_TICKCOUNT_8  tickCount.u8				/*!< Systemzeit [176 us] in 8 Bit */
#define TIMER_GET_TICKCOUNT_16 timer_get_tickcount_16()	/*!< Systemzeit [176 us] in 16 Bit */
#define TIMER_GET_TICKCOUNT_32 timer_get_tickcount_32()	/*!< Systemzeit [176 us] in 32 Bit */

/*!
 * Liefert die unteren 16 Bit der Systemzeit zurueck
 * @return	Ticks [176 us]
 */
static inline
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
uint16_t timer_get_tickcount_16(void) {
	uint8_t sreg = SREG;
	cli();
	uint16_t ticks = tickCount.u16;
	SREG = sreg;
	return ticks;
}

/*!
 * Liefert die vollen 32 Bit der Systemzeit zurueck
 * @return	Ticks [176 us]
 */
static inline
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
uint32_t timer_get_tickcount_32(void) {
	uint8_t sreg = SREG;
	cli();
	uint32_t ticks = tickCount.u32;
	SREG = sreg;
	return ticks;
}
#endif	// PC

// Die Werte fuer TIMER_X_CLOCK sind Angaben in Hz

/*! Frequenz von Timer 2 in Hz */
#define TIMER_2_CLOCK 5619	// Derzeit genutzt fuer RC5-Dekodierung

/*! Mikrosekunden, die zwischen zwei Timer-Aufrufen liegen */
#define TIMER_STEPS 176

/*!
 * Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind.
 * 32-Bit Version, fuer Code, der (teilweise) seltener als alle 11 s aufgerufen wird.
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 *
 * Die Funktion aktualisiert den Timestamp, der die alte Zeit zum Vergleich speichert, automatisch,
 * falls ms Millisekunden vergangen sind.
 * Man verwendet sie z.B. wie folgt:
 * static uint32_t old_time;
 * ...
 * if (timer_ms_passed(&old_time, 50)) {
 * 		// wird alle 50 ms ausgefuehrt //
 * }
 */
static inline uint8_t
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
timer_ms_passed_32(uint32_t * old_ticks, uint32_t ms) {
	uint32_t ticks = TIMER_GET_TICKCOUNT_32;
	if ((uint32_t)(ticks - *old_ticks) > MS_TO_TICKS(ms)) {
		*old_ticks = ticks;
		return True;
	}
	return False;
}

/*!
 * Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind.
 * Siehe auch timer_ms_passed_32()
 * 16-Bit Version, fuer Code, der alle 11 s oder oefter ausgefuehrt werden soll.
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 */
static inline uint8_t
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
timer_ms_passed_16(uint16_t * old_ticks, uint32_t ms) {
	uint16_t ticks = TIMER_GET_TICKCOUNT_16;
	if ((uint16_t)(ticks - *old_ticks) > MS_TO_TICKS(ms)) {
		*old_ticks = ticks;
		return True;
	}
	return False;
}

/*!
 * Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind.
 * Siehe auch timer_ms_passed_32()
 * 8-Bit Version, fuer Code, der alle 40 ms oder oefter ausgefuehrt werden soll.
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 */
static inline uint8_t
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
timer_ms_passed_8(uint8_t * old_ticks, uint16_t ms) {
	uint8_t ticks = TIMER_GET_TICKCOUNT_8;
	if ((uint8_t)(ticks - *old_ticks) > MS_TO_TICKS(ms)) {
		*old_ticks = ticks;
		return True;
	}
	return False;
}

/*!
 * Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind.
 * Siehe auch timer_ms_passed_32()
 * 32-Bit Version, fuer Code, der (teilweise) seltener als alle 11 s aufgerufen wird.
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 */
static inline uint8_t
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
timer_ms_passed(uint32_t * old_ticks, uint32_t ms) {
	return timer_ms_passed_32(old_ticks, ms);
}

/*!
 * Initialisiert Timer 2 und startet ihn
 */
void timer_2_init(void);

#ifdef MCU
/*!
 * Misst die Zeitspanne, die vergegangen ist,
 * waehrend __code ausgefuehrt wurde und gibt
 * diese per LOG_DEBUG und Display aus.
 * @param __code	Der auszufuehrende Code
 */
#define TIMER_MEASURE_TIME(__code) \
	uint16_t __us = 0; \
	{ \
	uint32_t start = TIMER_GET_TICKCOUNT_32; \
	int8_t start_reg = (int8_t) TCNT2; \
	{ __code; } \
	uint8_t sreg = SREG; \
	cli(); \
	int8_t end_reg = (int8_t) TCNT2; \
	uint32_t end = TIMER_GET_TICKCOUNT_32; \
	SREG = sreg; \
	uint16_t diff = (uint16_t) (end - start) * 176U; \
	int8_t diff_reg = (int8_t) ((float)(end_reg - start_reg) * 3.2f); \
	if (diff_reg < 0) { \
		diff_reg = (int8_t) (diff_reg + 55); \
	} \
	uint8_t diff_r = (uint8_t) diff_reg; \
	__us = diff + (uint16_t) diff_r; \
 	LOG_DEBUG("%u us", __us); \
	display_cursor(4, 1); \
	display_printf("%4u us", __us); \
}
#else // PC
#define TIMER_MEASURE_TIME(__code) \
	const uint16_t __us = 0; \
	__code
#endif // MCU
#endif	// TIMER_H_
