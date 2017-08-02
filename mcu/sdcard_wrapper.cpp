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
 * \brief	Wrapper for use of SdCard class and SdFat library by William Greiman with ct-Bot framework
 * \author	Timo Sandmann
 * \date 	23.10.2016
 */

#ifdef MCU
#include "sdcard_wrapper.h"
#include "ctbot_comp.h"
#include <stdio.h>


//#define DEBUG_SDFAT

#ifdef LOG_MMC_AVAILABLE
#undef DEBUG_SDFAT
#undef LOG_INFO
#define LOG_INFO(...) {}
#undef LOG_ERROR
#define LOG_ERROR(...) {}
#endif // LOG_MMC_AVAILABLE
#ifndef DEBUG_SDFAT
#undef LOG_AVAILABLE
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif

#ifdef MMC_AVAILABLE
static SdFat sd;
pSdFat const p_sd { &sd };

#if SDFAT_PRINT_SUPPORT
Uart_Print Serial;
#endif

extern "C" {
#include "os_thread.h"
#include "log.h"
#include "uart.h"
}

SdFatWrapper::debug_times_t SdFatWrapper::debug_times;
bool SdFatWrapper::init_state { false };

uint8_t SdFatWrapper::init(SdFat* p_instance, uint8_t devisor) {
	if (init_state) {
		return 0;
	}
	if (! p_instance) {
		return 1;
	}

	os_enterCS();
#ifndef SDFAT_AVAILABLE
	auto res(p_instance->card()->init(devisor));;
#else
	auto res(p_instance->init(devisor));
#endif // ! SDFAT_AVAILABLE
	os_exitCS();
	if (! res) {
		init_state = false;
		LOG_ERROR("SdFatWrapper::init(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
		return 2;
	}

	init_state = true;
	return 0;
}

uint8_t SdFatWrapper::read_block(SdFat* p_instance, uint32_t block, uint8_t* dst) {
	if (! p_instance) {
		return 0;
	}
	os_enterCS();
	const auto starttime(debug_mode ? timer_get_us8() : 0);
	const auto res(p_instance->card()->read_block(block, dst));
	const auto endtime(debug_mode ? timer_get_us8() : 0);
	os_exitCS();
	if (! res) {
		init_state = false;
		LOG_ERROR("SdFatWrapper::read_block(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
	}
	if (debug_mode) {
		LOG_DEBUG("SdFatWrapper::read_block() took %u us", endtime - starttime);
		LOG_DEBUG(" cardcommand took %u us", debug_times.cardcommand - starttime);
		LOG_DEBUG(" ready took %u us", debug_times.ready - debug_times.cardcommand);
		LOG_DEBUG(" spi_rcv took %u us", debug_times.spi_rcv - debug_times.ready);
		LOG_DEBUG(" discard_crc took %u us", debug_times.discard_crc - debug_times.spi_rcv);
		LOG_DEBUG(" readdata took %u us", debug_times.readdata - debug_times.cardcommand);
		(void) starttime;
		(void) endtime;
	}
	return res;
}

uint8_t SdFatWrapper::write_block(SdFat* p_instance, uint32_t block, const uint8_t* src, uint8_t sync) {
	if (! p_instance) {
		return 0;
	}
	os_enterCS();
	const auto res(p_instance->card()->write_block(block, src, static_cast<bool>(sync)));
	os_exitCS();
	if (! res) {
		init_state = false;
		LOG_ERROR("SdFatWrapper::write_block(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
	}
	return res;
}

uint32_t SdFatWrapper::get_size(SdFat* p_instance) {
	if (! init_state || ! p_instance) {
		return 0;
	}

	os_enterCS();
	const auto res(p_instance->card()->get_size() / 2U);
	os_exitCS();
	if (! res) {
		init_state = false;
		LOG_ERROR("SdFatWrapper::get_size(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
	}
	return res;
}

uint8_t SdFatWrapper::read_csd(SdFat* p_instance, csd_t* p_csd) {
	if (! p_instance) {
		return 0;
	}
	os_enterCS();
	const auto res(p_instance->card()->read_csd(p_csd));
	os_exitCS();
	if (! res) {
		init_state = false;
		LOG_ERROR("SdFatWrapper::read_csd(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
	}
	return res;
}

uint8_t SdFatWrapper::read_cid(SdFat* p_instance, cid_t* p_cid) {
	if (! p_instance) {
		return 0;
	}
	os_enterCS();
	const auto res(p_instance->card()->read_cid(p_cid));
	os_exitCS();
	if (! res) {
		init_state = false;
		LOG_ERROR("SdFatWrapper::read_cid(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
	}
	return res;
}

#ifdef SDFAT_AVAILABLE
uint8_t SdFatWrapper::remove(SdFat* p_instance, const char* path) {
	if (! p_instance) {
		return 1;
	}
	os_enterCS();
	const auto res(! p_instance->remove(path));
	os_exitCS();
	if (res) {
		LOG_ERROR("SdFatWrapper::remove(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
	}
	return res;
}

uint8_t SdFatWrapper::rename(SdFat* p_instance, const char* old_path, const char* new_path) {
	if (! p_instance) {
		return 1;
	}
	os_enterCS();
	const auto res(! p_instance->rename(old_path, new_path));
	os_exitCS();
	if (res) {
		LOG_ERROR("SdFatWrapper::rename(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
	}
	return res;
}

uint8_t SdFatWrapper::sync_vol(SdFat* p_instance) {
	if (! p_instance) {
		return 1;
	}
	os_enterCS();
	const auto res(! p_instance->vol()->cacheSync());
	os_exitCS();
	os_exitCS();
	if (res) {
		LOG_ERROR("SdFatWrapper::sync_vol(): error=0x%02x 0x%02x", p_instance->card()->get_error_code(), p_instance->card()->get_error_data());
	}
	return res;
}


uint8_t FatFileWrapper::open(const char* filename, FatFile** p_file, uint8_t mode) {
	auto ptr(new FatFile);
	if (! ptr) {
		return 1;
	}
	os_enterCS();
	const auto res(ptr->open(filename, mode));
	os_exitCS();
	if (res) {
		*p_file = ptr;
		return 0;
	} else {
		delete ptr;
		*p_file = nullptr;
		LOG_ERROR("FatFileWrapper::open(): open() failed: error=0x%x", static_cast<SdFatBase*>(ptr->volume())->card()->get_error_code());
		return 1;
	}
}

uint8_t FatFileWrapper::seek(FatFile* p_instance, int32_t offset, uint8_t origin) {
	if (! p_instance) {
		return 1;
	}
	bool res;
	switch (origin) {
	case SEEK_SET:
		res = ! p_instance->seekSet(offset);
		break;

	case SEEK_CUR:
		res  = ! p_instance->seekCur(offset);
		break;

	case SEEK_END:
		res = ! p_instance->seekEnd(offset);
		break;

	default:
		res = 1;
		break;
	}

	if (res) {
		LOG_ERROR("FatFileWrapper::seek(): error=0x%02x 0x%02x", static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_code(), static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_data());
	}

	return res;
}

int16_t FatFileWrapper::read(FatFile* p_instance, void* buffer, uint16_t length) {
	if (! p_instance) {
		return -1;
	}
	os_enterCS();
	const auto res(p_instance->read(buffer, length));
	os_exitCS();
	if (res < 0) {
		LOG_ERROR("FatFileWrapper::read(): error=0x%02x 0x%02x", static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_code(), static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_data());
	}

	return res;
}

int16_t FatFileWrapper::write(FatFile* p_instance, const void* buffer, uint16_t length) {
	if (! p_instance) {
		return -1;
	}
	os_enterCS();
	const auto res(p_instance->write(buffer, length));
	os_exitCS();
	if (res < 0) {
		LOG_ERROR("FatFileWrapper::write(): error=0x%02x 0x%02x", static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_code(), static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_data());
	}

	return res;
}

uint8_t FatFileWrapper::flush(FatFile* p_instance) {
	if (! p_instance) {
		return 1;
	}
	os_enterCS();
	const auto res(! p_instance->sync());
	os_exitCS();
	if (res) {
		LOG_ERROR("FatFileWrapper::flush(): error=0x%02x 0x%02x", static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_code(), static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_data());
	}

	return res;
}

uint8_t FatFileWrapper::close(FatFile* p_instance) {
	if (! p_instance) {
		return 1;
	}
	os_enterCS();
	const auto res(! p_instance->close());
	os_exitCS();
	if (res) {
		LOG_ERROR("FatFileWrapper::close(): error=0x%02x 0x%02x", static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_code(), static_cast<SdFatBase*>(p_instance->volume())->card()->get_error_data());
	}

	return res;
}

void FatFileWrapper::free(FatFile* p_instance) {
	if (! p_instance) {
		return;
	}
	if (p_instance->isOpen()) {
		close(p_instance);
	}
	delete p_instance;
}
#endif // SDFAT_AVAILABLE

extern "C" {
uint8_t (*sd_card_init)(pSdFat, uint8_t) { SdFatWrapper::init };
uint8_t (*sd_card_read_block)(pSdFat, uint32_t, uint8_t*) { SdFatWrapper::read_block };
uint8_t (*sd_card_write_block)(pSdFat, uint32_t, const uint8_t*, uint8_t) { SdFatWrapper::write_block };
uint32_t (*sd_card_get_size)(pSdFat) { SdFatWrapper::get_size };
uint8_t (*sd_card_get_type)(pSdFat) { SdFatWrapper::get_type };
uint8_t (*sd_card_get_error_code)(pSdFat) { SdFatWrapper::get_error_code };
uint8_t (*sd_card_get_error_data)(pSdFat) { SdFatWrapper::get_error_data };
uint8_t (*sd_card_read_csd)(pSdFat, csd_t*) { SdFatWrapper::read_csd };
uint8_t (*sd_card_read_cid)(pSdFat, cid_t*) { SdFatWrapper::read_cid };

#ifdef SDFAT_AVAILABLE
uint8_t (*sdfat_open)(const char*, pFatFile*, uint8_t) { FatFileWrapper::open };
uint8_t (*sdfat_seek)(pFatFile, int32_t, uint8_t) { FatFileWrapper::seek };
int32_t (*sdfat_tell)(pFatFile p_file) { FatFileWrapper::tell };
void (*sdfat_rewind)(pFatFile) { FatFileWrapper::rewind };
int16_t (*sdfat_read)(pFatFile, void*, uint16_t) { FatFileWrapper::read };
int16_t (*sdfat_write)(pFatFile, const void*, uint16_t) { FatFileWrapper::write };
uint8_t (*sdfat_remove)(pSdFat, const char*) { SdFatWrapper::remove };
uint8_t (*sdfat_rename)(pSdFat, const char*, const char*) { SdFatWrapper::rename };
uint8_t (*sdfat_flush)(pFatFile) { FatFileWrapper::flush };
uint8_t (*sdfat_close)(pFatFile) { FatFileWrapper::close };
void (*sdfat_free)(pFatFile) { FatFileWrapper::free };
uint32_t (*sdfat_get_filesize)(pFatFile) { FatFileWrapper::get_filesize };
uint8_t (*sdfat_get_filename)(pFatFile, char*, uint16_t) { FatFileWrapper::get_filename };
uint8_t (*sdfat_sync_vol)(pSdFat) { SdFatWrapper::sync_vol };
#endif // SDFAT_AVAILABLE


#ifdef SDFAT_AVAILABLE
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
#endif // SDFAT_AVAILABLE

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
