/**
 * \file   ll_command.h
 * \author Timo Sandmann
 * \date   26.12.2016
 * \brief  Low level command interface between ATmega and RPi.
 *
 * Efficient communication protocol for data exchange between ATmega and RPi.
 * Sends / receives with SerialProtocol (see serial_protocol.h).
 */

#ifndef LL_COMMAND_H_
#define LL_COMMAND_H_

#include "serial_protocol.h"
#include <cstdint>
#include <string>
#include <ostream>
#include <memory>
#include <streambuf>

namespace ctbot {


template <class TYPE>
class LLCommand {
protected:
	TYPE data;

	LLCommand() = default;

public:
	using Type = TYPE;

	LLCommand(std::streambuf& buf, SerialProtocol& protocol);
	LLCommand(const char* buf) noexcept;

	LLCommand(const TYPE& cmd_data) noexcept : data(cmd_data) {}

	bool send(SerialProtocol& protocol) {
		const char* ptr(reinterpret_cast<const char*>(&data));
		return protocol.master_send(ptr, sizeof(TYPE), data.type) == sizeof(TYPE);
	}

	const TYPE& get_data() const noexcept {
		return data;
	}

	bool operator==(const LLCommand& b) noexcept {
	    return std::memcmp(&get_data(), &b.get_data(), sizeof(TYPE)) == 0;
	}

	bool operator!=(const LLCommand& b) noexcept {
	    return ! operator ==(b);
	}
};


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
class LLCommandSens {
protected:
	uint8_t type:4;
	uint8_t bps:4;
	int16_t enc_l;
	int16_t enc_r;
	uint16_t rc5:13;
	uint8_t door:1;
	uint8_t error:1;
	uint8_t transport:1;
	uint16_t ir_l:10;
	uint16_t ir_r:10;
	uint16_t border_l:10;
	uint16_t border_r:10;
	uint16_t line_l:10;
	uint16_t line_r:10;
	uint16_t ldr_l:10;
	uint16_t ldr_r:10;

	LLCommandSens() noexcept : type(0) {}
	friend class LLCommand<LLCommandSens>;

public:
	uint8_t get_type() const noexcept {
		return type;
	}

	uint8_t get_bps() const noexcept {
		return bps;
	}

	int16_t get_enc_l() const noexcept {
		return enc_l;
	}

	int16_t get_enc_r() const noexcept {
		return enc_r;
	}

	uint16_t get_rc5() const noexcept {
		return rc5;
	}

	bool get_door() const noexcept {
		return door;
	}

	bool get_error() const noexcept {
		return error;
	}

	bool get_transport() const noexcept {
		return transport;
	}

	uint16_t get_ir_l() const noexcept {
		return ir_l;
	}

	uint16_t get_ir_r() const noexcept {
		return ir_r;
	}

	uint16_t get_border_l() const noexcept {
		return border_l;
	}

	uint16_t get_border_r() const noexcept {
		return border_r;
	}

	uint16_t get_line_l() const noexcept {
		return line_l;
	}

	uint16_t get_line_r() const noexcept {
		return line_r;
	}

	uint16_t get_ldr_l() const noexcept {
		return ldr_l;
	}

	uint16_t get_ldr_r() const noexcept {
		return ldr_r;
	}

	LLCommandSens(int16_t enc_l_, int16_t enc_r_, uint16_t rc5_, bool door_, bool error_, bool transport_, uint8_t bps_, uint16_t ir_l_, uint16_t ir_r_, uint16_t border_l_,
		uint16_t border_r_, uint16_t line_l_, uint16_t line_r_, uint16_t ldr_l_, uint16_t ldr_r_) noexcept : type(0), bps(bps_ & 0xf), enc_l(enc_l_), enc_r(enc_r_), rc5(rc5_ & 0x103f),
		door(door_), error (error_), transport(transport_), ir_l(ir_l_ & 0x3ff), ir_r(ir_r_ & 0x3ff), border_l(border_l_ & 0x3ff), border_r(border_r_ & 0x3ff),
		line_l(line_l_ & 0x3ff), line_r(line_r_ & 0x3ff), ldr_l(ldr_l_ & 0x3ff), ldr_r(ldr_r_ & 0x3ff) {}

