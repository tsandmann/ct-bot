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
 * \file 	mcu/sdcard_wrapper.cpp
 * \brief	Wrapper for use of SdCard class and SdFat library by William Greiman with ct-Bot framework.
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	23.10.2016
 */

#ifdef MCU
#include "sdcard_wrapper.h"
#include "ctbot_comp.h"
#include "SdFat.h"
#include <stdio.h>

#ifdef MMC_AVAILABLE
static SdFat sd;

static bool init_state = false;
#if SDFAT_PRINT_SUPPORT
Uart_Print Serial;
#endif

extern "C" {
#include "os_thread.h"
#include "log.h"
#include "uart.h"

//uint16_t starttime, cardcommand, readdata, ready, spi_rcv, discard_crc;

uint8_t sd_card_init() {
	if (init_state) {
		return 0;
	}

	os_enterCS();
	auto res(sd.card()->init(SPI_SPEED));
	os_exitCS();
	if (! res) {
		LOG_DEBUG("sd_card_init(): 1 error=0x%x", sd.card()->get_error_code());
		return 1;
	}

#ifdef SDFAT_AVAILABLE
	os_enterCS();
	res = sd.init(SPI_SPEED);
	os_exitCS();
	if (! res) {
		LOG_DEBUG("sd_card_init(): 2 error=0x%x", sd.card()->get_error_code());
		return 2;
	}
#endif // SDFAT_AVAILABLE

	init_state = true;
	return 0;
}

uint8_t sd_card_read_block(uint32_t addr, void* buffer) {
	os_enterCS();
//	starttime = timer_get_us8();
	const auto res(sd.card()->read_block(addr, static_cast<uint8_t*>(buffer)));
//	const auto endtime = timer_get_us8();
	os_exitCS();
	if (! res) {
		init_state = false;
	}
//	LOG_DEBUG("read_block() took %u us", endtime - starttime);
//	LOG_DEBUG(" cardcommand took %u us", cardcommand - starttime);
//	LOG_DEBUG(" ready took %u us", ready - cardcommand);
//	LOG_DEBUG(" spi_rcv took %u us", spi_rcv - ready);
//	LOG_DEBUG(" discard_crc took %u us", discard_crc - spi_rcv);
//	LOG_DEBUG(" readdata took %u us", readdata - cardcommand);
	return res;
}

uint8_t sd_card_write_block(uint32_t addr, const void* buffer) {
	os_enterCS();
	const auto res(sd.card()->write_block(addr, static_cast<const uint8_t*>(buffer), true));
	os_exitCS();
	if (! res) {
		init_state = false;
	}
	return res;
}

uint32_t sd_card_get_size() {
	os_enterCS();
	const auto res(sd.card()->get_size() / 2U);
	os_exitCS();
	if (! res) {
		init_state = false;
	}
	return res;
}

uint8_t sd_card_get_type() {
	return sd.card()->get_type();
}

uint8_t sd_card_read_csd(csd_t* p_csd) {
	os_enterCS();
	const auto res(sd.card()->read_csd(p_csd));
	os_exitCS();
	if (! res) {
		init_state = false;
	}
	return res;
}

uint8_t sd_card_read_cid(cid_t* p_cid) {
	os_enterCS();
	const auto res(sd.card()->read_cid(p_cid));
	os_exitCS();
	if (! res) {
		init_state = false;
	}
	return res;
}

#ifdef SDFAT_AVAILABLE
int8_t sdfat_open(const char* filename, FatFile** p_file, uint8_t mode) {
//	LOG_DEBUG("sdfat_open(\"%s\", 0x%x, %u)", filename, *((uint16_t*) p_file), mode);
	auto ptr(new FatFile);
	os_enterCS();
	const auto res(ptr->open(filename, mode));
	os_exitCS();
	if (res) {
		*p_file = ptr;
		return 0;
	} else {
		*p_file = nullptr;
		LOG_ERROR("sdfat_open(): ptr->open() failed");
		LOG_DEBUG("sdlib-error=0x%x", sd.card()->get_error_code());
		return 1;
	}
}

void sdfat_seek(pFatFile p_file, int16_t offset, uint8_t origin) {
	switch (origin) {
	case SEEK_SET:
		p_file->seekSet(offset);
		break;

	case SEEK_CUR:
		p_file->seekCur(offset);
		break;

	case SEEK_END:
		p_file->seekEnd(offset);
		break;
	}
}

void sdfat_rewind(pFatFile p_file) {
	p_file->rewind();
}

int16_t sdfat_read(pFatFile p_file, void* buffer, uint16_t length) {
	os_enterCS();
	auto res(p_file->read(buffer, length));
	os_exitCS();
	return res;
}

int16_t sdfat_write(pFatFile p_file, void* buffer, uint16_t length) {
	os_enterCS();
	const auto res(p_file->write(buffer, length));
	os_exitCS();
//	LOG_DEBUG("sdfat_write(0x%x, 0x%x, %u)=%d", p_file, buffer, length, res);
	return res;
}

int8_t sdfat_unlink(const char* filename) {
	os_enterCS();
	auto res(! sd.remove(filename));
	os_exitCS();
	return res;
}

int8_t sdfat_rename(const char* filename, const char* new_name) {
	os_enterCS();
	auto res(! sd.rename(filename, new_name));
	os_exitCS();
	return res;
}

int8_t sdfat_sync(pFatFile p_file) {
	os_enterCS();
	auto res(! p_file->sync());
	os_exitCS();
	return res;
}

int8_t sdfat_close(pFatFile p_file) {
	os_enterCS();
	auto res(! p_file->close());
	os_exitCS();
	return res;
}

void sdfat_free(pFatFile p_file) {
	if (p_file->isOpen()) {
		sdfat_close(p_file);
	}
	delete p_file;
}

uint32_t sdfat_get_filesize(pFatFile p_file) {
	return p_file->fileSize();
}

int8_t sdfat_get_filename(pFatFile p_file, char* p_name, uint16_t size) {
	return ! p_file->getName(p_name, size);
}

int8_t sdfat_sync_vol() {
	os_enterCS();
	auto res(sd.vol()->cacheSync());
	os_exitCS();
	return ! res;
}
#endif // SDFAT_AVAILABLE

#if 0
uint8_t sd_fat_test() {
	if (! sd.begin()) {
		if (sd.card()->get_error_code()) {
			LOG_ERROR("SD initialization failed.");
			LOG_ERROR(" errorCode: %X", sd.card()->get_error_code());
			LOG_ERROR(" errorData: %X", sd.card()->get_error_data());
			return false;
		}
		LOG_DEBUG("Card successfully initialized.");
		if (sd.vol()->fatType() == 0) {
			LOG_ERROR("Can't find a valid FAT16/FAT32 partition.");
			return false;
		}
		if (! sd.vwd()->isOpen()) {
			LOG_ERROR("Can't open root directory.");
			return false;
		}
		LOG_ERROR("Can't determine error type.");
		return false;
	}
	LOG_DEBUG("Card/FS successfully initialized.");

	uint32_t size = sd.card()->get_size();
	if (size == 0) {
		LOG_ERROR("Can't determine the card size.");
		return false;
	}
	uint32_t sizeMB = static_cast<uint32_t>(0.000512f * static_cast<float>(size) + 0.5f);
	LOG_INFO("Card size: %u MB", sizeMB);
	LOG_INFO("Volume is FAT%u", sd.vol()->fatType());
	LOG_INFO("Cluster size (bytes): %u", 512L * sd.vol()->blocksPerCluster());

	if ((sizeMB > 1100 && sd.vol()->blocksPerCluster() < 64) || (sizeMB < 2200 && sd.vol()->fatType() == 32)) {
		LOG_DEBUG("Card should be reformatted for best performance.");
		LOG_DEBUG("Use cluster size of 32 KB for cards > 1 GB.");
		LOG_DEBUG("Only cards > 2 GB should be formatted FAT32.");
	}

	os_enterCS();
#if SDFAT_PRINT_SUPPORT
	LOG_INFO("Files found (date time size name):");
	sd.ls(&Serial, /*LS_A |*/ LS_R | LS_DATE | LS_SIZE);
#endif // SDFAT_PRINT_SUPPORT

	LOG_INFO("contents of \"/test.txt\"");
	FatFile file("/test.txt", O_READ);
	if (file.isOpen()) {
	    while (file.available()) {
	    	const auto tmp(file.read());
	    	if (tmp >= 0) {
	    		const auto data(static_cast<char>(tmp));
	    		uart_write(&data, sizeof(data));
	    	}
	    }
		uart_write(LINE_FEED, sizeof(LINE_FEED));
		file.close();
	} else {
		LOG_ERROR("file.open(/test.txt, O_READ) failed.");
	}

	if (file.open("/test.txt", O_APPEND | O_RDWR)) {
		if (! file.seekEnd()) {
			LOG_ERROR("file.seekEnd() failed.");
		}
		if (! file.write(".")) {
			LOG_ERROR("file.write() failed.");
		}

		file.close();
	} else {
		LOG_ERROR("file.open(/test.txt, O_APPEND) failed.");
	}
	os_exitCS();

	return true;
}
#endif // 0

} // extern C

#if SDFAT_PRINT_SUPPORT
size_t Uart_Print::write(uint8_t data) {
	return uart_write(&data, 1);
}

size_t Uart_Print::write(const uint8_t* buffer, size_t size) {
	return uart_write(buffer, size);
}
#endif // SDFAT_PRINT_SUPPORT

#endif // MMC_AVAILABLE
#endif // MCU
