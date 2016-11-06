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
 * \file 	sdcard_wrapper.h
 * \brief	Wrapper for use of SdCard class and SdFat library by William Greiman with ct-Bot framework.
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	23.10.2016
 */

#ifndef MCU_SDCARD_WRAPPER_H_
#define MCU_SDCARD_WRAPPER_H_

#ifdef MCU
#include "ct-Bot.h"
#include "sdfat_fs.h"
#include <stdint.h>

#ifdef __cplusplus
#ifdef MMC_AVAILABLE
class SdFatWrapper : public SdFat {
	static bool init_state;

public:
	static uint8_t init(SdFat* p_instance, uint8_t devisor);
	static uint8_t read_block(SdFat* p_instance, uint32_t block, uint8_t* dst);
	static uint8_t write_block(SdFat* p_instance, uint32_t block, const uint8_t* src, uint8_t sync);
	static uint32_t get_size(SdFat* p_instance);
	static uint8_t get_type(SdFat* p_instance) {
		return p_instance->card()->get_type();
	}
	static uint8_t get_error_code(SdFat* p_instance) {
		return p_instance->card()->get_error_code();
	}
	static uint8_t get_error_data(SdFat* p_instance) {
		return p_instance->card()->get_error_data();
	}
	static uint8_t read_csd(SdFat* p_instance, csd_t* p_csd);
	static uint8_t read_cid(SdFat* p_instance, cid_t* p_cid);
	static uint8_t remove(SdFat* p_instance, const char* path);
	static uint8_t rename(SdFat* p_instance, const char* old_path, const char* new_path);
	static uint8_t sync_vol(SdFat* p_instance);

	static constexpr bool const debug_mode { false };
	struct debug_times_t {
		uint16_t starttime;
		uint16_t cardcommand;
		uint16_t readdata;
		uint16_t ready;
		uint16_t spi_rcv;
		uint16_t discard_crc;
	};
	static debug_times_t debug_times;
};

#ifdef SDFAT_AVAILABLE
class FatFileWrapper : public FatFile {
public:
	static uint8_t open(const char* filename, FatFile** p_file, uint8_t mode);
	static int32_t tell(FatFile* p_instance) {
		const auto pos(p_instance->curPosition());
		return static_cast<int32_t>(pos < INT32_MAX ? pos : -1);
	}
	static void seek(FatFile* p_instance, int32_t offset, uint8_t origin);
	static void rewind(FatFile* p_instance) {
		p_instance->rewind();
	}
	static int16_t read(FatFile* p_instance, void* buffer, uint16_t length);
	static int16_t write(FatFile* p_instance, const void* buffer, uint16_t length);
	static uint8_t flush(FatFile* p_instance);
	static uint8_t close(FatFile* p_instance);
	static void free(FatFile* p_instance);
	static uint32_t get_filesize(FatFile* p_instance) {
		return p_instance->fileSize();
	}
	static uint8_t get_filename(FatFile* p_instance, char* p_name, uint16_t size) {
		return ! p_instance->getName(p_name, size);
	}
};
#endif // SDFAT_AVAILABLE
#endif // MMC_AVAILABLE

#endif // __cplusplus
#endif // MCU
#endif /* MCU_SDCARD_WRAPPER_H_ */
