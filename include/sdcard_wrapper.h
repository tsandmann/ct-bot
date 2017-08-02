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
 * \brief	Wrapper for use of SdCard class and SdFat library by William Greiman with ct-Bot framework
 * \author	Timo Sandmann
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

/**
 * Wrapper class for SdFat class of SdFat library to support C language bindings
 */
class SdFatWrapper : protected SdFat {
	static bool init_state; /**< true, iff SD card has been initialized successfully */

public:
	/**
	 * Initializes the Fat filesystem on a SD card
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \param[in] devisor SPI speed divisor
	 * \return Error code: 0 for success, 1 for invalid instance pointer, 2 for error of SdCard::init()
	 */
	static uint8_t init(SdFat* p_instance, uint8_t devisor);

	/**
	 * Reads a 512 byte block from the SD card
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \param[in] block Block address on SD card in byte
	 * \param[out] dst Pointer to buffer for read data (buffer size >= 512 byte)
	 * \return Error code: 1 for success, 0 for error of SdCard::read_block()
	 */
	static uint8_t read_block(SdFat* p_instance, uint32_t block, uint8_t* dst);

	/**
	 * Writes a 512 byte block to the SD card
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \param[in] block Block address on SD card in byte
	 * \param[in] src Pointer to buffer for data to be written (buffer size >= 512 byte)
	 * \param[in] sync Set to 1 for blocking until flash programming has completed, 0 otherwise
	 * \return Error code: 1 for success, 0 for error of SdCard::write_block()
	 */
	static uint8_t write_block(SdFat* p_instance, uint32_t block, const uint8_t* src, uint8_t sync);

	/**
	 * Determines the size of the SD card
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \return The size of the SD card in KB or zero if an error occurs
	 */
	static uint32_t get_size(SdFat* p_instance);

	/**
	 * Returns the SD card type: SD V1, SD V2 or SDHC
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \return 0 for SD V1, 1 for SD V2, or 3 for SDHC
	 */
	static uint8_t get_type(SdFat* p_instance) {
		return p_instance->card()->get_type();
	}

	/**
	 * Returns the code of the last error. See sdinfo.h for a list of error codes.
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \return Error code
	 */
	static uint8_t get_error_code(SdFat* p_instance) {
		return p_instance->card()->get_error_code();
	}

	/**
	 * Returns the data byte received on SPI bus as the last error occurred
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \return Last data byte received
	 */
	static uint8_t get_error_data(SdFat* p_instance) {
		return p_instance->card()->get_error_data();
	}

	/**
	 * Reads the SD card's CSD register
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \param[out] p_csd Pointer to buffer for CSD content (buffer size >= 16 byte)
	 * \return Error code: 1 for success, 0 for error of SdCard::read_csd()
	 */
	static uint8_t read_csd(SdFat* p_instance, csd_t* p_csd);

	/**
	 * Reads the SD card's CID register
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \param[out] p_csd Pointer to buffer for CID content (buffer size >= 16 byte)
	 * \return Error code: 1 for success, 0 for error of SdCard::read_csd()
	 */
	static uint8_t read_cid(SdFat* p_instance, cid_t* p_cid);

	/**
	 * Removes a file from the volume working directory
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \param[in] path A path with a valid 8.3 DOS name for the file to be removed
	 * \return Error code: 0 for success, 1 for error of FatFileSystem::remove()
	 */
	static uint8_t remove(SdFat* p_instance, const char* path);

	/**
	 * Renames a file or subdirectory
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \param[in] old_path Path name to the file or subdirectory to be renamed
	 * \param[in] new_path New path name of the file or subdirectory
	 * \return Error code: 0 for success, 1 for error of FatFileSystem::rename()
	 *
	 * The \a newPath object must not exist before the rename call.
	 * The file to be renamed must not be open. The directory entry may be
	 * moved and file system corruption could occur if the file is accessed by
	 * a file object that was opened before the rename() call.
	 */
	static uint8_t rename(SdFat* p_instance, const char* old_path, const char* new_path);

	/**
	 * Writes current block to SD card, if it's dirty
	 * \param[in] p_instance Pointer to SdFat instance (for C bindings)
	 * \return Error code: 0 for success, 1 for error of FatVolume::cacheSync()
	 */
	static uint8_t sync_vol(SdFat* p_instance);

protected:
	static constexpr bool const debug_mode { false }; /**< Switch to enable the debug mode with debugging outputs (on UART) */

	struct debug_times_t {
		uint16_t starttime;
		uint16_t cardcommand;
		uint16_t readdata;
		uint16_t ready;
		uint16_t spi_rcv;
		uint16_t discard_crc;
	};
	static debug_times_t debug_times;
	friend class SdCard; // for access to debug_times
};

