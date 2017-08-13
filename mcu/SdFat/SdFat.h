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
 * \brief SdFat class
 */

#ifndef SDFAT_H_
#define SDFAT_H_

#ifdef MCU
#include "sdcard.h"
#include "Print.h"
#include "FatLib/FatLib.h"
#include "ctbot_comp.h"

extern "C" {
#include "ct-Bot.h"
}

#ifdef MMC_AVAILABLE

/** SdFat version YYYYMMDD */
#define SD_FAT_VERSION 20160719

/**
 * \class SdFatInterface
 * \brief Virtual base class interface for SdFat library.
 */
class SdFatInterface {
protected:
	virtual bool readBlock(uint32_t block, uint8_t* dst) = 0;
	virtual bool writeBlock(uint32_t block, const uint8_t* src) = 0;
	virtual bool readBlocks(uint32_t block, uint8_t* dst, size_t n) = 0;
	virtual bool writeBlocks(uint32_t block, const uint8_t* src, size_t n) = 0;
};

/**
 * \class SdFatBase
 * \brief Base class for SdFat library.
 */
class SdFatBase : public SdFatInterface
#ifdef SDFAT_AVAILABLE
	, public FatFileSystem
#endif
{
private:
	SdCard m_sdCard;

public:
	/** Initialize SD card and file system.
	 * \param[in] spi SPI object for the card.
	 * \param[in] divisor SPI divisor.
	 * \return true for success else false.
	 */
	bool begin(uint8_t divisor = 2) {
		const auto res(m_sdCard.init(divisor));
#ifdef SDFAT_AVAILABLE
		if (res) {
			const auto lock_set(m_sdCard.os_lock());
			const auto res(FatFileSystem::begin());
			m_sdCard.os_unlock(lock_set);
			return res;
		}
#endif
		return res;
	}

	/** \return Pointer to SD card object */
	decltype(m_sdCard)* card() {
		return &m_sdCard;
	}

#ifdef SDFAT_AVAILABLE
	/** Diagnostic call to initialize FatFileSystem - use for
	 *  diagnostic purposes only.
	 *  \return true for success else false.
	 */
	bool fsBegin() {
		return FatFileSystem::begin();
	}
#endif // SDFAT_AVAILABLE

private:
	uint8_t cardErrorCode() const {
		return m_sdCard.get_error_code();
	}

	uint8_t cardErrorData() const {
		return m_sdCard.get_error_data();
	}

	virtual bool readBlock(uint32_t block, uint8_t* dst) override {
		return m_sdCard.read_block(block, dst);
	}

	virtual bool writeBlock(uint32_t block, const uint8_t* src) override {
		return m_sdCard.write_block(block, src);
	}

	virtual bool readBlocks(uint32_t block, uint8_t* dst, size_t n) override {
		return m_sdCard.read_block(block, dst, n);
	}

	virtual bool writeBlocks(uint32_t block, const uint8_t* src, size_t n) override {
		return m_sdCard.write_block(block, src, n);
	}
};

/**
 * \class SdFat
 * \brief Main file system class for SdFat library.
 */
class SdFat : public SdFatBase {
public:
	/** Initialize SD card and file system.
	 * \param[in] divisor SPI divisor.
	 * \return true for success else false.
	 */
	bool init(uint8_t divisor = 2) {
		return SdFatBase::begin(divisor);
	}
};

#endif // MMC_AVAILABLE
#endif // MCU
#endif /* SDFAT_H_ */
