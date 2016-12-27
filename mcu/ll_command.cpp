/**
 * \file   ll_command.cpp
 * \author Timo Sandmann
 * \date   26.12.2016
 * \brief  Low level command interface between ATmega and RPi.
 */

#include "ll_command.h"
#include <stdexcept>

namespace ctbot {

template <class TYPE>
LLCommand<TYPE>::LLCommand(std::streambuf& buf, SerialProtocol& protocol) {
	if (protocol.master_receive(buf, sizeof(TYPE), data.type) != sizeof(TYPE)) {
#ifdef __EXCEPTIONS
		throw std::runtime_error("LLCommand::LLCommand(): reading command from connection failed");
#else
		return;
#endif
	}

	buf.sgetn(reinterpret_cast<char*>(&data), sizeof(TYPE));
}

template <class TYPE>
LLCommand<TYPE>::LLCommand(const char* buf) noexcept {
	std::memcpy(&data, buf, sizeof(TYPE));
}

std::ostream& operator <<(std::ostream& os, const LLCommandSens& v) {
	os << std::boolalpha << "\n";
	os << " type=" << static_cast<uint16_t>(v.get_type()) << "\n";
	os << " enc_l=" << v.get_enc_l() << " enc_r=" << v.get_enc_r() << "\n";
	os << " ir_l=" << v.get_ir_l() << " ir_r=" << v.get_ir_r() << "\n";
	os << " border_l=" << v.get_border_l() << " border_r=" << v.get_border_r() << "\n";
	os << " line_l=" << v.get_line_l() << " line_r=" << v.get_line_r() << "\n";
	os << " ldr_l=" << v.get_ldr_l() << " ldr_r=" << v.get_ldr_r() << "\n";
	os << " rc5=" << v.get_rc5() << " bps=" << static_cast<uint16_t>(v.get_bps()) << "\n";
	os << " door=" << v.get_door() << " error=" << v.get_error() << " transport=" << v.get_transport();
	return os;
}

std::ostream& operator <<(std::ostream& os, const LLCommandAct& v) {
	os << "\n type=" << static_cast<uint16_t>(v.get_type()) << "\n";
	os << " motor_l=" << v.get_motor_l() << " motor_r=" << v.get_motor_r() << "\n";
	os << " servo1=" << static_cast<uint16_t>(v.get_servo1()) << " servo2=" << static_cast<uint16_t>(v.get_servo2()) << "\n";
	os << " leds=" << static_cast<uint16_t>(v.get_leds()) << " shutdown=" << v.get_shutdown();
	return os;
}

std::ostream& operator <<(std::ostream& os, const LLCommandLcd& v) {
	os << "\n type=" << static_cast<uint16_t>(v.get_type()) << "\n";
	os << " ctrl=" << static_cast<uint16_t>(v.get_ctrl()) << "\n";
	os << " text=\"" << v.get_text() << "\"";
	return os;
}

template class LLCommand<LLCommandSens>;
template class LLCommand<LLCommandAct>;
template class LLCommand<LLCommandLcd>;

} /* namespace ctbot */
