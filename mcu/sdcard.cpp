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
 * \file 	mcu/sdcard.cpp
 * \brief 	SdCard class for V2 SD/SDHC cards. Based on Arduino SdSpiCard library by William Greiman.
 * \see		https://github.com/greiman/SdFat
 * \author	William Greiman
 * \author	Timo Sandmann
 * \date 	23.10.2016
 *
 * For use with Arduino SdFat library by William Greiman (https://github.com/greiman/SdFat).
 */

#ifdef MCU
#include "sdcard.h"
#include "sdcard_wrapper.h"

#ifdef MMC_AVAILABLE

#define DEBUG_SDFAT

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

extern "C" {
#include "ena.h"
#include "led.h"
#include "log.h"
}

#if USE_SD_CRC
// CRC functions
static uint8_t CRC7(const uint8_t* data, uint8_t n) {
	uint8_t crc = 0;
	for (uint8_t i = 0; i < n; i++) {
		uint8_t d = data[i];
		for (uint8_t j = 0; j < 8; j++) {
			crc <<= 1;
			if ((d & 0x80) ^ (crc & 0x80)) {
				crc ^= 0x09;
			}
			d <<= 1;
		}
	}
	return (crc << 1) | 1;
}

#if USE_SD_CRC == 1
// slower CRC-CCITT
// uses the x^16,x^12,x^5,x^1 polynomial
static uint16_t CRC_CCITT(const uint8_t* data, size_t n) {
	uint16_t crc = 0;
	for (size_t i(0); i < n; ++i) {
		crc = (uint8_t) (crc >> 8) | (crc << 8);
		crc ^= data[i];
		crc ^= (uint8_t) (crc & 0xff) >> 4;
		crc ^= crc << 12;
		crc ^= (crc & 0xff) << 5;
	}
	return crc;
}
#elif USE_SD_CRC > 1 // CRC_CCITT

// faster CRC-CCITT
// uses the x^16,x^12,x^5,x^1 polynomial
static const uint16_t crctab[] PROGMEM = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static uint16_t CRC_CCITT(const uint8_t* data, size_t n) {
	uint16_t crc = 0;
	for (size_t i(0); i < n; ++i) {
		crc = pgm_read_word(&crctab[(crc >> 8 ^ data[i]) & 0xff]) ^ (crc << 8);
	}
	return crc;
}
#endif // CRC_CCITT
#endif // USE_SD_CRC

bool SdCard::init(uint8_t sckDivisor) {
	m_errorCode = m_type = 0;
	const auto t0(TIMER_GET_TICKCOUNT_16);

	/* initialize SPI bus */
	SPI::init();
	/* set SCK rate for initialization commands */
	SPI::set_speed(SPI_SCK_INIT_DIVISOR);

	/* toggle chip select */
	cs_low();
	cs_high();

	/* must supply min of 74 clock cycles with CS high */
	for (uint8_t i(0); i < 16; ++i) {
		SPI::send(0xff);
	}

	/* command to go idle in SPI mode */
	while (send_cmd(CMD0, 0) != R1_IDLE_STATE) {
		if ((TIMER_GET_TICKCOUNT_16 - t0) > MS_TO_TICKS(SD_INIT_TIMEOUT)) {
			return error_handler(SD_CARD_ERROR_CMD0);
		}
	}

#if USE_SD_CRC
	if (send_cmd(CMD59, 1) != R1_IDLE_STATE) {
		return error_handler(SD_CARD_ERROR_CMD59);
	}
#endif // USE_SD_CRC

	/* check SD version */
	while (1) {
		if (send_cmd(CMD8, 0x1AA) == (R1_ILLEGAL_COMMAND | R1_IDLE_STATE)) {
			set_type(SD_CARD_TYPE_SD1);
			break;
		}

		for (uint8_t i(0); i < 4; ++i) {
			m_status = SPI::receive();
		}

		if (m_status == 0xaa) {
			set_type(SD_CARD_TYPE_SD2);
			break;
		}

		if ((TIMER_GET_TICKCOUNT_16 - t0) > MS_TO_TICKS(SD_INIT_TIMEOUT)) {
			return error_handler(SD_CARD_ERROR_CMD8);
		}
	}

	/* initialize card and send host supports SDHC if SD2 */
	const uint32_t arg(get_type() == SD_CARD_TYPE_SD2 ? 0x40000000 : 0);
	while (send_app_cmd(ACMD41, arg) != R1_READY_STATE) {
		/* check for timeout */
		if ((TIMER_GET_TICKCOUNT_16 - t0) > MS_TO_TICKS(SD_INIT_TIMEOUT)) {
			return error_handler(SD_CARD_ERROR_ACMD41);
		}
	}

	/* if SD2 read OCR register to check for SDHC card */
	if (get_type() == SD_CARD_TYPE_SD2) {
		if (send_cmd(CMD58, 0)) {
			return error_handler(SD_CARD_ERROR_CMD58);
		}

		if ((SPI::receive() & 0xc0) == 0xc0) {
			set_type(SD_CARD_TYPE_SDHC);
		}

		/* Discard rest of ocr - contains allowed voltage range */
		for (uint8_t i(0); i < 3; ++i) {
			SPI::receive();
		}
	}

	cs_high();
	SPI::set_speed(sckDivisor);

	LOG_DEBUG("SdCard::init(): done");
#ifdef LED_AVAILABLE
	LED_off(LED_GRUEN | LED_ROT | LED_TUERKIS);
#endif //LED_AVAILABLE
	return true;
}

// send command and return error code. Return zero for OK
uint8_t SdCard::send_cmd(uint8_t cmd, uint32_t arg) {
	/* select card */
	if (! m_selected) {
		cs_low();
	}

	/* wait if busy */
	if (! SPI::wait_not_busy(SD_WRITE_TIMEOUT) && cmd != CMD0) {
		m_status = 0xff;
		return ! error_handler(SD_CARD_ERROR_COMMAND);
	}

	/* form message */
	uint8_t buf[6];
	buf[0] = static_cast<uint8_t>(0x40U | cmd);
	buf[1] = static_cast<uint8_t>(arg >> 24U);
	buf[2] = static_cast<uint8_t>(arg >> 16U);
	buf[3] = static_cast<uint8_t>(arg >> 8U);
	buf[4] = static_cast<uint8_t>(arg);

	// add CRC
#if USE_SD_CRC
	buf[5] = CRC7(buf, 5);
#else // USE_SD_CRC
	buf[5] = cmd == CMD0 ? 0x95 : 0x87;
#endif // USE_SD_CRC

	/* send message */
	SPI::send(buf, sizeof(buf));

	/* skip stuff byte for stop read */
	if (cmd == CMD12) {
		SPI::receive();
	}

	/* wait for response */
	for (uint8_t i(0); ((m_status = SPI::receive()) & 0x80) && (i != 0xff); ++i) {}
	return m_status;
}

uint32_t SdCard::get_size() {
	csd_t csd;
	if (! read_csd(&csd)) {
		return 0;
	}
	if (csd.v1.csd_ver == 0) {
		const auto read_bl_len(csd.v1.read_bl_len);
		const uint16_t c_size((csd.v1.c_size_high << 10) | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low);
		const uint8_t c_size_mult((csd.v1.c_size_mult_high << 1) | csd.v1.c_size_mult_low);
		return static_cast<uint32_t>(c_size + 1) << (c_size_mult + read_bl_len - 7);
	} else if (csd.v2.csd_ver == 1) {
		const uint32_t c_size(0x10000L * csd.v2.c_size_high + 0x100L * (uint32_t) csd.v2.c_size_mid + csd.v2.c_size_low);
		return (c_size + 1) << 10;
	} else {
		set_error(SD_CARD_ERROR_BAD_CSD);
		return 0;
	}
}

void SdCard::cs_high() {
	m_selected = false;
	ENA_off(ENA_MMC);
	/* insure MISO goes high impedance */
	SPI::send(0xff);
}

void SdCard::cs_low() {
	m_selected = true;
	ENA_on(ENA_MMC);
}

bool SdCard::read_block(uint32_t blockNumber, uint8_t* dst) {
	/* use address if not SDHC card */
	if (get_type() != SD_CARD_TYPE_SDHC) {
		blockNumber <<= 9;
	}

	if (send_cmd(CMD17, blockNumber)) {
		return error_handler(SD_CARD_ERROR_CMD17);
	}
	if (SdFatWrapper::debug_mode) {
		SdFatWrapper::debug_times.cardcommand = timer_get_us8();
	}

	if (! read_data(dst, 512)) {
		return error_handler(0);
	}
	if (SdFatWrapper::debug_mode) {
		SdFatWrapper::debug_times.readdata = timer_get_us8();
	}

	cs_high();
	return true;
}

bool SdCard::read_block(uint32_t block, uint8_t* dst, size_t count) {
	if (! read_start(block)) {
		return false;
	}

	for (uint16_t b(0); b < count; ++b, dst += 512) {
		if (! read_data(dst, 512)) {
			return false;
		}
	}

	return read_stop();
}

bool SdCard::read_data(uint8_t* dst, size_t count) {
#ifdef LED_AVAILABLE
	LED_on(LED_GRUEN);
#endif // LED_AVAILABLE

#if USE_SD_CRC
	uint16_t crc;
#endif // USE_SD_CRC
	/* wait for start block token */
	const auto starttime(TIMER_GET_TICKCOUNT_16);
	const uint16_t timeout_ticks(SD_READ_TIMEOUT * (1000U / TIMER_STEPS + 1));
	while ((m_status = SPI::receive()) == 0xff) {
		if (static_cast<uint16_t>(tickCount.u16 - starttime) > timeout_ticks) {
			return error_handler(SD_CARD_ERROR_READ_TIMEOUT);
		}
	}

	if (m_status != DATA_START_BLOCK) {
//		LOG_DEBUG("read_data(): m_status=%u", m_status);
		return error_handler(SD_CARD_ERROR_READ);
	}
	if (SdFatWrapper::debug_mode) {
		SdFatWrapper::debug_times.ready = timer_get_us8();
	}

	/* transfer data */
	SPI::receive(dst, count);
	if (SdFatWrapper::debug_mode) {
		SdFatWrapper::debug_times.spi_rcv = timer_get_us8();
	}

#if USE_SD_CRC
	// get crc
	crc = (SPI::receive() << 8) | SPI::receive();
	if (crc != CRC_CCITT(dst, count)) {
		return error_handler(SD_CARD_ERROR_READ_CRC);
	}
#else
	/* discard crc */
	SPI::receive();
	SPI::receive();
#endif // USE_SD_CRC
	if (SdFatWrapper::debug_mode) {
		SdFatWrapper::debug_times.discard_crc = timer_get_us8();
	}

#ifdef LED_AVAILABLE
	LED_off(LED_GRUEN);
#endif // LED_AVAILABLE
	return true;
}

bool SdCard::read_ocr(uint32_t* ocr) {
	uint8_t* p(reinterpret_cast<uint8_t*>(ocr));
	if (send_cmd(CMD58, 0)) {
		return error_handler(SD_CARD_ERROR_CMD58);
	}

	for (uint8_t i(0); i < 4; ++i) {
		p[3 - i] = SPI::receive();
	}

	cs_high();
	return true;
}

bool SdCard::read_register(uint8_t cmd, void* buf) {
	uint8_t* dst(reinterpret_cast<uint8_t*>(buf));
	if (send_cmd(cmd, 0)) {
		return error_handler(SD_CARD_ERROR_READ_REG);
	}

	if (! read_data(dst, 16)) {
		return error_handler(0);
	}

	cs_high();
	return true;
}

bool SdCard::read_start(uint32_t blockNumber) {
	if (get_type() != SD_CARD_TYPE_SDHC) {
		blockNumber <<= 9;
	}

	if (send_cmd(CMD18, blockNumber)) {
		return error_handler(SD_CARD_ERROR_CMD18);
	}

	return true;
}

bool SdCard::read_stop() {
	if (send_cmd(CMD12, 0)) {
		return error_handler(SD_CARD_ERROR_CMD12);
	}

	cs_high();
	return true;
}

bool SdCard::write_block(uint32_t blockNumber, const uint8_t* src, bool sync) {
	if (get_type() != SD_CARD_TYPE_SDHC) {
		/* use address if not SDHC card */
		blockNumber <<= 9;
	}

	if (send_cmd(CMD24, blockNumber)) {
		return error_handler(SD_CARD_ERROR_CMD24);
	}

	if (! write_data(DATA_START_BLOCK, src)) {
		return error_handler(0);
	}

	if (sync) {
		/* wait for flash programming to complete */
		if (! SPI::wait_not_busy(SD_WRITE_TIMEOUT)) {
			return error_handler(SD_CARD_ERROR_WRITE_TIMEOUT);
		}

		/* response is r2 so get and check two bytes for nonzero */
		if (send_cmd(CMD13, 0) || SPI::receive()) {
			return error_handler(SD_CARD_ERROR_WRITE_PROGRAMMING);
		}
	}

	cs_high();
	return true;
}

bool SdCard::write_block(uint32_t block, const uint8_t* src, size_t count) {
	if (! write_start(block, count)) {
		return error_handler(0);
	}

	for (size_t b(0); b < count; ++b, src += 512) {
		if (! write_data(src)) {
			return error_handler(0);
		}
	}

	return write_stop();
}

bool SdCard::write_data(const uint8_t* src) {
	/* wait for previous write to finish */
	if (! SPI::wait_not_busy(SD_WRITE_TIMEOUT)) {
		return error_handler(SD_CARD_ERROR_WRITE_TIMEOUT);
	}

	if (! write_data(WRITE_MULTIPLE_TOKEN, src)) {
		return error_handler(0);
	}

	return true;
}

// send one block of data for write block or write multiple blocks
bool SdCard::write_data(uint8_t token, const uint8_t* src) {
#ifdef LED_AVAILABLE
	LED_on(LED_ROT);
#endif // LED_AVAILABLE
#if USE_SD_CRC
	uint16_t crc = CRC_CCITT(src, 512);
#else // USE_SD_CRC
	uint16_t crc = 0xffff;
#endif // USE_SD_CRC
	SPI::send(token);
	SPI::send(src, 512);
	SPI::send(crc >> 8);
	SPI::send(crc & 0xff);

	m_status = SPI::receive();

#ifdef LED_AVAILABLE
	LED_off(LED_ROT);
#endif // LED_AVAILABLE

	if ((m_status & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
		return error_handler(SD_CARD_ERROR_WRITE);
	}

	return true;
}

bool SdCard::write_start(uint32_t blockNumber, uint32_t eraseCount) {
	/* send pre-erase count */
	if (send_app_cmd(ACMD23, eraseCount)) {
		return error_handler(SD_CARD_ERROR_ACMD23);
	}

	/* use address if not SDHC card */
	if (get_type() != SD_CARD_TYPE_SDHC) {
		blockNumber <<= 9;
	}

	if (send_cmd(CMD25, blockNumber)) {
		return error_handler(SD_CARD_ERROR_CMD25);
	}

	return true;
}

bool SdCard::write_stop() {
	if (! SPI::wait_not_busy(SD_WRITE_TIMEOUT)) {
		return error_handler(SD_CARD_ERROR_STOP_TRAN);
	}

	SPI::send(STOP_TRAN_TOKEN);
	if (! SPI::wait_not_busy(SD_WRITE_TIMEOUT)) {
		return error_handler(SD_CARD_ERROR_STOP_TRAN);
	}

	cs_high();
	return true;
}

bool SdCard::error_handler(uint8_t error_code) {
#ifdef LOG_AVAILABLE
	const auto now32(TIMER_GET_TICKCOUNT_32);
#endif
	if (error_code) {
		set_error(error_code);
		LOG_DEBUG("SdCard::error_handler(): error=0x%02x 0x%02x", error_code, m_status);
		LOG_DEBUG(" time=0x%04x%04x", static_cast<uint16_t>(now32 >> 16), static_cast<uint16_t>(now32 & 0xffff));
	}
	cs_high();
#ifdef LED_AVAILABLE
	auto leds(LED_get());
	leds &= LED_GRUEN;
	leds &= LED_ROT;
	leds |= LED_TUERKIS;
	LED_set(leds);
#endif // LED_AVAILABLE
	return false;
}

#if SDCARD_ERASE_SUPPORT
bool SdCard::erase(uint32_t firstBlock, uint32_t lastBlock) {
	csd_t csd;
	if (! read_csd(&csd)) {
		cs_high();
		return false;
	}
	/* check for single block erase */
	if (! csd.v1.erase_blk_en) {
		/* erase size mask */
		uint8_t m = (csd.v1.sector_size_high << 1) | csd.v1.sector_size_low;
		if ((firstBlock & m) != 0 || ((lastBlock + 1) & m) != 0) {
			/* error card can't erase specified area */
			set_error(SD_CARD_ERROR_ERASE_SINGLE_BLOCK);
			cs_high();
			return false;
		}
	}
	if (m_type != SD_CARD_TYPE_SDHC) {
		firstBlock <<= 9;
		lastBlock <<= 9;
	}
	if (send_cmd(CMD32, firstBlock) || send_cmd(CMD33, lastBlock) || send_cmd(CMD38, 0)) {
		set_error(SD_CARD_ERROR_ERASE);
		cs_high();
		return false;
	}
	if (! SPI::wait_not_busy(SD_ERASE_TIMEOUT)) {
		set_error(SD_CARD_ERROR_ERASE_TIMEOUT);
		cs_high();
		return false;
	}
	cs_high();
	return true;
}

bool SdCard::single_block_erasable() {
	csd_t csd;
	return read_csd(&csd) ? csd.v1.erase_blk_en : false;
}

#endif // SDCARD_ERASE_SUPPORT
#endif // MMC_AVAILABLE
#endif // MCU
