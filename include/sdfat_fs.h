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
 * \file 	sdfat_fs.h
 * \brief 	FAT-Dateisystem Unterstuetzung
 * \author 	Timo Sandmann
 * \date 	06.11.2016
 */

#ifndef INCLUDE_SDFAT_FS_H_
#define INCLUDE_SDFAT_FS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "ct-Bot.h"
#ifdef __cplusplus
}
#endif

#define SD_BLOCK_SIZE 512U /**< Groesse eines Blocks in Byte */

#ifdef MCU
#ifdef MMC_AVAILABLE
#include "sdinfo.h"

#ifdef __cplusplus
#include "SdFat.h"
using pSdFat = SdFat*;
extern "C" {
#else
typedef void* pSdFat;
#endif // __cplusplus

extern pSdFat const p_sd;

extern uint8_t (*sd_card_init)(pSdFat, uint8_t);
extern uint8_t (*sd_card_read_block)(pSdFat, uint32_t , uint8_t*);
extern uint8_t (*sd_card_write_block)(pSdFat, uint32_t, const uint8_t*, uint8_t);
extern uint32_t (*sd_card_get_size)(pSdFat);
extern uint8_t (*sd_card_get_type)(pSdFat);
extern uint8_t (*sd_card_get_error_code)(pSdFat);
extern uint8_t (*sd_card_get_error_data)(pSdFat);
extern uint8_t (*sd_card_read_csd)(pSdFat, csd_t*);
extern uint8_t (*sd_card_read_cid)(pSdFat, cid_t*);

static inline uint8_t sd_init(uint8_t devisor) {
	return sd_card_init(p_sd, devisor);
}
static inline uint8_t sd_read_block(uint32_t block, uint8_t* dst) {
	return sd_card_read_block(p_sd, block, dst);
}
static inline uint8_t sd_write_block(uint32_t block, const uint8_t* src, uint8_t sync) {
	return sd_card_write_block(p_sd, block, src, sync);
}
static inline uint32_t sd_get_size(void) {
	return sd_card_get_size(p_sd);
}
static inline uint8_t sd_get_type(void) {
	return sd_card_get_type(p_sd);
}
static inline uint8_t sd_get_error_code(void) {
	return sd_card_get_error_code(p_sd);
}
static inline uint8_t sd_get_error_data(void) {
	return sd_card_get_error_data(p_sd);
}
static inline uint8_t sd_read_csd(csd_t* p_csd) {
	return sd_card_read_csd(p_sd, p_csd);
}
static inline uint8_t sd_read_cid(cid_t* p_cid) {
	return sd_card_read_cid(p_sd, p_cid);
}

#ifdef SDFAT_AVAILABLE
#ifdef __cplusplus
class FatFile;
using pFatFile = FatFile*;
#else
typedef void* pFatFile;
#endif // __cplusplus

extern uint8_t (*sdfat_open)(const char*, pFatFile*, uint8_t);
extern void (*sdfat_seek)(pFatFile, int32_t, uint8_t);
extern int32_t (*sdfat_tell)(pFatFile p_file);
extern void (*sdfat_rewind)(pFatFile);
extern int16_t (*sdfat_read)(pFatFile, void*, uint16_t);
extern int16_t (*sdfat_write)(pFatFile, const void*, uint16_t);
extern uint8_t (*sdfat_remove)(pSdFat, const char*);
extern uint8_t (*sdfat_rename)(pSdFat, const char*, const char*);
extern uint8_t (*sdfat_flush)(pFatFile);
extern uint8_t (*sdfat_close)(pFatFile);
extern void (*sdfat_free)(pFatFile);
extern uint32_t (*sdfat_get_filesize)(pFatFile);
extern uint8_t (*sdfat_get_filename)(pFatFile, char*, uint16_t);
extern uint8_t (*sdfat_sync_vol)(pSdFat);

uint8_t sd_fat_test(void);

static inline uint8_t sdfat_c_remove(const char* path) {
	return sdfat_remove(p_sd, path);
}
static inline uint8_t sdfat_c_rename(const char* old_path, const char* new_path) {
	return sdfat_rename(p_sd, old_path, new_path);
}
static inline uint8_t sdfat_c_sync_vol(void) {
	return sdfat_sync_vol(p_sd);
}
#endif // SDFAT_AVAILABLE
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // MMC_AVAILABLE
#endif // MCU


#ifdef PC
#ifdef SDFAT_AVAILABLE
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void* pFatFile;
typedef void* pSdFat;

uint8_t sdfat_open(const char* filename, pFatFile* p_file, uint8_t mode);

void sdfat_seek(pFatFile p_file, int32_t offset, uint8_t origin);

int32_t sdfat_tell(pFatFile p_file);

void sdfat_rewind(pFatFile p_file);

int16_t sdfat_read(pFatFile p_file, void* buffer, uint16_t length);

int16_t sdfat_write(pFatFile p_file, const void* buffer, uint16_t length);

uint8_t sdfat_remove(pSdFat p_instance, const char* path);

uint8_t sdfat_rename(pSdFat p_instance, const char* old_path, const char* new_path);

uint8_t sdfat_flush(pFatFile p_file);

uint8_t sdfat_close(pFatFile p_file);

void sdfat_free(pFatFile p_file);

uint32_t sdfat_get_filesize(pFatFile p_file);

uint8_t sdfat_get_filename(pFatFile p_file, char* p_name, uint16_t size);

uint8_t sdfat_sync_vol(pSdFat p_instance);

void sdfat_test(void);

static inline uint8_t sdfat_c_remove(const char* path) {
	return sdfat_remove(NULL, path);
}
static inline uint8_t sdfat_c_rename(const char* old_path, const char* new_path) {
	return sdfat_rename(NULL, old_path, new_path);
}
static inline uint8_t sdfat_c_sync_vol(void) {
	return sdfat_sync_vol(NULL);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // SDFAT_AVAILABLE
#endif // PC

#endif /* INCLUDE_SDFAT_FS_H_ */
