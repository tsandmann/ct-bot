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
 * \file 	ena.h
 * \brief 	Routinen zur Steuerung der Enable-Leitungen
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	26.12.2005
 */

#ifndef ENA_H_
#define ENA_H_

#define ENA_ABSTAND			(1<<0)		/**< Enable-Leitung Abstandssensoren */
#define ENA_RADLED			(1<<1)		/**< Enable-Leitung Radencoder */
#define ENA_SCHRANKE		(1<<2)		/**< Enable-Leitung Fachueberwachung */
#define ENA_KANTLED			(1<<3)		/**< Enable-Leitung Angrundsensor */
#define ENA_KLAPPLED		(1<<4)		/**< Enable-Leitung Schieberueberwachung */
#define ENA_LINE			(1<<5)		/**< Enable-Leitung Liniensensor auf Mausplatine (ENA_MAUS im Schaltplan) */

#ifdef EXPANSION_BOARD_MOD_AVAILABLE
#define ENA_WIPORT			(1<<6)		/**< Enable-Leitung Reserve 1 */
#define ENA_BPS				(1<<7)		/**< Enable-Leitung Reserve 2 */
#else
#define ENA_MMC				(1<<6)		/**< Enable-Leitung Reserve 1 */
#define ENA_MOUSE_SENSOR	(1<<7)		/**< Enable-Leitung Reserve 2 */
#endif // EXPANSION_BOARD_MOD_AVAILABLE

#ifndef __ASSEMBLER__

/**
 * Initialisiert die Enable-Leitungen
 */
void ENA_init(void);

/**
 * Schaltet einzelne Enable-Transistoren an
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!!
 * ENA_on schaltet einen Transistor durch
 * Daher zieht es die entsprechende ENA_XXX-Leitung (mit Transistor) auf Low und NICHT auf High
 * @param enable Bitmaske der anzuschaltenden ENA-Leitungen
 */
void ENA_on(uint8_t enable);

/**
 * Schaltet einzelne Enable-Transistoren aus
 * andere werden nicht beeinflusst
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!!
 * ENA_off schaltet einen Transistor ab
 * Daher zieht es die entsprechende ENA_XXX-Leitung (mit Transistor) auf High und NICHT auf Low
 * @param enable Bitmaske der abzuschaltenden ENA-Leitungen
 */
void ENA_off(uint8_t enable);

/**
 * Schaltet die Enable-Transistoren
 * Achtung, die Treiber-Transistoren sind Low-Aktiv!!!
 * ENA_set bezieht sich auf die Transistoren
 * Daher zieht es die entsprechende ENA_XXX-Leitung auf ~enable
 * @param enable	ENA-Wert, der gesetzt werden soll
 */
void ENA_set(uint8_t enable);

#endif // __ASSEMBLER__
#endif // ENA_H_