	friend std::ostream& operator <<(std::ostream& os, const ctbot::LLCommandSens& v);

}
#ifdef _MSC_VER
;
#pragma pack(pop)
#elif _WIN32
__attribute__((packed, gcc_struct));
#else
__attribute__((packed));
#endif

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
class LLCommandAct {
protected:
	uint8_t type:4;
	uint16_t motor_l:10;
	uint16_t motor_r:10;
	uint8_t servo1;
	uint8_t servo2;
	uint8_t leds;
	uint8_t shutdown:1;
	uint8_t reserved:3;

	LLCommandAct() noexcept : type(1) {}
	friend class LLCommand<LLCommandAct>;

public:
	static constexpr uint8_t SERVO_OFF = 0;
	static constexpr uint8_t SERVO_LEFT = 1;
	static constexpr uint8_t SERVO_RIGHT = 2;

	uint8_t get_type() const noexcept {
		return type;
	}

	int16_t get_motor_l() const noexcept {
		union tmp_t {
			uint16_t bits:10;
			int16_t value:10;
		};

		tmp_t tmp;
		tmp.bits = motor_l;
		return tmp.value;
	}

	int16_t get_motor_r() const noexcept {
		union tmp_t {
			uint16_t bits:10;
			int16_t value:10;
		};

		tmp_t tmp;
		tmp.bits = motor_r;
		return tmp.value;
	}

	uint8_t get_servo1() const noexcept {
		return servo1;
	}

	uint8_t get_servo2() const noexcept {
		return servo2;
	}

	uint8_t get_leds() const noexcept {
		return leds;
	}

	bool get_shutdown() const noexcept {
		return shutdown;
	}

	LLCommandAct(int16_t motor_l_, int16_t motor_r_, uint8_t servo1_, uint8_t servo2_, uint8_t leds_, bool shutdown_) noexcept : type(1), motor_l(motor_l_ & 0x3ff),
		motor_r(motor_r_ & 0x3ff), servo1(servo1_), servo2(servo2_), leds(leds_), shutdown(shutdown_), reserved(0) {}

	friend std::ostream& operator <<(std::ostream& os, const ctbot::LLCommandAct& v);

}
#ifdef _MSC_VER
;
#pragma pack(pop)
#elif _WIN32
__attribute__((packed, gcc_struct));
#else
__attribute__((packed));
#endif

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
class LLCommandLcd {
public:
	static constexpr uint8_t LINE_SIZE = 20;

protected:
	uint8_t type:4;
	uint8_t ctrl:4;
	uint8_t cursor_r:3;
	uint8_t cursor_c:5;
	char line[LINE_SIZE];

	LLCommandLcd() noexcept : type(2) {};
	friend class LLCommand<LLCommandLcd>;

public:
	uint8_t get_type() const noexcept {
		return type;
	}

	uint8_t get_ctrl() const noexcept {
		return ctrl;
	}

	uint8_t get_cursor_r() const noexcept {
		return cursor_r;
	}

	uint8_t get_cursor_c() const noexcept {
		return cursor_c;
	}

	const char* get_text() const noexcept {
		return line;
	}

	LLCommandLcd(uint8_t ctrl_, uint8_t cursor_r_, uint8_t cursor_c_, const std::string& text) noexcept : type(2), ctrl(ctrl_ & 0xf), cursor_r(cursor_r_ & 0x7), cursor_c(cursor_c_ & 0x1f) {
		std::strncpy(line, text.c_str(), sizeof(line));
	}

	friend std::ostream& operator <<(std::ostream& os, const ctbot::LLCommandLcd& v);

}
#ifdef _MSC_VER
;
#pragma pack(pop)
#elif _WIN32
__attribute__((packed, gcc_struct));
#else
__attribute__((packed));
#endif


using CommandSens = LLCommand<LLCommandSens>;
using CommandAct = LLCommand<LLCommandAct>;
using CommandLcd = LLCommand<LLCommandLcd>;

} /* namespace ctbot */

#endif /* LL_COMMAND_H_ */
