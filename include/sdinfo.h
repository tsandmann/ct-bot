/* Arduino SdSpiCard Library
 * Copyright (C) 2012 by William Greiman
 *
 * This file is part of the Arduino SdSpiCard Library
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
 * along with the Arduino SdSpiCard Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef SDINFO_H_
#define SDINFO_H_

#ifdef MCU
#include <stdint.h>

// Based on the document:
//
// SD Specifications
// Part 1
// Physical Layer
// Simplified Specification
// Version 3.01
// May 18, 2010
//
// http://www.sdcard.org/developers/tech/sdcard/pls/simplified_specs

#ifdef __cplusplus
/* SD card errors */
/** timeout error for command CMD0 (initialize card in SPI mode) */
constexpr uint8_t const SD_CARD_ERROR_CMD0 = 0x01;
/** CMD8 was not accepted - not a valid SD card */
constexpr uint8_t const SD_CARD_ERROR_CMD8 = 0x02;
/** card returned an error response for CMD12 (stop multiblock read) */
constexpr uint8_t const SD_CARD_ERROR_CMD12 = 0x03;
/** card returned an error response for CMD17 (read block) */
constexpr uint8_t const SD_CARD_ERROR_CMD17 = 0x04;
/** card returned an error response for CMD18 (read multiple block) */
constexpr uint8_t const SD_CARD_ERROR_CMD18 = 0x05;
/** card returned an error response for CMD24 (write block) */
constexpr uint8_t const SD_CARD_ERROR_CMD24 = 0x06;
/**  WRITE_MULTIPLE_BLOCKS command failed */
constexpr uint8_t const SD_CARD_ERROR_CMD25 = 0x07;
/** card returned an error response for CMD58 (read OCR) */
constexpr uint8_t const SD_CARD_ERROR_CMD58 = 0x08;
/** SET_WR_BLK_ERASE_COUNT failed */
constexpr uint8_t const SD_CARD_ERROR_ACMD23 = 0x09;
/** ACMD41 initialization process timeout */
constexpr uint8_t const SD_CARD_ERROR_ACMD41 = 0x0a;
/** card returned a bad CSR version field */
constexpr uint8_t const SD_CARD_ERROR_BAD_CSD = 0x0b;
/** erase block group command failed */
constexpr uint8_t const SD_CARD_ERROR_ERASE = 0x0c;
/** card not capable of single block erase */
constexpr uint8_t const SD_CARD_ERROR_ERASE_SINGLE_BLOCK = 0x0d;
/** Erase sequence timed out */
constexpr uint8_t const SD_CARD_ERROR_ERASE_TIMEOUT = 0x0e;
/** card returned an error token instead of read data */
constexpr uint8_t const SD_CARD_ERROR_READ = 0x0f;
/** read CID or CSD failed */
constexpr uint8_t const SD_CARD_ERROR_READ_REG = 0x10;
/** timeout while waiting for start of read data */
constexpr uint8_t const SD_CARD_ERROR_READ_TIMEOUT = 0x11;
/** card did not accept STOP_TRAN_TOKEN */
constexpr uint8_t const SD_CARD_ERROR_STOP_TRAN = 0x12;
/** card returned an error token as a response to a write operation */
constexpr uint8_t const SD_CARD_ERROR_WRITE = 0x13;
/** card returned an error to a CMD13 status check after a write */
constexpr uint8_t const SD_CARD_ERROR_WRITE_PROGRAMMING = 0x16;
/** timeout occurred during write programming */
constexpr uint8_t const SD_CARD_ERROR_WRITE_TIMEOUT = 0x17;
/** incorrect rate selected */
constexpr uint8_t const SD_CARD_ERROR_SCK_RATE = 0x18;
/** init() not called */
constexpr uint8_t const SD_CARD_ERROR_INIT_NOT_CALLED = 0x19;
/** card returned an error for CMD59 (CRC_ON_OFF) */
constexpr uint8_t const SD_CARD_ERROR_CMD59 = 0x1a;
/** invalid read CRC */
constexpr uint8_t const SD_CARD_ERROR_READ_CRC = 0x1b;
/** SPI DMA error */
constexpr uint8_t const SD_CARD_ERROR_SPI_DMA = 0x1c;
/** CMD6 not accepted */
constexpr uint8_t const SD_CARD_ERROR_CMD6 = 0x1d;
/** send command timeout */
constexpr uint8_t const SD_CARD_ERROR_COMMAND = 0x1e;

/* card types */
/** Standard capacity V1 SD card */
constexpr uint8_t const SD_CARD_TYPE_SD1  = 1;
/** Standard capacity V2 SD card */
constexpr uint8_t const SD_CARD_TYPE_SD2  = 2;
/** High Capacity SD card */
constexpr uint8_t const SD_CARD_TYPE_SDHC = 3;

