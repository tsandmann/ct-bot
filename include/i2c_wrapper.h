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
 * @file    i2c_wrapper.h
 * @brief   C++ Wrapper for I2C interface
 * @author  Timo Sandmann
 * @date    01.11.2020
 */

#pragma once

#ifdef MCU
#include "ct-Bot.h"

#ifdef I2C_AVAILABLE
#include <stdint.h>


class I2C_Wrapper {
    static constexpr bool DEBUG_ { true };

    uint8_t addr_;
    uint32_t freq_;
    uint8_t twbr_;

    uint32_t get_freq_internal() const;

public:
    I2C_Wrapper(const uint8_t bus, const uint8_t addr, const uint32_t freq);

    I2C_Wrapper(const uint8_t bus, const uint8_t addr) : I2C_Wrapper { bus, addr, 100'000 } {}

    I2C_Wrapper(const uint8_t bus) : I2C_Wrapper { bus, 255, 100'000 } {}

    uint8_t get_bus() const {
        return 0;
    }

    bool init();

    bool init(const uint8_t bus_id, const uint32_t freq);

    uint32_t get_freq() const {
        return freq_;
    }

    uint8_t get_address() const {
        return addr_;
    }

    void set_address(const uint8_t addr);

    uint8_t read_reg8(const uint8_t reg, uint8_t& data) const;
    uint8_t read_reg8(const uint16_t reg, uint8_t& data) const;
    uint8_t read_reg16(const uint8_t reg, uint16_t& data) const;
    uint8_t read_reg32(const uint8_t reg, uint32_t& data) const;
    uint8_t read_bytes(const uint8_t reg_addr, void* p_data, const uint8_t length) const;

    uint8_t write_reg8(const uint8_t reg, const uint8_t value) const;
    uint8_t write_reg8(const uint16_t reg, const uint8_t value) const;
    uint8_t write_reg16(const uint8_t reg, const uint16_t value) const;
    uint8_t write_reg32(const uint8_t reg, const uint32_t value) const;
    uint8_t write_bytes(const uint8_t reg_addr, const void* p_data, const uint8_t length) const;

    uint8_t set_bit(const uint8_t reg, const uint8_t bit, const bool value) const;
};

#endif // I2C_AVAILABLE
#endif // MCU
