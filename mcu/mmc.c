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
 * \file 	mcu/mmc.c
 * \brief 	Routinen zum Auslesen/Schreiben einer MMC-Karte
 * \author 	Benjamin Benz (bbe@heise.de)
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	07.11.2006
 */


/*
 *  Die MMC kann auf zwei Weisen angesprochen werden:
 * Entweder per Software-Steuerung (das ist die Standard-Einstellung), dafuer muss
 * SPI_AVAILABLE in ct-Bot.h AUS sein.
 * Oder per Hardware-SPI-Steuerung, dafuer ist ein kleiner Hardware-Umbau noetig, man
 * muss die Verbindung zwischen PC5 und dem Display trennen (busy-Leitung wird vom Display-
 * Treiber eh nicht genutzt) und auf PC5 den linken Radencoder legen. Ausserdem ist PB4
 * vom Radencoder zu trennen (der PB4-Pin kann fuer andere Zwecke genutzt werden, er muss
 * jedoch immer als OUTPUT konfiguriert sein). Schaltet man nun in ct-Bot.h SPI_AVAILABLE
 * AN, dann wird die Kommunikation mit der MMC per Hardware gesteuert - Vorteil ist eine
 * hoehere Transfer-Geschwindigkeit zur MMC (Faktor 2) und es sind ca. 430 Byte weniger im
 * Flash belegt.
 * Zu beachten ist, dass SPI_AVAILABLE von jetzt an immer eingeschaltet sein muss, auch
 * wenn man keine MMC-Unterstuetzung benoetigt, weil die Radencoder-Auswertung die
 * veraenderte Pin-Belegung immer beruecksichtigen muss.
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef MMC_AVAILABLE
#include "mmc.h"
#include "ena.h"
#include "timer.h"
#include "display.h"
#include "led.h"
#include "map.h"
#include "os_thread.h"
#include "sensor.h"
#include "log.h"
#include "sdfat_fs.h"
#include <stdlib.h>
#include <string.h>

//#define DEBUG_MMC // Schalter, um auf einmal alle Debugs an oder aus zu machen

#ifndef DEBUG_MMC
#undef LOG_AVAILABLE
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif // DEBUG_MMC

static uint32_t card_size, time_read, time_write, n;
static uint8_t last_error_code, last_error_data;

/**
 * Testet die MMC-Karte. Schreibt nacheinander 2 Sektoren a 512 Byte mit Testdaten voll und liest sie wieder aus
 * !!! Achtung loescht die Karte ab Sektor 0x20000 (^= 64 MB)
 * \param *buffer	Zeiger auf einen 512 Byte grossen Puffer
 * \return 			0, wenn alles ok
 */