/* SD operation timeouts */
/** init timeout ms */
constexpr uint32_t const SD_INIT_TIMEOUT = 2000UL;
/** erase timeout ms */
constexpr uint32_t const SD_ERASE_TIMEOUT = 10000UL;
/** read timeout ms */
constexpr uint32_t const SD_READ_TIMEOUT = 600UL;
/** write time out ms */
constexpr uint32_t const SD_WRITE_TIMEOUT = 2000UL;

/* SD card commands */
/** GO_IDLE_STATE - init card in spi mode if CS low */
constexpr uint8_t const CMD0 = 0x00;
/** SWITCH_FUNC - Switch Function Command */
constexpr uint8_t const CMD6 = 0x06;
/** SEND_IF_COND - verify SD Memory Card interface operating condition.*/
constexpr uint8_t const CMD8 = 0x08;
/** SEND_CSD - read the Card Specific Data (CSD register) */
constexpr uint8_t const CMD9 = 0x09;
/** SEND_CID - read the card identification information (CID register) */
constexpr uint8_t const CMD10 = 0x0a;
/** STOP_TRANSMISSION - end multiple block read sequence */
constexpr uint8_t const CMD12 = 0x0c;
/** SEND_STATUS - read the card status register */
constexpr uint8_t const CMD13 = 0x0d;
/** READ_SINGLE_BLOCK - read a single data block from the card */
constexpr uint8_t const CMD17 = 0x11;
/** READ_MULTIPLE_BLOCK - read a multiple data blocks from the card */
constexpr uint8_t const CMD18 = 0x12;
/** WRITE_BLOCK - write a single data block to the card */
constexpr uint8_t const CMD24 = 0x18;
/** WRITE_MULTIPLE_BLOCK - write blocks of data until a STOP_TRANSMISSION */
constexpr uint8_t const CMD25 = 0x19;
/** ERASE_WR_BLK_START - sets the address of the first block to be erased */
constexpr uint8_t const CMD32 = 0x20;
/** ERASE_WR_BLK_END - sets the address of the last block of the continuous range to be erased */
constexpr uint8_t const CMD33 = 0x21;
/** ERASE - erase all previously selected blocks */
constexpr uint8_t const CMD38 = 0x26;
/** APP_CMD - escape for application specific command */
constexpr uint8_t const CMD55 = 0x37;
/** READ_OCR - read the OCR register of a card */
constexpr uint8_t const CMD58 = 0x3a;
/** CRC_ON_OFF - enable or disable CRC checking */
constexpr uint8_t const CMD59 = 0x3b;
/** SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be pre-erased before writing */
constexpr uint8_t const ACMD23 = 0x17;
/** SD_SEND_OP_COMD - Sends host capacity support information and activates the card's initialization process */
constexpr uint8_t const ACMD41 = 0x29;

/** status for card in the ready state */
constexpr uint8_t const R1_READY_STATE = 0x00;
/** status for card in the idle state */
constexpr uint8_t const R1_IDLE_STATE = 0x01;
/** status bit for illegal command */
constexpr uint8_t const R1_ILLEGAL_COMMAND = 0x04;
/** start data token for read or write single block */
constexpr uint8_t const DATA_START_BLOCK = 0xfe;
/** stop token for write multiple blocks */
constexpr uint8_t const STOP_TRAN_TOKEN = 0xfd;
/** start data token for write multiple blocks */
constexpr uint8_t const WRITE_MULTIPLE_TOKEN = 0xfc;
/** mask for data response tokens after a write block operation */
constexpr uint8_t const DATA_RES_MASK = 0x1f;
/** write data accepted token */
constexpr uint8_t const DATA_RES_ACCEPTED = 0x05;
#endif // __cplusplus

/**
 * \class CID
 * \brief Card IDentification (CID) register
 */
typedef struct CID {
  // byte 0
  /** Manufacturer ID */
  unsigned char mid;
  // byte 1-2
  /** OEM/Application ID */
  char oid[2];
  // byte 3-7
  /** Product name */
  char pnm[5];
  // byte 8
  /** Product revision least significant digit */
  unsigned char prv_m : 4;
  /** Product revision most significant digit */
  unsigned char prv_n : 4;
  // byte 9-12
  /** Product serial number */
  uint32_t psn;
  // byte 13
  /** Manufacturing date year high digit */
  unsigned char mdt_year_high : 4;
  /** not used */
  unsigned char reserved : 4;
  // byte 14
  /** Manufacturing date month */
  unsigned char mdt_month : 4;
  /** Manufacturing date year low digit */
  unsigned char mdt_year_low : 4;
  // byte 15
  /** not used always 1 */
  unsigned char always1 : 1;
  /** CRC7 checksum */
  unsigned char crc : 7;
} __attribute__((packed)) cid_t;

/**
 * \class CSDV1
 * \brief CSD register for version 1.00 cards
 */
