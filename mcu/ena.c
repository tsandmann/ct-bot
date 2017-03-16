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
 * \file 	ena.c
 * \brief 	Routinen zur Steuerung der Enable-Leitungen
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#ifdef MCU

#include "ct-Bot.h"

#ifdef ENA_AVAILABLE
#include <avr/io.h>
#include "ena.h"
#include "shift.h"
#include "mouse.h"

static uint8_t ena = 0; /**< Sichert den Zustand der Enable-Leitungen */
#ifdef MOUSE_AVAILABLE
static uint8_t mmc_interrupted = 0; /**< Speichert, ob die MMC vom Maussensor ausgeschaltet wurde */
#endif

/**
 * Initialisiert die Enable-Leitungen
 */
void ENA_init(void) {
#ifdef EXPANSION_BOARD_AVAILABLE
	DDRD |= _BV(PD2);
#endif
	shift_init();
	ENA_set(0x00);
}

/**
 * Schaltet einzelne Enable-Transistoren an
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!!
 * ENA_on schaltet einen Transistor durch
 * Daher zieht es die entsprechende ENA_XXX-Leitung (mit Transistor) auf Low und NICHT auf High
 * @param enable Bitmaske der anzuschaltenden ENA-Leitungen
 */
void ENA_on(uint8_t enable) {
#ifdef MOUSE_AVAILABLE
	/* Maussensor und MMC-Karte haengen zusammen */
	if (enable == ENA_MOUSE_SENSOR) {
		if ((ena & ENA_MMC) == 0) {	// War die MMC an?
#ifdef SPI_AVAILABLE
			SPCR = (uint8_t) (SPCR & (~_BV(SPE))); // SPI aus
#endif
			/* MMC aus */
			ena |= ENA_MMC;
			mmc_interrupted = 1;
		}
		/* Maussensor an */
		ena = (uint8_t) (ena & (~ENA_MOUSE_SENSOR));
	} else
#endif // MOUSE_AVAILABLE

#ifdef EXPANSION_BOARD_AVAILABLE
	if (enable == ENA_MMC) {
#ifdef MOUSE_AVAILABLE
		/* Maussensor aus */
		if ((ena & ENA_MOUSE_SENSOR) == 0) { // War der Maussensor an?
			mouse_sens_highZ(); // Der Maussensor muss die Datenleitung freigeben
			ena |= ENA_MOUSE_SENSOR; // Maus aus
		}
#endif // MOUSE_AVAILABLE
		/* MMC an */
		ena &= (uint8_t) (~enable);	// CS der MMC und SCLK fuer Maus haengen an not-Q der FlipFlops!
	} else
#endif // EXPANSION_BOARD_AVAILABLE
	{
		ena |= enable;
	}
	ENA_set(ena);

#ifdef EXPANSION_BOARD_AVAILABLE
	if (enable & (ENA_MOUSE_SENSOR | ENA_MMC)) {
		/* Flipflops takten */
		PORTD |= _BV(PD2);
		PORTD = (uint8_t) (PORTD & ~_BV(PD2));
		if (enable == ENA_MMC) {
#ifdef SPI_AVAILABLE
			SPCR |= _BV(SPE) | _BV(MSTR); // SPI an
#else
			MMC_CLK_DDR |= _BV(SPI_CLK);
			MMC_DDR = (uint8_t) (MMC_DDR & ~_BV(SPI_DI));
			MMC_DDR |= _BV(SPI_DO);
#endif // SPI_AVAILABLE
		}
	}
#endif // EXPANSION_BOARD_AVAILABLE
}

/**
 * Schaltet einzelne Enable-Transistoren aus
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!!
 * ENA_off schaltet einen Transistor ab
 * Daher zieht es die entsprechende ENA_XXX-Leitung (mit Transistor) auf High und NICHT auf Low
 * @param enable Bitmaske der abzuschaltenden ENA-Leitungen
 */
void ENA_off(uint8_t enable) {
#ifdef EXPANSION_BOARD_AVAILABLE
	if (enable & (ENA_MMC | ENA_MOUSE_SENSOR)) {
		ena |= enable; // CS der MMC und SCLK fuer Maus haengen an not-Q der FlipFlops!
	} else
#endif // EXPANSION_BOARD_AVAILABLE

	{
		ena &= (uint8_t) (~enable);
	}

	ENA_set(ena);

#ifdef EXPANSION_BOARD_AVAILABLE
	if (enable & (ENA_MOUSE_SENSOR | ENA_MMC)) {
		/* Flipflops takten */
		PORTD |= _BV(PD2);
		PORTD = (uint8_t) (PORTD & ~_BV(PD2));
	}
#endif // EXPANSION_BOARD_AVAILABLE

#ifdef MOUSE_AVAILABLE
	if ((enable & ENA_MOUSE_SENSOR) && mmc_interrupted == 1) {
		mmc_interrupted = 0;
		ENA_on(ENA_MMC);
	}
#endif // MOUSE_AVAILABLE
}

/**
 * Schaltet die Enable-Transistoren
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!!
 * ENA_set bezieht sich auf die Transistoren
 * Daher zieht es die entsprechende ENA_XXX-Leitung auf ~enable
 * @param enable ENA-Wert, der gesetzt werden soll
 */
void ENA_set(uint8_t enable) {
	ena = enable;
	shift_data((uint8_t) (~enable), SHIFT_REGISTER_ENA);
}

#endif // ENA_AVAILABLE
#endif // MCU