#ifdef SDFAT_AVAILABLE
/**
 * Wrapper class for FatFile class of SdFat library to support C language bindings
 */
class FatFileWrapper : protected FatFile {
public:
	/**
	 * Opens a file in the current working directory
	 * \param[in] filename A path with a valid 8.3 DOS name for a file to be opened
	 * \param[out] p_file Pointer to a pointer for the FatFile instance of the opened file
	 * \param[in] mode bitwise-inclusive OR of open mode flags. \see FatFile::open(FatFile*, const char*, uint8_t).
	 * \return 0 for success, 1 for error of FatFile::open()
	 *
	 * Allocates memory for FatFile object in case of success.
	 */
	static uint8_t open(const char* filename, FatFile** p_file, uint8_t mode);

	/**
	 * Returns the current position for a file or directory
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 * \return File position in byte or -1 if size of file > 2^31 - 1
	 */
	static int32_t tell(FatFile* p_instance) {
		const auto pos(p_instance->curPosition());
		return static_cast<int32_t>(pos < INT32_MAX ? pos : -1);
	}

	/**
	 * Sets a file's position
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 * \param[in] offset Offset of new position
	 * \param[in] origin SEEK_SET, SEEK_CUR, or SEEK_END
	 *
	 * The offset and origin arguments specify the position at which the next read or write will occur.
     * Offset must be a 32 bit integer (which may be negative) and origin must be one of the following:
     * SEEK_SET The new access position will be offset bytes from  the  start of the file.
     * SEEK_CUR The new access position will be offset bytes from the current access position;
     *          a negative offset moves the access position backwards in the underlying file.
     * SEEK_END The new access position will be offset bytes from the end of the file.
     *          A negative offset places the access position before the end of file, and
     *          a positive offset places the access position after the end of file.
	 */
	static uint8_t seek(FatFile* p_instance, int32_t offset, uint8_t origin);

	/**
	 * Sets the file's current position to zero
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 */
	static void rewind(FatFile* p_instance) {
		p_instance->rewind();
	}

	/**
	 * Reads data from a file, starting at the current position
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 * \param[out] buffer Pointer to the location that will receive the data, >= length byte
	 * \param[in] length Maximum number of bytes to read
	 * \return For success read() returns the number of bytes read. A value less than \a nbyte, including zero,
	 * will be returned if end of file is reached. If an error occurs, read() returns -1.
	 */
	static int16_t read(FatFile* p_instance, void* buffer, uint16_t length);

	/**
	 * Writes data to an open file, starting at the current position
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 * \param[in] buffer Pointer to the location of the data to be written
	 * \param[in] length Number of bytes to write
	 * \return For success write() returns the number of bytes written, always \a nbyte. If an error occurs, write() returns -1.
	 *
	 * \note Data is moved to the cache but may not be written to the SD card until flush() or close() is called
	 */
	static int16_t write(FatFile* p_instance, const void* buffer, uint16_t length);

	/**
	 * Causes all modified data and directory fields to be written to the SD card
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 * \return 0 for success, 1 for error of FatFile::sync()
	 */
	static uint8_t flush(FatFile* p_instance);

	/**
	 * Closes a file and forces cached data and directory information to be written to the SD card
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 * \return 0 for success, 1 for error of FatFile::close()
	 */
	static uint8_t close(FatFile* p_instance);

	/**
	 * Calls close() and frees the memory allocated by open() for the FatFile object
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 */
	static void free(FatFile* p_instance);

	/**
	 * Returns the total number of bytes in a file
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 * \return The total number of bytes in a file
	 */
	static uint32_t get_filesize(FatFile* p_instance) {
		return p_instance->fileSize();
	}

	/**
	 * Gets a file's name followed by a zero byte
	 * \param[in] p_instance Pointer to FatFile instance returned by FatFileWrapper::open() (for C bindings)
	 * \param[out] p_name An array of characters for the file's name, >= size bytes
	 * \param[in] size The size of the array in bytes. The array must be at least 13 bytes long
	 * \return 0 for success, 1 for error of FatFile::getName()
	 *
	 * \note The file's name will be truncated if the file's name is too long
	 */
	static uint8_t get_filename(FatFile* p_instance, char* p_name, uint16_t size) {
		return ! p_instance->getName(p_name, size);
	}
};
#endif // SDFAT_AVAILABLE
#endif // MMC_AVAILABLE

#endif // __cplusplus
#endif // MCU
#endif /* MCU_SDCARD_WRAPPER_H_ */