typedef struct CSDV1 {
  // byte 0
  unsigned char reserved1 : 6;
  unsigned char csd_ver : 2;
  // byte 1
  unsigned char taac;
  // byte 2
  unsigned char nsac;
  // byte 3
  unsigned char tran_speed;
  // byte 4
  unsigned char ccc_high;
  // byte 5
  unsigned char read_bl_len : 4;
  unsigned char ccc_low : 4;
  // byte 6
  unsigned char c_size_high : 2;
  unsigned char reserved2 : 2;
  unsigned char dsr_imp : 1;
  unsigned char read_blk_misalign : 1;
  unsigned char write_blk_misalign : 1;
  unsigned char read_bl_partial : 1;
  // byte 7
  unsigned char c_size_mid;
  // byte 8
  unsigned char vdd_r_curr_max : 3;
  unsigned char vdd_r_curr_min : 3;
  unsigned char c_size_low : 2;
  // byte 9
  unsigned char c_size_mult_high : 2;
  unsigned char vdd_w_cur_max : 3;
  unsigned char vdd_w_curr_min : 3;
  // byte 10
  unsigned char sector_size_high : 6;
  unsigned char erase_blk_en : 1;
  unsigned char c_size_mult_low : 1;
  // byte 11
  unsigned char wp_grp_size : 7;
  unsigned char sector_size_low : 1;
  // byte 12
  unsigned char write_bl_len_high : 2;
  unsigned char r2w_factor : 3;
  unsigned char reserved3 : 2;
  unsigned char wp_grp_enable : 1;
  // byte 13
  unsigned char reserved4 : 5;
  unsigned char write_partial : 1;
  unsigned char write_bl_len_low : 2;
  // byte 14
  unsigned char reserved5: 2;
  unsigned char file_format : 2;
  unsigned char tmp_write_protect : 1;
  unsigned char perm_write_protect : 1;
  unsigned char copy : 1;
  /** Indicates the file format on the card */
  unsigned char file_format_grp : 1;
  // byte 15
  unsigned char always1 : 1;
  unsigned char crc : 7;
} __attribute__((packed)) csd1_t;

/**
 * \class CSDV2
 * \brief CSD register for version 2.00 cards
 */
typedef struct CSDV2 {
  // byte 0
  unsigned char reserved1 : 6;
  unsigned char csd_ver : 2;
  // byte 1
  /** fixed to 0X0E */
  unsigned char taac;
  // byte 2
  /** fixed to 0 */
  unsigned char nsac;
  // byte 3
  unsigned char tran_speed;
  // byte 4
  unsigned char ccc_high;
  // byte 5
  /** This field is fixed to 9h, which indicates READ_BL_LEN=512 Byte */
  unsigned char read_bl_len : 4;
  unsigned char ccc_low : 4;
  // byte 6
  /** not used */
  unsigned char reserved2 : 4;
  unsigned char dsr_imp : 1;
  /** fixed to 0 */
  unsigned char read_blk_misalign : 1;
  /** fixed to 0 */
  unsigned char write_blk_misalign : 1;
  /** fixed to 0 - no partial read */
  unsigned char read_bl_partial : 1;
  // byte 7
  /** high part of card size */
  unsigned char c_size_high : 6;
  /** not used */
  unsigned char reserved3 : 2;
  // byte 8
  /** middle part of card size */
  unsigned char c_size_mid;
  // byte 9
  /** low part of card size */
  unsigned char c_size_low;
  // byte 10
  /** sector size is fixed at 64 KB */
  unsigned char sector_size_high : 6;
  /** fixed to 1 - erase single is supported */
  unsigned char erase_blk_en : 1;
  /** not used */
  unsigned char reserved4 : 1;
  // byte 11
  unsigned char wp_grp_size : 7;
  /** sector size is fixed at 64 KB */
  unsigned char sector_size_low : 1;
  // byte 12
  /** write_bl_len fixed for 512 byte blocks */
  unsigned char write_bl_len_high : 2;
  /** fixed value of 2 */
  unsigned char r2w_factor : 3;
  /** not used */
  unsigned char reserved5 : 2;
  /** fixed value of 0 - no write protect groups */
  unsigned char wp_grp_enable : 1;
  // byte 13
  unsigned char reserved6 : 5;
  /** always zero - no partial block read*/
  unsigned char write_partial : 1;
  /** write_bl_len fixed for 512 byte blocks */
  unsigned char write_bl_len_low : 2;
  // byte 14
  unsigned char reserved7: 2;
  /** Do not use always 0 */
  unsigned char file_format : 2;
  unsigned char tmp_write_protect : 1;
  unsigned char perm_write_protect : 1;
  unsigned char copy : 1;
  /** Do not use always 0 */
  unsigned char file_format_grp : 1;
  // byte 15
  /** not used always 1 */
  unsigned char always1 : 1;
  /** checksum */
  unsigned char crc : 7;
} __attribute__((packed)) csd2_t;

/**
 * \class csd_t
 * \brief Union of old and new style CSD register.
 */
typedef union {
  csd1_t v1;
  csd2_t v2;
  uint8_t raw[sizeof(csd2_t)];
} csd_t;

#endif // MCU
#endif /* SDINFO_H_ */
