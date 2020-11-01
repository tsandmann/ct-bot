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
 * @file    i2c_wrapper.cpp
 * @brief   C++ Wrapper for I2C interface
 * @author  Timo Sandmann
 * @date    01.11.2020
 */

#ifdef MCU
#include "ct-Bot.h"

#ifdef I2C_AVAILABLE
#include "i2c_wrapper.h"

extern "C" {
#include "i2c.h"
#include "log.h"
#include "uart.h"
}

#include <stdlib.h>


I2C_Wrapper::I2C_Wrapper(const uint8_t, const uint8_t addr, const uint32_t freq) : addr_ { addr }, freq_ { freq } {}

bool I2C_Wrapper::init() {
    return init(0, freq_);
}

bool I2C_Wrapper::init(const uint8_t bus_id, const uint32_t freq) {
    if (bus_id > 0) {
        return false;
    }

    i2c_init(static_cast<uint8_t>((static_cast<uint16_t>(F_CPU / freq) - 16U) / 2U));

    freq_ = get_freq_internal();
    twbr_ = TWBR;

    if (DEBUG_) {
        LOG_DEBUG("I2C_Wrapper::init(): freq_=%lu", freq_);
        LOG_DEBUG("I2C_Wrapper::init(): twbr_=%u", twbr_);
    }

    return true;
}

uint32_t I2C_Wrapper::get_freq_internal() const {
    const auto tws { TWSR & 3 };
    uint8_t prescaler { 1 };
    for (uint8_t i {}; i < tws; ++i) {
        prescaler = static_cast<uint8_t>(prescaler * 4);
    }

    if (DEBUG_) {
        LOG_DEBUG("get_freq_internal(): prescaler=%u", prescaler);
    }

    return F_CPU / ((16 + 2 * TWBR) * prescaler);
}

void I2C_Wrapper::set_address(const uint8_t addr) {
    addr_ = addr;
}

uint8_t I2C_Wrapper::read_reg8(const uint8_t reg, uint8_t& data) const {
    i2c_wait();
    i2c_init(twbr_);

    i2c_read(static_cast<uint8_t>(addr_ << 1), reg, &data, 1);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_DEBUG("read_reg8(): i2c_wait()=%u", i2c_error);
    }

    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::read_reg8(const uint16_t reg, uint8_t& data) const {
    i2c_wait();
    i2c_init(twbr_);

    uint8_t tmp[2] { static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg) };
    i2c_write_read(static_cast<uint8_t>(addr_ << 1), tmp, 2, &data, 1);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("read_reg8(): i2c_wait()=%u", i2c_error);
    }
    
    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::read_reg16(const uint8_t reg, uint16_t& data) const {
    i2c_wait();
    i2c_init(twbr_);

    uint8_t tmp[2];
    i2c_read(static_cast<uint8_t>(addr_ << 1), reg, tmp, 2);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("read_reg16(): i2c_wait()=%u", i2c_error);
    }

    data = static_cast<uint16_t>(tmp[0]) << 8 | tmp[1];

    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::read_reg32(const uint8_t reg, uint32_t& data) const {
    i2c_wait();
    i2c_init(twbr_);

    uint8_t tmp[4];
    i2c_read(static_cast<uint8_t>(addr_ << 1), reg, tmp, 4);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("read_reg32(): i2c_wait()=%u", i2c_error);
    }

    data = static_cast<uint32_t>(tmp[0]) << 24 | static_cast<uint32_t>(tmp[1]) << 16 | static_cast<uint16_t>(tmp[2]) << 8 | tmp[3];

    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::read_bytes(const uint8_t reg_addr, void* p_data, const uint8_t length) const {
    i2c_wait();
    i2c_init(twbr_);

    i2c_read(static_cast<uint8_t>(addr_ << 1), reg_addr, p_data, length);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("read_bytes(): i2c_wait()=%u", i2c_error);
    }

    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::write_reg8(const uint8_t reg, const uint8_t value) const {
    i2c_wait();
    i2c_init(twbr_);

    uint8_t tmp[2] { reg, value };
    i2c_write(static_cast<uint8_t>(addr_ << 1), tmp, 2);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("write_reg8(): i2c_wait()=%u", i2c_error);
    }

    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::write_reg8(const uint16_t reg, const uint8_t value) const {
    i2c_wait();
    i2c_init(twbr_);

    uint8_t tmp[3] { static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg), value };
    i2c_write(static_cast<uint8_t>(addr_ << 1), tmp, 3);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("write_reg8(): i2c_wait()=%u", i2c_error);
    }

    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::write_reg16(const uint8_t reg, const uint16_t value) const {
    i2c_wait();
    i2c_init(twbr_);

    uint8_t tmp[3] { reg, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value) };
    i2c_write(static_cast<uint8_t>(addr_ << 1), tmp, 3);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("write_reg16(): i2c_wait()=%u", i2c_error);
    }

    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::write_reg32(const uint8_t reg, const uint32_t value) const {
    i2c_wait();
    i2c_init(twbr_);

    uint8_t tmp[5] { reg, static_cast<uint8_t>(value >> 24), static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value) };
    i2c_write(static_cast<uint8_t>(addr_ << 1), tmp, 5);

    const auto i2c_error { i2c_wait() };
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("write_reg32(): i2c_wait()=%u", i2c_error);
    }

    return i2c_error == 0xf8 ? 0 : 1;
}

uint8_t I2C_Wrapper::write_bytes(const uint8_t reg_addr, const void* p_data, const uint8_t length) const {
    i2c_wait();
    i2c_init(twbr_);

    i2c_write(static_cast<uint8_t>(addr_ << 1), &reg_addr, 1);
    
    auto i2c_error { i2c_wait() };
    if (i2c_error != 0xf8) {
        if (DEBUG_) {
            LOG_ERROR("write_bytes(): i2c_write() failed.");
        }
        return 1;
    }
    i2c_write(static_cast<uint8_t>(addr_ << 1), p_data, length);

    i2c_error = i2c_wait();
    if (DEBUG_ && i2c_error != 0xf8) {
        LOG_ERROR("write_bytes(): i2c_wait()=%u", i2c_error);
    }

    return i2c_error == 0xf8 ? 0 : 2;
}

uint8_t I2C_Wrapper::set_bit(const uint8_t reg, const uint8_t bit, const bool value) const {
    uint8_t data;
    if (read_reg8(reg, data)) {
        if (DEBUG_) {
            LOG_ERROR("set_bit(): read_reg8() failed.");
        }
        return 4;
    }

    if (value) {
        data = static_cast<uint8_t>(data | (1 << bit));
    } else {
        data = static_cast<uint8_t>(data & ~(1 << bit));
    }

    if (write_reg8(reg, data)) {
        if (DEBUG_) {
            LOG_ERROR("set_bit(): write_reg8() failed.");
        }
        return 4;
    }
    return 0;
}

#endif // I2C_AVAILABLE
#endif // MCU
