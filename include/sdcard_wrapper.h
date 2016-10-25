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
#include <stdint.h>

#ifdef __cplusplus
class FatFile;
using pFatFile = FatFile*;

extern "C" {
#include "ct-Bot.h"
#else
typedef void* pFatFile;
#endif // __cplusplus

#ifdef MMC_AVAILABLE
#include "sdinfo.h"

uint8_t sd_card_init(void);
uint8_t sd_card_read_block(uint32_t addr, void* buffer);
uint8_t sd_card_write_block(uint32_t addr, const void* buffer);
uint32_t sd_card_get_size(void);
uint8_t sd_card_get_type(void);
uint8_t sd_card_read_csd(csd_t* p_csd);
uint8_t sd_card_read_cid(cid_t* p_cid);

#ifdef SDFAT_AVAILABLE
int8_t sdfat_open(const char* filename, pFatFile* p_file, uint8_t mode);
void sdfat_seek(pFatFile p_file, int16_t offset, uint8_t origin);
void sdfat_rewind(pFatFile p_file);
int16_t sdfat_read(pFatFile p_file, void* buffer, uint16_t length);
int16_t sdfat_write(pFatFile p_file, void* buffer, uint16_t length);
int8_t sdfat_unlink(const char* filename);
int8_t sdfat_rename(const char* filename, const char* new_name);
int8_t sdfat_sync(pFatFile p_file);
int8_t sdfat_close(pFatFile p_file);
void sdfat_free(pFatFile p_file);
uint32_t sdfat_get_filesize(pFatFile p_file);
int8_t sdfat_get_filename(pFatFile p_file, char* p_name, uint16_t size);
int8_t sdfat_sync_vol(void);

#if 0
uint8_t sd_fat_test(void);
#endif // 0
#endif // SDFAT_AVAILABLE

#endif // MMC_AVAILABLE
#ifdef __cplusplus
}
#endif
#endif // MCU
#endif /* MCU_SDCARD_WRAPPER_H_ */