static inline uint8_t mmc_test(uint8_t* buffer) {
	static uint32_t sector = 0x20000;

	if (! card_size) {
		sector = 0x20000;
		return 1;
	}

	uint16_t i;
	uint8_t result = 0;

	/* Zeitmessung */
	uint32_t start_ticks, end_ticks;

	// Puffer vorbereiten
	for (i = 0; i < 512; i++) {
		buffer[i] = (uint8_t)(i & 0xff);
	}

	/* Zeitmessung starten */
	start_ticks = timer_get_us32();

	// und schreiben
	result = sd_write_block(sector, buffer, True);

	/* Zeitmessung beenden */
	end_ticks = timer_get_us32();
	time_write = time_write + (uint16_t) (end_ticks - start_ticks);

	if (! result) {
		return (uint8_t) (result * 10 + 2);
	}

	// Puffer vorbereiten
	for (i = 0; i < 512; i++) {
		buffer[i] = (uint8_t) (255 - (i & 0xff));
	}

	/* Zeitmessung starten */
	start_ticks = timer_get_us32();

	// und schreiben
	result = sd_write_block(sector + 1, buffer, True);

	/* Zeitmessung beenden */
	end_ticks = timer_get_us32();
	time_write = time_write + (uint16_t) (end_ticks - start_ticks);

	if (! result) {
		return (uint8_t) (result * 10 + 3);
	}

	/* Zeitmessung starten */
	start_ticks = timer_get_us32();

	// Puffer lesen
	result = sd_read_block(sector++, buffer);

	/* Zeitmessung beenden */
	end_ticks = timer_get_us32();
	time_read = time_read + (uint16_t) (end_ticks - start_ticks);

	if (! result) {
		sector--;
		return (uint8_t) (result * 10 + 4);
	}

	// und vergleichen
	for (i = 0; i < 512; i++) {
		if (buffer[i] != (i & 0xFF)) {
			LOG_ERROR("i:%u\tread:0x%x\texpected:0x%x", i, buffer[i], i & 0xff);
			return 5;
		}
	}

	/* Zeitmessung starten */
	start_ticks = timer_get_us32();

	// Puffer lesen
	result = sd_read_block(sector++, buffer);

	/* Zeitmessung beenden */
	end_ticks = timer_get_us32();
	time_read = time_read + (uint16_t) (end_ticks - start_ticks);

	if (! result) {
		sector--;
		return (uint8_t) (result * 10 + 6);
	}
	// und vergleichen
	for (i = 0; i < 512; i++) {
		if (buffer[i] != (255 - (i & 0xFF))) {
			LOG_ERROR("i:%u\tread:0x%x\texpected:0x%x", i, buffer[i], 255 - (i & 0xff));
			return 7;
		}
	}

	/* Statistik ausgeben */
	n += 2;
	const uint16_t t_read = (uint16_t) (time_read / n);
	const uint16_t t_write = (uint16_t) (time_write / n);
	os_enterCS();
	display_cursor(2, 1);
	display_printf("sector:   0x%04x", (sector - 2) >> 16);
	display_printf("%04x", (sector - 2) & 0xffff);
	display_cursor(3, 1);
	display_printf("raw  r/w:%5u KiB/s", (uint16_t) ((uint32_t) (512.f * 1000000.f / 1024.f * 2.f) / (t_read + t_write)));
	display_cursor(4, 1);
	display_printf("time r/w:%5u/%5u", t_read, t_write);
	os_exitCS();

	return 0;
}

#ifdef SDFAT_AVAILABLE
static inline uint8_t sdfat_test(void) {
	static uint8_t buffer[512];

	if (! card_size) {
		return 1;
	}

	LOG_DEBUG("sdfat_test(): opening file...");

	uint32_t start_ticks, end_ticks;
	pFatFile file;
	const uint8_t result = sdfat_open("test.bin", &file, 0x1 | 0x2 | 0x10 | 0x40);
	if (result) {
		LOG_ERROR("sdfat_test(): sdfat_open() failed: %d", result);
		return result;
	}

	LOG_DEBUG("sdfat_test(): file opened.");

	/* Puffer vorbereiten */
	uint16_t i;
	for (i = 0; i < 512; ++i) {
		buffer[i] = (uint8_t) (255 - (i & 0xff));
	}

	LOG_DEBUG("sdfat_test(): starting write...");

	/* Zeitmessung starten */
	start_ticks = timer_get_us32();

	/* Puffer schreiben */
	if (sdfat_write(file, buffer, 512) != 512) {
		LOG_ERROR("sdfat_test(): sdfat_write() failed");
		sdfat_free(file);
		return 2;
	}

	/* Zeitmessung beenden */
	end_ticks = timer_get_us32();

	LOG_DEBUG("sdfat_test(): write done.");

	time_write += end_ticks - start_ticks;

	/* Puffer loeschen */
	memset(buffer, 0, sizeof(buffer));

	LOG_DEBUG("sdfat_test(): starting read...");

	/* Zeitmessung starten */
	start_ticks = timer_get_us32();

	/* Puffer lesen */
	sdfat_rewind(file);
	if (sdfat_read(file, buffer, 512) != 512) {
		LOG_ERROR("sdfat_test(): sdfat_read() failed");
		sdfat_free(file);
		return 3;
	}

	/* Zeitmessung beenden */
	end_ticks = timer_get_us32();

	LOG_DEBUG("sdfat_test(): read done.");

	sdfat_free(file);

	time_read += end_ticks - start_ticks;

	/* Puffer vergleichen */
	for (i = 0; i < 512; ++i) {
		if (buffer[i] != 255 - (i & 0xff)) {
			LOG_ERROR("sdfat_test(): i:%u\tread:0x%x\texpected:0x%x", i, buffer[i], 255 - (i & 0xff));
			return 5;
		}
	}

	++n;
	const uint16_t t_read = (uint16_t) (time_read / n);
	const uint16_t t_write = (uint16_t) (time_write / n);
	display_cursor(3, 1);
	display_printf("file r/w:%5u KiB/s", (uint16_t) ((uint32_t) (512.f * 1000000.f / 1024.f * 2.f) / (t_read + t_write)));
	display_cursor(4, 1);
	display_printf("time r/w:%5u/%5u", t_read, t_write);

	LOG_DEBUG("sdfat_test(): done.");

	return 0;
}
#endif // SDFAT_AVAILABLE

