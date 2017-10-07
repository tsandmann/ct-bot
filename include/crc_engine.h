/*
 * crc_engine.h
 *
 *  Created on: 26.12.2016
 *      Author: ts
 */

#ifndef CRC_ENGINE_H_
#define CRC_ENGINE_H_

#ifdef MCU
#include <cstddef>
#include <streambuf>
#include <sstream>
#include "math_utils.h"

namespace ctbot {

struct streambuf_helper {
	template <class T>
	static const char* get_data(T& buf) {
		struct helper : public T {
			static auto& _get_data(T& buf) {
				auto& buffer(reinterpret_cast<helper&>(buf));
				return buffer.data;
			}
		};

		return helper::_get_data(buf).c_str();
	}
};

template <typename SizeType, unsigned long TruncPoly, unsigned long InitRem, unsigned long FinalXor>
class CrcEngine {
protected:
	SizeType value;

public:
	CrcEngine(SizeType init_rem = InitRem) : value(init_rem) {}

	void process_byte(uint8_t byte) {
		if (sizeof(SizeType) == sizeof(uint16_t)) {
			value = static_cast<SizeType>(_crc_xmodem_update(value, byte));
		} else if (sizeof(SizeType) == sizeof(uint8_t)) {
			value = _crc8_ccitt_update(static_cast<uint8_t>(value), byte);
		}
	}

	void process_bytes(const void* buffer, std::size_t byte_count) {
		const uint8_t* ptr(reinterpret_cast<const uint8_t*>(buffer));
		for (auto i(0U); i < byte_count; ++i, ++ptr) {
			if (sizeof(SizeType) == sizeof(uint16_t)) {
				value = static_cast<SizeType>(_crc_xmodem_update(value, *ptr));
			} else if (sizeof(SizeType) == sizeof(uint8_t)) {
				value = _crc8_ccitt_update(static_cast<uint8_t>(value), *ptr);
			}
		}
	}

	void process_block(const void* bytes_begin, const void* bytes_end) {
		for (auto ptr(reinterpret_cast<const uint8_t*>(bytes_begin)); ptr < bytes_end; ++ptr) {
			if (sizeof(SizeType) == sizeof(uint16_t)) {
				value = static_cast<SizeType>(_crc_xmodem_update(value, *ptr));
			} else if (sizeof(SizeType) == sizeof(uint8_t)) {
				value = _crc8_ccitt_update(static_cast<uint8_t>(value), *ptr);
			}
		}
	}

	SizeType checksum() const {
		return value;
	}

	void process_stream(std::streambuf& buf, std::size_t count, std::ptrdiff_t offset = 0) {
		auto& buffer(reinterpret_cast<std::stringbuf&>(buf));
		return process_bytes(streambuf_helper::get_data(buffer) + offset, count);
	}
};

using crc_header = CrcEngine<uint8_t, 0x8C, 0, 0>;
using crc_data   = CrcEngine<uint16_t, 0x1021, 0xFFFF, 0>;

} /* namespace ctbot */

#endif // MCU
#endif /* CRC_ENGINE_H_ */
