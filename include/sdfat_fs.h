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
 * \brief 	FAT filesystem support
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

#define SD_BLOCK_SIZE 512U /**< Size of a block in byte */

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

extern pSdFat const p_sd; /**< Pointer to global instance of SdFat class; consider this as a "pseudo-singleton" (used for C bindings) */

extern uint8_t (*sd_card_init)(pSdFat, uint8_t); /**< \see SdFatWrapper::init(), implements the C binding */
extern uint8_t (*sd_card_read_block)(pSdFat, uint32_t , uint8_t*); /**< \see SdFatWrapper::read_block(), implements the C binding */
extern uint8_t (*sd_card_write_block)(pSdFat, uint32_t, const uint8_t*, uint8_t); /**< \see SdFatWrapper::write_block(), implements the C binding */
extern uint32_t (*sd_card_get_size)(pSdFat); /**< \see SdFatWrapper::get_size(), implements the C binding */
extern uint8_t (*sd_card_get_type)(pSdFat); /**< \see SdFatWrapper::get_type(), implements the C binding */
extern uint8_t (*sd_card_get_error_code)(pSdFat); /**< \see SdFatWrapper::get_error_code(), implements the C binding */
extern uint8_t (*sd_card_get_error_data)(pSdFat); /**< \see SdFatWrapper::get_error_data(), implements the C binding */
extern uint8_t (*sd_card_read_csd)(pSdFat, csd_t*); /**< \see SdFatWrapper::read_csd(), implements the C binding */
extern uint8_t (*sd_card_read_cid)(pSdFat, cid_t*); /**< \see SdFatWrapper::read_cid(), implements the C binding */

/**
 * Initializes the Fat filesystem on a SD card
 * \param[in] devisor SPI speed divisor
 * \return Error code: 0 for success, 1 for invalid instance pointer, 2 for error of SdCard::init()
 */
static inline uint8_t sd_init(uint8_t devisor) {
	return sd_card_init(p_sd, devisor);
}

/**
 * Reads a 512 byte block from the SD card
 * \param[in] block Block address on SD card in byte
 * \param[out] dst Pointer to buffer for read data (buffer size >= 512 byte)
 * \return Error code: 1 for success, 0 for error of SdCard::read_block()
 */
static inline uint8_t sd_read_block(uint32_t block, uint8_t* dst) {
	return sd_card_read_block(p_sd, block, dst);
}

/**
 * Writes a 512 byte block to the SD card
 * \param[in] block Block address on SD card in byte
 * \param[in] src Pointer to buffer for data to be written (buffer size >= 512 byte)
 * \param[in] sync Set to 1 for blocking until flash programming has completed, 0 otherwise
 * \return Error code: 1 for success, 0 for error of SdCard::write_block()
 */
static inline uint8_t sd_write_block(uint32_t block, const uint8_t* src, uint8_t sync) {
	return sd_card_write_block(p_sd, block, src, sync);
}

/**
 * Determines the size of the SD card
 * \return The size of the SD card in KB or zero if an error occurs
 */
static inline uint32_t sd_get_size(void) {
	return sd_card_get_size(p_sd);
}

/**
 * Returns the SD card type: SD V1, SD V2 or SDHC
 * \return 0 for SD V1, 1 for SD V2, or 3 for SDHC
 */
static inline uint8_t sd_get_type(void) {
	return sd_card_get_type(p_sd);
}

/**
 * Returns the code of the last error. See sdinfo.h for a list of error codes.
 * \return Error code
 */
static inline uint8_t sd_get_error_code(void) {
	return sd_card_get_error_code(p_sd);
}

/**
 * Returns the data byte received on SPI bus as the last error occurred
 * \return Last data byte received
 */
static inline uint8_t sd_get_error_data(void) {
	return sd_card_get_error_data(p_sd);
}

/**
 * Reads the SD card's CSD register
 * \param[out] p_csd Pointer to buffer for CSD content (buffer size >= 16 byte)
 * \return Error code: 1 for success, 0 for error of SdCard::read_csd()
 */
static inline uint8_t sd_read_csd(csd_t* p_csd) {
	return sd_card_read_csd(p_sd, p_csd);
}

/**
 * Reads the SD card's CID register
 * \param[out] p_csd Pointer to buffer for CID content (buffer size >= 16 byte)
 * \return Error code: 1 for success, 0 for error of SdCard::read_csd()
 */
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