#ifdef DISPLAY_MMC_INFO
/**
 * Zeigt die Daten der MMC-Karte an
 */
void mmc_display(void) {
#ifdef MMC_INFO_AVAILABLE
	card_size = sd_get_size();
	if (! card_size) {
		time_read = time_write = n = 0;
		if (! last_error_code) {
			last_error_code = sd_get_error_code();
			last_error_data = sd_get_error_data();
		}
		display_clear();
		display_printf("No card detected");
		display_cursor(2, 1);
		display_printf("error code=0x%02x 0x%02x", last_error_code, last_error_data);
		if (sd_init(SPI_SPEED)) {
			last_error_code = sd_get_error_code();
			last_error_data = sd_get_error_data();
			LOG_ERROR("sd_card_init() failed: error code=0x%02x 0x%02x", last_error_code, last_error_data);
			return;
		}
		card_size = sd_get_size();
	}
	const uint8_t type = sd_get_type();
	display_clear();
	display_printf("%s: %6u MiB  ", type == 0 ? "SDv1" : type == 1 ? "SDv2" : "SDHC", card_size >> 10);

#if ! defined MMC_WRITE_TEST_AVAILABLE
	cid_t cid;
	sd_read_cid(&cid);
	csd_t csd;
	sd_read_csd(&csd);
	display_cursor(2, 1);
	display_printf("%.2s %.5s %2u/%4u ^%u ", cid.oid, cid.pnm, cid.mdt_month, 2000 + cid.mdt_year_low + 10 * cid.mdt_year_high, csd.v2.write_bl_len_high << 2 | csd.v2.write_bl_len_low);

#if ! defined SDFAT_AVAILABLE
	display_cursor(3, 1);
	uint8_t i;
	for (i = 0; i < 16; ++i) {
		if (i == 8) {
			display_cursor(4, 1);
		}
		if (i % 2 == 0) {
			display_puts(" ");
		}
		display_printf("%02x", csd.raw[i]);
	}
#endif // ! SDFAT_AVAILABLE
#endif // ! MMC_WRITE_TEST_AVAILABLE

#if ! defined MMC_WRITE_TEST_AVAILABLE && defined SDFAT_AVAILABLE
	if (card_size) {
		const uint8_t result = sdfat_test();
		if (result) {
			display_cursor(4, 1);
			display_printf("sdfat_test()=%d :(", result);
		}
	}
#endif // ! MMC_WRITE_TEST_AVAILABLE && SDFAT_AVAILABLE

#ifdef MMC_WRITE_TEST_AVAILABLE
	static uint8_t buffer[512];
	if (card_size) {
		uint8_t result = mmc_test(buffer);
		if (result) {
			display_cursor(3, 1);
			display_printf("mmc_test()=%u :(", result);
		}
	}
#endif // MMC_WRITE_TEST_AVAILABLE
#endif // MMC_INFO_AVAILABLE
}
#endif // DISPLAY_MMC_INFO

#endif // MMC_AVAILABLE
#endif // MCU
