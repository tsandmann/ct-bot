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
 * \file 	mmc.h
 * \brief 	Routinen zum Auslesen / Schreiben einer MMC-Karte
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	07.11.2006
 */

#ifndef MMC_H_
#define MMC_H_

#ifdef MMC_AVAILABLE
#include "ui/available_screens.h"
#include "sdcard_wrapper.h"

#define MMC_INFO_AVAILABLE	 			/**< Die Karte kann uns einiges ueber sich verrraten, wenn wir sie danach fragen. Aber es kostet halt Platz im Flash */
//#define MMC_RAW_WRITE_TEST_AVAILABLE	/**< Deprecated seit SdFat! Achtung dieser Test zerstoert die Daten auf der Karte ab Sektor 0x20000 (^= 64 MB)!!! DISPLAY_MMC_INFO muss an sein */

#if defined MMC_RAW_WRITE_TEST_AVAILABLE && defined MAP_AVAILABLE
#error "MMC_RAW_WRITE_TEST_AVAILABLE und MAP_AVAILABLE duerfen nicht gleichzeitig aktiv sein!"
#endif

#if defined MMC_RAW_WRITE_TEST_AVAILABLE && ! defined DISPLAY_MMC_INFO
#warning "MMC_RAW_WRITE_TEST_AVAILABLE braucht DISPLAY_MMC_INFO (ui/available_screens.h)"
#endif

#ifdef DISPLAY_MMC_INFO
/**
 * Zeigt die Daten der MMC-Karte an
 */
void mmc_display(void);
#endif // DISPLAY_MMC_INFO

#endif // MMC_AVAILABLE
#endif // MMC_H_