extern uint8_t (*sdfat_open)(const char*, pFatFile*, uint8_t); /**< \see FatFileWrapper::open(), implements the C binding */
extern void (*sdfat_seek)(pFatFile, int32_t, uint8_t); /**< \see FatFileWrapper::seek(), implements the C binding */
extern int32_t (*sdfat_tell)(pFatFile p_file); /**< \see FatFileWrapper::tell(), implements the C binding */
extern void (*sdfat_rewind)(pFatFile); /**< \see FatFileWrapper::rewind(), implements the C binding */
extern int16_t (*sdfat_read)(pFatFile, void*, uint16_t); /**< \see FatFileWrapper::read(), implements the C binding */
extern int16_t (*sdfat_write)(pFatFile, const void*, uint16_t); /**< \see FatFileWrapper::write(), implements the C binding */
extern uint8_t (*sdfat_remove)(pSdFat, const char*); /**< \see SdFatWrapper::remove(), implements the C binding */
extern uint8_t (*sdfat_rename)(pSdFat, const char*, const char*); /**< \see SdFatWrapper::rename(), implements the C binding */
extern uint8_t (*sdfat_flush)(pFatFile); /**< \see FatFileWrapper::flush(), implements the C binding */
extern uint8_t (*sdfat_close)(pFatFile); /**< \see FatFileWrapper::close(), implements the C binding */
extern void (*sdfat_free)(pFatFile); /**< \see FatFileWrapper::free(), implements the C binding */
extern uint32_t (*sdfat_get_filesize)(pFatFile); /**< \see FatFileWrapper::get_filesize(), implements the C binding */
extern uint8_t (*sdfat_get_filename)(pFatFile, char*, uint16_t); /**< \see FatFileWrapper::get_filename(), implements the C binding */
extern uint8_t (*sdfat_sync_vol)(pSdFat); /**< \see SdFatWrapper::sync_vol(), implements the C binding */

/**
 * Simple test code for SD Fat library
 * @return 1 in case of success, 0 otherwise
 */
uint8_t sd_fat_test(void);

/**
 * Removes a file from the volume working directory
 * \param[in] path A path with a valid 8.3 DOS name for the file to be removed
 * \return Error code: 0 for success, 1 for error of FatFileSystem::remove()
 */
static inline uint8_t sdfat_c_remove(const char* path) {
	return sdfat_remove(p_sd, path);
}

/**
 * Renames a file or subdirectory
 * \param[in] old_path Path name to the file or subdirectory to be renamed
 * \param[in] new_path New path name of the file or subdirectory
 * \return Error code: 0 for success, 1 for error of FatFileSystem::rename()
 *
 * \see SdFatWrapper::rename()
 */
static inline uint8_t sdfat_c_rename(const char* old_path, const char* new_path) {
	return sdfat_rename(p_sd, old_path, new_path);
}

/**
 * Writes current block to SD card, if it's dirty
 * \return Error code: 0 for success, 1 for error of FatVolume::cacheSync()
 */
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

uint8_t sdfat_open(const char* filename, pFatFile* p_file, uint8_t mode); /**< \see FatFileWrapper::open() */
void sdfat_seek(pFatFile p_file, int32_t offset, uint8_t origin); /**< \see FatFileWrapper::seek() */
int32_t sdfat_tell(pFatFile p_file); /**< \see FatFileWrapper::tell() */
void sdfat_rewind(pFatFile p_file); /**< \see FatFileWrapper::rewind() */
int16_t sdfat_read(pFatFile p_file, void* buffer, uint16_t length); /**< \see FatFileWrapper::read() */
int16_t sdfat_write(pFatFile p_file, const void* buffer, uint16_t length); /**< \see FatFileWrapper::write() */
uint8_t sdfat_remove(pSdFat p_instance, const char* path); /**< \see SdFatWrapper::remove() */
uint8_t sdfat_rename(pSdFat p_instance, const char* old_path, const char* new_path); /**< \see SdFatWrapper::rename() */
uint8_t sdfat_flush(pFatFile p_file); /**< \see FatFileWrapper::flush() */
uint8_t sdfat_close(pFatFile p_file); /**< \see FatFileWrapper::close() */
void sdfat_free(pFatFile p_file); /**< \see FatFileWrapper::free() */
uint32_t sdfat_get_filesize(pFatFile p_file); /**< \see FatFileWrapper::get_filesize() */
uint8_t sdfat_get_filename(pFatFile p_file, char* p_name, uint16_t size); /**< \see FatFileWrapper::get_filename() */
uint8_t sdfat_sync_vol(pSdFat p_instance); /**< \see SdFatWrapper::sync_vol() */

/**
 * Simple test code for SD Fat library
 * @return 1 in case of success, 0 otherwise
 */
void sdfat_test(void);

/**
 * Removes a file from the volume working directory
 * \param[in] path A path with a valid 8.3 DOS name for the file to be removed
 * \return Error code: 0 for success, 1 for error of FatFileSystem::remove()
 */
static inline uint8_t sdfat_c_remove(const char* path) {
	return sdfat_remove(NULL, path);
}

/**
 * Renames a file or subdirectory
 * \param[in] old_path Path name to the file or subdirectory to be renamed
 * \param[in] new_path New path name of the file or subdirectory
 * \return Error code: 0 for success, 1 for error of FatFileSystem::rename()
 *
 * \see SdFatWrapper::rename()
 */
static inline uint8_t sdfat_c_rename(const char* old_path, const char* new_path) {
	return sdfat_rename(NULL, old_path, new_path);
}

/**
 * Writes current block to SD card, if it's dirty
 * \return Error code: 0 for success, 1 for error of FatVolume::cacheSync()
 */
static inline uint8_t sdfat_c_sync_vol(void) {
	return sdfat_sync_vol(NULL);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // SDFAT_AVAILABLE
#endif // PC

#endif /* INCLUDE_SDFAT_FS_H_ */
