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
 * \file 	sdcard.h
 * \brief 	SdCard class for V2 SD/SDHC cards. Based on Arduino SdSpiCard library by William Greiman.
 * \see		https://github.com/greiman/SdFat
 * \author	William Greiman
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	23.10.2016
 *
 * For use with Arduino SdFat library by William Greiman (https://github.com/greiman/SdFat).
 */


#ifndef SPICARD_H_
#define SPICARD_H_

#ifdef MCU
#include "sdinfo.h"
#include "SdFatConfig.h"
#include "spimaster.h"
#include "spimaster_soft.h"
#include <avr/io.h>
#include <avr/interrupt.h>

extern "C" {
#include "ct-Bot.h"
}


template <class SPI>
class SdCardBase : public SPI {
protected:
	using spi_type = SPI;

};

namespace SdCardTypes {
#ifdef SPI_AVAILABLE
using SpiType = SpiMaster;
#else
using SpiType = SpiMasterSoft;
#endif
} // namespace SdCardTypes

/**
 * \class SdCard
 * \brief Raw access to SD and SDHC flash memory cards via SPI protocol.
 */
class SdCard : public SdCardBase<SdCardTypes::SpiType> {
public:
	/** Construct an instance of SdSpiCard. */
	SdCard() : m_selected(false), m_errorCode(SD_CARD_ERROR_INIT_NOT_CALLED), m_type(0) {}

	/** Initialize the SD card.
	 * \param[in] sckDivisor SPI speed divisor
	 * \return true for success else false.
	 */
	bool init(uint8_t sckDivisor);

	/**
	 * Determines the size of the card.
	 * \return The number of 512 byte data blocks in the card or zero if an error occurs.
	 */
	uint32_t get_size();

	/**
	 * \return code for the last error. See SdSpiCard.h for a list of error codes.
	 */
	int get_error_code() const {
		return m_errorCode;
	}

	/**
	 *  Set SD error code.
	 *  \param[in] code value for error code.
	 */
	void set_error(uint8_t code) {
		m_errorCode = code;
	}

	/** \return error data for last error. */
	int get_error_data() const {
		return m_status;
	}

	/** Return the card type: SD V1, SD V2 or SDHC
	 * \return 0 - SD V1, 1 - SD V2, or 3 - SDHC.
	 */
	uint8_t get_type() const {
		return m_type;
	}

	/**
	 * Read a card's CID register. The CID contains card identification
	 * information such as Manufacturer ID, Product name, Product serial
	 * number and Manufacturing date.
	 * \param[out] cid pointer to area for returned data.
	 * \return true for success or false for failure.
	 */
	bool read_cid(cid_t* cid) {
		return read_register(CMD10, cid);
	}

	/**
	 * Read a card's CSD register. The CSD contains Card-Specific Data that
	 * provides information regarding access to the card's contents.
	 * \param[out] csd pointer to area for returned data.
	 * \return true for success or false for failure.
	 */
	bool read_csd(csd_t* csd) {
		return read_register(CMD9, csd);
	}

	/** Read OCR register.
	 * \param[out] ocr Value of OCR register.
	 * \return true for success else false.
	 */
	bool read_ocr(uint32_t* ocr);

	/**
	 * Read a 512 byte block.
	 * \param[in] block Logical block to be read.
	 * \param[out] dst Pointer to the location that will receive the data.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool read_block(uint32_t block, uint8_t* dst);

	/**
	 * Read multiple 512 byte blocks.
	 * \param[in] block Logical block to be read.
	 * \param[in] count Number of blocks to be read.
	 * \param[out] dst Pointer to the location that will receive the data.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool read_block(uint32_t block, uint8_t* dst, size_t count);

	/**
	 * Writes a 512 byte block.
	 * \param[in] blockNumber Logical block to be written.
	 * \param[in] src Pointer to the location of the data to be written.
	 * \param sync Wait for flash programming to complete?
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool write_block(uint32_t blockNumber, const uint8_t* src, bool sync = false);

	/**
	 * Write multiple 512 byte blocks.
	 * \param[in] block Logical block to be written.
	 * \param[in] count Number of blocks to be written.
	 * \param[in] src Pointer to the location of the data to be written.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool write_block(uint32_t block, const uint8_t* src, size_t count);

private:
	using SPI = spi_type;

	/** Set the SD chip select pin high, send a dummy byte, and call SPI endTransaction.
	 * This function should only be called by programs doing raw I/O to the SD.
	 */
	void cs_high();

	/** Set the SD chip select pin low and call SPI beginTransaction.
	 * This function should only be called by programs doing raw I/O to the SD.
	 */
	void cs_low();

	/** \return the SD chip select status, true if selected else false. */
	bool get_selected() const {
		return m_selected;
	}

	bool error_handler(uint8_t error_code);

	uint8_t send_app_cmd(uint8_t cmd, uint32_t arg) {
		send_cmd(CMD55, 0);
		return send_cmd(cmd, arg);
	}

	uint8_t send_cmd(uint8_t cmd, uint32_t arg);

	bool read_register(uint8_t cmd, void* buf);

	/** Start a read multiple blocks sequence.
	 * \param[in] blockNumber Address of first block in sequence.
	 * \note This function is used with readData() and readStop() for optimized
	 * multiple block reads.  SPI chipSelect must be low for the entire sequence.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool read_start(uint32_t blockNumber);

	/** End a read multiple blocks sequence.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool read_stop();

	/** Read one data block in a multiple block read sequence
	 * \param[out] dst Pointer to the location for the data to be read.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool read_data(uint8_t *dst) {
		return read_data(dst, 512);
	}

	bool read_data(uint8_t* dst, size_t count);

	void set_type(uint8_t value) {
		m_type = value;
	}

	/** Write one data block in a multiple block write sequence.
	 * \param[in] src Pointer to the location of the data to be written.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool write_data(const uint8_t* src);

	bool write_data(uint8_t token, const uint8_t* src);

	/** Start a write multiple blocks sequence.
	 * \param[in] blockNumber Address of first block in sequence.
	 * \param[in] eraseCount The number of blocks to be pre-erased.
	 * \note This function is used with writeData() and writeStop()
	 * for optimized multiple block writes.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool write_start(uint32_t blockNumber, uint32_t eraseCount);

	/** End a write multiple blocks sequence.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool write_stop();

#if SDCARD_ERASE_SUPPORT
	/** Erase a range of blocks.
	 * \param[in] firstBlock The address of the first block in the range.
	 * \param[in] lastBlock The address of the last block in the range.
	 * \note This function requests the SD card to do a flash erase for a
	 * range of blocks.  The data on the card after an erase operation is
	 * either 0 or 1, depends on the card vendor.  The card must support
	 * single block erase.
	 * \return The value true is returned for success and
	 * the value false is returned for failure.
	 */
	bool erase(uint32_t firstBlock, uint32_t lastBlock);

	/** Determine if card supports single block erase.
	 *
	 * \return true is returned if single block erase is supported.
	 * false is returned if single block erase is not supported.
	 */
	bool single_block_erasable();
#endif // SDCARD_ERASE_SUPPORT

	bool m_selected;
	uint8_t m_errorCode;
	uint8_t m_status;
	uint8_t m_type;
};

#endif // MCU
#endif /* SPICARD_H_ */
