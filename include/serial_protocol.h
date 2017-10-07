/**
 * \file   serial_protocol.h
 * \author Timo Sandmann
 * \date   26.12.2016
 * \brief  Data transfer protocol for serial connections like UART, SPI, etc.
 *
 * Consider this protocol as a data link layer for serial connections like UART, SPI, etc.
 * The current implementation supports a UART connection only, SPI may (theoretically) also
 * be possible in the future.
 * This protocol includes data consistency checks and automatic retransmission in case of an
 * error (like TCP does). The protocol is designed for a point-to-point connection between
 * a master and a slave. The master is responsible for sending data to the slave or request
 * data from the slave.
 *
 * Documentation to be continued...
 */

#ifndef SERIAL_PROTOCOL_H_
#define SERIAL_PROTOCOL_H_

#include "logging.h"
#include "serial_protocol_clock.h"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <streambuf>

namespace tsio {
class ClientServerBase;
}

namespace ctbot {

class SerialProtocol : public SerialProtocolClock {
protected:
	static constexpr char STARTBYTE = 'U';
	static constexpr int TYPE_BITS = 2;
	static constexpr int TAG_BITS = 4;
	static constexpr int LENGTH_BITS = 10;
	static constexpr std::size_t max_size = (1 << LENGTH_BITS) - 1;
	static constexpr uint32_t max_seq_no = 255;

public:
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	struct header {
		char start;
		uint8_t seq;
		uint8_t type:TYPE_BITS;
		uint8_t tag:TAG_BITS;
		uint16_t length:LENGTH_BITS;
		uint8_t crc;
	} 
#ifdef _MSC_VER
	;
#pragma pack(pop)
#elif _WIN32
	__attribute__((packed, gcc_struct));
#else
	__attribute__((packed));
#endif

	enum class header_types : uint8_t {
		SEND = 0,
		ACK = 1,
		NACK = 2,
		REQUEST = 3,
	};
	static_assert(static_cast<uint8_t>(header_types::REQUEST) < (1 << TYPE_BITS), "header_types doesn't fit in type field of header");

	SerialProtocol(tsio::ClientServerBase& connection, uint32_t max_retries, const uint32_t recv_timeout_ms) noexcept;

	template <typename T>
	std::size_t master_receive(T& data, const std::size_t size, const uint8_t tag = 0);
	template <typename T>
	std::size_t master_send(T& data, const std::size_t size, const uint8_t tag = 0);
	bool slave_listen(header& head);
	template <typename T>
	int slave_process_request(header& head, T& buf, const std::size_t size);

	auto get_crc_errors() const noexcept {
		return crc_errors;
	}

	auto get_resends() const noexcept {
		return resends;
	}

protected:
	tslog::Log<tslog::L_OFF, true, false> logger;
	tsio::ClientServerBase& con;
	const uint32_t receive_timeout_ms;
	const uint32_t retries;
	uint32_t sequence_number;
	uint32_t crc_errors;
	uint32_t resends;

	bool receive_header(header& head, const bool timeout = false);
	bool receive_data(char* data, const uint16_t size, const std::size_t max_size);
	bool receive_data(std::streambuf& buf, const uint16_t size, const std::size_t max_size);
	bool send_header(header& head) const;
	bool send_data(const void* data, const uint16_t size) const;
	bool send_data(std::streambuf& buf, const uint16_t size) const;
};

} /* namespace ctbot */

#endif /* SERIAL_PROTOCOL_H_ */
