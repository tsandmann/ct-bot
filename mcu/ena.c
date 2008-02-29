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
 * @file 	ena.c 
 * @brief 	Routinen zur Steuerung der Enable-Leitungen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
 */

#ifdef MCU 

#include <avr/io.h>
#include "ena.h"
#include "ct-Bot.h"
#include "shift.h"
#include "mouse.h"

#ifdef ENA_AVAILABLE


static uint8_t ena = 0;				/*!< Sichert den Zustand der Enable-Leitungen */
#ifdef MAUS_AVAILABLE
	static uint8_t mmc_interrupted = 0;	/*!< Speichert, ob die MMC vom Maussensor ausgeschaltet wurde */
#endif

/*!
 * Initialisiert die Enable-Leitungen
 */
void ENA_init() {
	DDRD |= 4;
	shift_init();
	ENA_set(0x00);
}

/*! 
 * Schaltet einzelne Enable-Transistoren an
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_on schaltet einen Transistor durch
 * Daher zieht es die entsprechende ENA_XXX-Leitung (mit Transistor) auf Low und NICHT auf High
 * @param enable Bitmaske der anzuschaltenden ENA-Leitungen
 */
void ENA_on(uint8_t enable) {
	#ifdef MAUS_AVAILABLE
		/* Maussensor und MMC-Karte haengen zusammen */
		if (enable == ENA_MOUSE_SENSOR) {
			if ((ena & ENA_MMC) == 0) {	// War die MMC an?	
				#ifdef SPI_AVAILABLE
					SPCR = 0;	// SPI aus
				#endif
				/* MMC aus */
				ena |= ENA_MMC;
				#ifdef MAUS_AVAILABLE
					mmc_interrupted = 1;
				#endif
			}
			/* Maussensor an */
			ena &= ~ENA_MOUSE_SENSOR;
		} else
	#endif	// MAUS_AVAILABLE
	if (enable == ENA_MMC) {		
		#ifdef MAUS_AVAILABLE
			/* Maussensor aus */
			if ((ena & ENA_MOUSE_SENSOR) == 0) { // War der Maussensor an?
				maus_sens_highZ();	// Der Maussensor muss die Datenleitung freigeben
				ena |= ENA_MOUSE_SENSOR;	// Maus aus
			}
		#endif	// MAUS_AVAILABLE
		/* MMC an */
		ena &= ~enable;	// CS der MMC und SCLK fuer Maus haengen an not-Q der FlipFlops!
	} else {
		ena |= enable;
	}
	
	ENA_set(ena);
	
	if ((enable & (ENA_MOUSE_SENSOR | ENA_MMC)) != 0) {
        /* Flipflops takten */
        PORTD |= 4;
        PORTD &= ~4;
		#ifdef SPI_AVAILABLE
        	if (enable == ENA_MMC) {
        		SPCR = (1<<SPE) | (1<<MSTR);	// SPI an
        	}
		#endif	// SPI_AVAILABLE
	}
}

/*! 
 * Schaltet einzelne Enable-Transistoren aus
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_off schaltet einen Transistor ab
 * Daher zieht es die entsprechende ENA_XXX-Leitung (mit Transistor) auf High und NICHT auf Low
 * @param enable Bitmaske der abzuschaltenden ENA-Leitungen
 */
void ENA_off(uint8_t enable) {
	if ((enable & (ENA_MMC | ENA_MOUSE_SENSOR)) != 0) {
		ena |= enable;	// CS der MMC und SCLK fuer Maus haengen an not-Q der FlipFlops!
	} else {
		ena &= ~enable;
	}

	ENA_set(ena);

	#ifdef MAUS_AVAILABLE
		if (mmc_interrupted == 1) {
			ENA_on(ENA_MMC);
		}
	#endif	// MAUS_AVAILABLE
	
	if ((enable & (ENA_MOUSE_SENSOR | ENA_MMC)) != 0) {
		/* Flipflops takten */
		PORTD |= 4;
		PORTD &= ~4;
	}
}

/*!
 * Schaltet die Enable-Transistoren
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!! 
 * ENA_set bezieht sich auf die Transistor
 * Daher zieht es die entsprechende ENA_XXX-Leitung auf ~enable
 * @param enable ENA-Wert, der gesetzt werden soll
 */
void ENA_set(uint8_t enable) {
	ena = enable;
	shift_data(~enable, SHIFT_REGISTER_ENA); 
}

#endif	// ENA_AVAILABLE
#endif	// MCU
