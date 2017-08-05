/* Arduino SdFat Library
 * Copyright (C) 2012 by William Greiman
 *
 * This file is part of the Arduino SdFat Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino SdFat Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * \brief Configuration definitions. Adapted for ct-Bot framework.
 */
#ifndef SdFatConfig_h
#define SdFatConfig_h

#ifdef MCU
#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>

/**
 * Set USE_LONG_FILE_NAMES nonzero to use long file names (LFN).
 * Long File Name are limited to a maximum length of 255 characters.
 *
 * This implementation allows 7-bit characters in the range
 * 0X20 to 0X7E except the following characters are not allowed:
 *
 *  < (less than)
 *  > (greater than)
 *  : (colon)
 *  " (double quote)
 *  / (forward slash)
 *  \ (backslash)
 *  | (vertical bar or pipe)
 *  ? (question mark)
 *  * (asterisk)
 *
 */
#define USE_LONG_FILE_NAMES 0

/** 
 * Set MAINTAIN_FREE_CLUSTER_COUNT nonzero to keep the count of free clusters
 * updated.  This will increase the speed of the freeClusterCount() call
 * after the first call. Extra flash will be required.
 */
#define MAINTAIN_FREE_CLUSTER_COUNT 1

/**
 * To enable SD card CRC checking set USE_SD_CRC nonzero.
 *
 * Set USE_SD_CRC to 1 to use a smaller CRC-CCITT function.  This function
 * is slower for AVR but may be fast for ARM and other processors.
 *
 * Set USE_SD_CRC to 2 to used a larger table driven CRC-CCITT function.  This
 * function is faster for AVR but may be slower for ARM and other processors.
 */
#define USE_SD_CRC 0

/**
 * Set FAT12_SUPPORT nonzero to enable use if FAT12 volumes.
 * FAT12 has not been well tested and requires additional flash.
 */
#define FAT12_SUPPORT 0

/**
 * Set DESTRUCTOR_CLOSES_FILE nonzero to close a file in its destructor.
 *
 * Causes use of lots of heap in ARM.
 */
#define DESTRUCTOR_CLOSES_FILE 0

/**
 * SPI SCK divisor for SD initialization commands.
 */
constexpr uint8_t const SPI_SCK_INIT_DIVISOR = 128;

/**
 * Set USE_SEPARATE_FAT_CACHE nonzero to use a second 512 byte cache
 * for FAT table entries. This improves performance for large writes
 * that are not a multiple of 512 bytes.
 */
#define USE_SEPARATE_FAT_CACHE 1

#ifndef __AVR_ATmega1284P__
#undef USE_SEPARATE_FAT_CACHE
#define USE_SEPARATE_FAT_CACHE 0
#endif // __AVR_ATmega1284P__

/**
 * Set USE_MULTI_BLOCK_IO nonzero to use multi-block SD read/write.
 * Don't use mult-block read/write on small AVR boards.
 */
#define USE_MULTI_BLOCK_IO 1


#define SDCARD_ERASE_SUPPORT 1

#define SDFAT_PRINT_SUPPORT 0

#define SDFAT_WIPE_SUPPORT 0

#endif // MCU
#endif // SdFatConfig_h
